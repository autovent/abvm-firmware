#pragma once

#include <assert.h>

#include "clock.h"
#include "control_panel.h"
#include "math/dsp.h"
#include "ui/iui.h"

class UI_V1 : public IUI {
public:
    enum class View { ADJUST, PRESSURE };
    enum class AudioAlert {
        DONE_HOMING,
        STARTING,
        STOPING,
        ALARM_1,
        ALARM_2,
        ALARM_3,
    };

    UI_V1(ControlPanel *controls) : controls(controls), button_events{} {
        button_events[ControlPanel::START_MODE_BTN].hold_time_ms = 600;
        button_events[ControlPanel::STOP_BTN].hold_time_ms = 6000;
    }
    virtual ~UI_V1() = default;

    void init() {
        set_view(View::ADJUST);
    }

    void set_audio_alert(AudioAlert alert) {
        current_alert = alert;
        audio_alert_in_progress = true;
        audio_alert_start_time_ms = millis();
    }

    void handle_buzzer() {
        if (audio_alert_in_progress) {
            if (current_alert == AudioAlert::STARTING) {
                uint32_t time_elapsed = time_since_ms(audio_alert_start_time_ms);
                controls->set_buzzer_volume(0.8);
                controls->sound_buzzer(true);
                if (time_elapsed < 100) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_C7);
                } else if (time_elapsed < 200) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_G7);
                } else if (time_elapsed < 300) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
                } else if (time_elapsed < 400) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_C8);
                } else {
                    controls->sound_buzzer(false);

                    audio_alert_start_time_ms = 0;
                    audio_alert_in_progress = false;
                }
            } else if (current_alert == AudioAlert::DONE_HOMING) {
                uint32_t time_elapsed = time_since_ms(audio_alert_start_time_ms);
                controls->set_buzzer_volume(0.8);
                controls->sound_buzzer(true);
                if (time_elapsed < 100) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_C7);
                } else if (time_elapsed < 200) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_G7);
                } else if (time_elapsed < 300) {
                    controls->set_buzzer_tone(ControlPanel::BUZZER_C7);
                } else {
                    controls->sound_buzzer(false);

                    audio_alert_start_time_ms = 0;
                    audio_alert_in_progress = false;
                }
            }
        }
    }

    Event update() {
        // Handle Buzzer Noises
        handle_buzzer();

        // Set LED Values for Bars
        if (view == View::ADJUST) {
            controls->set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT,
                                        display_values[(uint32_t)DisplayValue::TIDAL_VOLUME] + 1);
            controls->set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT,
                                        display_values[(uint32_t)DisplayValue::RESPIRATORY_RATE] + 1);
        } else {
            float peak_pressure = display_values[(uint32_t)DisplayValue::PEAK_PRESSURE];
            float plateau_pressure = display_values[(uint32_t)DisplayValue::PLATEAU_PRESSURE];
            controls->set_led_bar_graph(
                  ControlPanel::BAR_GRAPH_RIGHT,
                  map<float>(peak_pressure, kPeakPressureDisplayMin, kPeakPressureDisplayMax, 1, 6));
            controls->set_led_bar_graph(
                  ControlPanel::BAR_GRAPH_LEFT,
                  map<float>(plateau_pressure, kPlateauPressureDisplayMin, kPlateauPressureDisplayMax, 1, 6));
        }

        // Gather all button events
        for (size_t i = 0; i < controls->num_buttons(); i++) {
            button_events[i].event = controls->get_buttons(i)->update();
            button_events[i].update();
        }

        Event result = Event::NONE;

        // Process to issue next event in series. Only issue one at a time in priority order
        // Priority
        //    - STOP
        //    - START
        //    - MODE CHANGE
        //    - TV UP/DOWN (DOWN has  higher priority for all)
        //    - RR UP/DOWN
        //    - Pressure Limit (UP/DOWN)
        //    - SILENCE_ALARM

        // Prevent start from being trigged by an attempt at bootloader enter mode, but clear if start is  released
        if (!button_events[ControlPanel::STOP_BTN].is_unpressed()) {
            disallow_start = true;
        }

        if (button_events[ControlPanel::START_MODE_BTN].is_unpressed()) {
            disallow_start = false;
        }

        if (button_events[ControlPanel::STOP_BTN].is_unpressed() ||
            button_events[ControlPanel::START_MODE_BTN].is_unpressed()) {
            is_bootloader_issued = false;
        }

        if (is_bootloader_event()) {
            result = Event::GO_TO_BOOTLOADER;
        }

        if (!is_bootloader_issued && button_events[ControlPanel::START_MODE_BTN].is_unpressed() &&
            button_events[ControlPanel::STOP_BTN].is_hold_event()) {
            result = Event::SILENCE_ALARM;
        }

        if (view == View::ADJUST) {
            if (button_events[ControlPanel::DN_RIGHT_BTN].is_pressed()) {
                result = Event::RESPIRATORY_RATE_DOWN;
            }

            if (button_events[ControlPanel::UP_RIGHT_BTN].is_pressed()) {
                result = Event::RESPIRATORY_RATE_UP;
            }

            if (button_events[ControlPanel::DN_LEFT_BTN].is_pressed()) {
                result = Event::TIDAL_VOLUME_DOWN;
            }

            if (button_events[ControlPanel::UP_LEFT_BTN].is_pressed()) {
                result = Event::TIDAL_VOLUME_UP;
            }
        } else {
            // Pressure Mode button actions
        }
        if (button_events[ControlPanel::STOP_BTN].is_pressed()) {
            disallow_start = true;
            result = Event::STOP;
        }

        if (!disallow_start && button_events[ControlPanel::START_MODE_BTN].is_released()) {
            if (!button_events[ControlPanel::START_MODE_BTN].is_held_triggered) {
                result = Event::CHANGE_MENU;
            }
        } else if (!disallow_start && (button_events[ControlPanel::START_MODE_BTN].is_hold_event())) {
            result = Event::START;
        }

        return result;
    }

    void set_state(State s) {}

    void set_view(View v) {
        view = v;

        if (view == View::ADJUST) {
            controls->set_status_led(ControlPanel::STATUS_LED_1, true);
            controls->set_status_led(ControlPanel::STATUS_LED_4, false);
        } else if (view == View::PRESSURE) {
            controls->set_status_led(ControlPanel::STATUS_LED_1, false);
            controls->set_status_led(ControlPanel::STATUS_LED_4, true);
        }
    }

    void toggle_view() {
        View next_view;
        if (view == View::ADJUST) {
            next_view = View::PRESSURE;
        } else {
            next_view = View::ADJUST;
        }
        set_view(next_view);
    }

    void set_value(DisplayValue param, float value) {
        assert((int)param >= 0 && (int)param < kNumDisplayValues);
        display_values[(size_t)param] = value;
    }

    void set_alarm(Alarm a) {}

private:
    // TODO(cw): Should some of this be wrapped  back into Button::Event? I think so.
    struct UIButtonEvent {
        Button::Event event;
        uint32_t hold_time_ms;
        bool is_held_triggered;

        bool is_pressed() {
            return event.state == Button::State::PRESSED;
        }

        bool is_unpressed() {
            return event.state == Button::State::UNPRESSED || event.state == Button::State::DEBOUNCING;
        }

        bool is_released() {
            return event.state == Button::State::RELEASED;
        }

        bool is_hold_event() {
            if (!is_held_triggered && (event.state == Button::State::HOLDING) && (event.held_time_ms >= hold_time_ms)) {
                is_held_triggered = true;
                return true;
            } else {
                return false;
            }
        }

        bool is_holding() {
            return event.state == Button::State::HOLDING;
        }

        void update() {
            if (event.state == Button::State::UNPRESSED) {
                is_held_triggered = false;
            }
        }
    };

    static constexpr float kPeakPressureDisplayMin = 25;
    static constexpr float kPeakPressureDisplayMax = 50;
    static constexpr float kPlateauPressureDisplayMin = 15;
    static constexpr float kPlateauPressureDisplayMax = 40;

    ControlPanel *controls;
    UIButtonEvent button_events[ControlPanel::num_buttons()];
    bool is_bootloader_issued = false;
    uint32_t bootloader_time_ms = 5000;
    bool disallow_start = false;
    View view;

    AudioAlert current_alert;
    bool audio_alert_in_progress;
    uint32_t audio_alert_start_time_ms;

    float display_values[kNumDisplayValues];

    bool is_bootloader_event() {
        bool ret = (button_events[ControlPanel::STOP_BTN].is_holding() &&
                    button_events[ControlPanel::STOP_BTN].event.held_time_ms > bootloader_time_ms) &&
                   (button_events[ControlPanel::START_MODE_BTN].is_holding() &&
                    button_events[ControlPanel::START_MODE_BTN].event.held_time_ms > bootloader_time_ms);
        if (ret && !is_bootloader_issued) {
            is_bootloader_issued = true;
            return ret;
        } else {
            return false;
        }
    }
};
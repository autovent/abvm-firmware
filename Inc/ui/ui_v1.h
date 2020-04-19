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

    UI_V1(ControlPanel *controls) : controls(controls) {
        controls->get_button_ptr(ControlPanel::START_MODE_BTN)->set_long_press_time_ms(600);
        controls->get_button_ptr(ControlPanel::STOP_BTN)->set_long_press_time_ms(6000);
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

        controls->button_update();

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
        if (!controls->get_button(ControlPanel::STOP_BTN).is_unpressed()) {
            disallow_start = true;
        }

        if (controls->get_button(ControlPanel::START_MODE_BTN).is_unpressed()) {
            disallow_start = false;
        }

        if (controls->get_button(ControlPanel::STOP_BTN).is_unpressed() ||
            controls->get_button(ControlPanel::START_MODE_BTN).is_unpressed()) {
            is_bootloader_issued = false;
        }

        if (is_bootloader_event()) {
            result = Event::GO_TO_BOOTLOADER;
        }

        if (!is_bootloader_issued && controls->get_button(ControlPanel::START_MODE_BTN).is_unpressed() &&
            controls->get_button(ControlPanel::STOP_BTN).is_long_press()) {
            result = Event::SILENCE_ALARM;
        }

        if (view == View::ADJUST) {
            if (controls->get_button(ControlPanel::DN_RIGHT_BTN).is_pressed()) {
                result = Event::RESPIRATORY_RATE_DOWN;
            }

            if (controls->get_button(ControlPanel::UP_RIGHT_BTN).is_pressed()) {
                result = Event::RESPIRATORY_RATE_UP;
            }

            if (controls->get_button(ControlPanel::DN_LEFT_BTN).is_pressed()) {
                result = Event::TIDAL_VOLUME_DOWN;
            }

            if (controls->get_button(ControlPanel::UP_LEFT_BTN).is_pressed()) {
                result = Event::TIDAL_VOLUME_UP;
            }
        } else {
            // Pressure Mode button actions
        }
        if (controls->get_button(ControlPanel::STOP_BTN).is_pressed()) {
            disallow_start = true;
            result = Event::STOP;
        }

        if (!disallow_start && controls->get_button(ControlPanel::START_MODE_BTN).is_released()) {
            if (!controls->get_button(ControlPanel::START_MODE_BTN).is_long_press_triggered()) {
                result = Event::CHANGE_MENU;
            }
        } else if (!disallow_start && (controls->get_button(ControlPanel::START_MODE_BTN).is_long_press())) {
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
    static constexpr float kPeakPressureDisplayMin = 25;
    static constexpr float kPeakPressureDisplayMax = 50;
    static constexpr float kPlateauPressureDisplayMin = 15;
    static constexpr float kPlateauPressureDisplayMax = 40;

    ControlPanel *controls;
    bool is_bootloader_issued = false;
    uint32_t bootloader_time_ms = 5000;
    bool disallow_start = false;
    View view;

    AudioAlert current_alert;
    bool audio_alert_in_progress;
    uint32_t audio_alert_start_time_ms;

    float display_values[kNumDisplayValues];

    bool is_bootloader_event() {
        bool ret = (controls->get_button(ControlPanel::STOP_BTN).held_for_more_than(bootloader_time_ms)) &&
                   (controls->get_button(ControlPanel::START_MODE_BTN).held_for_more_than(bootloader_time_ms));
        if (ret && !is_bootloader_issued) {
            is_bootloader_issued = true;
            return ret;
        } else {
            return false;
        }
    }
};
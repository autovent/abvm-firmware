#pragma once

#include "ui/ui_v1.h"

#include "config.h"

UI_V1::UI_V1(ControlPanel *controls) : controls(controls) {
    controls->get_button_ptr(ControlPanel::START_MODE_BTN)->set_long_press_time_ms(600);
    controls->get_button_ptr(ControlPanel::STOP_BTN)->set_long_press_time_ms(6000);
}

void UI_V1::init() {
    set_view(View::ADJUST);
}

void UI_V1::set_audio_alert(AudioAlert alert) {
 if (alert == AudioAlert::NONE || !audio_alert_in_progress || ((uint32_t) alert > (uint32_t) current_alert)) {
        current_alert = alert;
        audio_alert_in_progress = true;
        audio_alert_start_time_ms = millis();

    }
}

void UI_V1::handle_buzzer() {
    if (audio_alert_in_progress) {
        if (current_alert == AudioAlert::NONE) {
            controls->sound_buzzer(false);

            audio_alert_start_time_ms = 0;
            audio_alert_in_progress = false;

        } else if (current_alert == AudioAlert::STARTING) {
            uint32_t time_elapsed = time_since_ms(audio_alert_start_time_ms);
            controls->set_buzzer_volume(0.8);
            controls->sound_buzzer(true);
            if (time_elapsed < 100) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_C7);
            } else if (time_elapsed < 200) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
            } else if (time_elapsed < 300) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_G7);
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
            } else {
                controls->sound_buzzer(false);

                audio_alert_start_time_ms = 0;
                audio_alert_in_progress = false;
            }
        } else if (current_alert == AudioAlert::STOPPING) {
            uint32_t time_elapsed = time_since_ms(audio_alert_start_time_ms);
            controls->set_buzzer_volume(0.8);
            controls->sound_buzzer(true);
            if (time_elapsed < 100) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_C8);
            } else if (time_elapsed < 200) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_G7);
            } else if (time_elapsed < 300) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
            } else if (time_elapsed < 400) {
                controls->set_buzzer_tone(ControlPanel::BUZZER_C7);
            } else {
                controls->sound_buzzer(false);

                audio_alert_start_time_ms = 0;
                audio_alert_in_progress = false;
            }
        } else if (current_alert == AudioAlert::ALERT_BEEPING) {
            if (is_silence) {
                controls->sound_buzzer(false);
            } else {
                uint32_t time_elapsed = time_since_ms(audio_alert_start_time_ms);

                if (time_elapsed < 250) {
                    controls->set_buzzer_volume(0.8);
                    controls->sound_buzzer(true);
                    controls->set_buzzer_tone(ControlPanel::BUZZER_C7);
                } else if (time_elapsed < 500) {
                    controls->set_buzzer_volume(0);
                    controls->sound_buzzer(false);
                } else {
                    audio_alert_start_time_ms = millis();
                }
            }
        } else if (current_alert == AudioAlert::ALERT_CONTINUOUS_CRESCENDO) {
            if (is_silence) {
                controls->sound_buzzer(false);

            } else {
                uint32_t time_elapsed = time_since_ms(audio_alert_start_time_ms);

                if (time_elapsed < 5000) {
                    controls->set_buzzer_volume(0.4);
                    controls->sound_buzzer(true);
                    controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
                } else if (time_elapsed < 10000) {
                    controls->set_buzzer_volume(.6);
                    controls->sound_buzzer(true);
                    controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
                } else if (time_elapsed < 15000) {
                    controls->set_buzzer_volume(.8);
                    controls->sound_buzzer(true);
                    controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
                } else if (time_elapsed < 20000) {
                    controls->set_buzzer_volume(1.0);
                    controls->sound_buzzer(true);
                    controls->set_buzzer_tone(ControlPanel::BUZZER_E7);
                } 

            }
        }
    }
}

UI_V1::Event UI_V1::update() {
    // Handle Buzzer Noises
    handle_buzzer();

    if (is_silence && time_since_ms(silence_time_ms) > kSilenceTime_ms) {
        is_silence = false;
    }

    switch (view) {
        case View::ADJUST:
            controls->set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT,
                                        display_values[(uint32_t)DisplayValue::TIDAL_VOLUME] + 1);
            controls->set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT,
                                        display_values[(uint32_t)DisplayValue::RESPIRATORY_RATE] + 1);

            break;
        case View::PRESSURE: {
            float peak_pressure = display_values[(uint32_t)DisplayValue::PEAK_PRESSURE];
            float plateau_pressure = display_values[(uint32_t)DisplayValue::PLATEAU_PRESSURE];
            controls->set_led_bar_graph(
                  ControlPanel::BAR_GRAPH_RIGHT,
                  map<float>(peak_pressure, kPeakPressureDisplayMin, kPeakPressureDisplayMax, 1, 6));
            controls->set_led_bar_graph(
                  ControlPanel::BAR_GRAPH_LEFT,
                  map<float>(plateau_pressure, kPlateauPressureDisplayMin, kPlateauPressureDisplayMax, 1, 6));

            break;
        }
        case View::PRESSURE_LIMIT_ADJUST: {
            float peak_pressure = display_values[(uint32_t)DisplayValue::PEAK_PRESSURE_ALARM];
            float plateau_pressure = display_values[(uint32_t)DisplayValue::PLATEAU_PRESSURE];
            controls->set_led_bar_graph(
                  ControlPanel::BAR_GRAPH_RIGHT,
                  map<float>(peak_pressure, kPeakPressureDisplayMin, kPeakPressureDisplayMax, 1, 6));
            controls->set_led_bar_graph(
                  ControlPanel::BAR_GRAPH_LEFT,
                  map<float>(plateau_pressure, kPlateauPressureDisplayMin, kPlateauPressureDisplayMax, 1, 6));
            break;
        }
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

    switch (view) {
        case View::ADJUST:
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
            break;
        case View::PRESSURE:
            // Pressure Mode button actions
            if (controls->get_button(ControlPanel::DN_RIGHT_BTN).held_for_more_than(600)) {
                enter_pressure_limit_view();
            }

            if (controls->get_button(ControlPanel::UP_RIGHT_BTN).held_for_more_than(600)) {
                enter_pressure_limit_view();
            }
            break;
        case View::PRESSURE_LIMIT_ADJUST:
            if (controls->get_button(ControlPanel::UP_RIGHT_BTN).is_pressed()) {
                result = Event::PRESSURE_LIMIT_UP;
            }

            if (controls->get_button(ControlPanel::DN_RIGHT_BTN).is_pressed()) {
                result = Event::PRESSURE_LIMIT_DOWN;
            }
            break;
    }

    if (controls->get_button(ControlPanel::STOP_BTN).is_pressed()) {
        disallow_start = true;
        result = Event::STOP;
    }

    if (!disallow_start && controls->get_button(ControlPanel::START_MODE_BTN).is_released()) {
        if (!controls->get_button(ControlPanel::START_MODE_BTN).is_long_press_triggered()) {
            toggle_view();
        }
    } else if (!disallow_start && (controls->get_button(ControlPanel::START_MODE_BTN).is_long_press())) {
        result = Event::START;
    }

    return result;
}

void UI_V1::set_state(State s) {}

void UI_V1::set_view(View v) {
    view = v;

    if (view == View::ADJUST) {
        controls->set_led_bar_graph_blink(ControlPanel::BAR_GRAPH_LEFT, 0);
        controls->set_led_bar_graph_blink(ControlPanel::BAR_GRAPH_RIGHT, 0);

        controls->set_status_led(ControlPanel::STATUS_LED_1, true);
        controls->set_status_led(ControlPanel::STATUS_LED_4, false);
    } else if (view == View::PRESSURE) {
        controls->set_led_bar_graph_blink(ControlPanel::BAR_GRAPH_LEFT, 0);
        controls->set_led_bar_graph_blink(ControlPanel::BAR_GRAPH_RIGHT, 0);

        controls->set_status_led(ControlPanel::STATUS_LED_1, false);
        controls->set_status_led(ControlPanel::STATUS_LED_4, true);
    } else if (view == View::PRESSURE_LIMIT_ADJUST) {
        controls->set_led_bar_graph_blink(ControlPanel::BAR_GRAPH_LEFT, 0);
        controls->set_led_bar_graph_blink(ControlPanel::BAR_GRAPH_RIGHT, 250);

        controls->set_status_led(ControlPanel::STATUS_LED_1, false);
        controls->set_status_led(ControlPanel::STATUS_LED_4, true);
        controls->set_status_led_blink(ControlPanel::STATUS_LED_4, 250);
    }
}

void UI_V1::toggle_view() {
    View next_view;
    if (view == View::ADJUST) {
        next_view = View::PRESSURE;
    } else if (view == View::PRESSURE_LIMIT_ADJUST) {
        next_view = View::PRESSURE;
    } else {
        next_view = View::ADJUST;
    }

    set_view(next_view);
}

void UI_V1::enter_pressure_limit_view() {
    // Only allow a transition form Pressure to PRESSURE_LIMIT_ADJUST
    View next_view = view;
    if (view == View::PRESSURE) {
        next_view = View::PRESSURE_LIMIT_ADJUST;
    }

    set_view(next_view);
}

void UI_V1::set_value(DisplayValue param, float value) {
    assert((int)param >= 0 && (int)param < kNumDisplayValues);
    display_values[(size_t)param] = value;
}

void UI_V1::silence() {
    is_silence = true;
    silence_time_ms = millis();
}

void UI_V1::set_alarm(Alarms const &a) {
    if (a.is_any_alarmed()) {
        Alarms::Name next = a.get_highest_priority_alarm();
        if (current_alarm != next) {
            current_alarm = next;
            switch (current_alarm) {
                case Alarms::MOTION_FAULT:
                    controls->set_status_led(ControlPanel::STATUS_LED_3, true);
                    controls->set_status_led_blink(ControlPanel::STATUS_LED_3, 500);
                    set_audio_alert(AudioAlert::ALERT_BEEPING);

                case Alarms::OVER_PRESSURE:
                    controls->set_status_led(ControlPanel::STATUS_LED_3, true);
                    controls->set_status_led_blink(ControlPanel::STATUS_LED_3, 250);
                    set_audio_alert(AudioAlert::ALERT_BEEPING);

                    break;
                case Alarms::LOSS_OF_POWER:
                    controls->set_status_led(ControlPanel::STATUS_LED_3, true);
                    set_audio_alert(AudioAlert::ALERT_CONTINUOUS_CRESCENDO);

                    break;
                default:
                    break;
            }
        }
    } else if (current_alarm != Alarms::NUM_ALARMS) {
        current_alarm = Alarms::NUM_ALARMS;
        controls->set_status_led(ControlPanel::STATUS_LED_3, false);
        set_audio_alert(AudioAlert::NONE);
    }
}

bool UI_V1::is_bootloader_event() {
    bool ret = (controls->get_button(ControlPanel::STOP_BTN).held_for_more_than(bootloader_time_ms)) &&
               (controls->get_button(ControlPanel::START_MODE_BTN).held_for_more_than(bootloader_time_ms));
    if (ret && !is_bootloader_issued) {
        is_bootloader_issued = true;
        return ret;
    } else {
        return false;
    }
}
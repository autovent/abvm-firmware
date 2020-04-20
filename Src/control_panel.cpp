#include "control_panel.h"

#include <assert.h>

#include "clock.h"
#include "math/dsp.h"
#include "sys/array_helpers.h"

ControlPanel::ControlPanel(Pin *sw_start, Pin *sw_stop, Pin *sw_up_left, Pin *sw_dn_left, Pin *sw_up_right,
                           Pin *sw_dn_right, Pin *led_status1, Pin *led_status2, Pin *led_status3, Pin *led_status4,
                           Pin *led_bar_left_char1, Pin *led_bar_left_char2, Pin *led_bar_left_char3,
                           Pin *led_bar_right_char1, Pin *led_bar_right_char2, Pin *led_bar_right_char3,
                           // BUZZER
                           TIM_HandleTypeDef *buzzer_timer, uint32_t buzzer_timer_channel, uint32_t debounce_time_ms)
    : buttons{Button(sw_start, debounce_time_ms, true),    Button(sw_stop, debounce_time_ms, true),
              Button(sw_up_left, debounce_time_ms, true),  Button(sw_dn_left, debounce_time_ms, true),
              Button(sw_up_right, debounce_time_ms, true), Button(sw_dn_right, debounce_time_ms, true)},
      led_status{led_status1, led_status2, led_status3, led_status4},
      led_bars{{led_bar_left_char1, led_bar_left_char2, led_bar_left_char3},
               {led_bar_right_char1, led_bar_right_char2, led_bar_right_char3}},

      buzzer_timer(buzzer_timer),
      buzzer_timer_channel(buzzer_timer_channel) {}

void ControlPanel::button_update() {
    for (size_t i = 0; i < num_buttons(); i++) {
        buttons[i].update();
    }
}

void ControlPanel::set_status_led(StatusLed led, bool val) {
    assert(led >= 0 && led < countof(led_status));
    set_status_led_blink(led, 0);
    led_blink_states[led].is_on = val;
}

void ControlPanel::set_status_led_blink(StatusLed led, uint32_t blink_time) {
    assert(led >= 0 && led < countof(led_status));
    led_blink_states[led].blink.last_change_ms = millis();
    led_blink_states[led].blink.time_setting_ms = blink_time;
}



void ControlPanel::set_led_bar_graph_blink(BarGraph bar, uint32_t blink_time) {
    assert(bar == BAR_GRAPH_RIGHT || bar == BAR_GRAPH_LEFT);
    BarState &bs = bar_states[bar];
    bs.blink.last_change_ms = millis();
    bs.blink.time_setting_ms = blink_time;
}

void ControlPanel::set_led_bar_graph(BarGraph bar, uint8_t level) {
    assert(bar == BAR_GRAPH_RIGHT || bar == BAR_GRAPH_LEFT);
    BarState &bs = bar_states[bar];
    level = saturate(level, 0, 6);
    if (level != bs.level) {
        bs.level = level;
        bs.current = 1;
    }
}

void ControlPanel::set_buzzer_tone(BuzzerTone tone) {
    uint16_t oldArr = __HAL_TIM_GET_AUTORELOAD(buzzer_timer);
    uint16_t newArr;

    switch (tone) {
        case BUZZER_C7:
            newArr = 2149;
            break;
        case BUZZER_E7:
            newArr = 1900;
            break;
        case BUZZER_G7:
            newArr = 1434;
            break;
        case BUZZER_C8:
            newArr = 1074;
            break;
        default:
            return;
    }

    __HAL_TIM_SET_AUTORELOAD(buzzer_timer, newArr);

    // update compare to keep duty cycle the same
    uint16_t oldCompare = __HAL_TIM_GET_COMPARE(buzzer_timer, buzzer_timer_channel);
    uint16_t newCompare = (uint16_t)((float)oldCompare * (float)newArr / (float)oldArr);
    __HAL_TIM_SET_COMPARE(buzzer_timer, buzzer_timer_channel, newCompare);
}

void ControlPanel::set_buzzer_volume(float volume) {
    if (volume > 1) volume = 1;
    if (volume < 0) volume = 0;
    uint16_t arr = __HAL_TIM_GET_AUTORELOAD(buzzer_timer);
    uint16_t compare = (uint16_t)(volume * 0.5 * (float)arr);
    __HAL_TIM_SET_COMPARE(buzzer_timer, buzzer_timer_channel, compare);
}

void ControlPanel::sound_buzzer(bool on) {
    if (on) {
        HAL_TIM_PWM_Start(buzzer_timer, buzzer_timer_channel);
        HAL_TIMEx_PWMN_Start(buzzer_timer, buzzer_timer_channel);
    } else {
        HAL_TIM_PWM_Stop(buzzer_timer, buzzer_timer_channel);
        HAL_TIMEx_PWMN_Stop(buzzer_timer, buzzer_timer_channel);
    }
}

bool ControlPanel::BlinkState::update() {
    if (time_setting_ms > 0) {
        if (time_since_ms(last_change_ms) > time_setting_ms) {
            last_change_ms = millis();
            is_on = !is_on;
        }
    } else {
        is_on = true;
    }
            return is_on;

}

void ControlPanel::update() {
    if (millis() > last_charlie_update_ms + CHARLIE_UPDATE_INTERVAL_MS) {
        last_charlie_update_ms = millis();

        for (size_t i = 0; i < countof(led_blink_states); i++) {
            if (!led_blink_states[i].blink.update()) {
                led_status[i]->write(false);
            } else {
                led_status[i]->write(led_blink_states[i].is_on);
            }
        }

        for (size_t i = 0; i <= BAR_GRAPH_RIGHT; i++) {
            if (!bar_states[i].blink.update()) {
                    bar_states[i].current = 0;
            }

            charlieplex(led_bars[i], bar_states[i].current++);

            if (bar_states[i].current > bar_states[i].level) {
                bar_states[i].current = bar_states[i].level > 0 ? 1 : 0;
            }
        }
    }
}

void ControlPanel::charlieplex(Pin *ios[3], int value) {
    assert(value >= 0 && value <= 6);

    uint32_t mode[3];
    GPIO_PinState out[3];

    switch (value) {
        default:
        case 0:
            mode[0] = GPIO_MODE_OUTPUT_PP;
            mode[1] = GPIO_MODE_OUTPUT_PP;
            mode[2] = GPIO_MODE_OUTPUT_PP;
            out[0] = GPIO_PIN_RESET;
            out[1] = GPIO_PIN_RESET;
            out[2] = GPIO_PIN_RESET;
            break;
        case 1:
            mode[0] = GPIO_MODE_OUTPUT_PP;
            mode[1] = GPIO_MODE_INPUT;
            mode[2] = GPIO_MODE_OUTPUT_PP;
            out[0] = GPIO_PIN_RESET;
            out[1] = GPIO_PIN_RESET;
            out[2] = GPIO_PIN_SET;
            break;
        case 2:
            mode[0] = GPIO_MODE_OUTPUT_PP;
            mode[1] = GPIO_MODE_INPUT;
            mode[2] = GPIO_MODE_OUTPUT_PP;
            out[0] = GPIO_PIN_SET;
            out[1] = GPIO_PIN_RESET;
            out[2] = GPIO_PIN_RESET;
            break;
        case 3:
            mode[0] = GPIO_MODE_INPUT;
            mode[1] = GPIO_MODE_OUTPUT_PP;
            mode[2] = GPIO_MODE_OUTPUT_PP;
            out[0] = GPIO_PIN_RESET;
            out[1] = GPIO_PIN_RESET;
            out[2] = GPIO_PIN_SET;
            break;
        case 4:
            mode[0] = GPIO_MODE_INPUT;
            mode[1] = GPIO_MODE_OUTPUT_PP;
            mode[2] = GPIO_MODE_OUTPUT_PP;
            out[0] = GPIO_PIN_RESET;
            out[1] = GPIO_PIN_SET;
            out[2] = GPIO_PIN_RESET;
            break;
        case 5:
            mode[0] = GPIO_MODE_OUTPUT_PP;
            mode[1] = GPIO_MODE_OUTPUT_PP;
            mode[2] = GPIO_MODE_INPUT;
            out[0] = GPIO_PIN_RESET;
            out[1] = GPIO_PIN_SET;
            out[2] = GPIO_PIN_RESET;
            break;
        case 6:
            mode[0] = GPIO_MODE_OUTPUT_PP;
            mode[1] = GPIO_MODE_OUTPUT_PP;
            mode[2] = GPIO_MODE_INPUT;
            out[0] = GPIO_PIN_SET;
            out[1] = GPIO_PIN_RESET;
            out[2] = GPIO_PIN_RESET;
            break;
    }

    GPIO_InitTypeDef init;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    for (size_t i = 0; i < 3; i++) {
        init.Pin = ios[i]->pin;
        init.Mode = mode[i];
        HAL_GPIO_Init(ios[i]->port, &init);
        if (mode[i] == GPIO_MODE_OUTPUT_PP) {
            ios[i]->write(out[i]);
        }
    }
}

Button const &ControlPanel::get_button(PanelButton i) const {
    assert(i >= 0 && i < num_buttons());
    return buttons[i];
}

Button *ControlPanel::get_button_ptr(PanelButton i) {
    assert(i >= 0 && i < num_buttons());
    return &buttons[i];
}

#include "control_panel.h"

#include <assert.h>

ControlPanel::ControlPanel(
      // BUTTONS
      GPIO_TypeDef *sw_start_mode_port, uint16_t sw_start_mode_pin, GPIO_TypeDef *sw_stop_port, uint16_t sw_stop_pin,
      GPIO_TypeDef *sw_up_left_port, uint16_t sw_up_left_pin, GPIO_TypeDef *sw_dn_left_port, uint16_t sw_dn_left_pin,
      GPIO_TypeDef *sw_up_right_port, uint16_t sw_up_right_pin, GPIO_TypeDef *sw_dn_right_port,
      uint16_t sw_dn_right_pin,

      // STATUS LEDS
      GPIO_TypeDef *led_status1_port, uint16_t led_status1_pin, GPIO_TypeDef *led_status2_port,
      uint16_t led_status2_pin, GPIO_TypeDef *led_status3_port, uint16_t led_status3_pin,
      GPIO_TypeDef *led_status4_port, uint16_t led_status4_pin,

      // CHARLIEPLEXED BAR GRAPH LEDS
      GPIO_TypeDef *led_bar_left_char1_port, uint16_t led_bar_left_char1_pin, GPIO_TypeDef *led_bar_left_char2_port,
      uint16_t led_bar_left_char2_pin, GPIO_TypeDef *led_bar_left_char3_port, uint16_t led_bar_left_char3_pin,
      GPIO_TypeDef *led_bar_right_char1_port, uint16_t led_bar_right_char1_pin, GPIO_TypeDef *led_bar_right_char2_port,
      uint16_t led_bar_right_char2_pin, GPIO_TypeDef *led_bar_right_char3_port, uint16_t led_bar_right_char3_pin,

      // BUZZER
      TIM_HandleTypeDef *buzzer_timer, uint32_t buzzer_timer_channel)
    : buttons{{sw_start_mode_port, sw_start_mode_pin}, {sw_stop_port, sw_stop_pin},
              {sw_up_left_port, sw_up_left_pin},       {sw_up_right_port, sw_up_right_pin},
              {sw_dn_left_port, sw_dn_right_pin},      {sw_dn_right_port, sw_dn_right_pin}},
      led_status{{led_status1_port, led_status1_pin},
                 {led_status2_port, led_status2_pin},
                 {led_status3_port, led_status3_pin},
                 {led_status4_port, led_status4_pin}},
      led_bar_left{{led_bar_left_char1_port, led_bar_left_char1_pin},
                   {led_bar_left_char2_port, led_bar_left_char2_pin},
                   {led_bar_left_char3_port, led_bar_left_char3_pin}},
      led_bar_right{{led_bar_right_char1_port, led_bar_right_char1_pin},
                    {led_bar_right_char2_port, led_bar_right_char2_pin},
                    {led_bar_right_char3_port, led_bar_right_char3_pin}},

      buzzer_timer(buzzer_timer),
      buzzer_timer_channel(buzzer_timer_channel) {}

bool ControlPanel::button_pressed(PanelButton btn) {
    assert(btn >= 0 && btn < NUM_PANEL_BUTTONS);

    return GPIO_PIN_RESET == buttons[btn].read();
}

bool ControlPanel::button_pressed_singleshot(PanelButton btn) {
    bool button_state = button_pressed(btn);
    bool ret_state = button_state && !prev_panel_button_state[btn];
    prev_panel_button_state[btn] = button_state;

    return ret_state;
}

void ControlPanel::set_status_led(StatusLed led, bool val) {
    assert(led >= 0 && led < sizeof(led_status));
    led_status[led].write(val);
}

void ControlPanel::set_led_bar_graph(BarGraph bar, uint8_t level) {
    assert(bar == BAR_GRAPH_RIGHT || bar == BAR_GRAPH_LEFT);
    BarState &bs = bar_states[bar];

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

void ControlPanel::update() {
    if (HAL_GetTick() > last_charlie_update_ms + CHARLIE_UPDATE_INTERVAL_MS) {
        last_charlie_update_ms = HAL_GetTick();

        charlieplex(led_bar_left, bar_states[BAR_GRAPH_LEFT].current++);
        charlieplex(led_bar_right, bar_states[BAR_GRAPH_RIGHT].current++);

        if (bar_states[BAR_GRAPH_LEFT].current > bar_states[BAR_GRAPH_LEFT].level) {
            bar_states[BAR_GRAPH_LEFT].current = bar_states[BAR_GRAPH_LEFT].level > 0 ? 1 : 0;
        }

        if (bar_states[BAR_GRAPH_RIGHT].current > bar_states[BAR_GRAPH_RIGHT].level) {
            bar_states[BAR_GRAPH_RIGHT].current = bar_states[BAR_GRAPH_RIGHT].level > 0 ? 1 : 0;
        }
    }
}

void ControlPanel::charlieplex(Pin ios[3], int value) {
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
        init.Pin = ios[i].pin;
        init.Mode = mode[i];
        HAL_GPIO_Init(ios[i].port, &init);
        if (mode[i] == GPIO_MODE_OUTPUT_PP) {
            ios[i].write(out[i]);
        }
    }
}
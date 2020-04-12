#include "control_panel.h"
#include <assert.h>

ControlPanel::ControlPanel(
    // BUTTONS
    GPIO_TypeDef *sw_start_mode_port,
    uint16_t sw_start_mode_pin,
    GPIO_TypeDef *sw_stop_port,
    uint16_t sw_stop_pin,
    GPIO_TypeDef *sw_up_left_port,
    uint16_t sw_up_left_pin,
    GPIO_TypeDef *sw_dn_left_port,
    uint16_t sw_dn_left_pin,
    GPIO_TypeDef *sw_up_right_port,
    uint16_t sw_up_right_pin,
    GPIO_TypeDef *sw_dn_right_port,
    uint16_t sw_dn_right_pin,

    // STATUS LEDS
    GPIO_TypeDef *led_status1_port,
    uint16_t led_status1_pin,
    GPIO_TypeDef *led_status2_port,
    uint16_t led_status2_pin,
    GPIO_TypeDef *led_status3_port,
    uint16_t led_status3_pin,
    GPIO_TypeDef *led_status4_port,
    uint16_t led_status4_pin,

    // CHARLIEPLEXED BAR GRAPH LEDS
    GPIO_TypeDef *led_bar_left_char1_port,
    uint16_t led_bar_left_char1_pin,
    GPIO_TypeDef *led_bar_left_char2_port,
    uint16_t led_bar_left_char2_pin,
    GPIO_TypeDef *led_bar_left_char3_port,
    uint16_t led_bar_left_char3_pin,
    GPIO_TypeDef *led_bar_right_char1_port,
    uint16_t led_bar_right_char1_pin,
    GPIO_TypeDef *led_bar_right_char2_port,
    uint16_t led_bar_right_char2_pin,
    GPIO_TypeDef *led_bar_right_char3_port,
    uint16_t led_bar_right_char3_pin,
    
    // BUZZER
    TIM_HandleTypeDef *buzzer_timer,
    uint32_t buzzer_timer_channel
) :
    sw_start_mode_port(sw_start_mode_port),
    sw_start_mode_pin(sw_start_mode_pin),
    sw_stop_port(sw_stop_port),
    sw_stop_pin(sw_stop_pin),
    sw_up_left_port(sw_up_left_port),
    sw_up_left_pin(sw_up_left_pin),
    sw_dn_left_port(sw_dn_left_port),
    sw_dn_left_pin(sw_dn_left_pin),
    sw_up_right_port(sw_up_right_port),
    sw_up_right_pin(sw_up_right_pin),
    sw_dn_right_port(sw_dn_right_port),
    sw_dn_right_pin(sw_dn_right_pin),
    led_status1_port(led_status1_port),
    led_status1_pin(led_status1_pin),
    led_status2_port(led_status2_port),
    led_status2_pin(led_status2_pin),
    led_status3_port(led_status3_port),
    led_status3_pin(led_status3_pin),
    led_status4_port(led_status4_port),
    led_status4_pin(led_status4_pin),
    led_bar_left_char1_port(led_bar_left_char1_port),
    led_bar_left_char1_pin(led_bar_left_char1_pin),
    led_bar_left_char2_port(led_bar_left_char2_port),
    led_bar_left_char2_pin(led_bar_left_char2_pin),
    led_bar_left_char3_port(led_bar_left_char3_port),
    led_bar_left_char3_pin(led_bar_left_char3_pin),
    led_bar_right_char1_port(led_bar_right_char1_port),
    led_bar_right_char1_pin(led_bar_right_char1_pin),
    led_bar_right_char2_port(led_bar_right_char2_port),
    led_bar_right_char2_pin(led_bar_right_char2_pin),
    led_bar_right_char3_port(led_bar_right_char3_port),
    led_bar_right_char3_pin(led_bar_right_char3_pin),
    buzzer_timer(buzzer_timer),
    buzzer_timer_channel(buzzer_timer_channel)
{}

bool ControlPanel::button_pressed(panel_button btn) {
    GPIO_TypeDef *port;
    uint16_t pin;

    switch (btn) {
        case START_MODE_BTN:
            port = sw_start_mode_port;
            pin = sw_start_mode_pin;
            break;
        case STOP_BTN:
            port = sw_stop_port;
            pin = sw_stop_pin;
            break;
        case UP_LEFT_BTN:
            port = sw_up_left_port;
            pin = sw_up_left_pin;
            break;
        case DN_LEFT_BTN:
            port = sw_dn_left_port;
            pin = sw_dn_left_pin;
            break;
        case UP_RIGHT_BTN:
            port = sw_up_right_port;
            pin = sw_up_right_pin;
            break;
        case DN_RIGHT_BTN:
            port = sw_dn_right_port;
            pin = sw_dn_right_pin;
            break;
        default:
            return false;
    }

    volatile GPIO_PinState state = HAL_GPIO_ReadPin(port, pin);

    return state == GPIO_PIN_RESET;
}

bool ControlPanel::button_pressed_singleshot(panel_button btn) {
    bool button_state = button_pressed(btn);
    bool ret_state = button_state && !prev_panel_button_state[btn];
    prev_panel_button_state[btn] = button_state;

    return ret_state;
}

void ControlPanel::set_status_led(status_led led, bool val) {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState state = val ? GPIO_PIN_SET : GPIO_PIN_RESET;

    switch (led) {
        case STATUS_LED_1:
            port = led_status1_port;
            pin = led_status1_pin;
            break;
        case STATUS_LED_2:
            port = led_status2_port;
            pin = led_status2_pin;
            break;
        case STATUS_LED_3:
            port = led_status3_port;
            pin = led_status3_pin;
            break;
        case STATUS_LED_4:
            port = led_status4_port;
            pin = led_status4_pin;
            break;
        default:
            return;
    }

    HAL_GPIO_WritePin(port, pin, state);
}

void ControlPanel::set_led_bar_graph(bar_graph bar, uint8_t level) {
    assert(bar == BAR_GRAPH_RIGHT || bar == BAR_GRAPH_LEFT);
    BarState &bs = (bar == BAR_GRAPH_RIGHT) ? right_bar : left_bar;

    if (level != bs.level) {
        bs.level = level;
        bs.current = 1;
    }
}

void ControlPanel::set_buzzer_tone(buzzer_tone tone) {
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

        charlieplex(
            led_bar_left_char1_port,
            led_bar_left_char1_pin,
            led_bar_left_char2_port,
            led_bar_left_char2_pin,
            led_bar_left_char3_port,
            led_bar_left_char3_pin,
            left_bar.current++
        );
        charlieplex(
            led_bar_right_char1_port,
            led_bar_right_char1_pin,
            led_bar_right_char2_port,
            led_bar_right_char2_pin,
            led_bar_right_char3_port,
            led_bar_right_char3_pin,
            right_bar.current++
        );

        if (left_bar.current > left_bar.level) {
            left_bar.current = left_bar.level > 0 ? 1 : 0;
        }

        if (right_bar.current > right_bar.level) {
            right_bar.current = right_bar.level > 0 ? 1 : 0;
        }
    }
}

void ControlPanel::charlieplex(
    GPIO_TypeDef *port1,
    uint16_t pin1,
    GPIO_TypeDef *port2,
    uint16_t pin2,
    GPIO_TypeDef *port3,
    uint16_t pin3,
    int value
) {
    uint32_t mode1, mode2, mode3;
    GPIO_PinState out1, out2, out3;

    switch (value) {
        default:
        case 0:
            mode1 = GPIO_MODE_OUTPUT_PP;
            mode2 = GPIO_MODE_OUTPUT_PP;
            mode3 = GPIO_MODE_OUTPUT_PP;
            out1 = GPIO_PIN_RESET;
            out2 = GPIO_PIN_RESET;
            out3 = GPIO_PIN_RESET;
            break;
        case 1:
            mode1 = GPIO_MODE_OUTPUT_PP;
            mode2 = GPIO_MODE_INPUT;
            mode3 = GPIO_MODE_OUTPUT_PP;
            out1 = GPIO_PIN_RESET;
            out2 = GPIO_PIN_RESET;
            out3 = GPIO_PIN_SET;
            break;
        case 2:
            mode1 = GPIO_MODE_OUTPUT_PP;
            mode2 = GPIO_MODE_INPUT;
            mode3 = GPIO_MODE_OUTPUT_PP;
            out1 = GPIO_PIN_SET;
            out2 = GPIO_PIN_RESET;
            out3 = GPIO_PIN_RESET;
            break;
        case 3:
            mode1 = GPIO_MODE_INPUT;
            mode2 = GPIO_MODE_OUTPUT_PP;
            mode3 = GPIO_MODE_OUTPUT_PP;
            out1 = GPIO_PIN_RESET;
            out2 = GPIO_PIN_RESET;
            out3 = GPIO_PIN_SET;
            break;
        case 4:
            mode1 = GPIO_MODE_INPUT;
            mode2 = GPIO_MODE_OUTPUT_PP;
            mode3 = GPIO_MODE_OUTPUT_PP;
            out1 = GPIO_PIN_RESET;
            out2 = GPIO_PIN_SET;
            out3 = GPIO_PIN_RESET;
            break;
        case 5:
            mode1 = GPIO_MODE_OUTPUT_PP;
            mode2 = GPIO_MODE_OUTPUT_PP;
            mode3 = GPIO_MODE_INPUT;
            out1 = GPIO_PIN_RESET;
            out2 = GPIO_PIN_SET;
            out3 = GPIO_PIN_RESET;
            break;
        case 6:
            mode1 = GPIO_MODE_OUTPUT_PP;
            mode2 = GPIO_MODE_OUTPUT_PP;
            mode3 = GPIO_MODE_INPUT;
            out1 = GPIO_PIN_SET;
            out2 = GPIO_PIN_RESET;
            out3 = GPIO_PIN_RESET;
            break;    
    }

    GPIO_InitTypeDef init;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;

    init.Pin = pin1;
    init.Mode = mode1;
    HAL_GPIO_Init(port1, &init);

    init.Pin = pin2;
    init.Mode = mode2;
    HAL_GPIO_Init(port2, &init);

    init.Pin = pin3;
    init.Mode = mode3;
    HAL_GPIO_Init(port3, &init);

    if (mode1 == GPIO_MODE_OUTPUT_PP) {
        HAL_GPIO_WritePin(port1, pin1, out1);
    }
    if (mode2 == GPIO_MODE_OUTPUT_PP) {
        HAL_GPIO_WritePin(port2, pin2, out2);
    }
    if (mode3 == GPIO_MODE_OUTPUT_PP) {
        HAL_GPIO_WritePin(port3, pin3, out3);
    }
}
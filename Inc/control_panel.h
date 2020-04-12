#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include "stm32f1xx_hal.h"

class ControlPanel {
public:
    ControlPanel(
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
    );

    enum panel_button {
        START_MODE_BTN,
        STOP_BTN,
        UP_LEFT_BTN,
        DN_LEFT_BTN,
        UP_RIGHT_BTN,
        DN_RIGHT_BTN,
    };

    enum status_led {
        STATUS_LED_1,
        STATUS_LED_2,
        STATUS_LED_3,
        STATUS_LED_4,
    };

    enum bar_graph {
        BAR_GRAPH_LEFT,
        BAR_GRAPH_RIGHT,
    };

    enum bar_graph_level {
        OFF = 0,
        LEVEL_1,
        LEVEL_2,
        LEVEL_3,
        LEVEL_4,
        LEVEL_5,
        LEVEL_6,
    };

    enum buzzer_tone {
        BUZZER_C7,  // 2093Hz (~63dB)
        BUZZER_E7,  // 2637Hz (~68dB)
        BUZZER_G7,  // 3136Hz (~78dB)
        BUZZER_C8,  // 4186Hz (~85dB) (default)
    };

    struct BarState {
        int level;
        int current;
    };
    bool button_pressed(panel_button btn);

    void set_status_led(status_led led, bool val);
    void set_led_bar_graph(bar_graph bar, uint8_t level);

    void set_buzzer_tone(buzzer_tone tone);
    void set_buzzer_volume(float volume);  // volume between 0 and 1
    void sound_buzzer(bool on);

    void update();

private:
    static constexpr uint32_t CHARLIE_UPDATE_INTERVAL_MS = 1;

    uint32_t last_charlie_update_ms;

    // BUTTONS
    GPIO_TypeDef *sw_start_mode_port;
    uint16_t sw_start_mode_pin;
    GPIO_TypeDef *sw_stop_port;
    uint16_t sw_stop_pin;
    GPIO_TypeDef *sw_up_left_port;
    uint16_t sw_up_left_pin;
    GPIO_TypeDef *sw_dn_left_port;
    uint16_t sw_dn_left_pin;
    GPIO_TypeDef *sw_up_right_port;
    uint16_t sw_up_right_pin;
    GPIO_TypeDef *sw_dn_right_port;
    uint16_t sw_dn_right_pin;

    // STATUS LEDS
    GPIO_TypeDef *led_status1_port;
    uint16_t led_status1_pin;
    GPIO_TypeDef *led_status2_port;
    uint16_t led_status2_pin;
    GPIO_TypeDef *led_status3_port;
    uint16_t led_status3_pin;
    GPIO_TypeDef *led_status4_port;
    uint16_t led_status4_pin;

    // CHARLIEPLEXED BAR GRAPH LEDS
    GPIO_TypeDef *led_bar_left_char1_port;
    uint16_t led_bar_left_char1_pin;
    GPIO_TypeDef *led_bar_left_char2_port;
    uint16_t led_bar_left_char2_pin;
    GPIO_TypeDef *led_bar_left_char3_port;
    uint16_t led_bar_left_char3_pin;
    GPIO_TypeDef *led_bar_right_char1_port;
    uint16_t led_bar_right_char1_pin;
    GPIO_TypeDef *led_bar_right_char2_port;
    uint16_t led_bar_right_char2_pin;
    GPIO_TypeDef *led_bar_right_char3_port;
    uint16_t led_bar_right_char3_pin;
    
    // BUZZER
    TIM_HandleTypeDef *buzzer_timer;
    uint32_t buzzer_timer_channel;

    bar_graph_level bar_left_level;
    bar_graph_level bar_right_level;

    BarState left_bar;
    BarState right_bar;

    void charlieplex(
        GPIO_TypeDef *port1,
        uint16_t pin1,
        GPIO_TypeDef *port2,
        uint16_t pin2,
        GPIO_TypeDef *port3,
        uint16_t pin3,
        int value
    );
};

#endif

#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include "drivers/pin.h"
#include "platform.h"

class ControlPanel {
public:
    ControlPanel(
          // BUTTONS
          GPIO_TypeDef *sw_start_mode_port, uint16_t sw_start_mode_pin, GPIO_TypeDef *sw_stop_port,
          uint16_t sw_stop_pin, GPIO_TypeDef *sw_up_left_port, uint16_t sw_up_left_pin, GPIO_TypeDef *sw_dn_left_port,
          uint16_t sw_dn_left_pin, GPIO_TypeDef *sw_up_right_port, uint16_t sw_up_right_pin,
          GPIO_TypeDef *sw_dn_right_port, uint16_t sw_dn_right_pin,

          // STATUS LEDS
          GPIO_TypeDef *led_status1_port, uint16_t led_status1_pin, GPIO_TypeDef *led_status2_port,
          uint16_t led_status2_pin, GPIO_TypeDef *led_status3_port, uint16_t led_status3_pin,
          GPIO_TypeDef *led_status4_port, uint16_t led_status4_pin,

          // CHARLIEPLEXED BAR GRAPH LEDS
          GPIO_TypeDef *led_bar_left_char1_port, uint16_t led_bar_left_char1_pin, GPIO_TypeDef *led_bar_left_char2_port,
          uint16_t led_bar_left_char2_pin, GPIO_TypeDef *led_bar_left_char3_port, uint16_t led_bar_left_char3_pin,
          GPIO_TypeDef *led_bar_right_char1_port, uint16_t led_bar_right_char1_pin,
          GPIO_TypeDef *led_bar_right_char2_port, uint16_t led_bar_right_char2_pin,
          GPIO_TypeDef *led_bar_right_char3_port, uint16_t led_bar_right_char3_pin,

          // BUZZER
          TIM_HandleTypeDef *buzzer_timer, uint32_t buzzer_timer_channel);

    enum PanelButton {
        START_MODE_BTN,
        STOP_BTN,
        UP_LEFT_BTN,
        DN_LEFT_BTN,
        UP_RIGHT_BTN,
        DN_RIGHT_BTN,
        // Add new buttons here
        NUM_PANEL_BUTTONS  // ! Keep this at the end
    };

    enum StatusLed {
        STATUS_LED_1,
        STATUS_LED_2,
        STATUS_LED_3,
        STATUS_LED_4,
    };

    enum BarGraph {
        BAR_GRAPH_LEFT,
        BAR_GRAPH_RIGHT,
    };

    enum BuzzerTone {
        BUZZER_C7,  // 2093Hz (~63dB)
        BUZZER_E7,  // 2637Hz (~68dB)
        BUZZER_G7,  // 3136Hz (~78dB)
        BUZZER_C8,  // 4186Hz (~85dB) (default)
    };

    struct BarState {
        int level;
        int current;
    };

    bool button_pressed(PanelButton btn);
    bool button_pressed_singleshot(PanelButton btn);

    void set_status_led(StatusLed led, bool val);
    void set_led_bar_graph(BarGraph bar, uint8_t level);

    void set_buzzer_tone(BuzzerTone tone);
    void set_buzzer_volume(float volume);  // volume between 0 and 1
    void sound_buzzer(bool on);

    void update();

private:
    static constexpr uint32_t CHARLIE_UPDATE_INTERVAL_MS = 1;

    uint32_t last_charlie_update_ms;

    // BUTTONS
    Pin buttons[NUM_PANEL_BUTTONS];

    // STATUS LEDS
    Pin led_status[4];

    // CHARLIEPLEXED BAR GRAPH LEDS
    Pin led_bar_left[3];
    Pin led_bar_right[3];

    // BUZZER
    TIM_HandleTypeDef *buzzer_timer;
    uint32_t buzzer_timer_channel;

    bool prev_panel_button_state[NUM_PANEL_BUTTONS];

    BarState bar_states[2];

    void charlieplex(Pin ios[3], int value);
};

#endif

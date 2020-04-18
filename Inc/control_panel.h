#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include "drivers/pin.h"
#include "platform.h"
#include "ui/button.h"

class ControlPanel {
public:
    ControlPanel(Pin *sw_start, Pin *sw_stop, Pin *sw_up_left, Pin *sw_dn_left, Pin *sw_up_right, Pin *sw_dn_right,

                 // Status Leds
                 Pin *led_status1, Pin *led_status2, Pin *led_status3, Pin *led_status4,

                 // Charlieplex bar graph
                 Pin *led_bar_left_char1, Pin *led_bar_left_char2, Pin *led_bar_left_char3, Pin *led_bar_right_char1,
                 Pin *led_bar_right_char2, Pin *led_bar_right_char3,

                 // BUZZER
                 TIM_HandleTypeDef *buzzer_timer, uint32_t buzzer_timer_channel, uint32_t debounce_time_ms = 10);

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

    Button::Event button_update(PanelButton btn);
    static constexpr size_t num_buttons() {
        return static_cast<size_t>(PanelButton::NUM_PANEL_BUTTONS);
    }
    Button *get_buttons(size_t i);

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
    Button buttons[NUM_PANEL_BUTTONS];

    // STATUS LEDS
    Pin *led_status[4];

    // CHARLIEPLEXED BAR GRAPH LEDS
    Pin *led_bar_left[3];
    Pin *led_bar_right[3];

    // BUZZER
    TIM_HandleTypeDef *buzzer_timer;
    uint32_t buzzer_timer_channel;

    bool prev_panel_button_state[NUM_PANEL_BUTTONS];

    BarState bar_states[2];

    void charlieplex(Pin *ios[3], int value);
};

#endif

#include "factory/tests.h"
#include "control_panel.h"
#include "clock.h"

void control_panel_self_test(ControlPanel &controls) {
    controls.set_status_led(ControlPanel::STATUS_LED_1, true);
    controls.set_status_led(ControlPanel::STATUS_LED_2, true);
    controls.set_status_led(ControlPanel::STATUS_LED_3, true);
    controls.set_status_led(ControlPanel::STATUS_LED_4, true);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, 1);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, 6);

    controls.set_buzzer_tone(ControlPanel::BUZZER_C7);
    controls.set_buzzer_volume(0.8);
    controls.sound_buzzer(true);
    delay_ms(100);
    controls.set_buzzer_tone(ControlPanel::BUZZER_G7);
    delay_ms(100);
    controls.set_buzzer_tone(ControlPanel::BUZZER_E7);
    delay_ms(100);
    controls.set_buzzer_tone(ControlPanel::BUZZER_C8);
    delay_ms(100);
    controls.sound_buzzer(false);

    controls.set_status_led(ControlPanel::STATUS_LED_1, false);
    controls.set_status_led(ControlPanel::STATUS_LED_2, false);
    controls.set_status_led(ControlPanel::STATUS_LED_3, false);
    controls.set_status_led(ControlPanel::STATUS_LED_4, false);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, 0);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, 0);
}
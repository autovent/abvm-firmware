#include "abvm.h"
#include "drv8873.h"
#include "ads1231.h"
#include "encoder.h"
#include "lc064.h"
#include "record_store.h"
#include "usb_comm.h"
#include "control_panel.h"
#include "main.h"
#include "spi.h"
#include "i2c.h"
#include "tim.h"

ADS1231 pressure_sensor(
    ADC1_PWRDN_GPIO_Port,
    ADC1_PWRDN_Pin,
    &hspi1,
    ADC_SPI_MISO_GPIO_Port,
    ADC_SPI_MISO_Pin,
    ADC_SPI_SCK_GPIO_Port,
    ADC_SPI_SCK_Pin
);

ADS1231 load_cell(
    ADC2_PWRDN_GPIO_Port,
    ADC2_PWRDN_Pin,
    &hspi1,
    ADC_SPI_MISO_GPIO_Port,
    ADC_SPI_MISO_Pin,
    ADC_SPI_SCK_GPIO_Port,
    ADC_SPI_SCK_Pin
);

DRV8873 motor_driver(
    MC_SLEEP_GPIO_Port,
    MC_SLEEP_Pin,
    MC_DISABLE_GPIO_Port,
    MC_DISABLE_Pin,
    MC_FAULT_GPIO_Port,
    MC_FAULT_Pin,
    &htim2,
    TIM_CHANNEL_1,
    TIM_CHANNEL_3,
    &hspi2,
    MC_SPI_CS_GPIO_Port,
    MC_SPI_CS_Pin
);

Encoder encoder(&htim4);

USBComm usb_comm;

ControlPanel controls(
    SW_START_GPIO_Port,
    SW_START_Pin,
    SW_STOP_GPIO_Port,
    SW_STOP_Pin,
    SW_VOL_UP_GPIO_Port,
    SW_VOL_UP_Pin,
    SW_VOL_DN_GPIO_Port,
    SW_VOL_DN_Pin,
    SW_RATE_UP_GPIO_Port,
    SW_RATE_UP_Pin,
    SW_RATE_DN_GPIO_Port,
    SW_RATE_DN_Pin,
    LED_POWER_GPIO_Port,
    LED_POWER_Pin,
    LED_FAULT_GPIO_Port,
    LED_FAULT_Pin,
    LED_IN_GPIO_Port,
    LED_IN_Pin,
    LED_OUT_GPIO_Port,
    LED_OUT_Pin,
    VOL_CHAR_1_GPIO_Port,
    VOL_CHAR_1_Pin,
    VOL_CHAR_2_GPIO_Port,
    VOL_CHAR_2_Pin,
    VOL_CHAR_3_GPIO_Port,
    VOL_CHAR_3_Pin,
    RATE_CHAR_1_GPIO_Port,
    RATE_CHAR_1_Pin,
    RATE_CHAR_2_GPIO_Port,
    RATE_CHAR_2_Pin,
    RATE_CHAR_3_GPIO_Port,
    RATE_CHAR_3_Pin,
    &htim1,
    TIM_CHANNEL_1
);

LC064 eeprom(&hi2c1, 0);

RecordStore record_store(&eeprom);

void control_panel_self_test() {
    controls.set_status_led(ControlPanel::STATUS_LED_1, true);
    controls.set_status_led(ControlPanel::STATUS_LED_2, true);
    controls.set_status_led(ControlPanel::STATUS_LED_3, true);
    controls.set_status_led(ControlPanel::STATUS_LED_4, true);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, ControlPanel::LEVEL_1);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, ControlPanel::LEVEL_6);

    controls.set_buzzer_tone(ControlPanel::BUZZER_C7);
    controls.set_buzzer_volume(0.01);
    controls.sound_buzzer(true);
    HAL_Delay(100);
    controls.set_buzzer_tone(ControlPanel::BUZZER_G7);
    HAL_Delay(100);
    controls.set_buzzer_tone(ControlPanel::BUZZER_E7);
    HAL_Delay(100);
    controls.set_buzzer_tone(ControlPanel::BUZZER_C8);
    HAL_Delay(100);
    controls.sound_buzzer(false);

    controls.set_status_led(ControlPanel::STATUS_LED_1, false);
    controls.set_status_led(ControlPanel::STATUS_LED_2, false);
    controls.set_status_led(ControlPanel::STATUS_LED_3, false);
    controls.set_status_led(ControlPanel::STATUS_LED_4, false);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, ControlPanel::OFF);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, ControlPanel::OFF);
}

extern "C"
void abvm_init() {
    encoder.reset();
    usb_comm.setAsCDCConsumer();
    usb_comm.sendf("AutoVENT ABVM (autovent.org)\n");

    // load_cell.init();
    
    control_panel_self_test();

    motor_driver.init();
    motor_driver.set_pwm_enabled(false);
    motor_driver.set_sleep(true);
    motor_driver.set_disabled(true);

    encoder.init();

    eeprom.init();
    volatile bool success = false;
    success = record_store.init();
    uint32_t test_data = 1;
    success = record_store.add_entry("test_entry", &test_data, sizeof(test_data), sizeof(test_data));
    success = record_store.first_load();

    success = record_store.load("test_entry", false);
    test_data++;
    success = record_store.store("test_entry");

    // load_cell.set_powerdown(false);

}

uint32_t last = 0;
uint32_t interval = 1000;

extern "C"
void abvm_update() {
    // pressure_sensor.update();
    // load_cell.update();
    controls.update();

    if (HAL_GetTick() > last + interval) {
        last = HAL_GetTick();
    }
}

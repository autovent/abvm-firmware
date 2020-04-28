#include "abvm.h"

#include <string.h>

#include "ads1231.h"
#include "bootloader.h"
#include "clock.h"
#include "config.h"
#include "control_panel.h"
#include "controls/trapezoidal_planner.h"
#include "data_logger.h"
#include "drivers/pin.h"
#include "drv8873.h"
#include "encoder.h"
#include "homing_controller.h"
#include "i2c.h"
#include "lc064.h"
#include "main.h"
#include "record_store.h"
#include "servo.h"
#include "serial_comm.h"
#include "spi.h"
#include "tim.h"
#include "usb_comm.h"
#include "ventilator_controller.h"

uint8_t HW_REVISION = 1;

ADS1231 pressure_sensor(ADC2_PWRDN_GPIO_Port, ADC2_PWRDN_Pin, &hspi1, ADC_SPI_MISO_GPIO_Port, ADC_SPI_MISO_Pin,
                        ADC_SPI_SCK_GPIO_Port, ADC_SPI_SCK_Pin, 1.0f / (6.8948 * .0005),
                        (1 / (6.8948 * .0005)) * -0.00325f);

DRV8873 motor_driver(MC_SLEEP_GPIO_Port, MC_SLEEP_Pin, MC_DISABLE_GPIO_Port, MC_DISABLE_Pin, MC_FAULT_GPIO_Port,
                     MC_FAULT_Pin, &htim2, TIM_CHANNEL_1, TIM_CHANNEL_3, &hspi2, MC_SPI_CS_GPIO_Port, MC_SPI_CS_Pin,
                     false);

Encoder encoder(&htim4);

USBComm usb_comm;

ControlPanel controls(SW_START_GPIO_Port, SW_START_Pin, SW_STOP_GPIO_Port, SW_STOP_Pin, SW_VOL_UP_GPIO_Port,
                      SW_VOL_UP_Pin, SW_VOL_DN_GPIO_Port, SW_VOL_DN_Pin, SW_RATE_UP_GPIO_Port, SW_RATE_UP_Pin,
                      SW_RATE_DN_GPIO_Port, SW_RATE_DN_Pin, LED_POWER_GPIO_Port, LED_POWER_Pin, LED_FAULT_GPIO_Port,
                      LED_FAULT_Pin, LED_IN_GPIO_Port, LED_IN_Pin, LED_OUT_GPIO_Port, LED_OUT_Pin, VOL_CHAR_1_GPIO_Port,
                      VOL_CHAR_1_Pin, VOL_CHAR_2_GPIO_Port, VOL_CHAR_2_Pin, VOL_CHAR_3_GPIO_Port, VOL_CHAR_3_Pin,
                      RATE_CHAR_1_GPIO_Port, RATE_CHAR_1_Pin, RATE_CHAR_2_GPIO_Port, RATE_CHAR_2_Pin,
                      RATE_CHAR_3_GPIO_Port, RATE_CHAR_3_Pin, &htim1, TIM_CHANNEL_1);

TrapezoidalPlanner motion({.5, .5}, 10);

Servo motor(1, &motor_driver, &encoder, kMotorParams, kMotorVelPidParams, kMotorVelLimits, kMotorPosPidParams,
            kMotorPosLimits);

VentilatorController vent(&motion, &motor);
Pin homing_switch{LIMIT2_GPIO_Port, LIMIT2_Pin};
HomingController home(&motor, &homing_switch);

LC064 eeprom(&hi2c1, 0);
RecordStore record_store(&eeprom);

CommEndpoint hw_revision_endpoint(0, &HW_REVISION, sizeof(HW_REVISION), true);

DataLogger logger(10, &pressure_sensor, &motor, &motor_driver, &vent);
ConfigCommandRPC config_cmd(100, &record_store);

CommEndpoint *comm_endpoints[] = {
    &hw_revision_endpoint,
    &logger,
    &config_cmd,
};

SerialComm ser_comm(comm_endpoints, sizeof(comm_endpoints)/sizeof(comm_endpoints[0]), &usb_comm);

USBComm::packet_handler packet_handlers[] = {
    {SerialComm::packet_callback, &ser_comm},
};

void control_panel_self_test() {
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

extern "C" void abvm_init() {
    // BootLoader::set_next_boot(BootLoader::BOOT_SELECT_APP);

    encoder.reset();
    usb_comm.set_as_cdc_consumer();
    usb_comm.set_packet_handlers(packet_handlers, 1);

    pressure_sensor.init();
    pressure_sensor.set_powerdown(false);

    // control_panel_self_test();

    motor_driver.init();
    motor_driver.set_pwm_enabled(true);
    motor_driver.set_sleep(false);
    motor_driver.set_disabled(false);

    encoder.init();

    motor.init();

    motor_driver.set_reg(0x5, 4 << 2 | 3);
    controls.set_status_led(ControlPanel::STATUS_LED_1, true);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, 1);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, 1);
    home.start();

    logger.set_streaming(20);
}

uint32_t last = 0;
uint32_t last_motor = 0;
uint32_t last_motion = 0;
uint32_t interval = 1000;
uint32_t motor_interval = 1;

uint32_t stop_pressed = 0;
uint32_t start_pressed = 0;
uint32_t debounce_intvl = 10;

extern "C" void abvm_update() {
    controls.update();
    ser_comm.update();

    if (millis() > last_motor + motor_interval) {
        motor.update();
        last_motor = millis();
    }

    if (millis() > last_motion + 10) {
        pressure_sensor.update();

        if (!home.is_done()) {
            home.update();
            if (home.is_done()) {
                motor.set_pos_deg(0);
                motor_driver.set_pwm(0);
                vent.reset();
                vent.start();
                vent.update();
            }
        } else {
            vent.update();
        }

        last_motion = millis();
    }

    // if (HAL_GetTick() > last + 20) {
    //     float pressure = pressure_sensor.read();
    //     static char data[128];
    //     snprintf(data, sizeof(data),
    //              "%1.3f,"
    //              "%1.3f,"
    //              "%1.3f,"
    //              "%1.3f,"
    //              "%1.3f,"
    //              "%1.3f,"
    //              "%1.3f,"
    //              "%1.0f,"
    //              "%1.0f,"
    //              "%1.0f,"
    //              "%lu\r\n",
                //  msec_to_sec(HAL_GetTick()), psi_to_cmH2O(pressure), motor.velocity, motor.target_velocity,
    //              motor.position, motor.target_pos, motor_driver.get_current(), vent.get_rate(), vent.get_closed_pos(),
    //              vent.get_open_pos(), motor.faults.to_int());
    //     usb_comm.send((uint8_t *)data, strlen(data));
    //     last = HAL_GetTick();
    // }

    if (controls.button_pressed_singleshot(ControlPanel::START_MODE_BTN)) {
        vent.is_operational = true;
        controls.set_status_led(ControlPanel::STATUS_LED_2, true);
    }

    if (controls.button_pressed_singleshot(ControlPanel::STOP_BTN)) {
        vent.stop();
        controls.set_status_led(ControlPanel::STATUS_LED_2, false);
    }

    if (controls.button_pressed_singleshot(ControlPanel::UP_LEFT_BTN)) {
        vent.bump_tv(1);
        controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, vent.get_tv_idx() + 1);
    }

    if (controls.button_pressed_singleshot(ControlPanel::DN_LEFT_BTN)) {
        vent.bump_tv(-1);
        controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, vent.get_tv_idx() + 1);
    }

    if (controls.button_pressed_singleshot(ControlPanel::UP_RIGHT_BTN)) {
        vent.bump_rate(1);
        controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, vent.get_rate_idx() + 1);
    }

    if (controls.button_pressed_singleshot(ControlPanel::DN_RIGHT_BTN)) {
        vent.bump_rate(-1);
        controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, vent.get_rate_idx() + 1);
    }
}

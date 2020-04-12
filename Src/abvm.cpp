#include "abvm.h"

#include "ads1231.h"
#include "control_panel.h"
#include "controls/trapezoidal_planner.h"
#include "drv8873.h"
#include "encoder.h"
#include "lc064.h"
#include "record_store.h"
#include "main.h"
#include "motor.h"
#include "spi.h"
#include "i2c.h"
#include "tim.h"
#include "usb_comm.h"

#define MOTOR_GOBILDA_30RPM
#define CONFIG_LONG_SPIRIT_FINGERS
#include "config.h"

ADS1231 pressure_sensor(ADC1_PWRDN_GPIO_Port, ADC1_PWRDN_Pin, &hspi1,
                        ADC_SPI_MISO_GPIO_Port, ADC_SPI_MISO_Pin,
                        ADC_SPI_SCK_GPIO_Port, ADC_SPI_SCK_Pin);

ADS1231 load_cell(ADC2_PWRDN_GPIO_Port, ADC2_PWRDN_Pin, &hspi1,
                  ADC_SPI_MISO_GPIO_Port, ADC_SPI_MISO_Pin,
                  ADC_SPI_SCK_GPIO_Port, ADC_SPI_SCK_Pin);

DRV8873 motor_driver(MC_SLEEP_GPIO_Port, MC_SLEEP_Pin, MC_DISABLE_GPIO_Port,
                     MC_DISABLE_Pin, MC_FAULT_GPIO_Port, MC_FAULT_Pin, &htim2,
                     TIM_CHANNEL_1, TIM_CHANNEL_3, &hspi2, MC_SPI_CS_GPIO_Port,
                     MC_SPI_CS_Pin, false);

Encoder encoder(&htim4);

USBComm usb_comm;

ControlPanel controls(
    SW_START_GPIO_Port, SW_START_Pin, SW_STOP_GPIO_Port, SW_STOP_Pin,
    SW_VOL_UP_GPIO_Port, SW_VOL_UP_Pin, SW_VOL_DN_GPIO_Port, SW_VOL_DN_Pin,
    SW_RATE_UP_GPIO_Port, SW_RATE_UP_Pin, SW_RATE_DN_GPIO_Port, SW_RATE_DN_Pin,
    LED_POWER_GPIO_Port, LED_POWER_Pin, LED_FAULT_GPIO_Port, LED_FAULT_Pin,
    LED_IN_GPIO_Port, LED_IN_Pin, LED_OUT_GPIO_Port, LED_OUT_Pin,
    VOL_CHAR_1_GPIO_Port, VOL_CHAR_1_Pin, VOL_CHAR_2_GPIO_Port, VOL_CHAR_2_Pin,
    VOL_CHAR_3_GPIO_Port, VOL_CHAR_3_Pin, RATE_CHAR_1_GPIO_Port,
    RATE_CHAR_1_Pin, RATE_CHAR_2_GPIO_Port, RATE_CHAR_2_Pin,
    RATE_CHAR_3_GPIO_Port, RATE_CHAR_3_Pin, &htim1, TIM_CHANNEL_1);

LC064 eeprom(&hi2c1, 0);

RecordStore record_store(&eeprom);

void control_panel_self_test() {
    controls.set_status_led(ControlPanel::STATUS_LED_1, true);
    controls.set_status_led(ControlPanel::STATUS_LED_2, true);
    controls.set_status_led(ControlPanel::STATUS_LED_3, true);
    controls.set_status_led(ControlPanel::STATUS_LED_4, true);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT,
                                ControlPanel::LEVEL_1);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT,
                                ControlPanel::LEVEL_6);

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

TrapezoidalPlanner motion({.333, .333});

Motor motor(2, &motor_driver, &encoder, kMotorParams, kMotorVelPidParams,
            kMotorVelLimits, kMotorPosPidParams, kMotorPosLimits);
extern "C" void abvm_init() {
    encoder.reset();
    usb_comm.setAsCDCConsumer();
    usb_comm.sendf("AutoVENT ABVM (autovent.org)\n");

    // load_cell.init();

    control_panel_self_test();

    motor_driver.init();
    motor_driver.set_pwm_enabled(true);
    motor_driver.set_sleep(false);
    motor_driver.set_disabled(false);

    encoder.init();

    motor.init();
    motion.set_next({90, 4000.0f});
    // load_cell.set_powerdown(false);
}

uint32_t last = 0;
uint32_t last_motor = 0;
uint32_t last_motion= 0;
uint32_t interval = 1000;
uint32_t motor_interval = 2;

uint32_t stop_pressed = 0;
uint32_t start_pressed = 0;
uint32_t debounce_intvl = 10;

extern "C" void abvm_update() {
    // pressure_sensor.update();
    // load_cell.update();
    controls.update();

    // volatile float x = load_cell.read();

    static float speed = 0;
    static float dir = 1;
    if (HAL_GetTick() > last_motor + motor_interval) {
        motor.update();
        last_motor = HAL_GetTick();
    }
    if (HAL_GetTick() > last_motion + 10) {
        motor.set_pos(deg_to_rad(motion.run(motion.p_last)));
        last_motion = HAL_GetTick();
    }

    if (HAL_GetTick() > last + interval) {
        volatile uint8_t reg1, reg2, reg3, reg4, reg5, reg6;
        reg1 = motor_driver.get_reg(DRV8873_REG_FAULT_STATUS);
        reg2 = motor_driver.get_reg(DRV8873_REG_DIAG_STATUS);
        reg3 = motor_driver.get_reg(DRV8873_REG_IC1_CONTROL);
        reg4 = motor_driver.get_reg(DRV8873_REG_IC2_CONTROL);
        reg5 = motor_driver.get_reg(DRV8873_REG_IC3_CONTROL);
        reg6 = motor_driver.get_reg(DRV8873_REG_IC4_CONTROL);
        last = HAL_GetTick();

        // int16_t counts = encoder.get();
        // usb_comm.sendf("Encoder counts in last %.1fs: %d",
        //                float(HAL_GetTick() - last) / 1000.0f, counts);
        // encoder.reset();
        // speed += dir * .1;
        // if (speed >= .8) {
        //   dir = -1;
        // } else if (speed <= -.8) {
        //   dir = 1;
        // }

        // motor.set_pos(M_PI / 4);
        //   motion.set_next({M_PI, 4000});

    }

    if (controls.button_pressed(ControlPanel::START_MODE_BTN)) {
        // motor_driver.set_pwm(0.2);
    } else {
        // motor_driver.set_pwm(speed);
    }
}

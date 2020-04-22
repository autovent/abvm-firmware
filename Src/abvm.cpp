#include "abvm.h"

#include <string.h>

#include "ads1231.h"
#include "bootloader.h"
#include "clock.h"
#include "config.h"
#include "control_panel.h"
#include "controls/trapezoidal_planner.h"
#include "drivers/pin.h"
#include "drv8873.h"
#include "encoder.h"
#include "homing_controller.h"
#include "i2c.h"
#include "lc064.h"
#include "main.h"
#include "record_store.h"
#include "servo.h"
#include "spi.h"
#include "tim.h"
#include "ui/ui_v1.h"
#include "usb_comm.h"
#include "ventilator_controller.h"

ADS1231 pressure_sensor(ADC1_PWRDN_GPIO_Port, ADC1_PWRDN_Pin, &hspi1, ADC_SPI_MISO_GPIO_Port, ADC_SPI_MISO_Pin,
                        ADC_SPI_SCK_GPIO_Port, ADC_SPI_SCK_Pin, 1.0f / (6.8948 * .00054),
                        -.09 + (1 / (6.8948 /*kPA/psi*/ * .00054)) * -0.00325f);

DRV8873 motor_driver(MC_SLEEP_GPIO_Port, MC_SLEEP_Pin, MC_DISABLE_GPIO_Port, MC_DISABLE_Pin, MC_FAULT_GPIO_Port,
                     MC_FAULT_Pin, &htim2, TIM_CHANNEL_1, TIM_CHANNEL_3, &hspi2, MC_SPI_CS_GPIO_Port, MC_SPI_CS_Pin,
                     false);

Encoder encoder(&htim4);

USBComm usb_comm;

Pin sw_start_pin{SW_START_GPIO_Port, SW_START_Pin};
Pin sw_stop_pin{SW_STOP_GPIO_Port, SW_STOP_Pin};
Pin sw_vol_up_pin{SW_VOL_UP_GPIO_Port, SW_VOL_UP_Pin};
Pin sw_vol_dn_pin{SW_VOL_DN_GPIO_Port, SW_VOL_DN_Pin};
Pin sw_rate_up_pin{SW_RATE_UP_GPIO_Port, SW_RATE_UP_Pin};
Pin sw_rate_dn_pin{SW_RATE_DN_GPIO_Port, SW_RATE_DN_Pin};
Pin led_power_pin{LED_POWER_GPIO_Port, LED_POWER_Pin};
Pin led_fault_pin{LED_FAULT_GPIO_Port, LED_FAULT_Pin};
Pin led_in_pin{LED_IN_GPIO_Port, LED_IN_Pin};
Pin led_out_pin{LED_OUT_GPIO_Port, LED_OUT_Pin};
Pin vol_char_1_pin{VOL_CHAR_1_GPIO_Port, VOL_CHAR_1_Pin};
Pin vol_char_2_pin{VOL_CHAR_2_GPIO_Port, VOL_CHAR_2_Pin};
Pin vol_char_3_pin{VOL_CHAR_3_GPIO_Port, VOL_CHAR_3_Pin};
Pin rate_char_1_pin{RATE_CHAR_1_GPIO_Port, RATE_CHAR_1_Pin};
Pin rate_char_2_pin{RATE_CHAR_2_GPIO_Port, RATE_CHAR_2_Pin};
Pin rate_char_3_pin{RATE_CHAR_3_GPIO_Port, RATE_CHAR_3_Pin};

ControlPanel controls(&sw_start_pin, &sw_stop_pin, &sw_vol_up_pin, &sw_vol_dn_pin, &sw_rate_up_pin, &sw_rate_dn_pin,
                      &led_power_pin, &led_fault_pin, &led_in_pin, &led_out_pin, &vol_char_1_pin, &vol_char_2_pin,
                      &vol_char_3_pin, &rate_char_1_pin, &rate_char_2_pin, &rate_char_3_pin, &htim1, TIM_CHANNEL_1);

UI_V1 ui(&controls);
LC064 eeprom(&hi2c1, 0);

RecordStore record_store(&eeprom);
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

TrapezoidalPlanner motion({.5, .5}, 10);

Servo motor(1, &motor_driver, &encoder, kMotorParams, kMotorVelPidParams, kMotorVelLimits, kMotorPosPidParams,
            kMotorPosLimits);

Pin power_detect{MEASURE_12V_GPIO_Port, MEASURE_12V_Pin};

VentilatorController vent(&motion, &motor);
Pin homing_switch{LIMIT2_GPIO_Port, LIMIT2_Pin};
HomingController home(&motor, &homing_switch);

extern "C" void abvm_init() {
    encoder.reset();
    usb_comm.set_as_cdc_consumer();

    pressure_sensor.init();
    pressure_sensor.set_powerdown(false);

    // control_panel_self_test();

    // Reset motor driver faults
    motor_driver.set_sleep(true);
    HAL_Delay(1);
    motor_driver.init();
    motor_driver.set_sleep(false);
    motor_driver.set_disabled(false);
    motor_driver.set_pwm_enabled(true);

    encoder.init();

    motor.init();

    motor_driver.set_reg(0x5, 4 << 2 | 3);
    controls.set_status_led(ControlPanel::STATUS_LED_1, true);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_LEFT, 1);
    controls.set_led_bar_graph(ControlPanel::BAR_GRAPH_RIGHT, 1);
    home.start();
    ui.init();
}

uint32_t last = 0;
uint32_t last_motor = 0;
uint32_t last_motion = 0;
uint32_t interval = 1000;
uint32_t motor_interval = 1;


extern "C" void abvm_update() {
    controls.update();

    if (millis() > last_motor + motor_interval) {
        motor.update();
        last_motor = millis();
    }

    if (millis() > last_motion + 10) {
        pressure_sensor.update();
        float pressure = psi_to_cmH2O(pressure_sensor.read());

        if (!home.is_done()) {
            home.update();
            if (home.is_done()) {
                ui.set_audio_alert(UI_V1::AudioAlert::DONE_HOMING);
                motor.set_pos_deg(0);
                motor_driver.set_pwm(0);
                vent.reset();
                vent.stop();
                vent.update(pressure);
            }
        } else {
            vent.update(pressure);
        }

        last_motion = millis();
    }

    if (millis() > last + 40) {
        float pressure = pressure_sensor.read();
        static char data[128];
        snprintf(data, sizeof(data),
                 "%1.3f,"
                 "%1.3f,"
                 "%1.3f,"
                 "%1.3f,"
                 "%1.3f,"
                 "%1.3f,"
                 "%1.3f,"
                 "%1.0f,"
                 "%1.0f,"
                 "%1.0f,"
                 "%lu,"
                 "%1.0f,"
                 "%1.0f"
                 "\r\n",
                 msec_to_sec(millis()), psi_to_cmH2O(pressure), motor.velocity, motor.target_velocity, motor.position,
                 motor.target_pos, motor_driver.get_current(), vent.get_rate(), vent.get_closed_pos(),
                 vent.get_open_pos(), motor.faults.to_int(),
                 vent.get_peak_pressure_cmH2O(),
                 vent.get_plateau_pressure_cmH2O()
                 );
        usb_comm.send((uint8_t *)data, strlen(data));
        last = millis();
    }

    ui.set_value(IUI::DisplayValue::TIDAL_VOLUME, vent.get_tv_idx());
    ui.set_value(IUI::DisplayValue::RESPIRATORY_RATE, vent.get_rate_idx());
    ui.set_value(IUI::DisplayValue::PEAK_PRESSURE, vent.get_peak_pressure_cmH2O());
    ui.set_value(IUI::DisplayValue::PLATEAU_PRESSURE, vent.get_plateau_pressure_cmH2O());
    ui.set_value(IUI::DisplayValue::PEAK_PRESSURE_ALARM, vent.get_peak_pressure_limit_cmH2O());

    if (vent.get_peak_pressure_cmH2O() >= vent.get_peak_pressure_limit_cmH2O()) {
        ui.set_alarm(UI_V1::Alarm::OVERPRESSURE);
    }

    if (!power_detect.read()) {
        ui.set_alarm(UI_V1::Alarm::POWER_LOSS);
    }

    switch (ui.update()) {
        case IUI::Event::START:
            if (home.is_done() && !vent.is_running()) {
                ui.set_audio_alert(UI_V1::AudioAlert::STARTING);
                vent.start();
                controls.set_status_led(ControlPanel::STATUS_LED_2, true);
            } else if (vent.is_running()) {
                ui.set_alarm(UI_V1::Alarm::SILENCE);
            }
            break;
        case IUI::Event::STOP:
            vent.stop();
            ui.set_audio_alert(UI_V1::AudioAlert::STOPPING);
            controls.set_status_led(ControlPanel::STATUS_LED_2, false);
            break;
        case IUI::Event::TIDAL_VOLUME_UP:
            vent.bump_tv(1);
            break;
        case IUI::Event::TIDAL_VOLUME_DOWN:
            vent.bump_tv(-1);
            break;
        case IUI::Event::RESPIRATORY_RATE_UP:
            vent.bump_rate(1);
            break;
        case IUI::Event::RESPIRATORY_RATE_DOWN:
            vent.bump_rate(-1);
            break;
        case IUI::Event::PRESSURE_LIMIT_UP:
            vent.increment_peak_pressure_limit_cmH2O(kPeakPressureLimitIncrement);
            break;

        case IUI::Event::PRESSURE_LIMIT_DOWN:
            vent.increment_peak_pressure_limit_cmH2O(-kPeakPressureLimitIncrement);
            break;

        case IUI::Event::SILENCE_ALARM:
            ui.set_alarm(UI_V1::Alarm::SILENCE);
            // TODO: Fill in this implementation
            break;

        case IUI::Event::GO_TO_BOOTLOADER:
            // TODO: Fill in this implementation
            BootLoader::start_bootloader();
            break;
        default:
            break;
    }
}

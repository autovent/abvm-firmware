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
#include "factory/tests.h"
#include "homing_controller.h"
#include "i2c.h"
#include "iwdg.h"
#include "lc064.h"
#include "main.h"
#include "record_store.h"
#include "serial_comm.h"
#include "servo.h"
#include "spi.h"
#include "sys/alarms.h"
#include "sys/array_helpers.h"
#include "tim.h"
#include "ui/ui_v1.h"
#include "usb_comm.h"
#include "ventilator_controller.h"
#include "version.h"

constexpr uint32_t kIdleLoggingInterval = 1000;   // 1Hz
constexpr uint32_t kRunningLoggingInterval = 50;  // 20Hz

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
Pin homing_switch{LIMIT2_GPIO_Port, LIMIT2_Pin};

ADS1231 pressure_sensor(ADC1_PWRDN_GPIO_Port, ADC1_PWRDN_Pin, &hspi1, ADC_SPI_MISO_GPIO_Port, ADC_SPI_MISO_Pin,
                        ADC_SPI_SCK_GPIO_Port, ADC_SPI_SCK_Pin, &kSensorConfig.pressure_params);

DRV8873 motor_driver(MC_SLEEP_GPIO_Port, MC_SLEEP_Pin, MC_DISABLE_GPIO_Port, MC_DISABLE_Pin, MC_FAULT_GPIO_Port,
                     MC_FAULT_Pin, &htim2, TIM_CHANNEL_1, TIM_CHANNEL_3, &hspi2, MC_SPI_CS_GPIO_Port, MC_SPI_CS_Pin,
                     false);

Encoder encoder(&htim4);

USBComm usb_comm;

TrapezoidalPlanner motion({.4, .4}, 10);

Servo motor(1, &motor_driver, &encoder, &homing_switch, kMotorConfig.motor_params, kMotorConfig.motor_vel_pid_params,
            kMotorConfig.motor_vel_limits, kMotorConfig.motor_pos_pid_params, kMotorConfig.motor_pos_limits);

ControlPanel controls(&sw_start_pin, &sw_stop_pin, &sw_vol_up_pin, &sw_vol_dn_pin, &sw_rate_up_pin, &sw_rate_dn_pin,
                      &led_power_pin, &led_fault_pin, &led_in_pin, &led_out_pin, &vol_char_1_pin, &vol_char_2_pin,
                      &vol_char_3_pin, &rate_char_1_pin, &rate_char_2_pin, &rate_char_3_pin, &htim1, TIM_CHANNEL_1);

UI_V1 ui(&controls);

VentilatorController vent(&motion, &motor, &pressure_sensor, kVentTVSettings, countof(kVentTVSettings),
                          kVentRateSettings, countof(kVentRateSettings));
HomingController home(&motor);

LC064 eeprom(&hi2c1, 0);
RecordStore record_store(&eeprom);

CommEndpoint hw_revision_ep(0, &kHardwareRev, sizeof(kHardwareRev), true);
CommEndpoint version_ep(0x1, &APP_VERSION, sizeof(APP_VERSION));

DataLogger logger_ep(0x0A, &pressure_sensor, &motor, &motor_driver, &vent);

ConfigCommandRPC config_cmd_ep(0x64, &record_store);

// config endpoints
CommEndpoint motor_config_ep(0x65, &kMotorConfig, sizeof(kMotorConfig));
CommEndpoint vent_app_config_ep(0x66, &kVentAppConfig, sizeof(kVentAppConfig));
CommEndpoint vent_resp_config_ep(0x67, &kVentRespirationConfig, sizeof(kVentRespirationConfig));
CommEndpoint vent_motion_config_ep(0x68, &kVentMotionConfig, sizeof(kVentMotionConfig));
CommEndpoint sensor_config_ep(0x69, &kSensorConfig, sizeof(kSensorConfig));
CommEndpoint tv_config_ep(0x6A, &kVentTVSettings, sizeof(kVentTVSettings));
CommEndpoint rr_config_ep(0x6B, &kVentRateSettings, sizeof(kVentRateSettings));

CommEndpoint *comm_endpoints[] = {
      &hw_revision_ep,   &version_ep,         &logger_ep,           &config_cmd_ep,
      &motor_config_ep,  &vent_app_config_ep, &vent_resp_config_ep, &vent_motion_config_ep,
      &sensor_config_ep, &tv_config_ep,       &rr_config_ep,
};

SerialComm ser_comm(comm_endpoints, sizeof(comm_endpoints) / sizeof(comm_endpoints[0]), &usb_comm);

USBComm::packet_handler packet_handlers[] = {
      {SerialComm::packet_callback, &ser_comm},
};

Pin power_detect{MEASURE_12V_GPIO_Port, MEASURE_12V_Pin};

Alarms alarms;

extern "C" void abvm_init() {
    HAL_IWDG_Refresh(&hiwdg);
    alarms.clear_all();

    encoder.reset();
    usb_comm.set_as_cdc_consumer();
    usb_comm.set_packet_handlers(packet_handlers, 1);

    pressure_sensor.init();
    pressure_sensor.set_powerdown(false);

    if (kVentAppConfig.mode == Modes::FACTORY_TEST) {
        control_panel_self_test(controls);
    }

    // Reset motor driver faults
    motor_driver.set_sleep(true);
    delay_ms(1);
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

    logger_ep.set_streaming(kIdleLoggingInterval);

    ui.init();

    eeprom.init();
    record_store.init();
    record_store.add_entry("MotorConfig", &kMotorConfig, sizeof(kMotorConfig), sizeof(kMotorConfig));
    record_store.add_entry("VentAppConfig", &kVentAppConfig, sizeof(kVentAppConfig), sizeof(kVentAppConfig));
    record_store.add_entry("VentRespConfig", &kVentRespirationConfig, sizeof(kVentRespirationConfig),
                           sizeof(kVentRespirationConfig));
    record_store.add_entry("VentMotionConfig", &kVentMotionConfig, sizeof(kVentMotionConfig),
                           sizeof(kVentMotionConfig));
    record_store.add_entry("SensorConfig", &kSensorConfig, sizeof(kSensorConfig), sizeof(kSensorConfig));
    HAL_IWDG_Refresh(&hiwdg);

    if (!record_store.first_load()) {
        // TODO: handle load failure
    }

    // Power on self test here
}

uint32_t last_motor = 0;
uint32_t last_motion = 0;
uint32_t last_ui = 0;

uint32_t motor_interval = 1;

extern "C" void abvm_update() {
    controls.update();
    ser_comm.update();

    if (millis() > last_motor + motor_interval) {
        motor.update();
        last_motor = millis();
    }

    if (millis() > last_motion + 10) {
        HAL_IWDG_Refresh(&hiwdg);
        pressure_sensor.update();
        if (!home.is_done()) {
            home.update();
            if (home.is_done()) {
                ui.set_audio_alert(UI_V1::AudioAlert::DONE_HOMING);
                motor.set_pos_deg(0);
                motor_driver.set_pwm(0);
                vent.reset();
                vent.stop();
                vent.update();
            }
        } else {
            vent.update();
        }

        alarms.set(Alarms::OVER_PRESSURE, vent.get_peak_pressure_cmH2O() >= vent.get_peak_pressure_limit_cmH2O());
        alarms.set(Alarms::LOSS_OF_POWER, !power_detect.read());
        alarms.set(Alarms::MOTION_FAULT, motor.faults.to_int());
        alarms.set(Alarms::OVER_CURRENT, motor_driver.get_fault());
        last_motion = millis();
    }

    if (millis() > last_ui + 20) {
        switch (ui.update()) {
            case IUI::Event::START:
                if (alarms.is_any_alarmed()) {
                    ui.silence();
                } else if (home.is_done() && !vent.is_running()) {
                    ui.set_audio_alert(UI_V1::AudioAlert::STARTING);
                    vent.start();
                    logger_ep.set_streaming(kRunningLoggingInterval);
                    controls.set_status_led(ControlPanel::STATUS_LED_2, true);
                }
                break;
            case IUI::Event::STOP:
                if (alarms.is_any_alarmed()) {
                    vent.reset();
                    vent.stop();
                    home.start();
                } else {
                    vent.stop();
                }
                logger_ep.set_streaming(kIdleLoggingInterval);

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
                vent.increment_peak_pressure_limit_cmH2O(kVentRespirationConfig.peak_pressure_limit_increment);
                break;

            case IUI::Event::PRESSURE_LIMIT_DOWN:
                vent.increment_peak_pressure_limit_cmH2O(-kVentRespirationConfig.peak_pressure_limit_increment);
                break;

            case IUI::Event::SILENCE_ALARM:
                break;

            case IUI::Event::GO_TO_BOOTLOADER:
                // TODO: Fill in this implementation
                BootLoader::start_bootloader();
                break;
            default:
                break;
        }

        ui.set_alarm(alarms);
        ui.set_value(IUI::DisplayValue::TIDAL_VOLUME, vent.get_tv_idx());
        ui.set_value(IUI::DisplayValue::RESPIRATORY_RATE, vent.get_rate_idx());
        ui.set_value(IUI::DisplayValue::PEAK_PRESSURE, vent.get_peak_pressure_cmH2O());
        ui.set_value(IUI::DisplayValue::PLATEAU_PRESSURE, vent.get_plateau_pressure_cmH2O());
        ui.set_value(IUI::DisplayValue::PEAK_PRESSURE_ALARM, vent.get_peak_pressure_limit_cmH2O());

        last_ui = millis();
    }
}

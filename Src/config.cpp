#include "config.h"

uint8_t kHardwareRev = 1;

MotorConfig kMotorConfig = {
    .motor_params = {130, 2*500},
    .motor_vel_pid_params = {2.8, .0, .0},
    .motor_pos_pid_params = {14.5, .1, .0},                         //.02},
    .motor_vel_limits = {-25 * 0.104719755, 25 * 0.104719755},  // RPM to rads/sec
    .motor_pos_limits = {0, deg_to_rad(90)},
};

VentAppConfig kVentAppConfig = {
    .mode = Modes::VENTILATOR,
    .current_update_period_ms = 2,   // Match the 50 freq of the servo motors
    .position_update_period_ms = 10,
    .measurement_update_period_ms = 100,
    .alarm_silence_time_ms = 30000,
};

VentResiprationConfig kVentRespirationConfig = {
    .time_to_idle_ms = 500,
    .min_bpm = 8,
    .max_bpm = 18,
    .plateau_time_ms = 200,
    .fast_open_time_ms = 100,
    .default_peak_pressure_limit = 40,
    .peak_pressure_display_min = 25,
    .peak_pressure_display_max = 50,
    .plateau_pressure_display_min = 15,
    .plateau_pressure_display_max = 40,
    .peak_pressure_limit_increment = 5,
};

VentMotionConfig kVentMotionConfig = {
    .idle_pos_deg = 10,
    .open_pos_deg = 16,
    .expiration_part = 2,
    .invert_motion = true,

};

float kVentTVSettings[6] ={55, 62, 69, 76, 83, 90};
float kVentRateSettings[6] = {8, 10, 12, 14, 16, 18};

// 1/(.00054 V/kPA  / 10.1972 cmH20/kPA)
constexpr float kPressureSensorOffsetGain_cmH2O_per_mV = (10.1972 /  0.54);
SensorConfig kSensorConfig = {
    .pressure_params = {kPressureSensorOffsetGain_cmH2O_per_mV, kPressureSensorOffsetGain_cmH2O_per_mV * -3.25f},
};

ConfigCommandRPC::ConfigCommandRPC(uint8_t id, RecordStore *store) :
    CommEndpoint(id, (void *const)NULL, sizeof(uint32_t), false), store(store) {}

uint8_t ConfigCommandRPC::write(void *data, size_t size) {
    uint32_t *cmd_in = (uint32_t*)data;
    uint8_t ret = 0;
    switch (*cmd_in) {
        case CONFIG_SAVE_CMD: {
            if (!store->store_all()) {
                ret = CONFIG_ERR;
            }
            break;
        }
        case CONFIG_ERASE_CMD: {
            if (!store->get_eeprom()->erase()) {
                ret = CONFIG_ERR;
            }
            break;
        }
        case CONFIG_LOAD_CMD: {
            if (!store->load_all()) {
                ret = CONFIG_ERR;
            }
            break;
        }
        case CONFIG_RESET_CMD: {
            if (!store->get_eeprom()->erase() || !store->first_load()) {
                ret = CONFIG_ERR;
            }
            break;
        }
        default: {
            return INVALID_CMD_ERR;
            break;
        }
    }
    return ret;
}

uint8_t ConfigCommandRPC::read(void *data, size_t size) {
    // this is a write-only endpoint, so error on read
    return (uint8_t)CommError::ERROR_READ;
}

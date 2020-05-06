#include "config.h"

uint8_t kHardwareRev = 1;

MotorConfig kMotorConfig = {
#if defined(MOTOR_GOBILDA_30RPM)
    .motor_params = {188, 28},
    .motor_vel_pid_params = {2.8, .0, .0},
    .motor_pos_pid_params = {14.5, .1, .0},                         //.02},
    .motor_vel_limits = {-30 * 0.104719755, 30 * 0.104719755},  // RPM to rads/sec
    .motor_pos_limits = {0, deg_to_rad(95)},

#elif defined(MOTOR_ROBOTZONE_16RPM)
    .motor_params = {515.63, 48};
    .motor_vel_pid_params = {6.5, .05, 0};
    .motor_pos_pid_params = {16, 0, 0.0};
    .motor_vel_limits = {-15 * 0.104719755, 15 * 0.104719755};  // RPM to rads/sec
    .motor_pos_limits = {0, deg_to_rad(89)};
#endif
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
    .open_pos_deg = 25,
    .min_closed_pos_deg = 40,
    .max_closed_pos_deg = 90,
    .ie_ratio = {1, 2},
    .invert_motion = true,
};

SensorConfig kSensorConfig = {
    .pressure_params = {(1.0f / (6.8948 * .00054)), (.045 + (1 / (6.8948 /*kPA/psi*/ * .00054)) * -0.00325f)},
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

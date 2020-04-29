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

ConfigCommandRPC::ConfigCommandRPC(uint8_t id, RecordStore *store) :
    CommEndpoint(id, NULL, sizeof(uint32_t)), store(store) {}

uint8_t ConfigCommandRPC::write(void *data, size_t size) {
    uint32_t *cmd_in = (uint32_t*)data;
    switch (*cmd_in) {
        case CONFIG_SAVE_CMD: {
            store->store_all();
            break;
        }
        case CONFIG_ERASE_CMD: {
            store->get_eeprom()->erase();
            break;
        }
        case CONFIG_LOAD_CMD: {
            store->load_all();
            break;
        }
        case CONFIG_RESET_CMD: {
            store->get_eeprom()->erase();
            store->first_load();
            break;
        }
        default: {
            return INVALID_CMD_ERR;
            break;
        }
    }
}

uint8_t ConfigCommandRPC::read(void *data, size_t size) {
    // this is a write-only endpoint, so error on read
    return (uint8_t)CommError::ERROR_READ;
}

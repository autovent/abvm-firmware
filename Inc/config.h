#ifndef CONFIG_H_
#define CONFIG_H_
#include "controls/pid.h"
#include "drivers/sensor.h"
#include "math/conversions.h"
#include "record_store.h"
#include "serial_comm.h"
#include "servo.h"

#define MOTOR_GOBILDA_30RPM
#define CONFIG_LONG_SPIRIT_FINGERS

enum class Modes : uint8_t {
    FACTORY_TEST,
    CALIBRATION,
    VENTILATOR,
};

extern uint8_t kHardwareRev;

struct IERatio {
    float inspiration;
    float expiration;

    inline float inspiration_percent() {
        return inspiration / (inspiration + expiration);
    }
    inline float expiration_percent() {
        return expiration / (inspiration + expiration);
    }
};

extern struct __attribute__((__packed__)) MotorConfig {
    Servo::Config motor_params;
    PID::Params motor_vel_pid_params;
    PID::Params motor_pos_pid_params;
    Range<float> motor_vel_limits;  // rads/s
    Range<float> motor_pos_limits;  // rads
} kMotorConfig;

extern struct __attribute__((__packed__)) VentAppConfig {
    Modes mode;
    uint32_t current_update_period_ms;
    uint32_t position_update_period_ms;
    uint32_t measurement_update_period_ms;
    uint32_t alarm_silence_time_ms;
} kVentAppConfig;

// Configuration parameters
// ! EDIT theses as needed
extern struct __attribute__((__packed__)) VentResiprationConfig {
    uint32_t time_to_idle_ms;
    int32_t min_bpm;
    int32_t max_bpm;
    int32_t plateau_time_ms;
    int32_t fast_open_time_ms;
    float default_peak_pressure_limit;
    float peak_pressure_display_min;
    float peak_pressure_display_max;
    float plateau_pressure_display_min;
    float plateau_pressure_display_max;
    float peak_pressure_limit_increment;

    inline int32_t get_slowest_breath_time() {
        return 1000 * 60 / min_bpm;
    }

    inline int32_t get_fastest_breath_time() {
        return 1000 * 60 / max_bpm;
    }

} kVentRespirationConfig;

extern struct __attribute__((__packed__)) VentMotionConfig {
    float idle_pos_deg;
    float open_pos_deg;         // Change this to a value where the servo is just barely compressing the bag
    float min_closed_pos_deg;   // Change this to a value where the servo has displaced the appropriate amount.
    float max_closed_pos_deg;   // Change this to a value where the servo has displaced the appropriate amount.
    IERatio ie_ratio;           // I : E Inspiration to Expiration Ratio
    bool invert_motion;         // Change this if the motor is inverted. This will reflect it 180
} kVentMotionConfig;

extern struct __attribute__((__packed__)) SensorConfig {
    ISensor::LinearParams pressure_params;
} kSensorConfig;

class ConfigCommandRPC : public CommEndpoint {
public:
    explicit ConfigCommandRPC(uint8_t id, RecordStore *store);

    uint8_t write(void *data, size_t size) override;
    uint8_t read(void *data, size_t size) override;

private:
    static constexpr uint32_t CONFIG_SAVE_CMD  = 0x45564153; // ASCII "SAVE"
    static constexpr uint32_t CONFIG_ERASE_CMD = 0x53415245; // ASCII "ERAS"
    static constexpr uint32_t CONFIG_LOAD_CMD  = 0x44414f4c; // ASCII "LOAD"
    static constexpr uint32_t CONFIG_RESET_CMD = 0x45534553; // ASCII "RESE"

    static constexpr uint8_t INVALID_CMD_ERR = 0x10;
    static constexpr uint8_t CONFIG_ERR = 0x11;

    RecordStore *store;
};

#endif
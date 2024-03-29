#ifndef CONFIG_H_
#define CONFIG_H_
#include "controls/pid.h"
#include "math/conversions.h"
#include "math/linear_fit.h"
#include "record_store.h"
#include "serial_comm.h"
#include "servo.h"

enum class Modes : uint8_t {
    FACTORY_TEST,
    CALIBRATION,
    VENTILATOR,
};

extern uint8_t kHardwareRev;

extern struct MotorConfig {
    Servo::Config motor_params;
    PID::Params motor_vel_pid_params;
    PID::Params motor_pos_pid_params;
    Range<float> motor_vel_limits;  // rads/s
    Range<float> motor_pos_limits;  // rads
} kMotorConfig;

extern struct VentAppConfig {
    Modes mode;
    uint32_t current_update_period_ms;
    uint32_t position_update_period_ms;
    uint32_t measurement_update_period_ms;
    uint32_t alarm_silence_time_ms;
} kVentAppConfig;

// Configuration parameters
// ! EDIT theses as needed
extern struct VentResiprationConfig {
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
} kVentRespirationConfig;

extern struct VentMotionConfig {
    float idle_pos_deg;
    float open_pos_deg;      // Change this to a value where the servo is just barely compressing the bag
    float expiration_part;   // I : E with 1 fixed to 1.
    uint32_t invert_motion;  // Change this if the motor is inverted. This will reflect it 180
} kVentMotionConfig;

extern struct SensorConfig { LinearFit pressure_params; } kSensorConfig;

extern float kVentTVSettings[6];
extern float kVentRateSettings[6];

class ConfigCommandRPC : public CommEndpoint {
public:
    explicit ConfigCommandRPC(uint8_t id, RecordStore *store);

    uint8_t write(void *data, size_t size) override;
    uint8_t read(void *data, size_t size) override;

private:
    static constexpr uint32_t CONFIG_SAVE_CMD = 0x45564153;   // ASCII "SAVE"
    static constexpr uint32_t CONFIG_ERASE_CMD = 0x53415245;  // ASCII "ERAS"
    static constexpr uint32_t CONFIG_LOAD_CMD = 0x44414f4c;   // ASCII "LOAD"
    static constexpr uint32_t CONFIG_RESET_CMD = 0x45534553;  // ASCII "RESE"

    static constexpr uint8_t INVALID_CMD_ERR = 0x10;
    static constexpr uint8_t CONFIG_ERR = 0x11;

    RecordStore *store;
};

#endif
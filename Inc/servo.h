/**
 * Requirements:
 */
#ifndef MOTOR_H_
#define MOTOR_H_

#include <math.h>

#include "controls/pid.h"
#include "drv8873.h"
#include "encoder.h"
#include "math/conversions.h"
#include "math/dsp.h"
#include "math/range.h"

class Servo {
public:
    enum class Mode { OFF, VELOCITY, POSITION };

    struct Config {
        float gear_reduction;  // X : 1
        float counts_per_rev;  // Countable events per rotation at the input shaft
    };

    struct Faults {
        bool no_encoder;
        bool wrong_dir;
        bool overcurrent;
        bool excessive_pos_error;

        uint32_t to_int();
    };

    Servo(uint32_t update_period_ms, DRV8873 *driver, Encoder *encoder, Config cfg, PID::Params vel_pid_params,
          Range<float> vel_limits, PID::Params pos_pid_params, Range<float> pos_limits, bool is_inverted = false);

    void set_pos(float pos);
    void set_pos_deg(float pos);

    void set_velocity(float vel);

    void init();
    void zero();
    void reset();
    float to_rad_at_output(float x);
    void update();
    void set_mode(Mode m);

    float velocity = 0;
    float position = 0;
    float command = 0;
    float target_pos = 0;
    float commanded_pos = 0;

    float target_velocity = 0;
    float commanded_velocity = 0;
    Faults faults = {.no_encoder = false, .wrong_dir = false};

private:
    uint32_t period_ms;
    Mode mode;
    DRV8873 *driver;
    Encoder *encoder;

    Config config;
    PID vel_pid;
    Range<float> vel_limits;

    PID pos_pid;
    Range<float> pos_limits;

    int32_t last_pos = 0;
    uint32_t pwm_freq;
    int32_t no_encoder_counts = 0;
    int32_t wrong_dir_counts = 0;

    bool is_inverted;

    bool test_no_encoder_fault(int32_t counts);
    bool test_wrong_direction();
    bool test_excessive_pos_error();
};

#endif  // MOTOR_H_
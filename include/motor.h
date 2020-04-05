#ifndef MOTOR_H_
#define MOTOR_H_

#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Arduino.h>
#include <math.h>
#include "dsp_math.h"
#include "pid.h"

struct MotorParameters {
    float gear_reduction; // GEAR_REDUCTION : 1
    float counts_per_rev; // Countable events per rotation at the input shaft
};

class Motor {
public:
    Motor(float update_period_s,
          uint8_t pwm_pin,
          uint8_t dir_pin,
          uint8_t enca,
          uint8_t encb,
          MotorParameters params,
          PID::Params speed_pid_params,
          PID::Params pos_pid_params)
        : pwm_pin(pwm_pin)
        , dir_pin(dir_pin)
        , encoder(enca, encb)
        , motor_parameters(params)
        , period_s(update_period_s)
        , speed_pid(speed_pid_params, update_period_s)
        , pos_pid(pos_pid_params, update_period_s)
        , resolution(12)
        , pwm_freq(100000)
    {
        pinMode(dir_pin, OUTPUT);
        digitalWrite(dir_pin, LOW);
        analogWriteFrequency(pwm_pin, pwm_freq);
        analogWriteResolution(resolution);
        set_pwm(0);
        encoder.readAndReset();
        is_first = false;
    }

    void set_pos(float pos)
    {
        target_pos = pos;
    }

    void set_velocity(float vel)
    {
        target_velocity = vel;
    }

    void zero()
    {
        raw_pos = 0;
    }

    void reset()
    {
        speed_pid.reset();
        pos_pid.reset();
    }

    float to_rad_at_output(float x)
    {
        return 2 * M_PI * x / (motor_parameters.counts_per_rev) / motor_parameters.gear_reduction;
    }

    void update()
    {
        int32_t raw_dp = encoder.readAndReset();
        raw_pos = raw_pos + raw_dp;
        position = .2 * position + .8 * to_rad_at_output(raw_pos);

        // TODO: add a velocity filter
        velocity = .96 * velocity + .04 * to_rad_at_output(raw_dp) / period_s; // rad / s <--- Filter this

        if (is_pos_enabled) {
            set_velocity(pos_pid.update(target_pos, position));
        }
        if (is_speed_enabled) {
            set_pwm(speed_pid.update(target_velocity, velocity));
        }
    }

    // -1 to 1
    void set_pwm(float x)
    {
        bool d = (x < 0) ? 1 : 0;
        int pattern = saturate((float)fabsf(x) * maxPwmValue(), 0, maxPwmValue());
        digitalWrite(dir_pin, d);
        analogWrite(pwm_pin, pattern);
    }

    bool is_first;
    float target_pos = 0;
    bool is_pos_enabled = true;
    bool is_speed_enabled = true;

    float target_velocity = 0;
    float velocity = 0;
    float position = 0;

    MotorParameters motor_parameters;
    PID speed_pid;
    PID pos_pid;

    float period_s = 0.002;

    int32_t raw_pos = 0;
    uint32_t resolution;
    uint32_t pwm_pin;
    uint32_t dir_pin;
    Encoder encoder;
    uint32_t pwm_freq;
    inline int maxPwmValue()
    {
        return ((1 << resolution) - 1);
    }

private:
};

#endif // MOTOR_H_
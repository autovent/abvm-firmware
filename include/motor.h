#ifndef MOTOR_H_
#define MOTOR_H_

#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Arduino.h>
#include <math.h>

struct PIDParams {
    float Kp;
    float Ki;
    float Kd;

    float err_acc;
    float dT;
};

float saturate(float val, float min, float max)
{
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}

class Motor {
public:
    Motor(uint32_t pwm, uint32_t dir, uint32_t enca, uint32_t encb, float gear_reduction = 515.63)
        : pwm(pwm)
        , dir(dir)
        , encoder(enca, encb)
        , mc_pid {1, 0, 0}
    {
        pinMode(dir, OUTPUT);
        digitalWrite(dir, LOW);
        resolution = 16;
        analogWriteFrequency(pwm, 10000);
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

    void position_controller()
    {
        static float err_acc = 0;

        float err = target_pos - position;
        err_acc += err;
        set_velocity(8 * err + .2 * period_sec * err_acc);
    }

    float to_rad_at_output(float x)
    {
        /// 48 counts per rotation at the input shaft
        return 2 * M_PI * x / (48) / gear_reduction;
    }

    void update()
    {
        int32_t raw_dp = encoder.readAndReset();
        raw_pos = raw_pos + raw_dp;
        position = to_rad_at_output(raw_pos);

        // TODO: add a velocity filter
        velocity = .5 * velocity + .5 * to_rad_at_output(raw_dp) / period_sec; // rad / s <--- Filter this

        if (is_pos_enabled) {
            position_controller();
        }
        if (is_speed_enabled) {
            speed_controller();
        }
    }

    void speed_controller()
    {
        static float err_acc = 0;

        float err = target_velocity - velocity;
        err_acc += err;
        set_pwm(.8 * err + .01 * period_sec * err_acc);
    }

    // -1 to 1
    void set_pwm(float x)
    {
        bool d = (x < 0) ? 1 : 0;
        int pattern = saturate((float)fabsf(x) * maxPwmValue(), 0, maxPwmValue());
        digitalWrite(dir, d);
        analogWrite(pwm, pattern);
    }

    bool is_first;
    float target_pos = 0;
    bool is_pos_enabled = true;
    bool is_speed_enabled = true;

    float target_velocity = 0;
    float velocity = 0;
    float position = 0;
    float gear_reduction = 515.63;
    float period_sec = 0.001;

    int32_t raw_pos = 0;
    uint32_t resolution;
    PIDParams mc_pid;
    uint32_t last_raw_pos;
    uint32_t pwm;
    uint32_t dir;
    Encoder encoder;

private:
    inline int maxPwmValue()
    {
        return ((1 << resolution) - 1);
    }
};

#endif // MOTOR_H_
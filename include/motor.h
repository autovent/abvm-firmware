#ifndef MOTOR_H_
#define MOTOR_H_

#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Arduino.h>
#include <Encoder.h>
#include <math.h>

#include "dsp_math.h"
#include "pid.h"

struct MotorParameters {
  float gear_reduction;  // GEAR_REDUCTION : 1
  float counts_per_rev;  // Countable events per rotation at the input shaft
};

struct Range {
  float min;
  float max;

  float saturate(float a) { return ::saturate(a, min, max); }
};

class Motor {
public:
  Motor(float update_period_s, uint8_t pwm_pin, uint8_t dir_pin, uint8_t enca,
        uint8_t encb, MotorParameters params, PID::Params vel_pid_params,
        Range vel_limits, PID::Params pos_pid_params, Range pos_limits)
      : pwm_pin(pwm_pin),
        dir_pin(dir_pin),
        encoder(enca, encb),
        motor_parameters(params),
        period_s(update_period_s),
        vel_pid(vel_pid_params, update_period_s),
        pos_pid(pos_pid_params, update_period_s),
        resolution(10),
        pwm_freq(20000),
        pos_limits(pos_limits),
        vel_limits(vel_limits) {
    pinMode(dir_pin, OUTPUT);
    digitalWrite(dir_pin, LOW);
    analogWriteFrequency(pwm_pin, pwm_freq);
    analogWriteResolution(resolution);
    set_pwm(0);
    encoder.readAndReset();
  }

  void set_pos(float pos) { target_pos = pos; }

  void set_velocity(float vel) { target_velocity = vel; }

  void zero() { raw_pos = 0; }

  void reset() {
    vel_pid.reset();
    pos_pid.reset();
  }

  float to_rad_at_output(float x) {
    return 2 * M_PI * x / (motor_parameters.counts_per_rev) /
           motor_parameters.gear_reduction;
  }

  int32_t no_encoder_counts = 0;
  int32_t wrong_dir_counts = 0;

  struct Faults {
    bool no_encoder;
    bool wrong_dir;
    bool overcurrent;
    bool excessive_pos_error;
  };

  Faults faults = {.no_encoder = false, .wrong_dir = false};

  bool test_no_encoder_fault(int32_t counts) {
    if (counts == 0 && pwm_pattern > 300) {
      if (++no_encoder_counts > 20) {
        faults.no_encoder = true;
        return true;
      }
    } else {
      no_encoder_counts = 0;
    }

    return false;
  }

  bool test_wrong_direction() {
    if (signof(velocity) != signof(target_velocity) &&
        (fabsf(target_velocity - velocity) > 1)) {
      if (++wrong_dir_counts > 10) {
        faults.wrong_dir = true;
        return true;
      }
    } else {
      wrong_dir_counts = 0;
    }
    return false;
  }

  bool test_excessive_pos_error() {
    if (is_pos_enabled && fabsf(target_pos - position) > (PI / 2)) {
      faults.excessive_pos_error = true;
      return true;
    }
    return false;
  }
  void update() {
    int32_t counts = encoder.readAndReset();
    raw_pos = raw_pos + counts;
    position = .90 * position + .1 * to_rad_at_output(raw_pos);

    // TODO: add a velocity filter
    velocity = .97 * velocity + .03 * to_rad_at_output(counts) /
                                    period_s;  // rad / s <--- Filter this

    test_no_encoder_fault(counts);

    test_wrong_direction();
    // test_excessive_pos_error();


    if (faults.no_encoder || faults.wrong_dir || faults.overcurrent ||
        faults.excessive_pos_error) {
      set_pwm(0);
    } else {
      if (is_pos_enabled) {
        commanded_pos = pos_limits.saturate(target_pos);
        set_velocity(pos_pid.update(commanded_pos, position));
      }


      if (is_vel_enabled) {
        target_velocity = vel_limits.saturate(target_velocity);
        set_pwm(vel_pid.update(target_velocity, velocity));
      }
    }
  }

  // -1 to 1
  void set_pwm(float x) {
    bool d = (x < 0) ? 1 : 0;
    pwm_pattern = saturate((float)fabsf(x) * maxPwmValue(), 0, maxPwmValue());

    digitalWrite(dir_pin, d);
    analogWrite(pwm_pin, pwm_pattern);
  }

  bool is_pos_enabled = true;
  bool is_vel_enabled = true;

  float target_pos = 0;
  float commanded_pos = 0;

  float target_velocity = 0;
  float commanded_velocity = 0;

  Range pos_limits;
  Range vel_limits;

  float velocity = 0;
  float position = 0;
  float pwm_pattern = 0;

  MotorParameters motor_parameters;

  PID vel_pid;
  PID pos_pid;

  float period_s = 0.002;

  int32_t raw_pos = 0;
  uint32_t resolution;
  uint32_t pwm_pin;
  uint32_t dir_pin;
  Encoder encoder;
  uint32_t pwm_freq;
  inline int maxPwmValue() { return ((1 << resolution) - 1); }

private:
};

#endif  // MOTOR_H_
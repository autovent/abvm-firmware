/**
 * Requirements:
 */
#ifndef MOTOR_H_
#define MOTOR_H_

#include <math.h>

#include "controls/pid.h"
#include "drv8873.h"
#include "encoder.h"
#include "math/dsp.h"
#include "math/range.h"
#include "math/constants.h"

class Motor {
public:
  struct Config {
    float gear_reduction;    // X : 1
    float counts_per_rev;  // Countable events per rotation at the input shaft
  };

  struct Faults {
    bool no_encoder;
    bool wrong_dir;
    bool overcurrent;
    bool excessive_pos_error;

    uint32_t to_int() {
      return ((no_encoder ? 1 : 0) << 0) | ((wrong_dir ? 1 : 0) << 1) |
             ((overcurrent ? 1 : 0) << 2) |
             ((excessive_pos_error ? 1 : 0) << 3);
    }
  };

  Motor(uint32_t update_period_ms, DRV8873 *driver, Encoder *encoder,
        Config cfg, PID::Params vel_pid_params, Range<float> vel_limits,
        PID::Params pos_pid_params, Range<float> pos_limits)
      : driver(driver),
        encoder(encoder),
        config(cfg),
        period_ms(update_period_ms),
        vel_pid(vel_pid_params, update_period_ms / 1000.0),
        pos_pid(pos_pid_params, update_period_ms / 1000.0),
        pos_limits(pos_limits),
        vel_limits(vel_limits) {
  }

  void set_pos(float pos) { target_pos = pos; }

  void set_velocity(float vel) { target_velocity = vel; }

  void init() {
    encoder->reset();
    driver->set_pwm(0);
    zero();
    reset();
  }

  void zero() {
    last_pos = 0;
    position = 0;
  }

  void reset() {
    vel_pid.reset();
    pos_pid.reset();
  }

  float to_rad_at_output(float x) {
    return M_PI * x / (config.counts_per_rev) / config.gear_reduction;
  }

  bool test_no_encoder_fault(int32_t counts) {
    if (counts == 0 && command > 500) {
      if (++no_encoder_counts > 30) {
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
      if (++wrong_dir_counts > 20) {
        faults.wrong_dir = true;
        return true;
      }
    } else {
      wrong_dir_counts = 0;
    }
    return false;
  }

  bool test_excessive_pos_error() {
    if (is_pos_enabled && fabsf(target_pos - position) > (M_PI_2)) {
      faults.excessive_pos_error = true;
      return true;
    }
    return false;
  }

  void update() {
    int16_t next_pos = encoder->get();
    int16_t counts = next_pos - last_pos;

    // TODO: replace with a tested filter library
    position = 0 * position + 1 * to_rad_at_output(next_pos);
    velocity =
        .97 * velocity + .03 * to_rad_at_output(counts) /
                             (period_ms / 1000.0f);  // rad / s <--- Filter this

    test_no_encoder_fault(counts);
    test_wrong_direction();
    // test_excessive_pos_error();

    if (faults.no_encoder || faults.wrong_dir || faults.overcurrent ||
        faults.excessive_pos_error) {
      driver->set_pwm(0);
    } else {
      if (is_pos_enabled) {
        commanded_pos = pos_limits.saturate(target_pos);
        
        set_velocity(pos_pid.update(commanded_pos, position));
      }

      if (is_vel_enabled) {
        commanded_velocity =
            dt_rate_limit(target_velocity, commanded_velocity, .05);
        commanded_velocity = vel_limits.saturate(commanded_velocity);
        driver->set_pwm(vel_pid.update(commanded_velocity, velocity));
      }
    }

    last_pos = next_pos;
  }

  uint32_t period_ms = 2;

  DRV8873 *driver;
  Encoder *encoder;

  Config config;
  PID vel_pid;
  Range<float> vel_limits;

  PID pos_pid;
  Range<float> pos_limits;

  bool is_pos_enabled = true;
  bool is_vel_enabled = true;

  float target_pos = 0;
  float commanded_pos = 0;

  float target_velocity = 0;
  float commanded_velocity = 0;


  float velocity = 0;
  float position = 0;
  float command = 0;




  int32_t last_pos = 0;
  uint32_t pwm_freq;

private:
  int32_t no_encoder_counts = 0;
  int32_t wrong_dir_counts = 0;

  Faults faults = {.no_encoder = false, .wrong_dir = false};
};

#endif  // MOTOR_H_
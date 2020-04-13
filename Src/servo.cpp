#include "servo.h"

uint32_t Servo::Faults::to_int() {
  return ((no_encoder ? 1 : 0) << 0) | ((wrong_dir ? 1 : 0) << 1) |
         ((overcurrent ? 1 : 0) << 2) | ((excessive_pos_error ? 1 : 0) << 3);
}

Servo::Servo(uint32_t update_period_ms, DRV8873 *driver, Encoder *encoder,
             Config cfg, PID::Params vel_pid_params, Range<float> vel_limits,
             PID::Params pos_pid_params, Range<float> pos_limits, bool is_inverted)
    : driver(driver),
      encoder(encoder),
      config(cfg),
      period_ms(update_period_ms),
      vel_pid(vel_pid_params, update_period_ms / 1000.0),
      pos_pid(pos_pid_params, update_period_ms / 1000.0),
      pos_limits(pos_limits),
      vel_limits(vel_limits), 
      is_inverted(is_inverted) {
    encoder->is_inverted = true;
    driver->is_inverted = true;
}

void Servo::set_pos(float pos) { target_pos = pos; }
void Servo::set_pos_deg(float pos) { target_pos = deg_to_rad(pos); }

void Servo::set_velocity(float vel) { target_velocity = vel; }

void Servo::init() {
  encoder->reset();
  driver->set_pwm(0);
  zero();
  reset();
}

void Servo::zero() {
  last_pos = 0;
  position = 0;
  encoder->reset();
}

void Servo::reset() {
//   vel_pid.reset();
  pos_pid.reset();
}

float Servo::to_rad_at_output(float x) {
  return 2 * M_PI * x / (config.counts_per_rev) / config.gear_reduction;
}

bool Servo::test_no_encoder_fault(int32_t counts) {
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

bool Servo::test_wrong_direction() {
  if (signof(velocity) != signof(target_velocity) &&
      (fabsf(target_velocity - velocity) > 1.4)) {
    if (++wrong_dir_counts > 30) {
    //   faults.wrong_dir = true;
      return true;
    }
  } else {
    wrong_dir_counts = 0;
  }
  return false;
}

bool Servo::test_excessive_pos_error() {
  if (mode == Mode::POSITION && fabsf(target_pos - position) > (M_PI_2)) {
    faults.excessive_pos_error = true;
    return true;
  }
  return false;
}

void Servo::update() {
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
    if (mode == Mode::POSITION) {
      commanded_pos = pos_limits.saturate(target_pos);

      set_velocity(pos_pid.update(commanded_pos, position));
    }

    if (mode == Mode::VELOCITY || mode == Mode::POSITION) {
      commanded_velocity =
          dt_rate_limit(target_velocity, commanded_velocity, .05);
      commanded_velocity = vel_limits.saturate(commanded_velocity);
      driver->set_pwm(vel_pid.update(commanded_velocity, velocity));
    }
  }

  last_pos = next_pos;
}

void Servo::set_mode(Mode m) { mode = m; }
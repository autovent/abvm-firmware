#include "controls/pid.h"

PID::PID(float Kp, float Ki, float Kd, float dt)
    : params{Kp, Ki, Kd},
      err_acc_(0),
      period_(dt),
      err_last_(0),
      is_init_(false) {}

PID::PID(Params params, float dt)
    : params(params), err_acc_(0), period_(dt), err_last_(0), is_init_(false) {}

void PID::reset() {
  err_acc_ = 0;
  err_last_ = 0;
  is_init_ = false;
}

float PID::update(float target, float meas) {
  // Calculate the error
  float err = target - meas;

  // If we are not initialized yet:
  //   - Set the last error to the current error to prevent the D term from
  //   going nuts
  //   - Set the accumulated error to 0
  //   - Assert that we are initialized.
  if (!is_init_) {
    err_last_ = err;
    err_acc_ = 0;
    is_init_ = true;
  }

  err_acc_ += err;

  float derr = err - err_last_;
  err_last_ = err;

  return params.Kp * err + params.Ki * err_acc_ * period_ +
         (params.Kd * (derr) / period_);
}

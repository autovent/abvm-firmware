#pragma once

#include "config.h"
#include "math/dsp.h"
#include "servo.h"
#include "drivers/pin.h"

class HomingController {
public:
  enum class State { IDLE = 0, HOMING, DONE };

  // TODO: Add max current limit
  HomingController(Servo *motor, Pin *home);
  void start();
  
  bool is_done();
  State update();

private:
  Servo *motor;
  Pin *home_switch;
  State state;

  float homing_velocity;
  //   float max_current;
};

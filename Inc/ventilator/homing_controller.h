#pragma once

#include "config.h"
#include "math/dsp.h"
#include "servo.h"
#include "drivers/pin.h"

class HomingController {
public:
  enum class State { IDLE = 0, HOMING, DONE };

  // TODO: Add max current limit
  HomingController(Servo *motor, Pin *home)
      : motor(motor), state(State::IDLE), home_switch(home), homing_velocity(-.2) {}

  void start() {
    state = State::HOMING;
    motor->set_mode(Servo::Mode::VELOCITY);
    // motor->set_current_limit(max_current);
  }
  
  bool is_done() { return state == State::DONE; }

  float update() {
    if (state == State::HOMING) {
        if (!home_switch->read()) {
            state = State::DONE;
            motor->reset();
            motor->set_velocity(0);
            motor->zero();
        } else {
            motor->set_velocity(homing_velocity);
        }
    }
  }

private:
  Servo *motor;
  Pin *home_switch;
  State state;

  float homing_velocity;
  //   float max_current;
};

#include "homing_controller.h"

// TODO: Add max current limit
HomingController::HomingController(Servo *motor, Pin *home)
    : motor(motor),
      state(State::IDLE),
      home_switch(home),
      homing_velocity(-.2) {}

void HomingController::start() {
  state = State::HOMING;
  motor->set_mode(Servo::Mode::VELOCITY);
  // motor->set_current_limit(max_current);
}

bool HomingController::is_done() { return state == State::DONE; }

HomingController::State HomingController::update() {
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
  
  return state;
}
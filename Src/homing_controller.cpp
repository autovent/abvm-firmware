#include "homing_controller.h"

// TODO: Add max current limit
HomingController::HomingController(Servo *motor)
    : motor(motor), state(State::IDLE), homing_velocity_rad_per_sec(-.6), backoff_velocity_rad_per_sec(.2) {}

void HomingController::start() {
    state = State::HOMING;
    motor->set_mode(Servo::Mode::VELOCITY);
}

bool HomingController::is_done() {
    return state == State::DONE;
}

HomingController::State HomingController::update() {
    switch (state) {
        case State::HOMING: {
            // if the homming switch is pressed (low active) then we start back off.
            if (motor->limit_switch_pressed()) {
                state = State::HOMING_BACKUP;

                motor->set_velocity(backoff_velocity_rad_per_sec);
            } else {
                motor->set_velocity(homing_velocity_rad_per_sec);
            }
            break;
        }
        case State::HOMING_BACKUP: {
            if (motor->limit_switch_pressed()) {
                motor->set_velocity(backoff_velocity_rad_per_sec);
            } else {
                state = State::DONE;
                motor->reset();
                motor->set_velocity(0);
                motor->zero();
            }
        }
        default:
            break;
    }

    return state;
}
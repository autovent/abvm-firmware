#include "ventilator_controller.h"

VentilatorController::VentilatorController(IMotionPlanner *motion, Servo *motor)
    : motion(motion),
      motor(motor),
      state(State::GO_TO_IDLE),
      is_measure_plateau_cycle(false),
      is_operational(false),
      current_rate_idx(0),
      current_tv_idx(0),
      tidal_volume_settings{55, 62, 69, 76, 83, 95},
      rate_settings{8, 10, 12, 14, 16, 18} {}

void VentilatorController::start() {
    motor->set_pos_deg(0);
    motor->set_mode(Servo::Mode::POSITION);
    state = State::GO_TO_IDLE;

    motion->set_next({kIdlePositiong_deg, kTimeToIdle_ms});
    motor->set_pos_deg(motion->run(motor->position));
}

float VentilatorController::update() {
    if (!is_operational && state != State::IDLE) {
        state = State::GO_TO_IDLE;
    }

    if (motion->is_idle()) {
        switch (state) {
            case State::IDLE:
                if (is_operational) {
                    state = State::GO_TO_START;
                    motion->set_next({kOpenPosition_deg, 0, kTimeToIdle_ms});
                }
                break;
            case State::GO_TO_START:
                state = State::INSPIRATION;
                motion->set_next({kOpenPosition_deg, 0, kTimeToIdle_ms});
                break;
            case State::INSPIRATION:
                if (is_measure_plateau_cycle) {
                    state = State::INSPIRATORY_HOLD;
                } else {
                    state = State::EXPIRATION;
                }
                motion->set_next({tidal_volume_settings[current_tv_idx], 0,
                                  kIERatio.getInspirationPercent() * bpm_to_time_ms(rate_settings[current_rate_idx])});
                break;
            case State::INSPIRATORY_HOLD:
                state = State::EXPIRATION;
                motion->set_next({tidal_volume_settings[current_tv_idx], 0, kPlateauTime_ms});
                break;
            case State::EXPIRATION: {
                state = State::INSPIRATION;
                uint32_t plateau_time = is_measure_plateau_cycle ? kPlateauTime_ms : 0;
                motion->set_next({kOpenPosition_deg, 0,
                                  (kIERatio.getExpirationPercent() * bpm_to_time_ms(rate_settings[current_rate_idx])) -
                                        plateau_time});
                break;
            }
            case State::GO_TO_IDLE:
                state = State::IDLE;
                motion->set_next({kIdlePositiong_deg, 0, kTimeToIdle_ms});
                break;
            default:
                break;
        }
    }

    motor->set_pos_deg(motion->run(motion->get_pos()));

    return 0;
}

void VentilatorController::reset() {
    is_operational = false;
    state = State::GO_TO_IDLE;

    current_tv_idx = 0;
    current_rate_idx = 0;
}

#include "ventilator_controller.h"

VentilatorController::VentilatorController(IMotionPlanner *motion, Servo *motor)
    : motion(motion),
      motor(motor),
      state(State::GO_TO_IDLE),
      is_measure_plateau_cycle(true),
      is_operational(false),
      next_rate_idx(0),
      next_tv_idx(0),
      tidal_volume_settings{55, 62, 69, 76, 83, 95},
      rate_settings{8, 10, 12, 14, 16, 18} {}

void VentilatorController::start() {
    motor->set_pos_deg(0);
    motor->set_mode(Servo::Mode::POSITION);
    state = State::GO_TO_IDLE;

    motion->set_next({kIdlePositiong_deg, kTimeToIdle_ms});
    motor->set_pos_deg(motion->run(motor->position));
    is_operational = true;
}

void VentilatorController::stop() {
    state = State::GO_TO_IDLE;
    motion->force_next({kIdlePositiong_deg, kTimeToIdle_ms});
    motor->set_pos_deg(motion->run(motion->get_pos()));
    current_peak_pressure_cmH2O = 0;
    last_peak_pressure_cmH2O = 0;
    is_operational = false;
}

float VentilatorController::update(float pressure_cmH2O) {
    if (pressure_cmH2O > current_peak_pressure_cmH2O) {
        current_peak_pressure_cmH2O = pressure_cmH2O;
    }

    if (is_measure_plateau_cycle && (state == State::EXPIRATION)) {
        current_plateau_pressure = min(pressure_cmH2O, current_plateau_pressure);
    }

    if (pressure_cmH2O >= kOverPressure_cmH2O && state != State::INSPIRATION) {
        state = State::EXPIRATION;
        motion->force_next({kOpenPosition_deg, 0, kFastOpenTime_ms});
        motor->set_pos_deg(motion->run(motion->get_pos()));
        is_fast_open = true;
    }

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
                last_plateau_pressure = current_plateau_pressure;
                last_peak_pressure_cmH2O = current_peak_pressure_cmH2O;
                current_peak_pressure_cmH2O = 0;

                current_rate_idx = next_rate_idx;
                current_tv_idx = next_tv_idx;
                if (is_measure_plateau_cycle) {
                    current_plateau_pressure = INFINITY;

                    state = State::INSPIRATORY_HOLD;
                } else {
                    state = State::EXPIRATION;
                }

                motion->set_next({tidal_volume_settings[current_tv_idx], 0,
                                  kIERatio.inspiration_percent() * bpm_to_time_ms(rate_settings[current_rate_idx])});
                break;
            case State::INSPIRATORY_HOLD:
                state = State::EXPIRATION;
                motion->set_next({tidal_volume_settings[current_tv_idx], 0, kPlateauTime_ms});
                break;
            case State::EXPIRATION: {
                state = State::INSPIRATION;
                uint32_t plateau_time = is_measure_plateau_cycle ? kPlateauTime_ms : 0;
                if (is_fast_open) {
                    plateau_time = kFastOpenTime_ms;
                    is_fast_open = false;
                }
                motion->set_next({kOpenPosition_deg, 0,
                                  (kIERatio.expiration_percent() * bpm_to_time_ms(rate_settings[current_rate_idx])) -
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
}

float VentilatorController::get_peak_pressure_cmH2O() {
    return max(current_peak_pressure_cmH2O, last_peak_pressure_cmH2O);
}

float VentilatorController::get_plateau_pressure_cmH2O() {
    // If the last plateau pressur is infinity then return 0, this just  means  there is no valid imeasurement of the
    // plateau pressure yet.
    if (last_plateau_pressure == INFINITY) {
        return 0;
    } else {
        return last_plateau_pressure;
    }
}

#include "ventilator_controller.h"

VentilatorController::VentilatorController(IMotionPlanner *motion, Servo *motor, ISensor *pressure_sensor)
    : motion(motion),
      motor(motor),
      state(State::GO_TO_IDLE),
      is_measure_plateau_cycle(true),
      is_operational(false),
      next_rate_idx(0),
      next_tv_idx(0),
      tidal_volume_settings{55, 62, 69, 76, 83, 90},
      rate_settings{8, 10, 12, 14, 16, 18},
      peak_pressure_limit_cmH2O(kVentRespirationConfig.default_peak_pressure_limit),
      pressure_sensor(pressure_sensor) {}

void VentilatorController::start() {
    motor->set_pos_deg(0);
    motor->set_mode(Servo::Mode::POSITION);
    state = State::GO_TO_IDLE;

    motion->set_next({kVentMotionConfig.idle_pos_deg, 0, kVentRespirationConfig.time_to_idle_ms});
    motor->set_pos_deg(motion->run(motor->position));
    is_operational = true;
}

void VentilatorController::stop() {
    state = State::GO_TO_IDLE;
    motion->force_next({kVentMotionConfig.idle_pos_deg, 0, kVentRespirationConfig.time_to_idle_ms});
    motor->set_pos_deg(motion->run(motion->get_pos()));
    current_peak_pressure_cmH2O = 0;
    last_peak_pressure_cmH2O = 0;
    is_operational = false;
}

float VentilatorController::update() {
    // Filter the pressure sensor data
    pressure_cmH2O = .5 * pressure_cmH2O + .5 * psi_to_cmH2O(pressure_sensor->read());

    if (pressure_cmH2O > current_peak_pressure_cmH2O) {
        current_peak_pressure_cmH2O = pressure_cmH2O;
    }

    if (is_measure_plateau_cycle && (state == State::EXPIRATION)) {
        current_plateau_pressure = min(pressure_cmH2O, current_plateau_pressure);
    }

    if (pressure_cmH2O >= peak_pressure_limit_cmH2O && state != State::INSPIRATION) {
        state = State::EXPIRATION;
        motion->force_next({kVentMotionConfig.open_pos_deg, 0, kVentRespirationConfig.fast_open_time_ms});
        motor->set_pos_deg(motion->run(motion->get_pos()));
        is_fast_open = true;
    }

    if (motor->i_measured > 4.5 && state != State::INSPIRATION) {
        state = State::EXPIRATION;
        motion->force_next({kVentMotionConfig.open_pos_deg, 0, kVentRespirationConfig.fast_open_time_ms});
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
                    motion->set_next({kVentMotionConfig.open_pos_deg, 0, kVentRespirationConfig.time_to_idle_ms});
                }
                break;
            case State::GO_TO_START:
                state = State::INSPIRATION;
                motion->set_next({kVentMotionConfig.open_pos_deg, 0, kVentRespirationConfig.time_to_idle_ms});
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
                                  kVentMotionConfig.ie_ratio.inspiration_percent() *
                                  bpm_to_time_ms(rate_settings[current_rate_idx])});
                break;
            case State::INSPIRATORY_HOLD:
                state = State::EXPIRATION;
                motion->set_next({tidal_volume_settings[current_tv_idx], 0, kVentRespirationConfig.plateau_time_ms});
                break;
            case State::EXPIRATION: {
                state = State::INSPIRATION;
                uint32_t plateau_time = is_measure_plateau_cycle ? kVentRespirationConfig.plateau_time_ms : 0;
                if (is_fast_open) {
                    plateau_time = kVentRespirationConfig.fast_open_time_ms;
                    is_fast_open = false;
                }
                motion->set_next({kVentMotionConfig.open_pos_deg, 0,
                                  (kVentMotionConfig.ie_ratio.expiration_percent() *
                                  bpm_to_time_ms(rate_settings[current_rate_idx])) - plateau_time});
                break;
            }
            case State::GO_TO_IDLE:
                state = State::IDLE;
                motion->set_next({kVentMotionConfig.idle_pos_deg, 0, kVentRespirationConfig.time_to_idle_ms});
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

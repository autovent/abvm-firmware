#pragma once

#include "config.h"
#include "controls/motion_planner.h"
#include "math/dsp.h"
#include "math/conversions.h"
#include "motor.h"

class VentilatorController {
public:
  enum class State {
    IDLE = 0,
    GO_TO_START,
    INSPIRATION,
    INSPIRATORY_HOLD,
    EXPIRATION,
    GO_TO_IDLE,
    NUM_STATES
  };

  VentilatorController(IMotionPlanner *motion, Motor *motor)
      : motion(motion),
        motor(motor),
        state(State::GO_TO_IDLE),
        is_measure_plateau_cycle(false),
        is_operational(false),
        current_rate_idx(0),
        current_tv_idx(0),
        tidal_volume_settings{55, 62, 69, 76, 83, 90},
        rate_settings{8, 10, 12, 14, 16, 18} {}

  void start() {
    motor->set_mode(Motor::Mode::POSITION);
  }
  
  float update() {
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
                            kIERatio.getInspirationPercent() *
                                bpm_to_time_ms(rate_settings[current_rate_idx])});
          break;
        case State::INSPIRATORY_HOLD:
          state = State::EXPIRATION;
          motion->set_next(
              {tidal_volume_settings[current_tv_idx], 0, kPlateauTime_ms});
          break;
        case State::EXPIRATION: {
          state = State::INSPIRATION;
          uint32_t plateau_time =
              is_measure_plateau_cycle ? kPlateauTime_ms : 0;
          motion->set_next({kOpenPosition_deg, 0,
                            (kIERatio.getExpirationPercent() *
                             bpm_to_time_ms(rate_settings[current_rate_idx])) -
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

  void reset() {
    is_operational = false;
    state = State::GO_TO_IDLE;

    current_tv_idx = 0;
    current_rate_idx = 0;
  }

  inline void bump_tv(int i) {
    current_tv_idx = saturate(current_tv_idx + i, 0, kNumTVSettings - 1);
  }
  inline uint8_t get_tv_idx() { return current_tv_idx; }
  inline void bump_rate(int i) {
    current_rate_idx = saturate(current_rate_idx + i, 0, kNumRateSettings - 1);
  }
  inline uint8_t get_rate_idx() { return current_rate_idx; }

  bool is_operational;

private:
  IMotionPlanner *motion;
  Motor *motor;

  State state;

  bool is_measure_plateau_cycle;

  static constexpr uint8_t kNumTVSettings = 6;
  float tidal_volume_settings[kNumTVSettings];

  static constexpr uint8_t kNumRateSettings = 6;
  float rate_settings[kNumRateSettings];

  uint8_t current_tv_idx;
  uint8_t current_rate_idx;
};

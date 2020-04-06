#ifndef TRAJECTORY_PLANNER_H_
#define TRAJECTORY PLANNER_H_

#include <math.h>
#include <stdint.h>

#include "dsp_math.h"

class TrajectoryPlanner {
public:
  enum class State {
    IDLE,
    ACCELERATING,
    CONSTANT,
    DECELERATION,
  };

  struct Plan {
    float p_target;
    float time_total;
  };

  struct Parameters {
    float t_a_percent;
    float t_d_percent;
  };

  TrajectoryPlanner(Parameters params, uint32_t dt_ms = 10)
      : state(State::IDLE), current{}, next{}, dt_ms(dt_ms), params(params) {}

  void reset() {
    state = State::IDLE;
    is_next_available = false;
  }
  bool is_idle() { return state == State::IDLE; }
  void set_next(Plan const& p) {
    next = p;
    is_next_available = true;
  }

  void force_next(Plan const& p) {
    set_next(p);
    state = State::IDLE;
  }

  float run(float pos) {
    if (state == State::IDLE) {
      if (is_next_available) {
        current = next;
        is_next_available = false;

        // Discretize the time part of the spline.
        t_counts_accel =
            (int)((params.t_a_percent * current.time_total) / dt_ms);
        t_counts_decel =
            (int)((params.t_d_percent * current.time_total) / dt_ms);
        t_counts_c =
            (int)((current.time_total *
                   (saturate(1 - params.t_a_percent - params.t_d_percent, 0,
                             1))) /
                  dt_ms);

        current.time_total =
            dt_ms * (t_counts_accel + t_counts_decel + t_counts_c);
        // Find the area under the curve and the median
        float dp = current.p_target - pos;

        // NOTE ON UNITS.v_max, and accel are all discrete and do NOT have a
        // time component.
        v_max = (dp) / (.5 * t_counts_decel + .5 * t_counts_accel + t_counts_c);
        accel = (v_max) / t_counts_accel;

        decel = -(v_max) / t_counts_decel;

        v_last = 0;
        p_last = pos;

        t_counts_total = 0;

        state = State::ACCELERATING;
      } else {
        return pos;
      }
    }

    float v = v_max;

    switch (state) {
      case State::ACCELERATING:
        v = v_last + accel;
        break;

      case State::CONSTANT:
        v = v_max;
        break;

      case State::DECELERATION:
        v = v_last + decel;
        break;

      case State::IDLE:
      default:
        break;
    }

    pos = v + p_last;
    p_last = pos;
    v_last = v;
    t_counts_total += 1;

    switch (state) {
      case State::ACCELERATING:
        if (t_counts_total >= (t_counts_accel)) {
          state = State::CONSTANT;
          t_counts_total = 0;
        }
        break;

      case State::CONSTANT:
        if (t_counts_total >= (t_counts_c)) {
          state = State::DECELERATION;
          t_counts_total = 0;
        }
        break;

      case State::DECELERATION:
        if (t_counts_total >= t_counts_decel) {
          state = State::IDLE;
          t_counts_total = 0;
        }
        break;

      default:
      case State::IDLE:
        break;
    }

    return pos;
  }

  State get_state() const { return state; }

  State state;
  Plan current;

  Plan next;
  bool is_next_available;

  int32_t t_counts_accel;
  int32_t t_counts_decel;
  int32_t t_counts_c;
  float v_max;

  float accel;
  float decel;
  float v_last;
  float p_last;

  uint32_t dt_ms;

  int32_t t_counts_total;

  Parameters params;
};
#endif
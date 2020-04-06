#ifndef TRAJECTORY_PLANNER_H_
#define TRAJECTORY PLANNER_H_

#include <stdint.h>
#include <Arduino.h>
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
    float v_target;
    float time_total;
  };

  struct Parameters {
    float accel;
    float decel;
  };

  TrajectoryPlanner(Parameters params)
      : state(State::IDLE), current{}, next{}, t_last_ms(0), params(params){}

  bool is_idle() { return state == State::IDLE; }
  void set_next(Plan p) {
    next = p;
    is_next_available = true;
  }

  float run(float pos) {
    if (state == State::IDLE) {
      if (is_next_available) {
        current = next;
        is_next_available = false;

        // TODO: Add start ?
        t_accel = (current.v_target) / params.accel;
        t_decel = current.v_target / params.decel;
        t_c = current.time_total - (t_accel + t_decel);
        v_last = 0;
        p_last = pos;

        t_total = 0;

        t_last_ms = millis();

        state = State::ACCELERATING;
      } else {
        return pos;
      }
    }

    uint32_t now = millis();
    float dt = (now - t_last_ms) * .001;
    t_last_ms = now;
    float v = current.v_target;


    switch (state) {
      case State::ACCELERATING:
        v = v_last + params.accel * dt;
        break;

      case State::CONSTANT:
        v = current.v_target;
        break;

      case State::DECELERATION:
        v = v_last + params.decel * dt;
        break;

      case State::IDLE:
      default:
        break;
    }

    //     Serial.println((uint8_t)state, DEC);
    // Serial.println((float)t_total, DEC);
    // Serial.println((float)current.v_target, DEC);
    // Serial.println((float)v, DEC);
    // Serial.println((float)t_accel, DEC);
    // Serial.println((float)t_c, DEC);
    // Serial.println((float)t_decel, DEC);


    pos = v * dt + p_last;
    p_last = pos;
    t_total += dt;
    v_last = v;

    switch (state) {
      case State::ACCELERATING:
        if (t_total >= t_accel) {
          state = State::CONSTANT;
          t_total = 0;
        }
        break;

      case State::CONSTANT:
        if (t_total >= t_c) {
          state = State::DECELERATION;
          t_total = 0;
        }
        break;

      case State::DECELERATION:
        if (t_total >= t_decel) {
          state = State::IDLE;
          t_total = 0;
        }
        break;

      default:
      case State::IDLE:
        break;
    }

    return pos;
  }

  State get_state() const { return state; }
private:
  State state;
  Plan current;

  Plan next;
  bool is_next_available;

  float t_accel;
  float t_decel;
  float t_c;

  float v_last;
  float p_last;

  uint32_t t_last_ms;

  float t_total;

  Parameters params;
};
#endif
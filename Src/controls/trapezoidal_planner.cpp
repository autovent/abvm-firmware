#include "controls/trapezoidal_planner.h"

TrapezoidalPlanner::TrapezoidalPlanner(Parameters params, uint32_t dt_ms)
    : state(State::IDLE), is_next_available(false), current{}, next{}, dt_ms(dt_ms), params(params) {}

TrapezoidalPlanner::~TrapezoidalPlanner() {}

void TrapezoidalPlanner::reset() {
    state = State::IDLE;
    is_next_available = false;
}

bool TrapezoidalPlanner::is_idle() {
    return state == State::IDLE;
}

void TrapezoidalPlanner::set_next(MotionPlan const &p) {
    next = p;
    is_next_available = true;
}

void TrapezoidalPlanner::force_next(MotionPlan const &p) {
    set_next(p);
    state = State::IDLE;
}

float TrapezoidalPlanner::get_pos() {
    return p_last;
}

float TrapezoidalPlanner::run(float pos, float vel) {
    if (state == State::IDLE) {
        if (is_next_available) {
            current = next;
            is_next_available = false;

            if (dt_ms == 0 || current.time_total_ms <= dt_ms) {
                state = State::IDLE;
                return p_last;
            }

            // Discretize t he time part of the spline.
            t_counts_accel = (int)((params.t_a_percent * current.time_total_ms) / dt_ms);
            t_counts_decel = (int)((params.t_d_percent * current.time_total_ms) / dt_ms);
            t_counts_c = (int)((current.time_total_ms * (saturate(1 - params.t_a_percent - params.t_d_percent, 0, 1))) /
                               dt_ms);

            current.time_total_ms = dt_ms * (t_counts_accel + t_counts_decel + t_counts_c);
            // Find the area under the curve and the median
            float dp = current.p_target - pos;

            // NOTE ON UNITS: v_max, accel, decel are all discrete and do NOT have a
            // time component.
            v_max = (dp) / (.5 * t_counts_decel + .5 * t_counts_accel + t_counts_c);

            // Protect from divide by zero
            if (t_counts_accel == 0 || t_counts_decel == 0) {
                state = State::IDLE;
                return p_last;
            }

            accel = v_max / t_counts_accel;
            decel = -v_max / t_counts_decel;

            v_last = 0;
            p_last = pos;

            t_counts_total = 0;

            state = State::ACCELERATING;
        } else {
            return pos;
        }
    }

    float v = 0;
    switch (state) {
        case State::ACCELERATING:
            v = v_last + accel;
            if (++t_counts_total >= (t_counts_accel)) {
                state = State::CONSTANT;
                t_counts_total = 0;
            }

            break;

        case State::CONSTANT:
            v = v_max;
            if (++t_counts_total >= (t_counts_c)) {
                state = State::DECELERATION;
                t_counts_total = 0;
            }

            break;

        case State::DECELERATION:
            v = v_last + decel;
            if (++t_counts_total >= t_counts_decel) {
                state = State::IDLE;
                t_counts_total = 0;
            }
            break;

        case State::IDLE:
        default:
            break;
    }

    p_last = v + p_last;
    v_last = v;

    return p_last;
}

TrapezoidalPlanner::State TrapezoidalPlanner::get_state() const {
    return state;
}

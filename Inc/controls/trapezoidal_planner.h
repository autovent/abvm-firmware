#ifndef TRAJECTORY_PLANNER_H_
#define TRAJECTORY PLANNER_H_

#include <math.h>
#include <stdint.h>

#include "controls/motion_planner.h"
#include "math/dsp.h"

class TrapezoidalPlanner : public IMotionPlanner {
  public:
    enum class State {
        IDLE,
        ACCELERATING,
        CONSTANT,
        DECELERATION,
    };

    struct Parameters {
        float t_a_percent;
        float t_d_percent;
    };

    TrapezoidalPlanner(Parameters params, uint32_t dt_ms = 10);

    virtual ~TrapezoidalPlanner();
    virtual void reset();
    virtual bool is_idle();
    virtual void set_next(MotionPlan const &p);
    virtual void force_next(MotionPlan const &p);
    virtual float get_pos();
    virtual float run(float pos, float vel = 0);
    State get_state() const;

    State state;
    int32_t t_counts_total;

    MotionPlan current;

    MotionPlan next;
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

    Parameters params;
};
#endif
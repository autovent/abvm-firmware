#pragma once

struct MotionPlan {
    float p_target;
    float v_target;
    float time_total_ms;
};

class IMotionPlanner {
public:
    virtual void set_next(MotionPlan const &p) = 0;
    virtual void force_next(MotionPlan const &p) = 0;
    virtual float run(float pos, float vel = 0) = 0;
    virtual void reset() = 0;
    virtual bool is_idle() = 0;

    virtual float get_pos() = 0;
    virtual ~IMotionPlanner() {}
};
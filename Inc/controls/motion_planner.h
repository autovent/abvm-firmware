#pragma once

struct MotionPlan {
    float p_target;
    float v_target;
    float time_total_ms;
};

class IMotionPlanner {
public:

    virtual void set_next(MotionPlan const &p);
    virtual void force_next(MotionPlan const &p);
    virtual float run(float pos, float vel=0);
    virtual void reset();
    virtual bool is_idle();

    virtual float get_pos();
    virtual ~IMotionPlanner() {}
};
#ifndef PID_H
#define PID_H

class PID {
public:
    struct Params {
        float Kp;
        float Ki;
        float Kd;
    };

public:
    PID(float Kp, float Ki, float Kd, float dt)
        : params{Kp, Ki, Kd}
        , err_acc(0)
        , period(dt)
        , err_last(0)
        , is_init(false)
    {
    }

    PID(Params params, float dt)
        : params(params)
        , err_acc(0)
        , period(dt)
        , err_last(0)
        , is_init(false)
    {
    }

    void reset()
    {
        err_acc = 0;
        err_last = 0;
        is_init = false;
    }

    float update(float target, float meas)
    {
        float err = target - meas;
        if (!is_init) {
            err_last = err;
            err_acc = 0;
            is_init = true;
        }

        err_acc += err;
        float derr = err - err_last;
        err_last = err;

        return params.Kp * err + params.Ki * err_acc * period + (params.Kd * (derr) / period);
    }

    Params params;
private:
    float err_acc;
    float period;
    float err_last;
    bool is_init;

};

#endif
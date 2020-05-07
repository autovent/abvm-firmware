#pragma once

#include "config.h"
#include "drivers/pin.h"
#include "math/dsp.h"
#include "servo.h"

class HomingController {
public:
    enum class State { IDLE = 0, HOMING, HOMING_BACKUP, DONE };

    // TODO: Add max current limit
    HomingController(Servo *motor);
    void start();
    bool is_done();
    State update();

private:
    Servo *motor;
    State state;

    float homing_velocity_rad_per_sec;
    float backoff_velocity_rad_per_sec;
};

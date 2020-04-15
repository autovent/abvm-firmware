#pragma once
#include "math/dsp.h"

template <typename T = float>
struct Range {
    float min;
    float max;

    float saturate(float a) {
        return ::saturate(a, min, max);
    }
    bool in(float a) {
        return a >= min && a <= max;
    }
};
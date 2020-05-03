#pragma once

struct LinearFit {
    // mx + b params
    float m;
    float b;

    inline float calculate(float x) const {
        return m * x + b;
    }
};

#pragma once

#include <assert.h>

struct Ratio {
    float a;
    float b;


    inline float to_percent(bool part) {
        return (part ? b : a) / (a+b);
    }
};
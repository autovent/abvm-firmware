#ifndef DSP_MATH_H_
#define DSP_MATH_H_

#include <math.h>

constexpr float saturate(float val, float min, float max) {
    // clang-format off
    return (val < min) ? min 
                       : ((val > max) ? max 
                                      : val);
    // clang-format on
}

constexpr float signof(float x) {
    return (float)((x > 0) - (x < 0));
}

/**
 * Discrete Time Rate Limit
 */
inline float dt_rate_limit(float target, float current, float rate) {
    float dx = target - current;
    return (fabsf(dx) > rate) ? (current + signof(dx) * rate) : target;
}

template <typename T>
constexpr T map(T x, T in_min, T in_max, T out_min, T out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif
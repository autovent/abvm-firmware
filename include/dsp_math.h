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

constexpr float signof(float x) { return (float)((x > 0) - (x < 0)); }

bool approximatelyEqual(float a, float b, float epsilon)
{
    return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool essentiallyEqual(float a, float b, float epsilon)
{
    return fabs(a - b) <= ( (fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool definitelyGreaterThan(float a, float b, float epsilon)
{
    return (a - b) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool definitelyLessThan(float a, float b, float epsilon)
{
    return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

#endif
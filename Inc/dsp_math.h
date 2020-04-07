#ifndef DSP_MATH_H_
#define DSP_MATH_H_

constexpr float saturate(float val, float min, float max)
{
    // clang-format off
    return (val < min) ? min 
                       : ((val > max) ? max 
                                      : val);
    // clang-format on
}

constexpr float signof(float x)
{
    return (float)((x > 0) - (x < 0));
}

#endif
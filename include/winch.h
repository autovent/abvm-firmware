#ifndef WINCH_H
#define WINCH_H

#include "Arduino.h"

template <typename T> struct Range {
    T min;
    T max;
};

class Winch {
public:
    Winch(uint32_t p,
          Range<uint32_t> span_us = {600, 2400},
          Range<float> span_degrees = {0, 180},
          bool invert = false,
          float f = 50,
          uint32_t res = 16)
        : pin(p)
        , span_us(span_us)
        , span_degrees(span_degrees)
        , freq(f)
        , resolution(res)
        , invert(invert)
    {
    }

    void init()
    {
        analogWriteFrequency(pin, freq);
        analogWriteResolution(resolution);
    }

    void writeMicroseconds(uint32_t us)
    {
        uint32_t period_us = (1000000 / freq); // Freq in Hz to period in us

        uint32_t cmd = (1 << resolution) * (float)(period_us - us) / period_us;
        if (cmd != last_cmd) {
            last_cmd = cmd;
            analogWrite(pin, cmd);
        }
    }

    void writeDegrees(float degrees)
    {
        uint32_t us;
        if (invert) {
            us = map(degrees, span_degrees.min, span_degrees.max, span_us.max, span_us.min);
        } else {
            us = map(degrees, span_degrees.min, span_degrees.max, span_us.min, span_us.max);
        }

        writeMicroseconds(us);
    }

    uint32_t last_cmd = 0;
    uint32_t pin;
    Range<uint32_t> span_us;
    Range<float> span_degrees;

    float freq;
    uint32_t resolution;

    bool invert;
};

#endif // WINCH_H
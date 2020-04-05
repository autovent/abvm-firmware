#ifndef CURRENT_SENSOR_H_
#define CURRENT_SENSOR_H_
#include "circular_buffer.h"
#include "Arduino.h"

class CurrentSensor {
public:
    const float kVDC_V = 3.3;
    const float kK_V_per_A = .02;
    const uint32_t kAdcResolution = 12;
    const uint32_t kAdcMaxVal = (1 << kAdcResolution);

    CurrentSensor(uint32_t pin)
        : pin(pin)
    {
        analogReadResolution(kAdcResolution);
        m = kVDC_V / kAdcMaxVal / kK_V_per_A;
        b =  -.062 / kK_V_per_A/* 48mV */; // offset needs to be configurable probably.
    }

    void update()
    {
        buffer.push(m * analogRead(pin) + b);
    }

    float get()
    {
        // Apply a square FIR averaging filter over all elements in the buffer.
        float avg = 0;
        for (size_t i = 0; i < buffer.count(); i++) {
            avg += *buffer.peek(i) * (1.0 / buffer.max_size());
        }
        return avg;
    }

    uint32_t pin;
    CircularBuffer<float, 120, true> buffer;
    float m;
    float b;
    float offset;
};

#endif // CURRENT_SENSOR_H_

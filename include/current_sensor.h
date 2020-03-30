#ifndef CURRENT_SENSOR_H_
#define CURRENT_SENSOR_H_
#include "circular_buffer.h"
#include "Arduino.h"

class CurrentSensor
{
public:
  const float kVDC_V = 3.3;
  const float kK_A_per_V = 2.5;
  const float kAdcResolution = 1024;

  CurrentSensor(uint32_t pin) : pin(pin)
  {
    b = 0;
    m = kVDC_V / (kAdcResolution * kK_A_per_V);
    offset = 0;
  }

  void update()
  {
    buffer.push(m * (analogRead(pin) + offset) + b);
  }

  float get()
  {
    // Apply a square FIR averaging filter over all elements in the buffer.
    float avg = 0;
    for (size_t i = 0; i < buffer.count(); i++)
    {
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

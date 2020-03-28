#ifndef WINCH_H
#define WINCH_H

#include "Arduino.h"

class Winch {
public:

Winch(uint32_t p, float f=50 , uint32_t res=16) : pin(p), freq(f), resolution(res) {
}

void init() {
  analogWriteFrequency(pin, freq /* 50 Hz */);
  analogWriteResolution(resolution);
  
}

void writeMicroseconds(uint32_t us) {
  uint32_t period_us = (1000000 / freq);
  
  uint32_t cmd = (1<<resolution) * (float)(period_us - us) / period_us;
  if (cmd != last_cmd) {
    last_cmd = cmd;
    analogWrite(pin, cmd);
  }
}

uint32_t last_cmd = 0;
uint32_t pin;
float freq;
uint32_t resolution;
};

#endif // WINCH_H
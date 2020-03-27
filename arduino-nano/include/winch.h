#ifndef WINCH_H
#define WINCH_H

#include "Arduino.h"

class Winch {
public:

Winch(uint32_t p, float freq=50) : pin(p), freq(freq){
}

void init() {
  analogWriteFrequency(pin, freq /* 50 Hz */);
  analogWriteResolution(10);
  
}

void writeMicroseconds(uint32_t us) {
  uint32_t period_us = (1000000 / freq);
  Serial.println(period_us);
  Serial.println(1024 * (float)(period_us - us) / period_us, DEC);
  uint32_t cmd = 1024 * (float)(period_us - us) / period_us;
  if (cmd != last_cmd) {
    last_cmd = cmd;
    analogWrite(pin, cmd);
  }
}

uint32_t last_cmd = 0;
uint32_t pin;
float freq;
};

#endif // WINCH_H
#ifndef SENSIRION_SFM4100_H_
#define SENSIRION_SFM4100_H_
#include <Wire.h>
#include <stdint.h>

class Sensirion_SFM4100 {
public:
    static constexpr uint8_t kAddress = 1;
    static constexpr uint8_t kMeasureAndReadCommand = 0xF1;
    Sensirion_SFM4100(float dt) : dt(dt),integrator(0){
        Wire.begin();
    }

    void start() {
        Wire.beginTransmission(kAddress);
        Wire.write(0xF1);
        Wire.endTransmission();
        Wire.requestFrom(kAddress, (uint8_t) 2);
    }

    void recv() {
        if (Wire.available() == 2) {
            int16_t raw = Wire.read() << 8;
            raw |= Wire.read();
            cc_per_min = raw;
            integrator += get_cc_per_sec() * dt;
        }
    }

    void reset() {
        integrator = 0;
    }

    float get_mass_flow() {
        return integrator;
    }

    float get_reset_mass_flow() {
        float val = integrator;
        reset();
        return val;
    }

    float get_cc_per_min() {
        return cc_per_min;
    }

    float get_cc_per_sec() {
        return cc_per_min / 60;
    }


    float cc_per_min = 0;
    float integrator;
    float dt;
    uint32_t time_last_ms;
};
#endif
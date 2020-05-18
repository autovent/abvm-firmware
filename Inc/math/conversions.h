#pragma once
#include <math.h>
#include <stdint.h>

constexpr float deg_to_rad(float x) {
    return x * 2 * M_PI / 360.0f;
}
constexpr float rad_to_deg(float x) {
    return 360.0f * x / (2 * M_PI);
}
constexpr uint32_t bpm_to_time_ms(float bpm) {
    return 900 * 60 / bpm; //  1000 * 60 / bpm;
}
constexpr float psi_to_cmH2O(float x) {
    return 70.307 * x;
}
constexpr float msec_to_sec(float x) {
    return x / 1000.0f;
}

constexpr float  rad_per_sec_to_rpm(float x) { return x* 9.5493; }
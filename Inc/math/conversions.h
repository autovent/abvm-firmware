#pragma once

constexpr float deg_to_rad(float x) { return x * 2 * M_PI / 360.0f; }
constexpr float rad_to_deg(float x) { return 360.0f * x / (2 * M_PI); }
constexpr uint32_t bpm_to_time_ms(float bpm) { return 1000 * 60 / bpm; }

#pragma once
#include "constants.h"

constexpr float deg_to_rad(float x) { return x * 2 * M_PI / 360.0f; }
constexpr float rad_to_deg(float x) { return 360.0f * x / (2 * M_PI); }

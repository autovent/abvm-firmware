#pragma once

struct Version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
};

constexpr Version APP_VERSION = {0, 14, 0};
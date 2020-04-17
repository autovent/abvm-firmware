#pragma once

#include "platform.h"

struct Pin {
    GPIO_TypeDef *port;
    uint16_t pin;

    bool read();

    void write(bool val);
};
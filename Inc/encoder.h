#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

#include "platform.h"

class Encoder {
public:
    Encoder(TIM_HandleTypeDef *tim);

    void init();

    int16_t get();
    void reset();

    bool is_inverted;

private:
    TIM_HandleTypeDef *htim;
};

#endif  // ENCODER_H
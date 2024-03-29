#include "encoder.h"

#define HALFWAY 32768

Encoder::Encoder(TIM_HandleTypeDef *tim) : htim(tim) {}

void Encoder::init() {
    reset();
    HAL_TIM_Encoder_Start(htim, TIM_CHANNEL_ALL);
}

int16_t Encoder::get() {
    if (is_inverted) {
        return HALFWAY - htim->Instance->CNT;
    } else {
        // return the counter offset from the halfway point
        return htim->Instance->CNT - HALFWAY;
    }
}

void Encoder::reset() {
    // reset the counter to the halfway point
    htim->Instance->CNT = HALFWAY;
}

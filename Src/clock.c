#include "clock.h"

#include "platform.h"
#include "tim.h"

uint32_t millis() {
    return HAL_GetTick();
}
uint32_t micros() {
    return TIM_GetMicros();
}

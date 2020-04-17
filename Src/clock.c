#include "clock.h"

#include "platform.h"
#include "tim.h"

uint32_t millis() {
    return HAL_GetTick();
}

uint64_t micros() {
    return TIM_GetMicros();
}

#include "clock.h"

#include "platform.h"
#include "tim.h"

uint32_t millis() {
    return HAL_GetTick();
}

uint64_t micros() {
    return TIM_GetMicros();
}

uint32_t delay_ms(uint32_t x) {
    HAL_Delay(x);
}

uint32_t delay_us(uint32_t x) {
    TIM_DelayMicros(x);
}

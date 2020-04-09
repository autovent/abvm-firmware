#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_init(
    GPIO_TypeDef *sleep_port,
    uint16_t sleep_pin,
    GPIO_TypeDef *disable_port,
    uint16_t disable_pin,
    GPIO_TypeDef *fault_port,
    uint16_t fault_pin,
    TIM_HandleTypeDef *htim,
    uint32_t tim_channel_pwm1,
    uint32_t tim_channel_pwm2,
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *cs_port,
    uint16_t cs_pin
);

#ifdef __cplusplus
}
#endif

#endif
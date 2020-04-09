#include "motor_driver.h"
#include "drv8873.h"

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
) {
    DRV8873 drv(
        sleep_port,
        sleep_pin,
        disable_port,
        disable_pin,
        fault_port,
        fault_pin,
        htim,
        tim_channel_pwm1,
        tim_channel_pwm2,
        hspi,
        cs_port,
        cs_pin
    );

    drv.set_pwm_enabled(true);
    drv.set_pwm(DRV8873::PWM_CHANNEL_1, 0.5);
    drv.set_pwm(DRV8873::PWM_CHANNEL_2, 0.5);
}
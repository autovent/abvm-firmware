#include "drv8873.h"

#include <assert.h>
#include <math.h>

#include "adc.h"
#include "math/dsp.h"

DRV8873::DRV8873(GPIO_TypeDef *sleep_port, uint16_t sleep_pin, GPIO_TypeDef *disable_port, uint16_t disable_pin,
                 GPIO_TypeDef *fault_port, uint16_t fault_pin, TIM_HandleTypeDef *htim, uint32_t tim_channel_pwm1,
                 uint32_t tim_channel_pwm2, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin,
                 bool is_inverted)
    : sleep_port(sleep_port),
      sleep_pin(sleep_pin),
      disable_port(disable_port),
      disable_pin(disable_pin),
      fault_port(fault_port),
      fault_pin(fault_pin),
      htim(htim),
      tim_channel_pwm1(tim_channel_pwm1),
      tim_channel_pwm2(tim_channel_pwm2),
      hspi(hspi),
      cs_port(cs_port),
      cs_pin(cs_pin),
      is_inverted(is_inverted) {}

void DRV8873::init() {
    GPIO_InitTypeDef init;

    init.Pin = disable_pin;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(disable_port, &init);

    init.Pin = sleep_pin;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(sleep_port, &init);

    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
}

void DRV8873::set_current_raw_meas_dma(uint32_t *dma) {
    current_raw_dma = dma;
}

float DRV8873::get_current() {
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    volatile float adc_voltage = (val * VREF) / 4096.0f;
    float load_current = adc_voltage / R_LOAD;
    return load_current * I_MIRROR_RATIO;
}

void DRV8873::set_pwm_enabled(bool enable) {
    if (enable) {
        HAL_TIM_PWM_Start(htim, tim_channel_pwm1);
        HAL_TIM_PWM_Start(htim, tim_channel_pwm2);
    } else {
        HAL_TIM_PWM_Stop(htim, tim_channel_pwm1);
        HAL_TIM_PWM_Stop(htim, tim_channel_pwm2);
    }
}

void DRV8873::set_pwm(float value) {
    // assert(value >= -1 && value <= 1);
    value = saturate(value, -1, 1);
    uint32_t max = htim->Init.Period;
    uint32_t pwm = max * (1 - fabsf(value));

    bool dir = (value < 0) ^ is_inverted;
    __HAL_TIM_SET_COMPARE(htim, tim_channel_pwm1, dir ? pwm : max);
    __HAL_TIM_SET_COMPARE(htim, tim_channel_pwm2, dir ? max : pwm);
}

void DRV8873::set_disabled(bool disable) {
    if (disable) {
        HAL_GPIO_WritePin(disable_port, disable_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(disable_port, disable_pin, GPIO_PIN_RESET);
    }
}

void DRV8873::set_sleep(bool sleep) {
    if (sleep) {
        HAL_GPIO_WritePin(sleep_port, sleep_pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(sleep_port, sleep_pin, GPIO_PIN_SET);
    }
}

bool DRV8873::get_fault() {
    return HAL_GPIO_ReadPin(fault_port, fault_pin) == GPIO_PIN_RESET;
}

uint8_t DRV8873::get_reg(uint8_t reg_addr) {
    return run_spi_transaction(true, reg_addr, 0x00);
}

void DRV8873::set_reg(uint8_t reg_addr, uint8_t value) {
    run_spi_transaction(false, reg_addr, value);
}

uint8_t DRV8873::get_status_reg() {
    return status_reg;
}

uint8_t DRV8873::run_spi_transaction(bool read, uint8_t reg_addr, uint8_t data) {
    // TODO(cw): This is not working yet
    uint8_t tx_data[2] = {((read & 0x1) << 6) | ((reg_addr & 0x1F) << 1), data};
    uint8_t rx_data[2] = {0};
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(hspi, tx_data, rx_data, sizeof(tx_data), 100);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    status_reg = rx_data[0];
    return rx_data[1];
}

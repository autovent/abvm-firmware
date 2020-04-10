#include "drv8873.h"
#include <assert.h>


DRV8873::DRV8873(
    GPIO_TypeDef *sleep_port,
    uint16_t sleep_pin,  
    GPIO_TypeDef *disable_port,
    uint16_t disable_pin,
    GPIO_TypeDef *fault_port,
    uint16_t fault_pin,
    GPIO_TypeDef *direction_port,
    uint16_t direction_pin,
    TIM_HandleTypeDef *htim,
    uint32_t tim_channel_pwm,
    SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *cs_port,
    uint16_t cs_pin
) :
    sleep_port(sleep_port),
    disable_port(disable_port),
    disable_pin(disable_pin),
    fault_port(fault_port),
    fault_pin(fault_pin),
    direction_port(direction_port),
    direction_pin(direction_pin),
    htim(htim),
    tim_channel_pwm(tim_channel_pwm),
    hspi(hspi),
    cs_port(cs_port),
    cs_pin(cs_pin)
{}

void DRV8873::set_current_raw_meas_dma(uint32_t *dma) {
    current_raw_dma = dma;
}

float DRV8873::get_current() {
    float adc_voltage = (*current_raw_dma * VREF) / 4096.0f;
    float load_current = adc_voltage / R_LOAD;
    return load_current * I_MIRROR_RATIO;
}

void DRV8873::set_pwm_enabled(bool enable) {
    if (enable) {
        HAL_TIM_PWM_Start(htim, tim_channel_pwm);
    } else {
        HAL_TIM_PWM_Stop(htim, tim_channel_pwm);
    }
}

void DRV8873::set_pwm(float value) {
    assert(value >= 0 && value <= 1);

    uint32_t max = htim->Init.Period;
    uint32_t pwm = max * value;

    __HAL_TIM_SET_COMPARE(htim, tim_channel_pwm, pwm);
}

void DRV8873::set_direction(motor_direction direction) {
    if (direction == DIRECTION_FORWARD) {
        HAL_GPIO_WritePin(direction_port, direction_pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(direction_port, direction_pin, GPIO_PIN_RESET);
    }
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
    uint8_t tx_data[2] = {((read & 0x1) << 6) | ((reg_addr & 0x1F) << 1), data};
    uint8_t rx_data[2] = {0};
    HAL_SPI_TransmitReceive(hspi, tx_data, rx_data, sizeof(tx_data), 100);
    status_reg = rx_data[0];
    return rx_data[1];
}

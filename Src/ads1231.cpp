#include "ads1231.h"

#include <math.h>

#include "clock.h"
#include "spi.h"

#define SPI_TIMEOUT 10

ADS1231::ADS1231(GPIO_TypeDef *powerdown_port, uint32_t powerdown_pin, SPI_HandleTypeDef *hspi, GPIO_TypeDef *miso_port,
                 uint32_t miso_pin, GPIO_TypeDef *sclk_port, uint32_t sclk_pin, LinearFit const *linear_fit_mv)
    : powerdown_port(powerdown_port),
      powerdown_pin(powerdown_pin),
      miso_port(miso_port),
      miso_pin(miso_pin),
      sclk_port(sclk_port),
      sclk_pin(sclk_pin),
      hspi(hspi),
      vref(3.06),
      linear_fit_mv(linear_fit_mv) {}

void ADS1231::init() {
    enable_spi(false);
}

float ADS1231::read_volts() {
    return volts;
}

float ADS1231::read() {
    return linear_fit_mv->calculate(volts * 1e3); // Convert to mV first
}

/**
 * Check if a measurement is ready, if it is get the value.
 */
bool ADS1231::update() {
    if (is_ready()) {
        uint8_t data[3];
        enable_spi(true);
        HAL_SPI_Receive(hspi, data, sizeof(data), SPI_TIMEOUT);
        enable_spi(false);

        delay_us(1);
        HAL_GPIO_WritePin(sclk_port, sclk_pin, GPIO_PIN_SET);
        delay_us(1);
        HAL_GPIO_WritePin(sclk_port, sclk_pin, GPIO_PIN_RESET);

        // Two's complement of 24 bit value. Make sure the sign gets
        // extended into the upper byte.
        int32_t next_value = ((data[0] << 24) | (data[1] << 16) | (data[2] << 8)) >> 8;
        value = rejection_filter(next_value);
        volts = convert_to_volts(value, 128, vref);
        return true;
    } else {
        return false;
    }
}

bool ADS1231::is_ready() {
    return HAL_GPIO_ReadPin(miso_port, miso_pin) == GPIO_PIN_RESET;
}

// See datasheet page 12
constexpr float ADS1231::convert_to_volts(int32_t x, float gain, float vref) {
    return (static_cast<float>(x) / (1 << 24)) * vref / gain;
}

void ADS1231::set_powerdown(bool pwrdn) {
    if (pwrdn) {
        HAL_GPIO_WritePin(powerdown_port, powerdown_pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(powerdown_port, powerdown_pin, GPIO_PIN_SET);
    }
}

void ADS1231::enable_spi(bool spi_on) {
    if (spi_on) {
        MX_SPI1_Init();
    } else {
        HAL_SPI_DeInit(hspi);

        GPIO_InitTypeDef gpio_init;

        gpio_init.Pin = miso_pin;
        gpio_init.Mode = GPIO_MODE_INPUT;
        gpio_init.Pull = GPIO_PULLDOWN;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(miso_port, &gpio_init);

        gpio_init.Pin = sclk_pin;
        gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
        gpio_init.Pull = GPIO_PULLDOWN;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(sclk_port, &gpio_init);

        HAL_GPIO_WritePin(sclk_port, sclk_pin, GPIO_PIN_RESET);
    }
}

int32_t ADS1231::rejection_filter(int32_t next) {
    if (is_first) {
        value = next;
        is_first = false;
    }

    if (abs(next - value) > 1 << 18) {
        if (++rejects > 10) {
            rejects = 0;
        } else {
            return value;
        }
    } else {
        rejects = 0;
    }
    return next;
}

#include "ads1231.h"
#include "tim.h"

#define SPI_TIMEOUT 10

ADS1231::ADS1231()
    : powerdown_pin(0)
    , m(1)
    , offset(0)
    , b(0)
{}

ADS1231::ADS1231(GPIO_TypeDef *powerdown_port, uint32_t powerdown_pin, SPI_HandleTypeDef *hspi,
    GPIO_TypeDef *miso_port, uint32_t miso_pin, GPIO_TypeDef *sclk_port, uint32_t sclk_pin,
    float m, float offset, float b) :
    powerdown_port(powerdown_port),
    powerdown_pin(powerdown_pin),
    miso_port(miso_port),
    miso_pin(miso_pin),
    sclk_port(sclk_port),
    sclk_pin(sclk_pin),
    hspi(hspi),
    vref(3.3),
    m(m),
    offset(offset),
    b(b)
{}

void ADS1231::init() {
    HAL_SPI_DeInit(hspi);
    GPIO_InitTypeDef init;
    init.Pin = miso_pin;
    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_PULLDOWN;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(miso_port, &init);
}

float ADS1231::read_volts() {
    return volts;
}

float ADS1231::read() {
    return m * (volts + offset) + b;
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

        TIM_DelayMicros(1);
        HAL_GPIO_WritePin(sclk_port, sclk_pin, GPIO_PIN_SET);
        TIM_DelayMicros(1);
        HAL_GPIO_WritePin(sclk_port, sclk_pin, GPIO_PIN_RESET);

        int32_t value = (data[0] << 16) | (data[1] << 8) | (data[2]);

        volts = convert_to_volts(value, 128, vref);
        return true;
    } else {
        return false;
    }
}

void ADS1231::measure() {
    HAL_GPIO_WritePin(powerdown_port, powerdown_pin, GPIO_PIN_SET);
    is_measuring = true;
}

bool ADS1231::is_ready() {
    return HAL_GPIO_ReadPin(miso_port, miso_pin) == GPIO_PIN_RESET;
}

// See datasheet page 12
constexpr float ADS1231::convert_to_volts(uint32_t x, float gain, float vref) {
    return ((static_cast<float>(x) / kOffsetBinaryCodeZero) - 1) * vref / gain;
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
        HAL_SPI_Init(hspi);

        GPIO_InitTypeDef gpio_init;

        gpio_init.Pin = miso_pin;
        gpio_init.Mode = GPIO_MODE_INPUT;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(miso_port, &gpio_init);

        gpio_init.Pin = sclk_pin;
        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_LOW;

        HAL_GPIO_Init(sclk_port, &gpio_init);
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
#ifndef ADS1231_H
#define ADS1231_H

#include "drivers/sensor.h"
#include "math/linear_fit.h"
#include "platform.h"

class ADS1231 : public ISensor {
public:
    ADS1231(GPIO_TypeDef *powerdown_port, uint32_t powerdown_pin, SPI_HandleTypeDef *hspi, GPIO_TypeDef *miso_port,
            uint32_t miso_pin, GPIO_TypeDef *sclk_port, uint32_t sclk_pin, LinearFit const *linear_fit_mV);

    void init();

    float read_volts();

    float read();

    /**
     * Check if a measurement is ready, if it is get the value.
     */
    bool update();

    void set_powerdown(bool pwrdn);

private:
    static constexpr uint32_t kOffsetBinaryCodeZero = (1 << 23);  // 2^23 is the halfway point

    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *miso_port;
    uint32_t miso_pin;
    GPIO_TypeDef *sclk_port;
    uint32_t sclk_pin;

    GPIO_TypeDef *powerdown_port;
    uint32_t powerdown_pin;

    LinearFit const *linear_fit_mv;

    bool is_measuring;

    float volts;
    float vref;
    int32_t value;
    bool is_first = true;

    uint32_t rejects = 0;
    bool is_ready();

    void enable_spi(bool spi_on);
    int32_t rejection_filter(int32_t next);

    // See datasheet page 12
    static constexpr float convert_to_volts(int32_t x, float gain, float vref);
};

#endif

#include "stm32f1xx_hal.h"

class ADS1231 {
public:
    ADS1231();

    ADS1231(GPIO_TypeDef *powerdown_port, uint32_t powerdown_pin, SPI_HandleTypeDef *hspi,
        GPIO_TypeDef *miso_port, uint32_t miso_pin, GPIO_TypeDef *sclk_port, uint32_t sclk_pin,
        float m = 1, float offset = 0, float b = 0);

    void init();

    float read_volts();

    float read();

    /**
     * Check if a measurement is ready, if it is get the value.
     */
    bool update();

    void measure();

    void set_powerdown(bool pwrdn);

private:
    static constexpr uint8_t kGainMask = 0x04;
    static constexpr uint32_t kOffsetBinaryCodeZero = (1 << 23); // 2^23 is the halfway point

    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *miso_port;
    uint32_t miso_pin;
    GPIO_TypeDef *sclk_port;
    uint32_t sclk_pin;

    GPIO_TypeDef *powerdown_port;
    uint32_t powerdown_pin;

    bool is_measuring;

    float volts;
    float vref;

    float m;
    float offset;
    float b;

    bool is_ready();

    // See datasheet page 12
    static constexpr float convert_to_volts(uint32_t x, float gain, float vref);
};

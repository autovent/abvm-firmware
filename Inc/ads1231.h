#include "platform.h"

class ADS1231 {
public:
    ADS1231();

    ADS1231(GPIO_TypeDef *powerdown_port, uint32_t powerdown_pin, SPI_HandleTypeDef *hspi, GPIO_TypeDef *miso_port,
            uint32_t miso_pin, GPIO_TypeDef *sclk_port, uint32_t sclk_pin, float m = 1, float b = 0);

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

    bool is_measuring;

    float volts;
    float vref;
    int32_t value;
    bool is_first = true;
    float m;
    float b;

    uint32_t rejects = 0;
    bool is_ready();

    void enable_spi(bool spi_on);
    int32_t rejection_filter(int32_t next);

    // See datasheet page 12
    static constexpr float convert_to_volts(int32_t x, float gain, float vref);
};

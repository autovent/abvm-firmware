#ifndef LC064_H
#define LC064_H

#include "eeprom.h"
#include "stm32f1xx_hal.h"

class LC064 : public EEPROM<uint16_t, uint8_t> {
  public:
    LC064(I2C_HandleTypeDef *hi2c, uint8_t dev_addr);

    void init();

    bool read(uint16_t addr, uint8_t *data, uint16_t len) override;
    bool write(uint16_t addr, uint8_t *data, uint16_t len) override;

    bool allocate(uint16_t size, uint16_t *addr) override;

  private:
    static constexpr uint32_t I2C_TIMEOUT = 1000;
    static constexpr uint8_t NUM_TEST_TRIALS = 100;

    static constexpr uint8_t DEV_ADDR_BASE = 0xA0;
    static constexpr uint8_t DEV_ADDR_MASK = 0x07;
    static constexpr uint8_t DEV_ADDR_SHIFT = 1;

    static constexpr uint16_t TOTAL_SIZE = 8192;  // bytes
    static constexpr uint16_t PAGE_SIZE = 32;     // bytes

    I2C_HandleTypeDef *hi2c;
    uint8_t dev_addr;

    bool ready();

    uint16_t allocator_max_size;
    uint16_t allocator_offset;
};

#endif  // LC064

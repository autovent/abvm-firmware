#pragma once

#include "stm32f1xx_hal.h"

class MS4525DO {
  public:
    enum class State { IDLE, MEASURING };

    MS4525DO(I2C_HandleTypeDef *hi2c);

    void init();
    /**
     * Read the pressure transducer
     */
    bool request();
    bool measure();

    void update();

    float get_pressure();

    float get_temp();

  private:
    static constexpr uint8_t kDevAddress = 0x28;

    I2C_HandleTypeDef *hi2c;

    volatile uint32_t raw_pressure;
    volatile uint32_t raw_temp;

    State state;
};

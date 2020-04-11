#ifndef LC064_H
#define LC064_H

#include "eeprom.h"
#include "stm32f1xx_hal.h"

class LC064 : EEPROM {
    LC065(
        I2C_HandleTypeDef *hi2c
    );

    uint8_t read_byte(uint16_t addr);

    void read_bytes(uint16_t addr, uint8_t *data, uint16_t len);

    void write_byte(uint16_t addr, uint8_t data);

    void write_bytes(uint16_t addr, uint8_t *data, uint16_t len);
}

#endif  // _24LC064_H

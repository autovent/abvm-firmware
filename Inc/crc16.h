#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>
#include <stddef.h>

class CRC16 {
public:
    static uint16_t calc(uint8_t *data, size_t size);

private:
    static const uint16_t crc_table[256];
};

#endif  // CRC16_H

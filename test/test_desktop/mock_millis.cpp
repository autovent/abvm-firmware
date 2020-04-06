#include <stdint.h>

static uint32_t m_millis = 0;

uint32_t millis() {
    return m_millis;
}

void set_millis(uint32_t millis) {
    m_millis = millis;
}
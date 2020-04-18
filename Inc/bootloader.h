#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include <stdint.h>

class BootLoader {
public:
    static void start_bootloader();

private:
    static const uint32_t *BOOTLOADER_RESET_VECTOR;
    static const void (*bootloader)(void);
};

#endif
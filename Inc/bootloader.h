#ifndef BOOTLOADER_H
#define BOOTLOADER_H
#include <stdint.h>

class BootLoader {
public:
    enum boot_select {
        BOOT_SELECT_UNKNOWN,
        BOOT_SELECT_BOOTLOADER,
        BOOT_SELECT_APP,
    };

    static void set_next_boot(boot_select sel);
    static boot_select get_next_boot();
    static void reboot();

private:
    static volatile uint32_t *BKP_REG;
    static constexpr uint32_t BOOTLOADER_CODE = 0x544F4F42UL;
    static constexpr uint32_t APP_CODE = 0x3F82722AUL;
};

#endif;
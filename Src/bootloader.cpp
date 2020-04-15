#include "bootloader.h"
#include "stm32f1xx_hal.h"

volatile uint32_t *BootLoader::BKP_REG = (volatile uint32_t*)0x20004000;

void BootLoader::set_next_boot(boot_select sel) {
    if (sel == BOOT_SELECT_APP) {
        *BKP_REG = APP_CODE;
    } else if (sel == BOOT_SELECT_BOOTLOADER) {
        *BKP_REG = BOOTLOADER_CODE;
    }
}

BootLoader::boot_select BootLoader::get_next_boot() {
    if (*BKP_REG == APP_CODE) {
        return BOOT_SELECT_APP;
    } else if (*BKP_REG == BOOTLOADER_CODE) {
        return BOOT_SELECT_BOOTLOADER;
    } else {
        return BOOT_SELECT_UNKNOWN;
    }
}

void BootLoader::reboot() {
    NVIC_SystemReset();
}

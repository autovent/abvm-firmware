#include "bootloader.h"
#include "platform.h"

// bootloader reset vector is the second word of system memory
const uint32_t *BootLoader::BOOTLOADER_RESET_VECTOR = (uint32_t*)0x1fffd804;
const void (*BootLoader::bootloader)(void) = (const void (*)(void))(*BOOTLOADER_RESET_VECTOR);

void BootLoader::start_bootloader() {
    // deinit peripherals
    HAL_DeInit();

    // turn off systick
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // set stack pointer to default value
    __set_MSP(0x20001000);

    // jump to the bootloader
    bootloader();

    while(1);
}

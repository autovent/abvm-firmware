#include "lc064.h"

#include <assert.h>

LC064::LC064(I2C_HandleTypeDef *hi2c, uint8_t dev_addr) : hi2c(hi2c) {}

void LC064::init() {
    dev_addr = DEV_ADDR_BASE | ((dev_addr & DEV_ADDR_MASK) << DEV_ADDR_SHIFT);
}

bool LC064::read(uint16_t addr, uint8_t *data, uint16_t len) {
    assert(addr <= TOTAL_SIZE);
    assert(len <= TOTAL_SIZE);

    if (!ready()) return false;

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hi2c, dev_addr, addr, I2C_MEMADD_SIZE_16BIT, data, len, I2C_TIMEOUT);
    return status == HAL_OK;
}

bool LC064::write(uint16_t addr, uint8_t *data, uint16_t len) {
    assert(addr <= TOTAL_SIZE);
    assert(len <= TOTAL_SIZE);

    uint32_t bytes_written = 0;
    do {
        uint32_t max_bytes = PAGE_SIZE - ((addr + bytes_written) % PAGE_SIZE);
        uint32_t bytes_to_write = len - bytes_written;
        if (bytes_to_write > max_bytes) bytes_to_write = max_bytes;

        if (!ready()) return false;

        HAL_StatusTypeDef status = HAL_I2C_Mem_Write(hi2c, dev_addr, addr + bytes_written, I2C_MEMADD_SIZE_16BIT,
                                                     &data[bytes_written], bytes_to_write, I2C_TIMEOUT);

        if (status != HAL_OK) {
            return false;
        }

        bytes_written += bytes_to_write;
    } while (bytes_written < len);

    return true;
}

bool LC064::allocate(uint16_t size, uint16_t *addr) {
    if (allocator_max_size == 0) {
        allocator_max_size = TOTAL_SIZE;
    }

    if ((allocator_offset + size) < allocator_max_size) {
        *addr = allocator_offset;

        // align the next offset to the next page
        allocator_offset += size + (PAGE_SIZE - (size % PAGE_SIZE));
        return true;
    }

    return false;
}

bool LC064::erase() {
    uint8_t erase_buf[PAGE_SIZE] = {0};
    bool success;
    for (uint16_t addr = 0; addr < TOTAL_SIZE; addr += PAGE_SIZE) {
        success = write(addr, erase_buf, PAGE_SIZE);
        if (!success) return false;
    }
    return true;
}

bool LC064::ready() {
    volatile HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c, dev_addr, NUM_TEST_TRIALS, I2C_TIMEOUT);
    return status == HAL_OK;
}

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

    if (!ready()) return false;

    uint16_t last_addr = addr + len;

    do {
        uint16_t next_page_boundary = (addr - (addr % PAGE_SIZE)) + PAGE_SIZE;
        uint16_t bytes_to_write = next_page_boundary - addr;

        if (bytes_to_write > last_addr - addr) bytes_to_write = last_addr - addr;

        HAL_StatusTypeDef status =
              HAL_I2C_Mem_Write(hi2c, dev_addr, addr, I2C_MEMADD_SIZE_16BIT, data, bytes_to_write, I2C_TIMEOUT);

        if (status != HAL_OK) {
            return false;
        }

        addr += bytes_to_write;
    } while (addr < last_addr);

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

bool LC064::ready() {
    volatile HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hi2c, dev_addr, NUM_TEST_TRIALS, I2C_TIMEOUT);
    return status == HAL_OK;
}

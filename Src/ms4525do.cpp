#include "drivers/ms4525do.h"

MS4525DO::MS4525DO(I2C_HandleTypeDef *hi2c)
    : hi2c(hi2c)
    , state(State::IDLE)
{
}

void MS4525DO::init()
{
    state = State::IDLE;
}

void MS4525DO::measure()
{
    uint8_t data[4] = {0};
    if (HAL_I2C_Master_Receive(hi2c, kDevAddress, data, 4, 100) != HAL_OK) {
        asm("bkpt #0");
        return; // ERROR
    }

    raw_pressure = data[0] << 8 | data[1];
    raw_temp = data[2] << 8 | data[3];
}

float MS4525DO::get_pressure()
{
    return raw_pressure;
}

float MS4525DO::get_temp()
{
    return raw_temp;
}

bool MS4525DO::request()
{
    uint8_t data[1];
    return HAL_I2C_Master_Receive(hi2c, kDevAddress, data, 0, 100) == HAL_OK;
}

void MS4525DO::update()
{
    switch (state) {
    case State::IDLE:
        request();
        state = State::MEASURING;
        break;
    case State::MEASURING:
        measure();
        state = State::IDLE;
        break;
    default:
        break;
    }
}

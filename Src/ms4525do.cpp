#include "drivers/ms4525do.h"

#include "i2c.h"
MS4525DO::MS4525DO(I2C_HandleTypeDef *hi2c) : hi2c(hi2c), state(State::IDLE) {}

void MS4525DO::init() { state = State::IDLE; }

bool MS4525DO::measure() {
    uint8_t data[4] = {0};
    if (HAL_I2C_Master_Receive(hi2c, kDevAddress << 1, data, 4, 100) != HAL_OK) {
        return 0;  // ERROR
    }

    raw_pressure = ((data[0] & 0x3f) << 8) | data[1];
    raw_temp = (((unsigned int)data[2]) << 3) | (data[3] >> 5);
    return 1;
}

float MS4525DO::get_pressure() {
    float pressure_psi;

    pressure_psi = (float)((raw_pressure - 819.15) / (14744.7));
    pressure_psi = (pressure_psi - 0.49060678);

    return pressure_psi;
}

float MS4525DO::get_temp() {
    float temperature_c;

    temperature_c = (float)((raw_temp * 0.09770395701));
    temperature_c = temperature_c - 50;

    return temperature_c;
}

bool MS4525DO::request() {
    uint8_t data[1];
    if (HAL_I2C_Master_Receive(hi2c, kDevAddress << 1, data, 0, 100) != HAL_OK) {
        HAL_I2C_ClearBusyFlagErrata_2_14_7(hi2c);
        HAL_I2C_DeInit(hi2c);
        HAL_I2C_Init(hi2c);
        return 0;  // ERROR
    }
    return 1;
}

void MS4525DO::update() {
    switch (state) {
        case State::IDLE:
            if (request()) {
                state = State::MEASURING;
            }
            break;
        case State::MEASURING:
            measure();
            state = State::IDLE;
            break;
        default:
            break;
    }
}

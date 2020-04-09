#include "abvm.h"
#include "drv8873.h"
#include "ads1231.h"
#include "encoder.h"
#include "usb_comm.h"
#include "main.h"
#include "spi.h"
#include "tim.h"

ADS1231 pressure_sensor(
    SPI1_CS1_GPIO_Port,
    SPI1_CS1_Pin,
    &hspi1,
    GPIOA,
    GPIO_PIN_6,
    GPIOA,
    GPIO_PIN_5
);

ADS1231 load_cell(
    SPI1_CS2_GPIO_Port,
    SPI1_CS2_Pin,
    &hspi1,
    GPIOA,
    GPIO_PIN_6,
    GPIOA,
    GPIO_PIN_5
);

DRV8873 motor_driver(
    MC_SLEEP_GPIO_Port,
    MC_SLEEP_Pin,
    MC_DISABLE_GPIO_Port,
    MC_DISABLE_Pin,
    MC_FAULT_GPIO_Port,
    MC_FAULT_Pin,
    &htim2,
    TIM_CHANNEL_1,
    TIM_CHANNEL_3,
    &hspi2,
    SPI2_CS_GPIO_Port,
    SPI2_CS_Pin
);

Encoder encoder(&htim4);

USBComm usb_comm;

extern "C"
void abvm_init() {
    encoder.reset();
    usb_comm.setAsCDCConsumer();
}

extern "C"
void abvm_update() {
    pressure_sensor.update();
    load_cell.update();
}

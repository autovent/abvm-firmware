#include "abvm.h"
#include "drv8873.h"
#include "ads1231.h"
#include "encoder.h"
#include "usb_comm.h"
#include "main.h"
#include "spi.h"
#include "tim.h"

ADS1231 pressure_sensor(
    ADC1_PWRDN_GPIO_Port,
    ADC1_PWRDN_Pin,
    &hspi1,
    ADC_SPI_MISO_GPIO_Port,
    ADC_SPI_MISO_Pin,
    ADC_SPI_SCK_GPIO_Port,
    ADC_SPI_SCK_Pin
);

ADS1231 load_cell(
    ADC2_PWRDN_GPIO_Port,
    ADC2_PWRDN_Pin,
    &hspi1,
    ADC_SPI_MISO_GPIO_Port,
    ADC_SPI_MISO_Pin,
    ADC_SPI_SCK_GPIO_Port,
    ADC_SPI_SCK_Pin
);

DRV8873 motor_driver(
    MC_SLEEP_GPIO_Port,
    MC_SLEEP_Pin,
    MC_DISABLE_GPIO_Port,
    MC_DISABLE_Pin,
    MC_FAULT_GPIO_Port,
    MC_FAULT_Pin,
    MC_DIRECTION_GPIO_Port,
    MC_DIRECTION_Pin,
    &htim2,
    TIM_CHANNEL_1,
    &hspi2,
    MC_SPI_CS_GPIO_Port,
    MC_SPI_CS_Pin
);

Encoder encoder(&htim4);

USBComm usb_comm;

extern "C"
void abvm_init() {
    encoder.reset();
    usb_comm.setAsCDCConsumer();
    usb_comm.sendf("AutoVENT ABVM (autovent.org)");
}

extern "C"
void abvm_update() {
    pressure_sensor.update();
    load_cell.update();
}

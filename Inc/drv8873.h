#ifndef DRV8873_H
#define DRV8873_H

#include "stm32f1xx_hal.h"

#define DRV8873_FAULT_STATUS 0x00  // (RO) |   RSVD    |   FAULT  |   OTW   |   UVLO   |   CPUV   |  OCP     |  TSD   |  OLD   |
#define DRV8873_DIAG_STATUS  0x01  // (RO) |   OL1     |   OL2    | ITRIP1  |   ITRIP  |   OCP_H1 |  OCP_L1  | OCP_H2 | OCP_L2 |
#define DRV8873_IC1_CONTROL  0x02  // (RW) |          TOFF        | SPI_IN  |           SR                   |       MODE      |
#define DRV8873_IC2_CONTROL  0x03  // (RW) | ITRIP_REP | TSD_MODE | OTW_REP | DIS_CPUV |      OCP_TRETRY     |     OCP_MODE    |
#define DRV8873_IC3_CONTROL  0x04  // (RW) |  CLR_FLT  |             LOCK              | OUT2_DIS | OUT2_DIS | EN_IN1 | PH_IN2 |
#define DRV8873_IC4_CONTROL  0x05  // (RW) |   RSVD    |  EN_OLP  | OLP_DLY |  EN_OLA  |      ITRIP_LVL      |    DIS_ITRIP    |

#define DRV8873_FAULT_MASK 0x40
#define DRV8873_FAULT_POS  6

#define DRV8873_OTW_MASK 0x20
#define DRV8873_OTW_POS  5

#define DRV8873_UVLO_MASK 0x10
#define DRV8873_UVLO_POS  4

#define DRV8873_CPUV_MASK 0x08
#define DRV8873_CPUV_POS  3

#define DRV8873_OCP_MASK 0x04
#define DRV8873_OCP_POS  2

#define DRV8873_TSD_MASK 0x02
#define DRV8873_TSD_POS  1

#define DRV8873_OLD_MASK 0x01
#define DRV8873_OLD_POS  0

#define DRV8873_OL1_MASK 0x80
#define DRV8873_OL1_POS  7

#define DRV8873_OL2_MASK 0x40
#define DRV8873_OL2_POS  6

#define DRV8873_ITRIP1_MASK 0x20
#define DRV8873_ITRIP1_POS  5

#define DRV8873_ITRIP2_MASK 0x10
#define DRV8873_ITRIP2_POS  4

#define DRV8873_OCP_H1_MASK 0x08
#define DRV8873_OCP_H1_POS  3

#define DRV8873_OCP_L1_MASK 0x04
#define DRV8873_OCP_L1_POS  2

#define DRV8873_OCP_H2_MASK 0x02
#define DRV8873_OCP_H2_POS  1

#define DRV8873_OCP_L2_MASK 0x01
#define DRV8873_OCP_L2_POS  0

#define DRV8873_TOFF_MASK 0xC0
#define DRV8873_TOFF_POS  6

#define DRV8873_SPI_IN_MASK 0x20
#define DRV8873_SPI_IN_POS  5

#define DRV8873_SR_MASK 0x1C
#define DRV8873_SR_POS  2

#define DRV8873_MODE_MASK 0x03
#define DRV8873_MODE_POS  0

#define DRV8873_ITRIP_REP_MASK 0x80
#define DRV8873_ITRIP_REP_POS  7

#define DRV8873_TSD_MODE_MASK 0x40
#define DRV8873_TSD_MODE_POS  6

#define DRV8873_OTW_REP_MASK 0x20
#define DRV8873_OTW_REP_POS  5

#define DRV8873_DIS_CPUV_MASK 0x10
#define DRV8873_DIS_CPUV_POS  4

#define DRV8873_OCP_TRETRY_MASK 0x0C
#define DRV8873_OCP_TRETRY_POS  2

#define DRV8873_TSD_OCP_MODE_MASK 0x40
#define DRV8873_TSD_OCP_MODE_POS  6

#define DRV8873_CLR_FLT_MASK 0x80
#define DRV8873_CLR_FLT_POS  7

#define DRV8873_LOCK_MASK 0x70
#define DRV8873_LOCK_POS  4

#define DRV8873_OUT1_DIS_MASK 0x08
#define DRV8873_OUT1_DIS_POS  3

#define DRV8873_EN_IN1_MASK 0x02
#define DRV8873_EN_IN1_POS  1

#define DRV8873_PH_IN2_MASK 0x01
#define DRV8873_PH_IN2_POS  0

#define DRV8873_EN_OLP_MASK 0x40
#define DRV8873_EN_OLP_POS  6

#define DRV8873_OLP_DLY_MASK 0x20
#define DRV8873_OLP_DLY_POS  5

#define DRV8873_EN_OLA_MASK 0x10
#define DRV8873_EN_OLA_POS  4

#define DRV8873_ITRIP_LVL_MASK 0x0C
#define DRV8873_ITRIP_LVL_POS  2

#define DRV8873_DIS_ITRIP_MASK 0x03
#define DRV8873_DIS_ITRIP_POS  0

#define DRV8873_REG_GET_VAL(reg, mask, pos) ((reg) & (mask)) >> (pos)
#define DRV8873_REG_SET_VAL(reg, mask, pos, val) (reg) |= (((val) << (pos)) & (mask))

class DRV8873 {
public:
    DRV8873(
        GPIO_TypeDef *sleep_port,
        uint16_t sleep_pin,  
        GPIO_TypeDef *disable_port,
        uint16_t disable_pin,
        GPIO_TypeDef *fault_port,
        uint16_t fault_pin,
        TIM_HandleTypeDef *htim,
        uint32_t tim_channel_pwm1,
        uint32_t tim_channel_pwm2,
        SPI_HandleTypeDef *hspi,
        GPIO_TypeDef *cs_port,
        uint16_t cs_pin
    );
    
    void init();

    void set_current_raw_meas_dma(uint32_t *dma);
    float get_current();

    void set_pwm_enabled(bool enabled);
    void set_pwm(float value);

    enum motor_direction {
        DIRECTION_FORWARD,
        DIRECTION_REVERSE,
    };

    void set_direction(motor_direction dir);

    void set_disabled(bool disable);
    void set_sleep(bool sleep);

    bool get_fault();

    uint8_t get_reg(uint8_t reg_addr);
    void set_reg(uint8_t reg_addr, uint8_t value);
    uint8_t get_status_reg();

private:
    static constexpr float VREF = 3.3f;
    static constexpr float I_MIRROR_RATIO = 1100.0f;
    static constexpr float R_LOAD = 330.0f;

    GPIO_TypeDef *sleep_port;
    uint16_t sleep_pin;
    
    GPIO_TypeDef *disable_port;
    uint16_t disable_pin;

    GPIO_TypeDef *fault_port;
    uint16_t fault_pin;

    TIM_HandleTypeDef *htim;
    uint32_t tim_channel_pwm1;
    uint32_t tim_channel_pwm2;

    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;

    uint32_t *current_raw_dma;

    uint8_t status_reg;

    motor_direction direction;

    uint8_t run_spi_transaction(bool read, uint8_t reg_addr, uint8_t data);
};

#endif  // DRV8873_H

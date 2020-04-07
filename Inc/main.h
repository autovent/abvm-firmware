/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RATE_CHAR_1_Pin GPIO_PIN_13
#define RATE_CHAR_1_GPIO_Port GPIOC
#define RATE_CHAR_2_Pin GPIO_PIN_14
#define RATE_CHAR_2_GPIO_Port GPIOC
#define RATE_CHAR_3_Pin GPIO_PIN_15
#define RATE_CHAR_3_GPIO_Port GPIOC
#define LED_POWER_Pin GPIO_PIN_0
#define LED_POWER_GPIO_Port GPIOC
#define LED_FAULT_Pin GPIO_PIN_1
#define LED_FAULT_GPIO_Port GPIOC
#define LED_IN_Pin GPIO_PIN_2
#define LED_IN_GPIO_Port GPIOC
#define LED_OUT_Pin GPIO_PIN_3
#define LED_OUT_GPIO_Port GPIOC
#define MOTOR_Isense1_Pin GPIO_PIN_0
#define MOTOR_Isense1_GPIO_Port GPIOA
#define MEASURE_12V_Pin GPIO_PIN_1
#define MEASURE_12V_GPIO_Port GPIOA
#define MC_PWM_2_Pin GPIO_PIN_2
#define MC_PWM_2_GPIO_Port GPIOA
#define SW_VOL_UP_Pin GPIO_PIN_3
#define SW_VOL_UP_GPIO_Port GPIOA
#define SW_VOL_DN_Pin GPIO_PIN_4
#define SW_VOL_DN_GPIO_Port GPIOA
#define BUZZER_N_Pin GPIO_PIN_7
#define BUZZER_N_GPIO_Port GPIOA
#define SPI1_CS1_Pin GPIO_PIN_4
#define SPI1_CS1_GPIO_Port GPIOC
#define SPI1_CS2_Pin GPIO_PIN_5
#define SPI1_CS2_GPIO_Port GPIOC
#define SW_START_Pin GPIO_PIN_0
#define SW_START_GPIO_Port GPIOB
#define SW_STOP_Pin GPIO_PIN_1
#define SW_STOP_GPIO_Port GPIOB
#define LIMIT2_Pin GPIO_PIN_2
#define LIMIT2_GPIO_Port GPIOB
#define SPI2_CS_Pin GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define MC_SLEEP_Pin GPIO_PIN_7
#define MC_SLEEP_GPIO_Port GPIOC
#define MC_DISABLE_Pin GPIO_PIN_8
#define MC_DISABLE_GPIO_Port GPIOC
#define MC_FAULT_Pin GPIO_PIN_9
#define MC_FAULT_GPIO_Port GPIOC
#define BUZZER_P_Pin GPIO_PIN_8
#define BUZZER_P_GPIO_Port GPIOA
#define LIMIT1_Pin GPIO_PIN_10
#define LIMIT1_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define MC_PWM_1_Pin GPIO_PIN_15
#define MC_PWM_1_GPIO_Port GPIOA
#define VOL_CHAR_1_Pin GPIO_PIN_10
#define VOL_CHAR_1_GPIO_Port GPIOC
#define VOL_CHAR_2_Pin GPIO_PIN_11
#define VOL_CHAR_2_GPIO_Port GPIOC
#define VOL_CHAR_3_Pin GPIO_PIN_12
#define VOL_CHAR_3_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define SW_RATE_DN_Pin GPIO_PIN_4
#define SW_RATE_DN_GPIO_Port GPIOB
#define SW_RATE_UP_Pin GPIO_PIN_5
#define SW_RATE_UP_GPIO_Port GPIOB
#define ENCODER_A_Pin GPIO_PIN_6
#define ENCODER_A_GPIO_Port GPIOB
#define ENCODER_B_Pin GPIO_PIN_7
#define ENCODER_B_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

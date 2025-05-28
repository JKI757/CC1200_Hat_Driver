/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32f4xx_hal.h"

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
#define BP_LED_BLUE_Pin GPIO_PIN_13
#define BP_LED_BLUE_GPIO_Port GPIOC
#define BP_KEY_BTN_Pin GPIO_PIN_0
#define BP_KEY_BTN_GPIO_Port GPIOA
#define _CC_RST_Pin GPIO_PIN_0
#define _CC_RST_GPIO_Port GPIOB
#define CC_GPIO0_Pin GPIO_PIN_12
#define CC_GPIO0_GPIO_Port GPIOB
#define CC_GPIO2_Pin GPIO_PIN_13
#define CC_GPIO2_GPIO_Port GPIOB
#define CC_GPIO3_Pin GPIO_PIN_14
#define CC_GPIO3_GPIO_Port GPIOB
#define MCU_TX_Pin GPIO_PIN_9
#define MCU_TX_GPIO_Port GPIOA
#define MCU_RX_Pin GPIO_PIN_10
#define MCU_RX_GPIO_Port GPIOA
#define SVC_LED_Pin GPIO_PIN_3
#define SVC_LED_GPIO_Port GPIOB
#define RX_LED_Pin GPIO_PIN_4
#define RX_LED_GPIO_Port GPIOB
#define TX_LED_Pin GPIO_PIN_5
#define TX_LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : globals.h
  * @brief          : Header for globals.cpp file.
  *                   This file contains the common defines and global objects
  *                   for the application.
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
#ifndef __GLOBALS_H
#define __GLOBALS_H


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "iwdg.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#include "Radio.h"

/**
 * @brief  Globals class to hold peripheral handles and provide access methods
 */
class Globals {
public:
    /**
     * @brief  Constructor for Globals class
     * @param  uart: Pointer to UART handle
     * @param  iwdg: Pointer to IWDG handle
     * @param  spi: Pointer to SPI handle
     */
    Globals(UART_HandleTypeDef* uart, IWDG_HandleTypeDef* iwdg, SPI_HandleTypeDef* spi,
    		GPIO_TypeDef* LedGPIO, uint16_t LedPin, GPIO_TypeDef* KeyButtonGPIO, uint16_t KeyButtonPin);

    /**
     * @brief  Destructor for Globals class
     */
    ~Globals();

    /**
     * @brief  Initialize the Radio
     * @retval true if initialization was successful
     */
    bool initRadio();

    /**
     * @brief  Get the UART handle
     * @retval UART handle
     */
    UART_HandleTypeDef* getUART() { return uart; }

    /**
     * @brief  Get the IWDG handle
     * @retval IWDG handle
     */
    IWDG_HandleTypeDef* getIWDG() { return iwdg; }

    /**
     * @brief  Get the SPI handle
     * @retval SPI handle
     */
    SPI_HandleTypeDef* getSPI() { return spi; }

    /**
     * @brief  Get the Radio instance
     * @retval Radio instance
     */
    Radio* getRadio() { return radio; }

    /**
     * @brief  Refresh the watchdog
     */
    void refreshWatchdog();

    /**
     * @brief  Set the service LED state
     * @param  state: LED state (0 = off, 1 = on)
     */
    void setServiceLED(uint8_t state);

    /**
     * @brief  Set the RX LED state
     * @param  state: LED state (0 = off, 1 = on)
     */
    void setRxLED(uint8_t state);

    /**
     * @brief  Set the TX LED state
     * @param  state: LED state (0 = off, 1 = on)
     */
    void setTxLED(uint8_t state);

    /**
     * @brief  Reset the CC1200 radio
     */
    void resetCC1200();

private:
    // Peripheral handle pointers
    UART_HandleTypeDef* uart;
    IWDG_HandleTypeDef* iwdg;
    SPI_HandleTypeDef* spi;
    GPIO_TypeDef* LedGPIO;
    uint16_t LedPin;
    GPIO_TypeDef* KeyButtonGPIO;
    uint16_t KeyButtonPin;
    
    // Radio instance
    Radio* radio;
};

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
/* USER CODE BEGIN EFP */

/* USER CODE END EFP */


#endif /* __GLOBALS_H */

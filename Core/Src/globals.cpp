/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : globals.cpp
  * @brief          : Implementation for globals class
  *                   This file contains the implementation of the Globals class
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

/* Includes ------------------------------------------------------------------*/
#include "globals.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  Constructor for Globals class
  * @param  uart: Pointer to UART handle
  * @param  iwdg: Pointer to IWDG handle
  * @param  spi: Pointer to SPI handle
  * @retval None
  */
Globals::Globals(UART_HandleTypeDef* uart, IWDG_HandleTypeDef* iwdg, SPI_HandleTypeDef* spi)
    : m_uart(uart), m_iwdg(iwdg), m_spi(spi), m_radio(nullptr) {
    // Store the peripheral handles provided
}

/**
  * @brief  Destructor for Globals class
  * @retval None
  */
Globals::~Globals() {
    // Clean up resources
    if (m_radio != nullptr) {
        delete m_radio;
        m_radio = nullptr;
    }
}

/**
  * @brief  Initialize the Radio
  * @retval true if initialization was successful
  */
bool Globals::initRadio() {
    // Create the Radio instance if it doesn't exist
    if (m_radio == nullptr) {
        // Initialize Radio with the appropriate GPIO pins and SPI
        m_radio = new Radio(m_spi, 
                           GPIOA, GPIO_PIN_4, // CS pin (SPI1_NSS)
                           _CC_RST_GPIO_Port, _CC_RST_Pin); // Reset pin
    }
    
    // Initialize the radio
    return m_radio->init();
}

/**
  * @brief  Refresh the watchdog
  * @retval None
  */
void Globals::refreshWatchdog() {
    HAL_IWDG_Refresh(m_iwdg);
}

/**
  * @brief  Set the service LED state
  * @param  state: LED state (0 = off, 1 = on)
  * @retval None
  */
void Globals::setServiceLED(uint8_t state) {
    HAL_GPIO_WritePin(SVC_LED_GPIO_Port, SVC_LED_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  Set the RX LED state
  * @param  state: LED state (0 = off, 1 = on)
  * @retval None
  */
void Globals::setRxLED(uint8_t state) {
    HAL_GPIO_WritePin(RX_LED_GPIO_Port, RX_LED_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  Set the TX LED state
  * @param  state: LED state (0 = off, 1 = on)
  * @retval None
  */
void Globals::setTxLED(uint8_t state) {
    HAL_GPIO_WritePin(TX_LED_GPIO_Port, TX_LED_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief  Reset the CC1200 radio
  * @retval None
  */
void Globals::resetCC1200() {
    HAL_GPIO_WritePin(_CC_RST_GPIO_Port, _CC_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(1); // Short delay for reset pulse
    HAL_GPIO_WritePin(_CC_RST_GPIO_Port, _CC_RST_Pin, GPIO_PIN_SET);
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

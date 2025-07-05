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
#include <bitset>
#include <sstream>
#include <functional>
#include "CC1200_HAL.h"
#include "usbd_cdc_if.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <iomanip>
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
Globals::Globals(UART_HandleTypeDef* uart, IWDG_HandleTypeDef* iwdg, SPI_HandleTypeDef* spi,
		GPIO_TypeDef* LedGPIO, uint16_t LedPin, GPIO_TypeDef* KeyButtonGPIO, uint16_t KeyButtonPin)
    : uart(uart), iwdg(iwdg), spi(spi), LedGPIO(LedGPIO), LedPin(LedPin),
	  debugUart(uart) {
    // Store the peripheral handles provided
    
    // Create CC1200 instance with hardware configuration
    // Using the defined pins from the STM32 configuration
    cc1200 = new CC1200(spi, 
                        // Note: Parameter order is CS then RST
                        GPIOA, GPIO_PIN_4,  // CS pin - needs to be verified/configured
                        _CC_RST_GPIO_Port, _CC_RST_Pin,  // Reset pin
                        [this](const std::string& msg) { this->sendDebugUSB(msg); },
                        false);  // Not CC1201
}

/**
  * @brief  Destructor for Globals class
  * @retval None
  */
Globals::~Globals() {
    // Clean up resources
    if (cc1200 != nullptr) {
        delete cc1200;
        cc1200 = nullptr;
    }
}

/**
  * @brief  Refresh the watchdog
  * @retval None
  */
void Globals::refreshWatchdog() {
    HAL_IWDG_Refresh(this->iwdg);
}

/**
  * @brief  Set the service LED state
  * @param  state: LED state (0 = off, 1 = on)
  * @retval None
  */
void Globals::setServiceLED(uint8_t state) {
    HAL_GPIO_WritePin(SVC_LED_GPIO_Port, SVC_LED_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(this->LedGPIO, this->LedPin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
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
 * @brief Initialize the CC1200 radio
 * @retval true if successful, false otherwise
 */
bool Globals::initCC1200() {
    if (cc1200 == nullptr) {
        return false;
    }
    
    // Simple initialization - just call begin()
    return cc1200->begin();
}

/**
 * @brief Reset the CC1200 radio
 */
void Globals::resetCC1200() {
    if (cc1200 != nullptr) {
        cc1200->reset();
    }
}

/* USER CODE BEGIN 1 */

void Globals::sendUART(UART_HandleTypeDef* uart, uint8_t* buf, uint16_t len){
  HAL_UART_Transmit(uart, (uint8_t*)buf, len, 100);
}

void Globals::sendUART(UART_HandleTypeDef* uart, std::string s){
  HAL_UART_Transmit(uart, (uint8_t*)s.c_str(), s.length(), 100);
}

void Globals::sendDebugUART(std::string s){
	if (debugUart != nullptr){
		sendUART(this->debugUart, s);
	}
}

void Globals::sendUSB(uint8_t* buf, uint16_t len){
	// Send data over USB VCP using CDC interface
	CDC_Transmit_FS(buf, len);
}

void Globals::sendDebugUSB(std::string s){
	// Send string directly to USB VCP
	if (!s.empty()) {
		sendUSB((uint8_t*)s.c_str(), s.length());
	}
}

void Globals::addDebugMessage(std::string s){
	if (this->debugDeque.size() < 200){
		//arbitrary, but if we've got more than 50 messages in the deque
		//something is wrong and we shouldn't keep filling up memory
		 this->debugDeque.push_back(s);

	}else{
		this->debugDeque.clear();
		//just clear the deque if the debug send task has stopped servicing it
	}
}

void Globals::addDebugMessage(uint8_t *buf, int len){
	this->addDebugMessage(buf_to_string(buf, len) + "\n");
}

std::string Globals::getHextoStringDebugMessage(uint64_t s){
	std::stringstream ss;
	ss << "0x" << std::hex << std::setfill('0') << std::setw(8) << s ;
	return ss.str();
}

std::string Globals::getBintoStringDebugMessage(uint64_t s){
	std::stringstream ss;
	ss << "0b" << std::setfill('0') << std::setw(64) << std::bitset<64>(s) ;
	return ss.str();
}

void Globals::displayNextDebugMessage(){
	if (this->getNumberOfDebugMessages() > 0){
		std::string s = this->getNextDebugMessage();
			this->sendDebugUSB(s);
	}
}

std::string Globals::getNextDebugMessage(){
	std::string s = this->debugDeque.front();
	this->debugDeque.pop_front();
	return (s);
}
int Globals::getNumberOfDebugMessages(){
	return (this->debugDeque.size());
}

std::string Globals::buf_to_string(uint8_t* buf, int len){
	std::stringstream ss;
	ss << std::hex ;
	for (int i=0;i<len;++i){
		ss << std::setw(2) << std::setfill('0') << (int)(buf[i]);
	}
	return ss.str();
}


/* USER CODE END 1 */

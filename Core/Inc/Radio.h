#ifndef __RADIO_H
#define __RADIO_H

#include "main.h"
#include "CC1200_HAL.h"
#include <cstdint>
#include <cstddef>

/**
 * @brief Radio class to interface with the CC1200 RF front end
 * 
 * This class provides an interface to the CC1200 radio chip, handling
 * configuration, transmission, and reception of data.
 */
class Radio {
public:
    /**
     * @brief Constructor for Radio class
     * @param spi SPI handle for communication with CC1200
     * @param cs_port GPIO port for chip select
     * @param cs_pin GPIO pin for chip select
     * @param rst_port GPIO port for reset
     * @param rst_pin GPIO pin for reset
     */
    Radio(SPI_HandleTypeDef* spi, 
          GPIO_TypeDef* cs_port, uint16_t cs_pin,
          GPIO_TypeDef* rst_port, uint16_t rst_pin);
    
    /**
     * @brief Initialize the radio
     * @return true if initialization was successful
     */
    bool init();
    
    /**
     * @brief Configure the radio for 4FSK modulation
     * @return true if configuration was successful
     */
    bool configure4FSK();
    
    /**
     * @brief Configure infinite read mode
     * @return true if configuration was successful
     */
    bool configureInfiniteRead();
    
    /**
     * @brief Configure infinite write mode
     * @return true if configuration was successful
     */
    bool configureInfiniteWrite();
    
    /**
     * @brief Configure DMA for data transfer
     * @return true if configuration was successful
     */
    bool configureDMA();
    
    /**
     * @brief Configure GPIO pins for interrupts
     * @return true if configuration was successful
     */
    bool configureGPIOInterrupts();
    
    /**
     * @brief Transmit data
     * @param data Pointer to data buffer
     * @param len Length of data
     * @return true if transmission was successful
     */
    bool transmit(const char* data, size_t len);
    
    /**
     * @brief Receive data
     * @param buffer Buffer to store received data
     * @param bufferLen Size of buffer
     * @return Number of bytes received
     */
    size_t receive(char* buffer, size_t bufferLen);
    
    /**
     * @brief Start continuous transmission mode
     * @return true if successful
     */
    bool startContinuousTransmit();
    
    /**
     * @brief Stop continuous transmission mode
     * @return true if successful
     */
    bool stopContinuousTransmit();
    
    /**
     * @brief Start continuous reception mode
     * @return true if successful
     */
    bool startContinuousReceive();
    
    /**
     * @brief Stop continuous reception mode
     * @return true if successful
     */
    bool stopContinuousReceive();
    
    /**
     * @brief Set the radio frequency
     * @param frequencyHz Frequency in Hz
     * @return true if successful
     */
    bool setFrequency(float frequencyHz);
    
    /**
     * @brief Set the symbol rate
     * @param symbolRateHz Symbol rate in Hz
     * @return true if successful
     */
    bool setSymbolRate(float symbolRateHz);
    
    /**
     * @brief Get the CC1200 driver instance
     * @return Pointer to CC1200 driver
     */
    CC1200* getCC1200() { return &cc1200; }
    
    /**
     * @brief Handle GPIO interrupt
     * @param gpio_pin GPIO pin that triggered the interrupt
     */
    void handleGPIOInterrupt(uint16_t gpio_pin);

private:
    // CC1200 driver instance
    CC1200 cc1200;
    
    // State variables
    bool initialized = false;
    bool transmitting = false;
    bool receiving = false;
    
    // Default configuration values
    static constexpr float DEFAULT_FREQUENCY = 915000000.0f; // 915 MHz
    static constexpr float DEFAULT_SYMBOL_RATE = 50000.0f;   // 50 kBaud
    static constexpr float DEFAULT_DEVIATION = 25000.0f;     // 25 kHz
};

#endif // __RADIO_H

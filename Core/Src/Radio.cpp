#include "Radio.h"
#include "CC1200Bits.h"
#include <cstdio>

/**
 * @brief Constructor for Radio class
 */
Radio::Radio(SPI_HandleTypeDef* spi, 
             GPIO_TypeDef* cs_port, uint16_t cs_pin,
             GPIO_TypeDef* rst_port, uint16_t rst_pin)
    : cc1200(spi, cs_port, cs_pin, rst_port, rst_pin, nullptr, false)
{
    // Constructor initializes the CC1200 driver with the provided parameters
}

/**
 * @brief Initialize the radio
 */
bool Radio::init()
{
    // Initialize the CC1200 driver
    if (!cc1200.begin()) {
        return false;
    }
    
    // Configure for 4FSK modulation
    if (!configure4FSK()) {
        return false;
    }
    
    // Configure infinite read/write modes
    if (!configureInfiniteRead() || !configureInfiniteWrite()) {
        return false;
    }
    
    // Configure DMA
    if (!configureDMA()) {
        return false;
    }
    
    // Configure GPIO interrupts
    if (!configureGPIOInterrupts()) {
        return false;
    }
    
    // Set default frequency and symbol rate
    setFrequency(DEFAULT_FREQUENCY);
    setSymbolRate(DEFAULT_SYMBOL_RATE);
    
    initialized = true;
    return true;
}

/**
 * @brief Configure the radio for 4FSK modulation
 */
bool Radio::configure4FSK()
{
    // Set modulation format to 4FSK
    cc1200.setModulationFormat(CC1200::ModFormat::FSK4);
    
    // Set FSK deviation
    cc1200.setFSKDeviation(DEFAULT_DEVIATION);
    
    // Configure sync word (example value, can be changed as needed)
    cc1200.configureSyncWord(0x930B51DE, CC1200::SyncMode::REQUIRE_30_OF_32_SYNC_BITS);
    
    // Configure preamble (example values, can be adjusted)
    cc1200.configurePreamble(0x08, 0x22);
    
    // Set packet mode to fixed length for simplicity
    cc1200.setPacketMode(CC1200::PacketMode::FIXED_LENGTH, true);
    
    // Enable CRC
    cc1200.setCRCEnabled(true);
    
    // Configure RX filter bandwidth (adjust based on symbol rate and deviation)
    cc1200.setRXFilterBandwidth(DEFAULT_SYMBOL_RATE * 1.2f);
    
    // Set output power (adjust as needed)
    cc1200.setOutputPower(10.0f); // 10 dBm
    
    return true;
}

/**
 * @brief Configure infinite read mode
 */
bool Radio::configureInfiniteRead()
{
    // Configure FIFO mode for continuous reception
    cc1200.configureFIFOMode();
    
    // Configure GPIO0 for RXFIFO_THR (RX FIFO threshold)
    cc1200.configureGPIO(0, CC1200::GPIOMode::RXFIFO_THR);
    
    // Set on-receive state to continue receiving
    cc1200.setOnReceiveState(CC1200::State::RX, CC1200::State::RX);
    
    return true;
}

/**
 * @brief Configure infinite write mode
 */
bool Radio::configureInfiniteWrite()
{
    // Configure FIFO mode for continuous transmission
    cc1200.configureFIFOMode();
    
    // Configure GPIO2 for TXFIFO_THR (TX FIFO threshold)
    cc1200.configureGPIO(2, CC1200::GPIOMode::TXFIFO_THR);
    
    // Set on-transmit state to continue transmitting
    cc1200.setOnTransmitState(CC1200::State::TX);
    
    return true;
}

/**
 * @brief Configure DMA for data transfer
 */
bool Radio::configureDMA()
{
    // The SPI DMA is already configured in the SPI initialization
    // We just need to ensure our CC1200 driver uses it correctly
    
    // Note: The actual DMA handling is done within the CC1200 driver
    // when using the readStream/writeStream methods
    
    return true;
}

/**
 * @brief Configure GPIO pins for interrupts
 */
bool Radio::configureGPIOInterrupts()
{
    // Configure GPIO3 for PKT_SYNC_RXTX (packet sync received/transmitted)
    cc1200.configureGPIO(3, CC1200::GPIOMode::PKT_SYNC_RXTX);
    
    // Note: The actual interrupt handling will be set up in the GPIO initialization
    // and handled by the STM32 HAL GPIO interrupt callbacks
    
    return true;
}

/**
 * @brief Transmit data
 */
bool Radio::transmit(const char* data, size_t len)
{
    // Flush TX FIFO before transmitting
    cc1200.sendCommand(CC1200::Command::FLUSH_TX);
    
    // Set packet length
    cc1200.setPacketLength(len);
    
    // Enqueue the packet
    if (!cc1200.enqueuePacket(data, len)) {
        return false;
    }
    
    // Send TX command to start transmission
    cc1200.sendCommand(CC1200::Command::TX);
    
    return true;
}

/**
 * @brief Receive data
 */
size_t Radio::receive(char* buffer, size_t bufferLen)
{
    // Check if a packet has been received
    if (!cc1200.hasReceivedPacket()) {
        return 0;
    }
    
    // Receive the packet
    return cc1200.receivePacket(buffer, bufferLen);
}

/**
 * @brief Start continuous transmission mode
 */
bool Radio::startContinuousTransmit()
{
    if (transmitting) {
        return true; // Already transmitting
    }
    
    // Flush TX FIFO
    cc1200.sendCommand(CC1200::Command::FLUSH_TX);
    
    // Send TX command to start transmission
    cc1200.sendCommand(CC1200::Command::TX);
    
    transmitting = true;
    return true;
}

/**
 * @brief Stop continuous transmission mode
 */
bool Radio::stopContinuousTransmit()
{
    if (!transmitting) {
        return true; // Not transmitting
    }
    
    // Send IDLE command to stop transmission
    cc1200.sendCommand(CC1200::Command::IDLE);
    
    transmitting = false;
    return true;
}

/**
 * @brief Start continuous reception mode
 */
bool Radio::startContinuousReceive()
{
    if (receiving) {
        return true; // Already receiving
    }
    
    // Flush RX FIFO
    cc1200.sendCommand(CC1200::Command::FLUSH_RX);
    
    // Send RX command to start reception
    cc1200.sendCommand(CC1200::Command::RX);
    
    receiving = true;
    return true;
}

/**
 * @brief Stop continuous reception mode
 */
bool Radio::stopContinuousReceive()
{
    if (!receiving) {
        return true; // Not receiving
    }
    
    // Send IDLE command to stop reception
    cc1200.sendCommand(CC1200::Command::IDLE);
    
    receiving = false;
    return true;
}

/**
 * @brief Set the radio frequency
 */
bool Radio::setFrequency(float frequencyHz)
{
    // Set radio frequency using the appropriate band
    // Determine the band based on the frequency
    CC1200::Band band;
    
    if (frequencyHz >= 820e6 && frequencyHz <= 960e6) {
        band = CC1200::Band::BAND_820_960MHz;
    } else if (frequencyHz >= 410e6 && frequencyHz <= 480e6) {
        band = CC1200::Band::BAND_410_480MHz;
    } else if (frequencyHz >= 136e6 && frequencyHz <= 160e6) {
        band = CC1200::Band::BAND_136_160MHz;
    } else {
        // Frequency out of supported range
        return false;
    }
    
    cc1200.setRadioFrequency(band, frequencyHz);
    return true;
}

/**
 * @brief Set the symbol rate
 */
bool Radio::setSymbolRate(float symbolRateHz)
{
    cc1200.setSymbolRate(symbolRateHz);
    
    // Update RX filter bandwidth based on symbol rate
    cc1200.setRXFilterBandwidth(symbolRateHz * 1.2f);
    
    return true;
}

/**
 * @brief Handle GPIO interrupt
 */
void Radio::handleGPIOInterrupt(uint16_t gpio_pin)
{
    // Handle different GPIO interrupts
    if (gpio_pin == CC_GPIO0_Pin) {
        // RX FIFO threshold reached
        // This would typically trigger reading from the FIFO
    } else if (gpio_pin == CC_GPIO2_Pin) {
        // TX FIFO threshold reached
        // This would typically trigger writing to the FIFO
    } else if (gpio_pin == CC_GPIO3_Pin) {
        // Packet sync received/transmitted
        // This would typically indicate a complete packet
    }
}

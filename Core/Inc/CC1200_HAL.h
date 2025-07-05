/*
 * Copyright (c) 2019-2023 USC Rocket Propulsion Lab
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CC1200_CC1200_HAL_H
#define CC1200_CC1200_HAL_H

#include "stm32f4xx_hal.h"
#include <cstdint>
#include <chrono>
#include <functional>
#include <string>

/**
 *  Driver for the CC1200 radio communications IC using STM32 HAL.
 *  This class provides basic functions and register level IO with the chip.
 */
class CC1200
{
private:
    // STM32 HAL handles
    SPI_HandleTypeDef* hspi;
    GPIO_TypeDef* csPort;
    uint16_t csPin;
    GPIO_TypeDef* rstPort;
    uint16_t rstPin;

    // Output to print debug messages to
    FILE* debugStream;

public:
    // register definitions
    enum class Register : uint8_t
    {
        IOCFG3 = 0x00,
        IOCFG2 = 0x01,
        IOCFG1 = 0x02,
        IOCFG0 = 0x03,
        SYNC3 = 0x4,
        SYNC2 = 0x5,
        SYNC1 = 0x6,
        SYNC0 = 0x7,
        SYNC_CFG1 = 0x8,
        SYNC_CFG0 = 0x9,
        DEVIATION_M = 0xA,
        MODCFG_DEV_E = 0xB,
        DCFILT_CFG  = 0xC,
        PREAMBLE_CFG1 = 0xD,
        PREAMBLE_CFG0 = 0xE,
        IQIC = 0xF,
        CHAN_BW = 0x10,
        MDMCFG1 = 0x11,
        MDMCFG0 = 0x12,
        SYMBOL_RATE2 = 0x13,
        SYMBOL_RATE1 = 0x14,
        SYMBOL_RATE0 = 0x15,
        AGC_REF = 0x16,
        AGC_CS_THR = 0x17,
        AGC_GAIN_ADJUST = 0x18,
        AGC_CFG3 = 0x19,
        AGC_CFG2 = 0x1A,
        AGC_CFG1 = 0x1B,
        AGC_CFG0 = 0x1C,
        FIFO_CFG = 0x1D,
        DEV_ADDR = 0x1E,
        SETTLING_CFG = 0x1F,
        FS_CFG = 0x20,
        WOR_CFG1 = 0x21,
        WOR_CFG0 = 0x22,
        WOR_EVENT0_MSB = 0x23,
        WOR_EVENT0_LSB = 0x24,
        RXDCM_TIME = 0x25,
        PKT_CFG2 = 0x26,
        PKT_CFG1 = 0x27,
        PKT_CFG0 = 0x28,
        RFEND_CFG1 = 0x29,
        RFEND_CFG0 = 0x2A,
        PA_CFG1 = 0x2B,
        PA_CFG0 = 0x2C,
        ASK_CFG = 0x2D,
        PKT_LEN = 0x2E
    };

    // extended register definitions
    enum class ExtRegister : uint8_t
    {
        IF_MIX_CFG = 0x0,
        FREQOFF_CFG = 0x1,
        TOC_CFG = 0x2,
        MDMCFG2 = 0x5,
        FREQOFF1 = 0xA,
        FREQOFF2 = 0xB,
        FREQ2 = 0xC,
        FREQ1 = 0xD,
        FREQ0 = 0xE,
        IF_ADC2 = 0xF,
        IF_ADC1 = 0x10,
        IF_ADC0 = 0x11,
        FS_DIG1 = 0x12,
        FS_DIG0 = 0x13,
        FS_CAL1 = 0x16,
        FS_CAL0 = 0x17,
        FS_CHP = 0x18,
        FS_DIVTWO = 0x19,
        FS_DSM1 = 0x1A,
        FS_DSM0 = 0x1B,
        FS_DVC1 = 0x1C,
        FS_DVC0 = 0x1D,
        FS_LBI = 0x1E,
        FS_PFD = 0x1F,
        FS_PRE = 0x20,
        FS_REG_DIV_CML = 0x21,
        FS_SPARE = 0x22,
        FS_VCO4 = 0x23,
        FS_VCO3 = 0x24,
        FS_VCO2 = 0x25,
        FS_VCO1 = 0x26,
        FS_VCO0 = 0x27,
        IFAMP = 0x2F,
        XOSC5 = 0x32,
        XOSC4 = 0x33,
        XOSC3 = 0x34,
        XOSC2 = 0x35,
        XOSC1 = 0x36,
        XOSC0 = 0x37,
        RSSI1 = 0x71,
        RSSI0 = 0x72,
        MARCSTATE = 0x73,
        LQI_VAL = 0x74,
        FREQOFF_EST1 = 0x77,
        FREQOFF_EST2 = 0x78,
        FSCAL_CTRL = 0x8D,
        PARTNUMBER = 0x8F,
        PARTVERSION = 0x90,
        RXFIRST = 0xD2,
        TXFIRST = 0xD3,
        RXLAST = 0xD4,
        TXLAST = 0xD5,
        NUM_TXBYTES = 0xD6,
        NUM_RXBYTES = 0xD7,
        RXFIFO_PRE_BUF = 0xDA
    };

    // Command strobe definitions.  See user guide section 3.2.2
    enum class Command : uint8_t
    {
        SOFT_RESET = 0x30,
        FAST_TX_ON = 0x31,
        OSC_OFF = 0x32,
        CAL_FREQ_SYNTH = 0x33,
        RX = 0x34,
        TX = 0x35,
        IDLE = 0x36,
        AUTO_FREQ_COMP = 0x37,
        WAKE_ON_RADIO = 0x38,
        SLEEP = 0x39,
        FLUSH_RX = 0x3A,
        FLUSH_TX = 0x3B,
        WOR_RESET = 0x3C,
        NOP = 0x3D
    };

    // State of the radio chip.  See user guide Figure 2.
    enum class State : uint8_t
    {
        IDLE = 0x0,
        RX = 0x1,
        TX = 0x2,
        FAST_ON = 0x3,
        CALIBRATE = 0x4,
        SETTLING = 0x5,
        RX_FIFO_ERROR = 0x6,
        TX_FIFO_ERROR = 0x7
    };

    // Frequency bands available for the radio
    enum class Band
    {
        BAND_820_960MHz,
        BAND_410_480MHz,
        BAND_136_160MHz,
        BAND_410_480MHz_HIGH_IF
    };

    // Modulation formats available
    enum class ModFormat : uint8_t
    {
        FSK2 = 0,
        GFSK2 = 1,
        ASK_OOK = 3,
        FSK4 = 4,
        GFSK4 = 5
    };

    // GPIO pin modes
    enum class GPIOMode : uint8_t
    {
        RXFIFO_THR = 0,
        RXFIFO_THR_PKT = 1,
        TXFIFO_THR = 2,
        TXFIFO_THR_PKT = 3,
        RXFIFO_OVERFLOW = 4,
        TXFIFO_UNDERFLOW = 5,
        PKT_SYNC_RXTX = 6,
        CRC_OK = 7,
        SERIAL_CLK = 8,
        SERIAL_RX = 9,
        SERIAL_TX = 10,
        CARRIER_SENSE = 11,
        CCA = 12,
        RXFIFO_BYTES = 13,
        TXFIFO_BYTES = 14,
        MAGN_VALID = 15,
        RSSI_VALID = 16,
        CLK_XOSC_16 = 17,
        CLK_XOSC_192 = 18,
        CLK_XOSC_32 = 19,
        CLK_XOSC_DIV3 = 20,
        SYNC_LOW0_HIGH1 = 21,
        PKT_RXRXTX = 22,
        CARRIER_SENSE_NEG = 23,
        TXONCCA_FAILED = 24,
        TXONCCA_DONE = 25,
        ANTENNA_SELECT = 26,
        CHIP_RDY = 36,
        HW0 = 40,
        HW1 = 41,
        EXT_CLOCK = 42,
        RX0TX1_CFG = 43,
        RX0TX1_STATUS = 44,
        HIGHZ = 48,
        HW_TO_0 = 52,
        HW_TO_1 = 53,
    };

    // Packet modes
    enum class PacketMode
    {
        FIXED_LENGTH,
        VARIABLE_LENGTH
    };

    // Frequency synthesizer calibration modes
    enum class FSCalMode
    {
        CALIBRATE_MANUAL_IDLE,
        CALIBRATE_MANUAL_RX,
        CALIBRATE_MANUAL_TX,
        CALIBRATE_AUTO_IDLE_TO_RX,
        CALIBRATE_AUTO_RX_TO_TX,
        CALIBRATE_AUTO_TX_TO_RX,
        CALIBRATE_AUTO_TX_TO_IDLE,
        CALIBRATE_AUTO_RX_TO_IDLE
    };

    // Sync word detection mode
    enum class SyncMode : uint8_t
    {
        NO_PREAMBLE_SYNC_THRESHOLD_0 = 0,
        REQUIRE_15_OF_16_SYNC_BITS = 1,
        REQUIRE_16_OF_16_SYNC_BITS = 2,
        REQUIRE_30_OF_32_SYNC_BITS = 3,
        NO_PREAMBLE_SYNC_THRESHOLD_15_OF_16 = 4,
        NO_PREAMBLE_SYNC_THRESHOLD_16_OF_16 = 5,
        NO_PREAMBLE_SYNC_THRESHOLD_30_OF_32 = 6,
        CARRIER_SENSE_ALWAYS_ON = 7
    };

    // AGC sync behavior
    enum class SyncBehavior : uint8_t
    {
        FREEZE_AGC_ON_SYNC = 0,
        UPDATE_AGC_ON_SYNC = 1,
        UPDATE_AGC_ON_FIRST_SYNC = 2,
        UPDATE_AGC_ON_NEVER_SYNC = 3
    };

    // AGC gain table options
    enum class GainTable : uint8_t
    {
        LOW_LINEARITY = 0,
        HIGH_LINEARITY = 1
    };

    // IF configuration options
    enum class IFCfg : uint8_t
    {
        DISABLE = 0,
        ENABLE_INTERNAL = 1,
        ENABLE_EXTERNAL = 2
    };

    // PA ramp time options
    enum class RampTime : uint8_t
    {
        RAMP_3US = 0,
        RAMP_5US = 1,
        RAMP_10US = 2,
        RAMP_20US = 3,
        RAMP_40US = 4,
        RAMP_80US = 5,
        RAMP_200US = 6,
        RAMP_400US = 7
    };

private:
    // chip data variables
    bool chipReady = false;
    State state = State::IDLE;
    // Debug function (must be declared before isCC1201 for initialization order)
    std::function<void(const std::string&)> sendStringToDebugUart;
    bool isCC1201;

    // current state variables
    float currentSymbolRate = 0;
    float currentFrequency = 0;
    float currentRXBandwidth = 0;
    PacketMode _packetMode = PacketMode::FIXED_LENGTH;
    bool _appendStatus = false;

    // Status byte from the last SPI transaction
    uint8_t lastStatus = 0;
    
    // Debug flag for SPI communication
    bool debugEnabled = false;

    // DMA transfer state
    volatile bool dmaTransferComplete = false;
    volatile bool dmaTransferError = false;
    
    // DMA buffers (must be aligned for DMA)
    uint8_t dmaTxBuffer[256] __attribute__((aligned(4)));
    uint8_t dmaRxBuffer[256] __attribute__((aligned(4)));
    
    // Continuous streaming state
    volatile bool continuousStreamingTx = false;
    volatile bool continuousStreamingRx = false;
    uint8_t streamingTxPattern[128];
    size_t streamingTxPatternLen = 0;
    uint32_t streamingTxCount = 0;
    uint32_t streamingRxCount = 0;
    uint32_t streamingTxErrors = 0;
    uint32_t streamingRxErrors = 0;

    // Helper functions
    void loadStatusByte(uint8_t status);
    void select();
    void deselect();
    uint8_t spiTransfer(uint8_t data);
    
    // DMA helper functions
    bool spiTransferDMA(uint8_t* txData, uint8_t* rxData, size_t len);

public:
    /**
     * Constructor for the CC1200 driver
     * @param hspi_handle Pointer to SPI handle
     * @param cs_port GPIO port for chip select pin
     * @param cs_pin GPIO pin for chip select
     * @param rst_port GPIO port for reset pin
     * @param rst_pin GPIO pin for reset
     * @param _debugStream File stream for debug output
     * @param _isCC1201 Set to true if using CC1201 variant
     */
    CC1200(SPI_HandleTypeDef* hspi_handle, 
           GPIO_TypeDef* cs_port, uint16_t cs_pin,
           GPIO_TypeDef* rst_port, uint16_t rst_pin,
           std::function<void(const std::string&)> _sendStringToDebugUart, 
           bool _isCC1201 = false);

    /**
     * Reset the CC1200 chip
     */
    void reset();
    /**
     * Initialize the CC1200 chip
     * @return true if initialization was successful
     */
    bool begin();

    /**
     * Get the number of bytes in the TX FIFO
     * @return Number of bytes in TX FIFO
     */
    size_t getTXFIFOLen();

    /**
     * Get the number of bytes in the RX FIFO
     * @return Number of bytes in RX FIFO
     */
    size_t getRXFIFOLen();

    /**
     * Enqueue a packet to be transmitted
     * @param data Pointer to data buffer
     * @param len Length of data
     * @return true if packet was successfully enqueued
     */
    bool enqueuePacket(char const* data, size_t len);

    /**
     * Check if a packet has been received
     * @return true if a packet is available
     */
    bool hasReceivedPacket();

    /**
     * Receive a packet
     * @param buffer Buffer to store received data
     * @param bufferLen Size of buffer
     * @return Number of bytes received
     */
    size_t receivePacket(char* buffer, size_t bufferLen);

    /**
     * Write data to the TX FIFO in stream mode
     * @param buffer Data to write
     * @param count Number of bytes to write
     * @return Number of bytes written
     */
    size_t writeStream(const char* buffer, size_t count);

    /**
     * Write data to the TX FIFO in stream mode, blocking until all data is written
     * @param buffer Data to write
     * @param count Number of bytes to write
     * @return true if all data was written
     */
    bool writeStreamBlocking(const char* buffer, size_t count);

    /**
     * Read data from the RX FIFO in stream mode
     * @param buffer Buffer to store data
     * @param maxLen Maximum number of bytes to read
     * @return Number of bytes read
     */
    size_t readStream(char* buffer, size_t maxLen);

    /**
     * Read data from the RX FIFO in stream mode, blocking until all data is read or timeout
     * @param buffer Buffer to store data
     * @param count Number of bytes to read
     * @param timeout Timeout duration
     * @return true if all data was read
     */
    bool readStreamBlocking(char* buffer, size_t count, std::chrono::microseconds timeout);

    /**
     * DMA-enabled packet transmission
     * @param data Pointer to data buffer
     * @param len Length of data
     * @return true if packet was successfully enqueued
     */
    bool enqueuePacketDMA(char const* data, size_t len);

    /**
     * DMA-enabled packet reception
     * @param buffer Buffer to store received data
     * @param bufferLen Size of buffer
     * @return Number of bytes received
     */
    size_t receivePacketDMA(char* buffer, size_t bufferLen);

    /**
     * DMA-enabled stream write
     * @param buffer Data to write
     * @param count Number of bytes to write
     * @return Number of bytes written
     */
    size_t writeStreamDMA(const char* buffer, size_t count);

    /**
     * DMA-enabled stream read
     * @param buffer Buffer to store data
     * @param maxLen Maximum number of bytes to read
     * @return Number of bytes read
     */
    size_t readStreamDMA(char* buffer, size_t maxLen);

    /**
     * Start continuous streaming transmission
     * @param pattern Data pattern to transmit repeatedly
     * @param patternLen Length of pattern
     * @return true if continuous streaming started
     */
    bool startContinuousStreamingTx(const char* pattern, size_t patternLen);

    /**
     * Start continuous streaming reception
     * @return true if continuous streaming started
     */
    bool startContinuousStreamingRx();

    /**
     * Stop continuous streaming transmission
     */
    void stopContinuousStreamingTx();

    /**
     * Stop continuous streaming reception
     */
    void stopContinuousStreamingRx();

    /**
     * Check if continuous streaming TX is active
     * @return true if streaming TX is active
     */
    bool isContinuousStreamingTx() const { return continuousStreamingTx; }

    /**
     * Check if continuous streaming RX is active
     * @return true if streaming RX is active
     */
    bool isContinuousStreamingRx() const { return continuousStreamingRx; }

    /**
     * Get continuous streaming statistics
     * @param txCount Number of TX packets sent
     * @param rxCount Number of RX packets received
     * @param txErrors Number of TX errors
     * @param rxErrors Number of RX errors
     */
    void getContinuousStreamingStats(uint32_t& txCount, uint32_t& rxCount, 
                                   uint32_t& txErrors, uint32_t& rxErrors);

    /**
     * Process continuous streaming (call regularly from main loop)
     */
    void processContinuousStreaming();

    /**
     * DMA transfer complete callback (called by HAL)
     */
    void dmaTransferCompleteCallback();

    /**
     * DMA transfer error callback (called by HAL)
     */
    void dmaTransferErrorCallback();

    /**
     * Set the state to transition to after receiving a packet
     * @param goodPacket State to transition to after receiving a good packet
     * @param badPacket State to transition to after receiving a bad packet
     */
    void setOnReceiveState(State goodPacket, State badPacket);

    /**
     * Set the state to transition to after transmitting a packet
     * @param txState State to transition to after transmitting
     */
    void setOnTransmitState(State txState);

    /**
     * Set the frequency synthesizer calibration mode
     * @param mode Calibration mode
     */
    void setFSCalMode(FSCalMode mode);

    /**
     * Configure a GPIO pin
     * @param gpioNumber GPIO pin number (0-3)
     * @param mode GPIO mode
     * @param outputInvert Whether to invert the output
     */
    void configureGPIO(uint8_t gpioNumber, GPIOMode mode, bool outputInvert = false);

    /**
     * Configure FIFO mode
     */
    void configureFIFOMode();

    /**
     * Set the packet mode
     * @param mode Packet mode
     * @param appendStatus Whether to append status bytes to received packets
     */
    void setPacketMode(PacketMode mode, bool appendStatus = false);

    /**
     * Set the packet length for fixed length mode
     * @param length Packet length
     * @param bitLength Length field size in bits
     */
    void setPacketLength(uint16_t length, uint8_t bitLength = 8);

    /**
     * Enable or disable CRC
     * @param enabled Whether CRC is enabled
     */
    void setCRCEnabled(bool enabled);

    /**
     * Set the modulation format
     * @param format Modulation format
     */
    void setModulationFormat(ModFormat format);

    /**
     * Set the FSK deviation
     * @param deviation Deviation in Hz
     */
    void setFSKDeviation(float deviation);

    /**
     * Set the symbol rate
     * @param symbolRateHz Symbol rate in Hz
     */
    void setSymbolRate(float symbolRateHz);

    /**
     * Set the output power
     * @param outPower Output power in dBm
     */
    void setOutputPower(float outPower);

    /**
     * Set ASK power levels
     * @param maxPower Maximum power in dBm
     * @param minPower Minimum power in dBm
     */
    void setASKPowers(float maxPower, float minPower);

    /**
     * Set the radio frequency
     * @param band Frequency band
     * @param frequencyHz Frequency in Hz
     */
    void setRadioFrequency(Band band, float frequencyHz);

    /**
     * Set the RX filter bandwidth
     * @param bandwidthHz Bandwidth in Hz
     * @param preferHigherCICDec Whether to prefer higher CIC decimation
     */
    void setRXFilterBandwidth(float bandwidthHz, bool preferHigherCICDec = false);

    /**
     * Configure the DC filter
     * @param enableAutoFilter Whether to enable auto filter
     * @param settlingCfg Settling configuration
     * @param cutoffCfg Cutoff configuration
     */
    void configureDCFilter(bool enableAutoFilter, uint8_t settlingCfg, uint8_t cutoffCfg);

    /**
     * Configure the sync word
     * @param syncWord Sync word
     * @param mode Sync mode
     * @param syncThreshold Sync threshold
     */
    void configureSyncWord(uint32_t syncWord, SyncMode mode, uint8_t syncThreshold = 0);

    /**
     * Check if frequency synthesizer is locked
     * @return true if locked
     */
    bool isFSLocked();

    /**
     * Configure preamble
     * @param preambleLengthCfg Preamble length configuration
     * @param preambleFormatCfg Preamble format configuration
     */
    void configurePreamble(uint8_t preambleLengthCfg, uint8_t preambleFormatCfg);

    /**
     * Set PA ramp rate
     * @param firstRampLevel First ramp level
     * @param secondRampLevel Second ramp level
     * @param rampTime Ramp time
     */
    void setPARampRate(uint8_t firstRampLevel, uint8_t secondRampLevel, RampTime rampTime);

    /**
     * Disable PA ramping
     */
    void disablePARamping();

    /**
     * Set AGC reference level
     * @param level Reference level
     */
    void setAGCReferenceLevel(uint8_t level);

    /**
     * Set AGC sync behavior
     * @param behavior Sync behavior
     */
    void setAGCSyncBehavior(SyncBehavior behavior);

    /**
     * Set AGC gain table
     * @param table Gain table
     * @param minGainIndex Minimum gain index
     * @param maxGainIndex Maximum gain index
     */
    void setAGCGainTable(GainTable table, uint8_t minGainIndex, uint8_t maxGainIndex);

    /**
     * Set AGC hysteresis
     * @param hysteresisCfg Hysteresis configuration
     */
    void setAGCHysteresis(uint8_t hysteresisCfg);

    /**
     * Set AGC slew rate
     * @param slewrateCfg Slew rate configuration
     */
    void setAGCSlewRate(uint8_t slewrateCfg);

    /**
     * Set AGC settle wait
     * @param settleWaitCfg Settle wait configuration
     */
    void setAGCSettleWait(uint8_t settleWaitCfg);

    /**
     * Get RSSI register value
     * @return RSSI value in dBm
     */
    float getRSSIRegister();

    /**
     * Set RSSI offset
     * @param adjust RSSI offset
     */
    void setRSSIOffset(int8_t adjust);

    /**
     * Get LQI register value
     * @return LQI value
     */
    uint8_t getLQIRegister();

    /**
     * Set IF configuration
     * @param value IF configuration
     * @param enableIQIC Whether to enable IQIC
     */
    void setIFCfg(IFCfg value, bool enableIQIC = false);

    /**
     * Read a register
     * @param reg Register address
     * @return Register value
     */
    uint8_t readRegister(Register reg);

    /**
     * Write to a register
     * @param reg Register address
     * @param value Value to write
     */
    void writeRegister(Register reg, uint8_t value);

    /**
     * Write to multiple registers
     * @param startReg Starting register address
     * @param values Values to write
     * @param numRegisters Number of registers to write
     */
    void writeRegisters(Register startReg, uint8_t const* values, size_t numRegisters);

    /**
     * Read an extended register
     * @param reg Extended register address
     * @return Register value
     */
    uint8_t readRegister(ExtRegister reg);

    /**
     * Write to an extended register
     * @param reg Extended register address
     * @param value Value to write
     */
    void writeRegister(ExtRegister reg, uint8_t value);

    /**
     * Write to multiple extended registers
     * @param startReg Starting extended register address
     * @param values Values to write
     * @param numRegisters Number of registers to write
     */
    void writeRegisters(ExtRegister startReg, uint8_t const* values, size_t numRegisters);

    /**
     * Send a command
     * @param command Command to send
     */
    void sendCommand(Command command);

    /**
     * Read a byte from the RX FIFO
     * @param address FIFO address
     * @return Byte read
     */
    uint8_t readRXFIFOByte(uint8_t address);

    /**
     * Update the internal state
     */
    void updateState();
    
    /**
     * Enable SPI debug output
     */
    void enableDebug() { debugEnabled = true; }
    
    /**
     * Disable SPI debug output
     */
    void disableDebug() { debugEnabled = false; }
    
    /**
     * Check if debug is enabled
     * @return true if debug is enabled, false otherwise
     */
    bool isDebugEnabled() const { return debugEnabled; }

    // Constants
    static constexpr float ASK_MIN_POWER_OFF = -17.5f;
};

#endif //CC1200_CC1200_HAL_H

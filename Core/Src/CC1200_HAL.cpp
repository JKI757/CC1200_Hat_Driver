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

#include "CC1200_HAL.h"
#include "CC1200Bits.h"

#include <cinttypes>
#include <cmath>
#include <array>

// Debug macro
#if CC1200_DEBUG
#define cc1200DebugPrintf(...) { if(debugStream != nullptr) (void)fprintf(debugStream, __VA_ARGS__); }
#else
#define cc1200DebugPrintf(...) // empty
#endif

// miscellaneous constants
#define CC1200_READ (1 << 7) // SPI initial byte flag indicating read
#define CC1200_WRITE 0 // SPI initial byte flag indicating write
#define CC1200_BURST (1 << 6) // SPI initial byte flag indicating burst access

// SPI commands to access data buffers. Can be used with CC1200_BURST.
#define CC1200_ENQUEUE_TX_FIFO 0x3F
#define CC1200_DEQUEUE_RX_FIFO 0xBF

// SPI command to access FIFO memory (or several other areas depending on mode)
#define CC1200_MEM_ACCESS 0x3E

#define CC1200_RX_FIFO (1 << 7) // address flag to access RX FIFO
#define CC1200_TX_FIFO 0 // address flag to access TX FIFO

#define CC1200_PART_NUMBER ((uint8_t)0x20) // part number we expect the chip to read
#define CC1201_PART_NUMBER ((uint8_t)0x21)
#define CC1200_EXT_ADDR 0x2F // SPI initial byte address indicating extended register space

#define SPI_FREQ 5000000 // hz
// NOTE: the chip supports a higher frequency for most operations but reads to extended registers require a lower frequency

// frequency of the chip's crystal oscillator
#define CC1200_OSC_FREQ 40000000 // hz
#define CC1200_OSC_FREQ_LOG2 25.253496f // log2 of above number

// length of the TX and RX FIFOs
#define CC1200_FIFO_SIZE 128

// maximum length of the packets we can send, including the length byte which we add.
// Since the TX and RX FIFOs are 128 bytes, supporting packet lengths longer than 128 bytes
// requires streaming bytes in during the transmission, which would make things complicated.
#define MAX_PACKET_LENGTH 128

// Length of the status bytes that can be appended to packets
#define PACKET_STATUS_LEN 2U

// utility function: compile-time power calculator.
// Works on all signed and unsigned integer types for T.
template <typename T>
constexpr T constexpr_pow(T num, unsigned int pow)
{
    return pow == 0 ? 1 : num * constexpr_pow(num, pow-1);
}

// power of two constants
const float twoToThe16 = constexpr_pow(2.0f, 16);
const float twoToThe20 = constexpr_pow(2.0f, 20);
const float twoToThe21 = constexpr_pow(2.0f, 21);
const float twoToThe22 = constexpr_pow(2.0f, 22);
const float twoToThe38 = constexpr_pow(2.0f, 38);
const float twoToThe39 = constexpr_pow(2.0f, 39);

// binary value size constants
const size_t maxValue3Bits = constexpr_pow(2, 3) - 1;
const size_t maxValue4Bits = constexpr_pow(2, 4) - 1;
const size_t maxValue8Bits = constexpr_pow(2, 8) - 1;
const size_t maxValue20Bits = constexpr_pow(2, 20) - 1;
const size_t maxValue24Bits = constexpr_pow(2, 24) - 1;

// Constructor
CC1200::CC1200(SPI_HandleTypeDef* hspi_handle, 
               GPIO_TypeDef* cs_port, uint16_t cs_pin,
               GPIO_TypeDef* rst_port, uint16_t rst_pin,
               FILE* _debugStream, bool _isCC1201) :
    hspi(hspi_handle),
    csPort(cs_port),
    csPin(cs_pin),
    rstPort(rst_port),
    rstPin(rst_pin),
    debugStream(_debugStream),
    isCC1201(_isCC1201)
{
    // Initialize CS pin as output and set it high (deselected)
    HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
    
    // Initialize RST pin as output and set it high
    HAL_GPIO_WritePin(rstPort, rstPin, GPIO_PIN_SET);
}

// Helper functions for SPI communication
void CC1200::select()
{
    HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
}

void CC1200::deselect()
{
    HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
}

uint8_t CC1200::spiTransfer(uint8_t data)
{
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(hspi, &data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

bool CC1200::begin()
{
    chipReady = false;

    // Reset the chip
    HAL_GPIO_WritePin(rstPort, rstPin, GPIO_PIN_RESET);
    HAL_Delay(1); // 1ms delay
    HAL_GPIO_WritePin(rstPort, rstPin, GPIO_PIN_SET);

    uint32_t resetTimeout = 10; // 10ms timeout
    uint32_t startTime = HAL_GetTick();

    while(!chipReady)
    {
        // datasheet specifies 240us reset time
        HAL_Delay(1); // 1ms delay
        updateState();

        if(HAL_GetTick() - startTime > resetTimeout)
        {
            cc1200DebugPrintf("Timeout waiting for ready response from CC1200\n");
            break;
        }
    }

    // read ID register
    uint8_t partNumber = readRegister(ExtRegister::PARTNUMBER);
    uint8_t partVersion = readRegister(ExtRegister::PARTVERSION);

    uint8_t expectedPartNumber = isCC1201 ? CC1201_PART_NUMBER : CC1200_PART_NUMBER;
    if(partNumber != expectedPartNumber)
    {
        cc1200DebugPrintf("Read incorrect part number 0x%" PRIx8 " from CC1200, expected 0x%" PRIx8 "\n", partNumber, expectedPartNumber);
        return false;
    }

    cc1200DebugPrintf("Detected CC1200, Part Number 0x%" PRIx8 ", Hardware Version %" PRIx8 "\n", partNumber, partVersion);

    // Set packet format settings for this driver
    // enable CRC but disable status bytes
    writeRegister(Register::PKT_CFG1, (0b01 << PKT_CFG1_CRC_CFG));

    return true;
}

void CC1200::updateState()
{
    // Read the status byte to update the internal state
    select();
    uint8_t status = spiTransfer(CC1200_NOP);
    deselect();
    
    loadStatusByte(status);
}

void CC1200::loadStatusByte(uint8_t status)
{
    lastStatus = status;
    
    // Extract the chip state from the status byte
    uint8_t stateValue = (status >> 4) & 0x7;
    state = static_cast<State>(stateValue);
    
    // Check if the chip is ready
    chipReady = (status & 0x80) == 0;
}

size_t CC1200::getTXFIFOLen()
{
    return readRegister(ExtRegister::NUM_TXBYTES);
}

size_t CC1200::getRXFIFOLen()
{
    return readRegister(ExtRegister::NUM_RXBYTES);
}

bool CC1200::enqueuePacket(char const* data, size_t len)
{
    uint8_t totalLength = len + 1; // add one byte for length byte

    if(totalLength > MAX_PACKET_LENGTH)
    {
        // packet too big
        return false;
    }

    uint8_t txFreeBytes = CC1200_FIFO_SIZE - getTXFIFOLen();
    if(totalLength > txFreeBytes)
    {
        // packet doesn't fit in TX FIFO
        return false;
    }

    // burst write to TX FIFO
    select();
    loadStatusByte(spiTransfer(CC1200_ENQUEUE_TX_FIFO | CC1200_BURST));
    if(_packetMode == PacketMode::VARIABLE_LENGTH)
    {
        spiTransfer(len);
    }
    for(size_t byteIndex = 0; byteIndex < len; ++byteIndex)
    {
        spiTransfer(data[byteIndex]);
    }
    deselect();

#if CC1200_DEBUG
    cc1200DebugPrintf("Wrote packet of data length %zu:", len);
    if(_packetMode == PacketMode::VARIABLE_LENGTH)
    {
        cc1200DebugPrintf(" %02" PRIx8, static_cast<uint8_t>(len));
    }
    for(size_t byteIndex = 0; byteIndex < len; ++byteIndex)
    {
        cc1200DebugPrintf(" %02" PRIx8, static_cast<uint8_t>(data[byteIndex]));
    }
    cc1200DebugPrintf("\n");
#endif

    return true;
}

bool CC1200::hasReceivedPacket()
{
    size_t rxBytes = getRXFIFOLen();
    if(rxBytes == 0)
    {
        return false;
    }

    if(_packetMode == PacketMode::FIXED_LENGTH)
    {
        return true;
    }
    else
    {
        // In variable length mode, we need at least 1 byte (the length byte)
        if(rxBytes < 1)
        {
            return false;
        }

        // Read the length byte
        uint8_t packetLen = readRXFIFOByte(0);
        
        // Check if we have received the complete packet
        // Add 1 for the length byte itself
        return rxBytes >= packetLen + 1 + (_appendStatus ? PACKET_STATUS_LEN : 0);
    }
}

size_t CC1200::receivePacket(char* buffer, size_t bufferLen)
{
    size_t rxBytes = getRXFIFOLen();
    if(rxBytes == 0)
    {
        return 0;
    }

    size_t packetLen;
    size_t headerLen;

    if(_packetMode == PacketMode::FIXED_LENGTH)
    {
        packetLen = rxBytes;
        headerLen = 0;
    }
    else
    {
        // In variable length mode, first byte is the length
        if(rxBytes < 1)
        {
            return 0;
        }

        // Read the length byte
        uint8_t lengthByte = readRXFIFOByte(0);
        packetLen = lengthByte;
        headerLen = 1;

        // Check if we have received the complete packet
        if(rxBytes < packetLen + headerLen + (_appendStatus ? PACKET_STATUS_LEN : 0))
        {
            return 0;
        }
    }

    // Limit to buffer size
    size_t bytesToRead = packetLen;
    if(bytesToRead > bufferLen)
    {
        bytesToRead = bufferLen;
    }

    // Read the packet data
    select();
    loadStatusByte(spiTransfer(CC1200_DEQUEUE_RX_FIFO | CC1200_BURST));
    
    // Skip the length byte in variable length mode
    if(_packetMode == PacketMode::VARIABLE_LENGTH)
    {
        spiTransfer(0);
    }
    
    // Read the actual data
    for(size_t i = 0; i < bytesToRead; i++)
    {
        buffer[i] = spiTransfer(0);
    }
    
    // Skip any remaining bytes (including status bytes if appended)
    for(size_t i = bytesToRead; i < packetLen + (_appendStatus ? PACKET_STATUS_LEN : 0); i++)
    {
        spiTransfer(0);
    }
    
    deselect();

    return bytesToRead;
}

// Register access functions
uint8_t CC1200::readRegister(Register reg)
{
    select();
    spiTransfer(static_cast<uint8_t>(reg) | CC1200_READ);
    uint8_t value = spiTransfer(0);
    deselect();
    return value;
}

void CC1200::writeRegister(Register reg, uint8_t value)
{
    select();
    spiTransfer(static_cast<uint8_t>(reg) | CC1200_WRITE);
    spiTransfer(value);
    deselect();
}

void CC1200::writeRegisters(Register startReg, uint8_t const* values, size_t numRegisters)
{
    select();
    spiTransfer(static_cast<uint8_t>(startReg) | CC1200_WRITE | CC1200_BURST);
    for(size_t i = 0; i < numRegisters; i++)
    {
        spiTransfer(values[i]);
    }
    deselect();
}

uint8_t CC1200::readRegister(ExtRegister reg)
{
    select();
    spiTransfer(CC1200_EXT_ADDR | CC1200_READ);
    spiTransfer(static_cast<uint8_t>(reg));
    uint8_t value = spiTransfer(0);
    deselect();
    return value;
}

void CC1200::writeRegister(ExtRegister reg, uint8_t value)
{
    select();
    spiTransfer(CC1200_EXT_ADDR | CC1200_WRITE);
    spiTransfer(static_cast<uint8_t>(reg));
    spiTransfer(value);
    deselect();
}

void CC1200::writeRegisters(ExtRegister startReg, uint8_t const* values, size_t numRegisters)
{
    select();
    spiTransfer(CC1200_EXT_ADDR | CC1200_WRITE | CC1200_BURST);
    spiTransfer(static_cast<uint8_t>(startReg));
    for(size_t i = 0; i < numRegisters; i++)
    {
        spiTransfer(values[i]);
    }
    deselect();
}

void CC1200::sendCommand(Command command)
{
    select();
    loadStatusByte(spiTransfer(static_cast<uint8_t>(command)));
    deselect();
}

uint8_t CC1200::readRXFIFOByte(uint8_t address)
{
    select();
    spiTransfer(CC1200_MEM_ACCESS | CC1200_READ);
    spiTransfer(CC1200_RX_FIFO | address);
    uint8_t value = spiTransfer(0);
    deselect();
    return value;
}

// ASK_MIN_POWER_OFF is already defined as a constexpr in the header file
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

// This file contains additional implementation functions for CC1200_HAL.cpp
// These functions should be appended to the main implementation file

size_t CC1200::writeStream(const char* buffer, size_t count)
{
    if(count == 0)
    {
        return 0;
    }

    // Check how much space is available in the TX FIFO
    uint8_t txFreeBytes = CC1200_FIFO_SIZE - getTXFIFOLen();
    if(txFreeBytes == 0)
    {
        return 0;
    }

    // Limit to available space
    size_t bytesToWrite = count;
    if(bytesToWrite > txFreeBytes)
    {
        bytesToWrite = txFreeBytes;
    }

    // Write to TX FIFO
    select();
    loadStatusByte(spiTransfer(CC1200_ENQUEUE_TX_FIFO | CC1200_BURST));
    for(size_t i = 0; i < bytesToWrite; i++)
    {
        spiTransfer(buffer[i]);
    }
    deselect();

    return bytesToWrite;
}

bool CC1200::writeStreamBlocking(const char* buffer, size_t count)
{
    size_t bytesWritten = 0;
    
    while(bytesWritten < count)
    {
        // Try to write remaining bytes
        size_t written = writeStream(buffer + bytesWritten, count - bytesWritten);
        if(written == 0)
        {
            // If no bytes were written, wait a bit and try again
            HAL_Delay(1);
        }
        else
        {
            bytesWritten += written;
        }
    }
    
    return bytesWritten == count;
}

size_t CC1200::readStream(char* buffer, size_t maxLen)
{
    if(maxLen == 0)
    {
        return 0;
    }

    // Check how many bytes are available in the RX FIFO
    size_t rxBytes = getRXFIFOLen();
    if(rxBytes == 0)
    {
        return 0;
    }

    // Limit to buffer size
    size_t bytesToRead = rxBytes;
    if(bytesToRead > maxLen)
    {
        bytesToRead = maxLen;
    }

    // Read from RX FIFO
    select();
    loadStatusByte(spiTransfer(CC1200_DEQUEUE_RX_FIFO | CC1200_BURST));
    for(size_t i = 0; i < bytesToRead; i++)
    {
        buffer[i] = spiTransfer(0);
    }
    deselect();

    return bytesToRead;
}

bool CC1200::readStreamBlocking(char* buffer, size_t count, std::chrono::microseconds timeout)
{
    size_t bytesRead = 0;
    uint32_t startTime = HAL_GetTick();
    uint32_t timeoutMs = timeout.count() / 1000; // Convert microseconds to milliseconds
    
    while(bytesRead < count)
    {
        // Check for timeout
        if(HAL_GetTick() - startTime > timeoutMs)
        {
            return false;
        }
        
        // Try to read remaining bytes
        size_t read = readStream(buffer + bytesRead, count - bytesRead);
        if(read == 0)
        {
            // If no bytes were read, wait a bit and try again
            HAL_Delay(1);
        }
        else
        {
            bytesRead += read;
        }
    }
    
    return bytesRead == count;
}

// helper function: convert a state to the bits for RXOFF_MODE and TXOFF_MODE
uint8_t getOffModeBits(CC1200::State state)
{
    switch(state)
    {
        case CC1200::State::IDLE:
            return 0;
        case CC1200::State::FAST_ON:
            return 1;
        case CC1200::State::TX:
            return 2;
        case CC1200::State::RX:
            return 3;
        default:
            // Invalid state for RXOFF_MODE or TXOFF_MODE, use IDLE
            return 0;
    }
}

void CC1200::setOnReceiveState(State goodPacket, State badPacket)
{
    uint8_t rfendCfg0 = readRegister(Register::RFEND_CFG0);
    
    // Clear the RXOFF_MODE bits (bits 2-3)
    rfendCfg0 &= ~(0b11 << RFEND_CFG0_RXOFF_MODE);
    
    // Set the new RXOFF_MODE bits
    rfendCfg0 |= (getOffModeBits(goodPacket) << RFEND_CFG0_RXOFF_MODE);
    
    // Clear the RX_FINITE bits (bits 0-1)
    rfendCfg0 &= ~(0b11 << RFEND_CFG0_RX_TIME);
    
    // Set the new RX_FINITE bits (use same state for now)
    rfendCfg0 |= (getOffModeBits(badPacket) << RFEND_CFG0_RX_TIME);
    
    writeRegister(Register::RFEND_CFG0, rfendCfg0);
}

void CC1200::setOnTransmitState(State txState)
{
    uint8_t rfendCfg0 = readRegister(Register::RFEND_CFG0);
    
    // Clear the TXOFF_MODE bits (bits 4-5)
    rfendCfg0 &= ~(0b11 << RFEND_CFG0_TXOFF_MODE);
    
    // Set the new TXOFF_MODE bits
    rfendCfg0 |= (getOffModeBits(txState) << RFEND_CFG0_TXOFF_MODE);
    
    writeRegister(Register::RFEND_CFG0, rfendCfg0);
}

void CC1200::setFSCalMode(FSCalMode mode)
{
    uint8_t fsCfg = readRegister(Register::FS_CFG);
    
    // Clear the FS_AUTOCAL bits (bits 4-6)
    fsCfg &= ~(0b111 << FS_CFG_FS_AUTOCAL);
    
    // Set the new FS_AUTOCAL bits
    fsCfg |= (static_cast<uint8_t>(mode) << FS_CFG_FS_AUTOCAL);
    
    writeRegister(Register::FS_CFG, fsCfg);
}

void CC1200::configureGPIO(uint8_t gpioNumber, GPIOMode mode, bool outputInvert)
{
    if(gpioNumber > 3)
    {
        // Invalid GPIO number
        return;
    }
    
    Register reg;
    switch(gpioNumber)
    {
        case 0:
            reg = Register::IOCFG0;
            break;
        case 1:
            reg = Register::IOCFG1;
            break;
        case 2:
            reg = Register::IOCFG2;
            break;
        case 3:
            reg = Register::IOCFG3;
            break;
        default:
            return;
    }
    
    uint8_t value = static_cast<uint8_t>(mode);
    if(outputInvert)
    {
        value |= (1 << IOCFG_GPIO_INV);
    }
    
    writeRegister(reg, value);
}

void CC1200::configureFIFOMode()
{
    // Configure FIFO_CFG register for FIFO mode
    uint8_t fifoCfg = 0;
    
    // Set CRC_AUTOFLUSH to 0 (don't auto-flush on CRC error)
    // Set FIFO_THR to 0 (threshold at 32 bytes)
    
    writeRegister(Register::FIFO_CFG, fifoCfg);
}

void CC1200::setPacketMode(PacketMode mode, bool appendStatus)
{
    _packetMode = mode;
    _appendStatus = appendStatus;
    
    uint8_t pktCfg0 = readRegister(Register::PKT_CFG0);
    
    // Set LENGTH_CONFIG field (bits 0-1)
    if(mode == PacketMode::FIXED_LENGTH)
    {
        // Clear the LENGTH_CONFIG bits for fixed length
        pktCfg0 &= ~(0b11 << PKT_CFG0_LENGTH_CONFIG);
    }
    else // VARIABLE_LENGTH
    {
        // Set LENGTH_CONFIG to 1 for variable length
        pktCfg0 &= ~(0b11 << PKT_CFG0_LENGTH_CONFIG);
        pktCfg0 |= (0b01 << PKT_CFG0_LENGTH_CONFIG);
    }
    
    // Set PKT_FORMAT field (bits 4-5) to 0 for normal mode
    pktCfg0 &= ~(0b11 << PKT_CFG0_PKT_FORMAT);
    
    // Set APPEND_STATUS field (bit 2)
    if(appendStatus)
    {
        pktCfg0 |= (1 << PKT_CFG0_APPEND_STATUS);
    }
    else
    {
        pktCfg0 &= ~(1 << PKT_CFG0_APPEND_STATUS);
    }
    
    writeRegister(Register::PKT_CFG0, pktCfg0);
}

void CC1200::setPacketLength(uint16_t length, uint8_t bitLength)
{
    if(bitLength > 8)
    {
        // For packets longer than 255 bytes, we need to use the PKT_CFG1 register
        uint8_t pktCfg1 = readRegister(Register::PKT_CFG1);
        
        // Set PQT_EN field (bit 5) to 0
        pktCfg1 &= ~(1 << PKT_CFG1_PQT_EN);
        
        // Set LENGTH_POSITION field (bit 6) to 0
        pktCfg1 &= ~(1 << PKT_CFG1_LENGTH_POSITION);
        
        // Set the high bits of the length
        pktCfg1 &= ~(0b11 << PKT_CFG1_LENGTH_FIELD_SIZE);
        pktCfg1 |= ((bitLength - 1) << PKT_CFG1_LENGTH_FIELD_SIZE);
        
        writeRegister(Register::PKT_CFG1, pktCfg1);
    }
    
    // Set the packet length
    writeRegister(Register::PKT_LEN, length & 0xFF);
}

void CC1200::setCRCEnabled(bool enabled)
{
    uint8_t pktCfg1 = readRegister(Register::PKT_CFG1);
    
    // Clear the CRC_CFG bits (bits 2-3)
    pktCfg1 &= ~(0b11 << PKT_CFG1_CRC_CFG);
    
    if(enabled)
    {
        // Set CRC_CFG to 1 for CRC enabled
        pktCfg1 |= (0b01 << PKT_CFG1_CRC_CFG);
    }
    
    writeRegister(Register::PKT_CFG1, pktCfg1);
}

void CC1200::setModulationFormat(ModFormat format)
{
    uint8_t modcfgDevE = readRegister(Register::MODCFG_DEV_E);
    
    // Clear the MOD_FORMAT bits (bits 4-6)
    modcfgDevE &= ~(0b111 << MODCFG_DEV_E_MOD_FORMAT);
    
    // Set the new MOD_FORMAT bits
    modcfgDevE |= (static_cast<uint8_t>(format) << MODCFG_DEV_E_MOD_FORMAT);
    
    writeRegister(Register::MODCFG_DEV_E, modcfgDevE);
}

void CC1200::setFSKDeviation(float deviation)
{
    // Calculate the register values for the given deviation
    // See CC1200 user guide section 9.12
    
    float normalizedDeviation = deviation / CC1200_OSC_FREQ;
    
    // Find the best exponent (0-7)
    uint8_t devE = 0;
    float devM = normalizedDeviation * 524288.0f; // 2^19
    
    while(devM > 255.0f && devE < 7)
    {
        devM /= 2.0f;
        devE++;
    }
    
    // Round to nearest integer
    uint8_t devMInt = static_cast<uint8_t>(devM + 0.5f);
    
    // Make sure we don't exceed the maximum value
    if(devMInt > 255)
    {
        devMInt = 255;
    }
    
    // Update the DEVIATION_M register
    writeRegister(Register::DEVIATION_M, devMInt);
    
    // Update the MODCFG_DEV_E register (preserve the MOD_FORMAT bits)
    uint8_t modcfgDevE = readRegister(Register::MODCFG_DEV_E);
    modcfgDevE &= ~(0b111 << MODCFG_DEV_E_DEV_E); // Clear DEV_E bits (bits 0-2)
    modcfgDevE |= (devE << MODCFG_DEV_E_DEV_E); // Set new DEV_E bits
    writeRegister(Register::MODCFG_DEV_E, modcfgDevE);
    
    // Calculate and store the actual deviation
    float actualDeviation = static_cast<float>(devMInt) * std::pow(2.0f, devE) * CC1200_OSC_FREQ / 524288.0f;
    cc1200DebugPrintf("Set FSK deviation to %.2f Hz (requested %.2f Hz)\n", actualDeviation, deviation);
}
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

// This file contains additional implementation functions for CC1200_HAL.cpp
// These functions should be appended to the main implementation file

void CC1200::setSymbolRate(float symbolRateHz)
{
    // Calculate the register values for the given symbol rate
    // See CC1200 user guide section 9.11
    
    float normalizedSymbolRate = symbolRateHz / CC1200_OSC_FREQ;
    
    // Find the best exponent (0-20)
    uint8_t srE = 0;
    float srM = normalizedSymbolRate * twoToThe20;
    
    while(srM < 128.0f && srE < 20)
    {
        srM *= 2.0f;
        srE++;
    }
    
    // Round to nearest integer
    uint32_t srMInt = static_cast<uint32_t>(srM + 0.5f);
    
    // Make sure we don't exceed the maximum value
    if(srMInt > 255)
    {
        srMInt = 255;
    }
    
    // Calculate the actual symbol rate
    float actualSymbolRate = static_cast<float>(srMInt) * CC1200_OSC_FREQ / (twoToThe20 * std::pow(2.0f, srE));
    currentSymbolRate = actualSymbolRate;
    
    // Update the SYMBOL_RATE registers
    writeRegister(Register::SYMBOL_RATE0, srMInt & 0xFF);
    
    uint8_t symbolRate1 = readRegister(Register::SYMBOL_RATE1);
    symbolRate1 &= ~(0b1111 << SYMBOL_RATE1_SRATE_E); // Clear SRATE_E bits (bits 0-3)
    symbolRate1 |= (srE << SYMBOL_RATE1_SRATE_E); // Set new SRATE_E bits
    writeRegister(Register::SYMBOL_RATE1, symbolRate1);
    
    cc1200DebugPrintf("Set symbol rate to %.2f Hz (requested %.2f Hz)\n", actualSymbolRate, symbolRateHz);
}

// helper function for power setting
uint8_t dBPowerToRegValue(float powerDB)
{
    // Convert dBm to register value
    // The mapping is approximately linear from -16 dBm (0x00) to +14 dBm (0x3F)
    
    // Clamp the power to the valid range
    if(powerDB < -16.0f)
    {
        powerDB = -16.0f;
    }
    else if(powerDB > 14.0f)
    {
        powerDB = 14.0f;
    }
    
    // Convert to register value (0-63)
    uint8_t regValue = static_cast<uint8_t>((powerDB + 16.0f) * 2.0f + 0.5f);
    
    return regValue;
}

void CC1200::setOutputPower(float outPower)
{
    uint8_t paValue = dBPowerToRegValue(outPower);
    
    // Update the PA_CFG1 register
    writeRegister(Register::PA_CFG1, paValue);
    
    cc1200DebugPrintf("Set output power to %.2f dBm (register value: 0x%02X)\n", outPower, paValue);
}

void CC1200::setASKPowers(float maxPower, float minPower)
{
    // For ASK modulation, we need to set both the maximum and minimum power levels
    
    uint8_t maxPaValue = dBPowerToRegValue(maxPower);
    uint8_t minPaValue = dBPowerToRegValue(minPower);
    
    // Update the PA_CFG1 register for maximum power
    writeRegister(Register::PA_CFG1, maxPaValue);
    
    // Update the ASK_CFG register for minimum power
    // The ASK_CFG register contains a 6-bit power level field
    writeRegister(Register::ASK_CFG, minPaValue);
    
    cc1200DebugPrintf("Set ASK powers to %.2f dBm (max) and %.2f dBm (min)\n", maxPower, minPower);
}

void CC1200::setRadioFrequency(Band band, float frequencyHz)
{
    // Calculate the register values for the given frequency
    // See CC1200 user guide section 9.10
    
    uint32_t fregValue;
    
    switch(band)
    {
        case Band::BAND_820_960MHz:
            // For 820-960 MHz band, use the formula: FREQ = f * 2^16 / f_XOSC
            fregValue = static_cast<uint32_t>((frequencyHz * twoToThe16 / CC1200_OSC_FREQ) + 0.5f);
            break;
            
        case Band::BAND_410_480MHz:
            // For 410-480 MHz band, use the formula: FREQ = f * 2^17 / f_XOSC
            fregValue = static_cast<uint32_t>((frequencyHz * twoToThe16 * 2.0f / CC1200_OSC_FREQ) + 0.5f);
            break;
            
        case Band::BAND_136_160MHz:
            // For 136-160 MHz band, use the formula: FREQ = f * 2^21 / f_XOSC
            fregValue = static_cast<uint32_t>((frequencyHz * twoToThe21 / CC1200_OSC_FREQ) + 0.5f);
            break;
            
        case Band::BAND_410_480MHz_HIGH_IF:
            // For 410-480 MHz band with high IF, use the formula: FREQ = f * 2^17 / f_XOSC
            fregValue = static_cast<uint32_t>((frequencyHz * twoToThe16 * 2.0f / CC1200_OSC_FREQ) + 0.5f);
            break;
            
        default:
            // Invalid band
            return;
    }
    
    // Make sure we don't exceed the maximum value
    if(fregValue > maxValue24Bits)
    {
        fregValue = maxValue24Bits;
    }
    
    // Update the FREQ registers
    writeRegister(ExtRegister::FREQ0, fregValue & 0xFF);
    writeRegister(ExtRegister::FREQ1, (fregValue >> 8) & 0xFF);
    writeRegister(ExtRegister::FREQ2, (fregValue >> 16) & 0xFF);
    
    // Configure the band-specific settings
    uint8_t fs_cfg = readRegister(Register::FS_CFG);
    fs_cfg &= ~(0b11 << FS_CFG_FSD_BANDSELECT); // Clear FSD_BANDSELECT bits (bits 0-1)
    
    switch(band)
    {
        case Band::BAND_820_960MHz:
            fs_cfg |= (0b00 << FS_CFG_FSD_BANDSELECT); // Set FSD_BANDSELECT to 0 for 820-960 MHz
            break;
            
        case Band::BAND_410_480MHz:
            fs_cfg |= (0b01 << FS_CFG_FSD_BANDSELECT); // Set FSD_BANDSELECT to 1 for 410-480 MHz
            break;
            
        case Band::BAND_136_160MHz:
            fs_cfg |= (0b10 << FS_CFG_FSD_BANDSELECT); // Set FSD_BANDSELECT to 2 for 136-160 MHz
            break;
            
        case Band::BAND_410_480MHz_HIGH_IF:
            fs_cfg |= (0b11 << FS_CFG_FSD_BANDSELECT); // Set FSD_BANDSELECT to 3 for 410-480 MHz high IF
            break;
            
        default:
            break;
    }
    
    writeRegister(Register::FS_CFG, fs_cfg);
    
    // Store the current frequency
    currentFrequency = frequencyHz;
    
    // Calculate the actual frequency
    float actualFrequency;
    switch(band)
    {
        case Band::BAND_820_960MHz:
            actualFrequency = static_cast<float>(fregValue) * CC1200_OSC_FREQ / twoToThe16;
            break;
            
        case Band::BAND_410_480MHz:
            actualFrequency = static_cast<float>(fregValue) * CC1200_OSC_FREQ / (twoToThe16 * 2.0f);
            break;
            
        case Band::BAND_136_160MHz:
            actualFrequency = static_cast<float>(fregValue) * CC1200_OSC_FREQ / twoToThe21;
            break;
            
        case Band::BAND_410_480MHz_HIGH_IF:
            actualFrequency = static_cast<float>(fregValue) * CC1200_OSC_FREQ / (twoToThe16 * 2.0f);
            break;
            
        default:
            actualFrequency = 0.0f;
            break;
    }
    
    cc1200DebugPrintf("Set radio frequency to %.2f Hz (requested %.2f Hz)\n", actualFrequency, frequencyHz);
}

// helper function for setRXFilterBandwidth:
// calculate actual receive bandwidth from the given decimations.
float calcReceiveBandwidth(uint8_t adcDecimation, uint8_t cicDecimation)
{
    return CC1200_OSC_FREQ / (8.0f * adcDecimation * cicDecimation);
}

void CC1200::setRXFilterBandwidth(float bandwidthHz, bool preferHigherCICDec)
{
    // Calculate the register values for the given bandwidth
    // See CC1200 user guide section 9.13
    
    // The bandwidth is determined by the formula: BW = f_XOSC / (8 * ADC_DEC * CIC_DEC)
    // where ADC_DEC is 16, 24, 32, or 40, and CIC_DEC is 1, 2, 4, 8, 16, 32, or 64
    
    // Find the best combination of ADC_DEC and CIC_DEC
    uint8_t bestAdcDec = 0;
    uint8_t bestCicDec = 0;
    float bestBandwidth = 0.0f;
    float bestError = std::numeric_limits<float>::max();
    
    // ADC decimation options
    const uint8_t adcDecOptions[] = {16, 24, 32, 40};
    const uint8_t adcDecRegValues[] = {0, 1, 2, 3};
    
    // CIC decimation options
    const uint8_t cicDecOptions[] = {1, 2, 4, 8, 16, 32, 64};
    const uint8_t cicDecRegValues[] = {0, 1, 2, 3, 4, 5, 6};
    
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 7; j++)
        {
            float bw = calcReceiveBandwidth(adcDecOptions[i], cicDecOptions[j]);
            float error = std::abs(bw - bandwidthHz);
            
            if(error < bestError || (error == bestError && preferHigherCICDec && cicDecOptions[j] > bestCicDec))
            {
                bestError = error;
                bestBandwidth = bw;
                bestAdcDec = adcDecRegValues[i];
                bestCicDec = cicDecRegValues[j];
            }
        }
    }
    
    // Update the CHAN_BW register
    uint8_t chanBw = (bestAdcDec << CHAN_BW_ADC_CIC_DECFACT) | bestCicDec;
    writeRegister(Register::CHAN_BW, chanBw);
    
    // Store the current RX bandwidth
    currentRXBandwidth = bestBandwidth;
    
    cc1200DebugPrintf("Set RX filter bandwidth to %.2f Hz (requested %.2f Hz)\n", bestBandwidth, bandwidthHz);
}

void CC1200::configureDCFilter(bool enableAutoFilter, uint8_t settlingCfg, uint8_t cutoffCfg)
{
    uint8_t dcfiltCfg = 0;
    
    if(enableAutoFilter)
    {
        dcfiltCfg |= (1 << DCFILT_CFG_DCFILT_FREEZE_COEFF);
    }
    
    dcfiltCfg |= ((settlingCfg & 0x3) << DCFILT_CFG_DCFILT_BW_SETTLE);
    dcfiltCfg |= ((cutoffCfg & 0x3) << DCFILT_CFG_DCFILT_BW);
    
    writeRegister(Register::DCFILT_CFG, dcfiltCfg);
}

void CC1200::configureSyncWord(uint32_t syncWord, SyncMode mode, uint8_t syncThreshold)
{
    // Update the SYNC registers with the sync word
    writeRegister(Register::SYNC3, (syncWord >> 24) & 0xFF);
    writeRegister(Register::SYNC2, (syncWord >> 16) & 0xFF);
    writeRegister(Register::SYNC1, (syncWord >> 8) & 0xFF);
    writeRegister(Register::SYNC0, syncWord & 0xFF);
    
    // Configure the sync mode
    uint8_t syncCfg0 = readRegister(Register::SYNC_CFG0);
    syncCfg0 &= ~(0b111 << SYNC_CFG0_SYNC_MODE); // Clear SYNC_MODE bits (bits 0-2)
    syncCfg0 |= (static_cast<uint8_t>(mode) << SYNC_CFG0_SYNC_MODE); // Set new SYNC_MODE bits
    writeRegister(Register::SYNC_CFG0, syncCfg0);
    
    // Set the sync threshold if applicable
    if(syncThreshold > 0)
    {
        uint8_t syncCfg1 = readRegister(Register::SYNC_CFG1);
        syncCfg1 &= ~(0b111 << SYNC_CFG1_SYNC_THR); // Clear SYNC_THR bits (bits 0-2)
        syncCfg1 |= ((syncThreshold & 0x7) << SYNC_CFG1_SYNC_THR); // Set new SYNC_THR bits
        writeRegister(Register::SYNC_CFG1, syncCfg1);
    }
}

bool CC1200::isFSLocked()
{
    // Check if the frequency synthesizer is locked
    // This is indicated by bit 6 of the FSCAL_CTRL register
    uint8_t fscalCtrl = readRegister(ExtRegister::FSCAL_CTRL);
    return (fscalCtrl & (1 << 6)) != 0;
}

void CC1200::configurePreamble(uint8_t preambleLengthCfg, uint8_t preambleFormatCfg)
{
    // Configure the preamble length and format
    uint8_t preambleCfg0 = readRegister(Register::PREAMBLE_CFG0);
    preambleCfg0 &= ~(0b111 << PREAMBLE_CFG0_NUM_PREAMBLE); // Clear NUM_PREAMBLE bits (bits 0-2)
    preambleCfg0 |= ((preambleLengthCfg & 0x7) << PREAMBLE_CFG0_NUM_PREAMBLE); // Set new NUM_PREAMBLE bits
    
    preambleCfg0 &= ~(0b111 << PREAMBLE_CFG0_PREAMBLE_WORD); // Clear PREAMBLE_WORD bits (bits 5-7)
    preambleCfg0 |= ((preambleFormatCfg & 0x7) << PREAMBLE_CFG0_PREAMBLE_WORD); // Set new PREAMBLE_WORD bits
    
    writeRegister(Register::PREAMBLE_CFG0, preambleCfg0);
}
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

// This file contains additional implementation functions for CC1200_HAL.cpp
// These functions should be appended to the main implementation file

void CC1200::setPARampRate(uint8_t firstRampLevel, uint8_t secondRampLevel, RampTime rampTime)
{
    // Configure the PA ramp rate
    uint8_t paCfg0 = 0;
    
    // Set the PA_RAMP_SHAPE bits (bits 0-1)
    paCfg0 |= ((firstRampLevel & 0x3) << PA_CFG0_PA_RAMP_SHAPE_0);
    
    // Set the PA_RAMP_SHAPE bits (bits 2-3)
    paCfg0 |= ((secondRampLevel & 0x3) << PA_CFG0_PA_RAMP_SHAPE_1);
    
    // Set the PA_RAMP_STEP_TIME bits (bits 4-6)
    paCfg0 |= (static_cast<uint8_t>(rampTime) << PA_CFG0_PA_RAMP_STEP_TIME);
    
    writeRegister(Register::PA_CFG0, paCfg0);
}

void CC1200::disablePARamping()
{
    // Disable PA ramping by setting all ramp levels to 0
    writeRegister(Register::PA_CFG0, 0);
}

void CC1200::setAGCReferenceLevel(uint8_t level)
{
    // Set the AGC reference level
    writeRegister(Register::AGC_REF, level);
}

void CC1200::setAGCSyncBehavior(SyncBehavior behavior)
{
    // Configure the AGC sync behavior
    uint8_t agcCfg1 = readRegister(Register::AGC_CFG1);
    agcCfg1 &= ~(0b11 << AGC_CFG1_SYNC_BEHAVIOR); // Clear SYNC_BEHAVIOR bits (bits 0-1)
    agcCfg1 |= (static_cast<uint8_t>(behavior) << AGC_CFG1_SYNC_BEHAVIOR); // Set new SYNC_BEHAVIOR bits
    writeRegister(Register::AGC_CFG1, agcCfg1);
}

void CC1200::setAGCGainTable(GainTable table, uint8_t minGainIndex, uint8_t maxGainIndex)
{
    // Configure the AGC gain table
    uint8_t agcCfg0 = readRegister(Register::AGC_CFG0);
    
    // Set the AGC_GAIN_TABLE bit (bit 6)
    if(table == GainTable::HIGH_LINEARITY)
    {
        agcCfg0 |= (1 << AGC_CFG0_AGC_GAIN_TABLE);
    }
    else
    {
        agcCfg0 &= ~(1 << AGC_CFG0_AGC_GAIN_TABLE);
    }
    
    // Set the AGC_MIN_GAIN bits (bits 0-2)
    agcCfg0 &= ~(0b111 << AGC_CFG0_AGC_MIN_GAIN);
    agcCfg0 |= ((minGainIndex & 0x7) << AGC_CFG0_AGC_MIN_GAIN);
    
    // Set the AGC_MAX_GAIN bits (bits 3-5)
    agcCfg0 &= ~(0b111 << AGC_CFG0_AGC_MAX_GAIN);
    agcCfg0 |= ((maxGainIndex & 0x7) << AGC_CFG0_AGC_MAX_GAIN);
    
    writeRegister(Register::AGC_CFG0, agcCfg0);
}

void CC1200::setAGCHysteresis(uint8_t hysteresisCfg)
{
    // Configure the AGC hysteresis
    uint8_t agcCfg1 = readRegister(Register::AGC_CFG1);
    agcCfg1 &= ~(0b11 << AGC_CFG1_AGC_HYST_LEVEL); // Clear AGC_HYST_LEVEL bits (bits 6-7)
    agcCfg1 |= ((hysteresisCfg & 0x3) << AGC_CFG1_AGC_HYST_LEVEL); // Set new AGC_HYST_LEVEL bits
    writeRegister(Register::AGC_CFG1, agcCfg1);
}

void CC1200::setAGCSlewRate(uint8_t slewrateCfg)
{
    // Configure the AGC slew rate
    uint8_t agcCfg2 = readRegister(Register::AGC_CFG2);
    agcCfg2 &= ~(0b11 << AGC_CFG2_AGC_SLEWRATE_LIMIT); // Clear AGC_SLEWRATE_LIMIT bits (bits 3-4)
    agcCfg2 |= ((slewrateCfg & 0x3) << AGC_CFG2_AGC_SLEWRATE_LIMIT); // Set new AGC_SLEWRATE_LIMIT bits
    writeRegister(Register::AGC_CFG2, agcCfg2);
}

void CC1200::setAGCSettleWait(uint8_t settleWaitCfg)
{
    // Configure the AGC settle wait time
    uint8_t agcCfg2 = readRegister(Register::AGC_CFG2);
    agcCfg2 &= ~(0b111 << AGC_CFG2_AGC_SETTLE_WAIT); // Clear AGC_SETTLE_WAIT bits (bits 0-2)
    agcCfg2 |= ((settleWaitCfg & 0x7) << AGC_CFG2_AGC_SETTLE_WAIT); // Set new AGC_SETTLE_WAIT bits
    writeRegister(Register::AGC_CFG2, agcCfg2);
}

float CC1200::getRSSIRegister()
{
    // Read the RSSI registers
    uint8_t rssi0 = readRegister(ExtRegister::RSSI0);
    uint8_t rssi1 = readRegister(ExtRegister::RSSI1);
    
    // Combine the two bytes
    int16_t rssiValue = (static_cast<int16_t>(rssi1) << 8) | rssi0;
    
    // Convert to dBm
    // The RSSI value is in 0.5 dBm steps
    float rssiDbm = rssiValue / 2.0f;
    
    return rssiDbm;
}

void CC1200::setRSSIOffset(int8_t adjust)
{
    // Set the RSSI offset
    uint8_t agcCfg3 = readRegister(Register::AGC_CFG3);
    agcCfg3 &= ~(0xFF << AGC_CFG3_RSSI_ADJUST); // Clear RSSI_ADJUST bits (bits 0-7)
    agcCfg3 |= (adjust & 0xFF); // Set new RSSI_ADJUST bits
    writeRegister(Register::AGC_CFG3, agcCfg3);
}

uint8_t CC1200::getLQIRegister()
{
    // Read the LQI register
    return readRegister(ExtRegister::LQI_VAL);
}

void CC1200::setIFCfg(IFCfg value, bool enableIQIC)
{
    // Configure the IF settings
    uint8_t ifMixCfg = readRegister(ExtRegister::IF_MIX_CFG);
    ifMixCfg &= ~(0b11 << 0); // Clear IF_MODE bits (bits 0-1)
    ifMixCfg |= static_cast<uint8_t>(value); // Set new IF_MODE bits
    writeRegister(ExtRegister::IF_MIX_CFG, ifMixCfg);
    
    // Configure IQIC if requested
    if(enableIQIC)
    {
        uint8_t iqic = readRegister(Register::IQIC);
        iqic |= (1 << IQIC_IQIC_EN); // Set IQIC_EN bit
        writeRegister(Register::IQIC, iqic);
    }
    else
    {
        uint8_t iqic = readRegister(Register::IQIC);
        iqic &= ~(1 << IQIC_IQIC_EN); // Clear IQIC_EN bit
        writeRegister(Register::IQIC, iqic);
    }
}

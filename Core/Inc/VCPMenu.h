#ifndef __VCP_MENU_H
#define __VCP_MENU_H

#include "main.h"
#include "globals.h"
#include "usbd_cdc_if.h"
#include <cstdint>
#include <cstddef>
#include <cstring>

// Maximum buffer sizes
#define VCP_RX_BUFFER_SIZE 256
#define VCP_TX_BUFFER_SIZE 256
#define VCP_CMD_BUFFER_SIZE 64
#define VCP_MAX_ARGS 8

/**
 * @brief VCP Menu class to handle command parsing and menu display
 */
class VCPMenu {
public:
    /**
     * @brief Constructor for VCPMenu class
     * @param globals Pointer to Globals instance
     */
    VCPMenu(Globals* globals);
    
    /**
     * @brief Initialize the VCP Menu
     */
    void init();
    
    /**
     * @brief Process received data
     * @param data Pointer to data buffer
     * @param len Length of data
     */
    void processData(uint8_t* data, uint32_t len);
    
    /**
     * @brief Process commands
     */
    void processCommands();
    
    /**
     * @brief Display welcome message
     */
    void displayWelcome();
    
    /**
     * @brief Display help menu
     */
    void displayHelp();
    
    /**
     * @brief Display status
     */
    void displayStatus();
    
    /**
     * @brief Send data over VCP
     * @param data Pointer to data buffer
     * @param len Length of data
     */
    void sendData(const char* data, uint16_t len);
    
    /**
     * @brief Print formatted string to VCP
     * @param format Format string
     * @param ... Variable arguments
     */
    void printf(const char* format, ...);
    
    /**
     * @brief Handle received data callback
     * @param data Pointer to data buffer
     * @param len Length of data
     */
    void handleRxData(uint8_t* data, uint32_t len);

private:
    // Globals instance
    Globals* globals;
    
    // Receive buffer
    uint8_t rxBuffer[VCP_RX_BUFFER_SIZE];
    uint32_t rxBufferHead;
    uint32_t rxBufferTail;
    
    // Transmit buffer
    uint8_t txBuffer[VCP_TX_BUFFER_SIZE];
    
    // Command buffer
    char cmdBuffer[VCP_CMD_BUFFER_SIZE];
    uint32_t cmdBufferIndex;
    
    // Command line parsing
    void parseCommand(char* cmd);
    void executeCommand(const char* cmd, int argc, char* argv[]);
    
    // Command handlers
    void cmdHelp(int argc, char* argv[]);
    void cmdStatus(int argc, char* argv[]);
    void cmdTransmit(int argc, char* argv[]);
    void cmdReceive(int argc, char* argv[]);
    void cmdSetFreq(int argc, char* argv[]);
    void cmdSetRate(int argc, char* argv[]);
    void cmdReset(int argc, char* argv[]);
    
    // New radio command handlers
    void cmdRadioInit(int argc, char* argv[]);
    void cmdRadioLQI(int argc, char* argv[]);
    void cmdRadioRSSI(int argc, char* argv[]);
    void cmdRadioRX(int argc, char* argv[]);
    void cmdRadioStatus(int argc, char* argv[]);
    void cmdRadioStreamRX(int argc, char* argv[]);
    void cmdRadioStreamTX(int argc, char* argv[]);
    void cmdRadioTX(int argc, char* argv[]);
    void cmdRadioVersion(int argc, char* argv[]);
    void cmdRestart(int argc, char* argv[]);
    void cmdSysInfo(int argc, char* argv[]);
    void cmdRadioDebugOn(int argc, char* argv[]);
    void cmdRadioDebugOff(int argc, char* argv[]);
    
    // DMA command handlers
    void cmdRadioTXDMA(int argc, char* argv[]);
    void cmdRadioRXDMA(int argc, char* argv[]);
    void cmdRadioStreamTXDMA(int argc, char* argv[]);
    void cmdRadioStreamRXDMA(int argc, char* argv[]);
    
    // Continuous streaming command handlers
    void cmdRadioStreamStartTX(int argc, char* argv[]);
    void cmdRadioStreamStartRX(int argc, char* argv[]);
    void cmdRadioStreamStop(int argc, char* argv[]);
    void cmdRadioStreamStats(int argc, char* argv[]);

};

// Global callback function for USB CDC reception
extern "C" void VCP_RxCallback(uint8_t* data, uint32_t len);

#endif // __VCP_MENU_H

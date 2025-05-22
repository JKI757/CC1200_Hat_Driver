#include "VCPMenu.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cctype>

// Global VCPMenu instance for callback
static VCPMenu* g_vcpMenu = nullptr;

// Global callback function for USB CDC reception
extern "C" void VCP_RxCallback(uint8_t* data, uint32_t len) {
    if (g_vcpMenu != nullptr) {
        g_vcpMenu->handleRxData(data, len);
    }
}

/**
 * @brief Constructor for VCPMenu class
 */
VCPMenu::VCPMenu(Globals* globals)
    : m_globals(globals), m_rxBufferHead(0), m_rxBufferTail(0), m_cmdBufferIndex(0) {
    // Store the global instance for callback
    g_vcpMenu = this;
}

/**
 * @brief Initialize the VCP Menu
 */
void VCPMenu::init() {
    // Clear buffers
    memset(m_rxBuffer, 0, sizeof(m_rxBuffer));
    memset(m_txBuffer, 0, sizeof(m_txBuffer));
    memset(m_cmdBuffer, 0, sizeof(m_cmdBuffer));
    
    // Display welcome message
    displayWelcome();
}

/**
 * @brief Process received data
 */
void VCPMenu::processData(uint8_t* data, uint32_t len) {
    // Add data to receive buffer
    for (uint32_t i = 0; i < len; i++) {
        // Check if buffer is full
        if ((m_rxBufferHead + 1) % VCP_RX_BUFFER_SIZE == m_rxBufferTail) {
            // Buffer is full, discard data
            continue;
        }
        
        // Add data to buffer
        m_rxBuffer[m_rxBufferHead] = data[i];
        m_rxBufferHead = (m_rxBufferHead + 1) % VCP_RX_BUFFER_SIZE;
    }
}

/**
 * @brief Process commands
 */
void VCPMenu::processCommands() {
    // Process data in receive buffer
    while (m_rxBufferTail != m_rxBufferHead) {
        // Get next byte
        uint8_t byte = m_rxBuffer[m_rxBufferTail];
        m_rxBufferTail = (m_rxBufferTail + 1) % VCP_RX_BUFFER_SIZE;
        
        // Echo character back to terminal
        char echo[2] = {static_cast<char>(byte), 0};
        sendData(echo, 1);
        
        // Process byte
        if (byte == '\r' || byte == '\n') {
            // End of line, process command
            if (m_cmdBufferIndex > 0) {
                // Null-terminate command
                m_cmdBuffer[m_cmdBufferIndex] = '\0';
                
                // Parse and execute command
                parseCommand(m_cmdBuffer);
                
                // Clear command buffer
                m_cmdBufferIndex = 0;
                memset(m_cmdBuffer, 0, sizeof(m_cmdBuffer));
                
                // Print prompt
                sendData("\r\n> ", 4);
            }
        } else if (byte == '\b' || byte == 127) {
            // Backspace
            if (m_cmdBufferIndex > 0) {
                m_cmdBufferIndex--;
                m_cmdBuffer[m_cmdBufferIndex] = '\0';
                
                // Echo backspace sequence
                sendData("\b \b", 3);
            }
        } else if (byte >= 32 && byte < 127) {
            // Printable character
            if (m_cmdBufferIndex < VCP_CMD_BUFFER_SIZE - 1) {
                m_cmdBuffer[m_cmdBufferIndex++] = static_cast<char>(byte);
            }
        }
    }
}

/**
 * @brief Parse command
 */
void VCPMenu::parseCommand(char* cmd) {
    // Tokenize command
    char* argv[VCP_MAX_ARGS];
    int argc = 0;
    
    char* token = strtok(cmd, " \t");
    while (token != nullptr && argc < VCP_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(nullptr, " \t");
    }
    
    // Execute command
    if (argc > 0) {
        executeCommand(cmd, argc, argv);
    }
}

/**
 * @brief Execute command
 */
void VCPMenu::executeCommand(const char* cmd, int argc, char* argv[]) {
    // Check command
    if (strcmp(argv[0], "help") == 0) {
        cmdHelp(argc, argv);
    } else if (strcmp(argv[0], "status") == 0) {
        cmdStatus(argc, argv);
    } else if (strcmp(argv[0], "tx") == 0 || strcmp(argv[0], "transmit") == 0) {
        cmdTransmit(argc, argv);
    } else if (strcmp(argv[0], "rx") == 0 || strcmp(argv[0], "receive") == 0) {
        cmdReceive(argc, argv);
    } else if (strcmp(argv[0], "freq") == 0) {
        cmdSetFreq(argc, argv);
    } else if (strcmp(argv[0], "rate") == 0) {
        cmdSetRate(argc, argv);
    } else if (strcmp(argv[0], "reset") == 0) {
        cmdReset(argc, argv);
    } else {
        // Unknown command
        printf("Unknown command: %s\r\n", argv[0]);
        printf("Type 'help' for a list of commands\r\n");
    }
}

/**
 * @brief Display welcome message
 */
void VCPMenu::displayWelcome() {
    printf("\r\n\r\n");
    printf("*************************************************\r\n");
    printf("*                                               *\r\n");
    printf("*           CC1200 Radio HAT Terminal           *\r\n");
    printf("*                                               *\r\n");
    printf("*************************************************\r\n");
    printf("\r\n");
    printf("Type 'help' for a list of commands\r\n");
    printf("\r\n> ");
}

/**
 * @brief Display help menu
 */
void VCPMenu::displayHelp() {
    printf("\r\nAvailable commands:\r\n");
    printf("  help                 - Display this help message\r\n");
    printf("  status               - Display radio status\r\n");
    printf("  tx <data>            - Transmit data\r\n");
    printf("  rx                   - Start receiving data\r\n");
    printf("  freq <frequency>     - Set radio frequency in Hz\r\n");
    printf("  rate <symbol_rate>   - Set symbol rate in Hz\r\n");
    printf("  reset                - Reset the radio\r\n");
    printf("\r\n");
}

/**
 * @brief Display status
 */
void VCPMenu::displayStatus() {
    Radio* radio = m_globals->getRadio();
    if (radio == nullptr) {
        printf("Radio not initialized\r\n");
        return;
    }
    
    CC1200* cc1200 = radio->getCC1200();
    
    printf("\r\nRadio Status:\r\n");
    printf("  Initialized: Yes\r\n");
    printf("  RSSI: %.1f dBm\r\n", cc1200->getRSSIRegister());
    printf("  LQI: %u\r\n", cc1200->getLQIRegister());
    printf("\r\n");
}

/**
 * @brief Send data over VCP
 */
void VCPMenu::sendData(const char* data, uint16_t len) {
    // Send data over USB CDC
    CDC_Transmit_FS((uint8_t*)data, len);
}

/**
 * @brief Print formatted string to VCP
 */
void VCPMenu::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Format string
    vsnprintf((char*)m_txBuffer, VCP_TX_BUFFER_SIZE, format, args);
    
    va_end(args);
    
    // Send data
    sendData((char*)m_txBuffer, strlen((char*)m_txBuffer));
}

/**
 * @brief Handle received data callback
 */
void VCPMenu::handleRxData(uint8_t* data, uint32_t len) {
    processData(data, len);
}

/**
 * @brief Command handler: help
 */
void VCPMenu::cmdHelp(int argc, char* argv[]) {
    displayHelp();
}

/**
 * @brief Command handler: status
 */
void VCPMenu::cmdStatus(int argc, char* argv[]) {
    displayStatus();
}

/**
 * @brief Command handler: transmit
 */
void VCPMenu::cmdTransmit(int argc, char* argv[]) {
    Radio* radio = m_globals->getRadio();
    if (radio == nullptr) {
        printf("Radio not initialized\r\n");
        return;
    }
    
    if (argc < 2) {
        printf("Usage: tx <data>\r\n");
        return;
    }
    
    // Reconstruct data from arguments
    char txData[VCP_TX_BUFFER_SIZE] = {0};
    size_t txLen = 0;
    
    for (int i = 1; i < argc; i++) {
        size_t argLen = strlen(argv[i]);
        
        // Check if buffer has enough space
        if (txLen + argLen + 1 >= VCP_TX_BUFFER_SIZE) {
            printf("Data too long\r\n");
            return;
        }
        
        // Add argument to buffer
        if (i > 1) {
            txData[txLen++] = ' ';
        }
        
        strcpy(txData + txLen, argv[i]);
        txLen += argLen;
    }
    
    // Transmit data
    printf("Transmitting: %s\r\n", txData);
    
    if (radio->transmit(txData, txLen)) {
        printf("Transmission successful\r\n");
        
        // Turn on TX LED for visual feedback
        m_globals->setTxLED(1);
        
        // Turn off TX LED after a short delay
        HAL_Delay(100);
        m_globals->setTxLED(0);
    } else {
        printf("Transmission failed\r\n");
    }
}

/**
 * @brief Command handler: receive
 */
void VCPMenu::cmdReceive(int argc, char* argv[]) {
    Radio* radio = m_globals->getRadio();
    if (radio == nullptr) {
        printf("Radio not initialized\r\n");
        return;
    }
    
    printf("Starting continuous receive mode...\r\n");
    printf("Press any key to stop\r\n");
    
    // Start continuous receive mode
    radio->startContinuousReceive();
    
    // Turn on RX LED for visual feedback
    m_globals->setRxLED(1);
    
    // Receive buffer
    char rxBuffer[VCP_RX_BUFFER_SIZE];
    
    // Receive loop
    bool receiving = true;
    while (receiving) {
        // Check for received data
        size_t rxLen = radio->receive(rxBuffer, sizeof(rxBuffer) - 1);
        
        if (rxLen > 0) {
            // Null-terminate received data
            rxBuffer[rxLen] = '\0';
            
            // Display received data
            printf("Received: %s\r\n", rxBuffer);
        }
        
        // Check if user pressed a key to stop
        if (m_rxBufferTail != m_rxBufferHead) {
            receiving = false;
        }
        
        // Give other tasks a chance to run
        HAL_Delay(10);
    }
    
    // Stop continuous receive mode
    radio->stopContinuousReceive();
    
    // Turn off RX LED
    m_globals->setRxLED(0);
    
    printf("Receive mode stopped\r\n");
    
    // Clear any pending input
    m_rxBufferHead = m_rxBufferTail;
}

/**
 * @brief Command handler: set frequency
 */
void VCPMenu::cmdSetFreq(int argc, char* argv[]) {
    Radio* radio = m_globals->getRadio();
    if (radio == nullptr) {
        printf("Radio not initialized\r\n");
        return;
    }
    
    if (argc < 2) {
        printf("Usage: freq <frequency>\r\n");
        return;
    }
    
    // Parse frequency
    float freq = strtof(argv[1], nullptr);
    
    if (freq < 1e6) {
        // Assume MHz if value is small
        freq *= 1e6;
    }
    
    // Set frequency
    if (radio->setFrequency(freq)) {
        printf("Frequency set to %.3f MHz\r\n", freq / 1e6);
    } else {
        printf("Failed to set frequency\r\n");
    }
}

/**
 * @brief Command handler: set symbol rate
 */
void VCPMenu::cmdSetRate(int argc, char* argv[]) {
    Radio* radio = m_globals->getRadio();
    if (radio == nullptr) {
        printf("Radio not initialized\r\n");
        return;
    }
    
    if (argc < 2) {
        printf("Usage: rate <symbol_rate>\r\n");
        return;
    }
    
    // Parse symbol rate
    float rate = strtof(argv[1], nullptr);
    
    if (rate < 1e3) {
        // Assume kBaud if value is small
        rate *= 1e3;
    }
    
    // Set symbol rate
    if (radio->setSymbolRate(rate)) {
        printf("Symbol rate set to %.3f kBaud\r\n", rate / 1e3);
    } else {
        printf("Failed to set symbol rate\r\n");
    }
}

/**
 * @brief Command handler: reset
 */
void VCPMenu::cmdReset(int argc, char* argv[]) {
    printf("Resetting radio...\r\n");
    
    // Reset the radio
    m_globals->resetCC1200();
    
    // Re-initialize the radio
    if (m_globals->initRadio()) {
        printf("Radio reset and initialized successfully\r\n");
    } else {
        printf("Failed to initialize radio after reset\r\n");
    }
}

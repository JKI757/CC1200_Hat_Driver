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
    : globals(globals), rxBufferHead(0), rxBufferTail(0), cmdBufferIndex(0) {
    // Store the global instance for callback
    g_vcpMenu = this;
}

/**
 * @brief Initialize the VCP Menu
 */
void VCPMenu::init() {
    // Clear buffers
    memset(this->rxBuffer, 0, sizeof(this->rxBuffer));
    memset(this->txBuffer, 0, sizeof(this->txBuffer));
    memset(this->cmdBuffer, 0, sizeof(this->cmdBuffer));
    
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
        if ((this->rxBufferHead + 1) % VCP_RX_BUFFER_SIZE == this->rxBufferTail) {
            // Buffer is full, discard data
            continue;
        }
        
        // Add data to buffer
        this->rxBuffer[this->rxBufferHead] = data[i];
        this->rxBufferHead = (this->rxBufferHead + 1) % VCP_RX_BUFFER_SIZE;
    }
}

/**
 * @brief Process commands
 */
void VCPMenu::processCommands() {
    // Process data in receive buffer
    while (this->rxBufferTail != this->rxBufferHead) {
        // Get next byte
        uint8_t byte = this->rxBuffer[this->rxBufferTail];
        this->rxBufferTail = (this->rxBufferTail + 1) % VCP_RX_BUFFER_SIZE;
        
        // Echo character back to terminal
        char echo[2] = {static_cast<char>(byte), 0};
        sendData(echo, 1);
        
        // Process byte
        if (byte == '\r' || byte == '\n') {
            // End of line, process command
            if (this->cmdBufferIndex > 0) {
                // Null-terminate command
                this->cmdBuffer[this->cmdBufferIndex] = '\0';
                
                // Parse and execute command
                parseCommand(this->cmdBuffer);
                
                // Clear command buffer
                this->cmdBufferIndex = 0;
                memset(this->cmdBuffer, 0, sizeof(this->cmdBuffer));
                
                // Print prompt
                sendData("\r\n> ", 4);
            }
        } else if (byte == '\b' || byte == 127) {
            // Backspace
            if (this->cmdBufferIndex > 0) {
                this->cmdBufferIndex--;
                this->cmdBuffer[this->cmdBufferIndex] = '\0';
                
                // Echo backspace sequence
                sendData("\b \b", 3);
            }
        } else if (byte >= 32 && byte < 127) {
            // Printable character
            if (this->cmdBufferIndex < VCP_CMD_BUFFER_SIZE - 1) {
                this->cmdBuffer[this->cmdBufferIndex++] = static_cast<char>(byte);
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
    Radio* radio = this->globals->getRadio();
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
    // Maximum number of retries
    const uint8_t maxRetries = 10;
    uint8_t retries = 0;
    uint8_t result = USBD_BUSY;
    
    // Try to send data with retries if busy
    while (retries < maxRetries) {
        result = CDC_Transmit_FS((uint8_t*)data, len);
        
        if (result == USBD_OK) {
            break; // Transmission started successfully
        } else if (result == USBD_BUSY) {
            // If busy, wait a bit and retry
            HAL_Delay(1); // Short delay before retry
            retries++;
        } else {
            // Other error, exit
            break;
        }
    }
    
    // Small delay after successful transmission to ensure it completes
    // This helps with terminal display issues
    if (result == USBD_OK) {
        HAL_Delay(1);
    }
}

/**
 * @brief Print formatted string to VCP
 */
void VCPMenu::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Format string
    vsnprintf((char*)this->txBuffer, VCP_TX_BUFFER_SIZE, format, args);
    
    va_end(args);
    
    // Get the length of the formatted string
    uint16_t len = strlen((char*)this->txBuffer);
    
    // For longer messages, break them into smaller chunks to ensure reliable transmission
    const uint16_t chunkSize = 64; // Smaller chunks are more reliable
    
    if (len <= chunkSize) {
        // Send short message in one go
        sendData((char*)this->txBuffer, len);
    } else {
        // Send longer message in chunks
        for (uint16_t i = 0; i < len; i += chunkSize) {
            uint16_t remaining = len - i;
            uint16_t sendSize = (remaining > chunkSize) ? chunkSize : remaining;
            sendData((char*)&this->txBuffer[i], sendSize);
        }
    }
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
    Radio* radio = this->globals->getRadio();
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
        this->globals->setTxLED(1);
        
        // Turn off TX LED after a short delay
        HAL_Delay(100);
        this->globals->setTxLED(0);
    } else {
        printf("Transmission failed\r\n");
    }
}

/**
 * @brief Command handler: receive
 */
void VCPMenu::cmdReceive(int argc, char* argv[]) {
    Radio* radio = this->globals->getRadio();
    if (radio == nullptr) {
        printf("Radio not initialized\r\n");
        return;
    }
    
    printf("Starting continuous receive mode...\r\n");
    printf("Press any key to stop\r\n");
    
    // Start continuous receive mode
    radio->startContinuousReceive();
    
    // Turn on RX LED for visual feedback
    this->globals->setRxLED(1);
    
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
        if (this->rxBufferTail != this->rxBufferHead) {
            receiving = false;
        }
        
        // Give other tasks a chance to run
        HAL_Delay(10);
    }
    
    // Stop continuous receive mode
    radio->stopContinuousReceive();
    
    // Turn off RX LED
    this->globals->setRxLED(0);
    
    printf("Receive mode stopped\r\n");
    
    // Clear any pending input
    this->rxBufferHead = this->rxBufferTail;
}

/**
 * @brief Command handler: set frequency
 */
void VCPMenu::cmdSetFreq(int argc, char* argv[]) {
    Radio* radio = this->globals->getRadio();
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
    Radio* radio = this->globals->getRadio();
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
    this->globals->resetCC1200();
    
    // Re-initialize the radio
    if (this->globals->initRadio()) {
        printf("Radio reset and initialized successfully\r\n");
    } else {
        printf("Failed to initialize radio after reset\r\n");
    }
}

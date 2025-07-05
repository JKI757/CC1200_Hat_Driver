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
    } 
    // New radio commands
    else if (strcmp(argv[0], "radio_init") == 0) {
        cmdRadioInit(argc, argv);
    } else if (strcmp(argv[0], "radio_lqi") == 0) {
        cmdRadioLQI(argc, argv);
    } else if (strcmp(argv[0], "radio_rssi") == 0) {
        cmdRadioRSSI(argc, argv);
    } else if (strcmp(argv[0], "radio_rx") == 0) {
        cmdRadioRX(argc, argv);
    } else if (strcmp(argv[0], "radio_status") == 0) {
        cmdRadioStatus(argc, argv);
    } else if (strcmp(argv[0], "radio_stream_rx") == 0) {
        cmdRadioStreamRX(argc, argv);
    } else if (strcmp(argv[0], "radio_stream_tx") == 0) {
        cmdRadioStreamTX(argc, argv);
    } else if (strcmp(argv[0], "radio_tx") == 0) {
        cmdRadioTX(argc, argv);
    } else if (strcmp(argv[0], "radio_version") == 0) {
        cmdRadioVersion(argc, argv);
    } else if (strcmp(argv[0], "radio_debug_on") == 0) {
        cmdRadioDebugOn(argc, argv);
    } else if (strcmp(argv[0], "radio_debug_off") == 0) {
        cmdRadioDebugOff(argc, argv);
    } else if (strcmp(argv[0], "restart") == 0) {
        cmdRestart(argc, argv);
    } else if (strcmp(argv[0], "sysinfo") == 0) {
        cmdSysInfo(argc, argv);
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
    
    printf("New radio commands:\r\n");
    printf("  radio_init           - Initialize radio with default settings\r\n");
    printf("  radio_lqi            - Get Link Quality Indicator\r\n");
    printf("  radio_rssi           - Get current RSSI value\r\n");
    printf("  radio_rx <timeout_ms> - Start receiving with timeout\r\n");
    printf("  radio_status         - Get CC1200 radio status\r\n");
    printf("  radio_stream_rx <bytes> <timeout_ms> - Start receiving stream\r\n");
    printf("  radio_stream_tx <hex_data> - Start transmitting stream\r\n");
    printf("  radio_tx <hex_data>  - Transmit data as hex\r\n");
    printf("  radio_version        - Get CC1200 part version\r\n");
    printf("  radio_debug_on       - Enable SPI debug output to UART\r\n");
    printf("  radio_debug_off      - Disable SPI debug output\r\n");
    printf("\r\n");
    
    printf("System commands:\r\n");
    printf("  restart              - Restart the system\r\n");
    printf("  sysinfo              - Display system information\r\n");
    printf("\r\n");
}

/**
 * @brief Display status
 */
void VCPMenu::displayStatus() {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
        return;
    }
    
    
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
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
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
    
    if (cc1200->enqueuePacket(txData, txLen)) {
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
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
        return;
    }
    
    printf("Starting continuous receive mode...\r\n");
    printf("Press any key to stop\r\n");
    
    // Start continuous receive mode
    cc1200->sendCommand(CC1200::Command::RX);
    
    // Turn on RX LED for visual feedback
    this->globals->setRxLED(1);
    
    // Receive buffer
    char rxBuffer[VCP_RX_BUFFER_SIZE];
    
    // Receive loop
    bool receiving = true;
    while (receiving) {
        // Check for received data
        size_t rxLen = cc1200->receivePacket(rxBuffer, sizeof(rxBuffer) - 1);
        
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
    cc1200->sendCommand(CC1200::Command::IDLE);
    
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
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
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
    // Note: setRadioFrequency requires a band parameter - using ISM band for 915MHz
    cc1200->setRadioFrequency(CC1200::Band::BAND_820_960MHz, freq);
    printf("Frequency set to %.0f Hz\r\n", freq);
    // Frequency set successfully
}

/**
 * @brief Command handler: set symbol rate
 */
void VCPMenu::cmdSetRate(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
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
    cc1200->setSymbolRate(rate); // Returns void, not bool
    printf("Symbol rate set to %.0f Hz\r\n", rate);
    // Symbol rate set successfully
}

/**
 * @brief Command handler: reset
 */
void VCPMenu::cmdReset(int argc, char* argv[]) {
    printf("Resetting radio...\r\n");
    
    // Reset the radio
    this->globals->resetCC1200();
    
    // Re-initialize the radio
    if (this->globals->initCC1200()) {
        printf("Radio reset and initialized successfully\r\n");
    } else {
        printf("Failed to initialize radio after reset\r\n");
    }
}

/**
 * @brief Command handler: radio_init - Initialize radio with default settings
 */
void VCPMenu::cmdRadioInit(int argc, char* argv[]) {
    // Use simplified CC1200 initialization
    if (this->globals->getCC1200() == nullptr) {
        printf("Error: CC1200 not available\r\n");
        return;
    }
    
    printf("Initializing radio with default settings...\r\n");
    
    // Reset the radio hardware first
    this->globals->resetCC1200();
    
    // Initialize the radio
    if (this->globals->initCC1200()) {
        printf("Radio initialized successfully\r\n");
    } else {
        printf("Failed to initialize radio\r\n");
    }
}

/**
 * @brief Command handler: enable radio SPI debug output
 */
void VCPMenu::cmdRadioDebugOn(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
        return;
    }
    
    cc1200->enableDebug();
    
    // Print debug status to confirm it's enabled
    printf("Radio SPI debug output enabled (debug flag: %s)\r\n", 
           cc1200->isDebugEnabled() ? "true" : "false");
    
    // Test UART output directly
    if (globals->getUART() != nullptr) {
        const char* testMsg = "UART test message from radio_debug_on command\r\n";
        HAL_UART_Transmit(globals->getUART(), (uint8_t*)testMsg, strlen(testMsg), HAL_MAX_DELAY);
        printf("Test message sent to UART\r\n");
    } else {
        printf("UART handle is null!\r\n");
    }
    
    // Force a simple SPI transaction to generate debug output
    uint8_t partNumber = cc1200->readRegister(CC1200::ExtRegister::PARTNUMBER);
    printf("Triggered SPI read of part number: 0x%02X\r\n", partNumber);
}

/**
 * @brief Command handler: disable radio SPI debug output
 */
void VCPMenu::cmdRadioDebugOff(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("CC1200 not initialized\r\n");
        return;
    }
    
    cc1200->disableDebug();
    
    printf("Radio SPI debug output disabled\r\n");
}

/**
 * @brief Command handler: radio_rssi - Get current RSSI value
 */
void VCPMenu::cmdRadioRSSI(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    float rssi = cc1200->getRSSIRegister();
    
    printf("RSSI: %.1f dBm\r\n", rssi);
}

/**
 * @brief Command handler: radio_lqi - Get CC1200 Link Quality Indicator value
 */
void VCPMenu::cmdRadioLQI(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    uint8_t lqi = cc1200->getLQIRegister();
    
    printf("LQI: %u\r\n", lqi);
}

/**
 * @brief Command handler: radio_rx - Start receiving (Usage: radio_rx <timeout_ms>)
 */
void VCPMenu::cmdRadioRX(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    // Check arguments
    if (argc < 2) {
        printf("Usage: radio_rx <timeout_ms>\r\n");
        return;
    }
    
    // Parse timeout
    uint32_t timeout = strtoul(argv[1], NULL, 10);
    if (timeout == 0) {
        printf("Invalid timeout value\r\n");
        return;
    }
    
    printf("Starting receive mode for %lu ms...\r\n", timeout);
    
    // Start receiving
    cc1200->sendCommand(CC1200::Command::RX);
    
    // Turn on RX LED for visual feedback
    this->globals->setRxLED(1);
    
    // Receive buffer
    char rxBuffer[VCP_RX_BUFFER_SIZE];
    uint32_t startTime = HAL_GetTick();
    bool received = false;
    
    // Receive loop
    while (HAL_GetTick() - startTime < timeout) {
        // Check for received data
        size_t rxLen = cc1200->receivePacket(rxBuffer, sizeof(rxBuffer) - 1);
        
        if (rxLen > 0) {
            // Null-terminate received data
            rxBuffer[rxLen] = '\0';
            
            // Display received data as hex
            printf("Received %u bytes: ", (unsigned int)rxLen);
            for (size_t i = 0; i < rxLen; i++) {
                printf("%02X ", (uint8_t)rxBuffer[i]);
            }
            printf("\r\n");
            
            received = true;
            break;
        }
        
        // Give other tasks a chance to run
        HAL_Delay(10);
    }
    
    // Stop receiving
    cc1200->sendCommand(CC1200::Command::IDLE);
    
    // Turn off RX LED
    this->globals->setRxLED(0);
    
    if (!received) {
        printf("No data received within timeout period\r\n");
    }
}

/**
 * @brief Command handler: radio_status - Get CC1200 radio status
 */
void VCPMenu::cmdRadioStatus(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    
    printf("\r\nRadio Status:\r\n");
    printf("  RSSI: %.1f dBm\r\n", cc1200->getRSSIRegister());
    printf("  LQI: %u\r\n", cc1200->getLQIRegister());
    
    // Update the radio state to get current status
    cc1200->updateState();
    
    // Display the current state
    const char* stateStr = "Unknown";
    // Read the MARCSTATE register to get the current state
    uint8_t stateVal = cc1200->readRegister(CC1200::ExtRegister::MARCSTATE);
    CC1200::State currentState = static_cast<CC1200::State>(stateVal & 0x1F); // Mask to get the state bits
    
    switch (currentState) {
        case CC1200::State::IDLE:
            stateStr = "IDLE";
            break;
        case CC1200::State::RX:
            stateStr = "RX";
            break;
        case CC1200::State::TX:
            stateStr = "TX";
            break;
        case CC1200::State::FAST_ON:
            stateStr = "FAST_ON";
            break;
        case CC1200::State::CALIBRATE:
            stateStr = "CALIBRATE";
            break;
        case CC1200::State::SETTLING:
            stateStr = "SETTLING";
            break;
        case CC1200::State::RX_FIFO_ERROR:
            stateStr = "RX_FIFO_ERROR";
            break;
        case CC1200::State::TX_FIFO_ERROR:
            stateStr = "TX_FIFO_ERROR";
            break;
    }
    printf("  State: %s\r\n", stateStr);
    
    // Display FIFO status
    printf("  TX FIFO: %u bytes\r\n", (unsigned int)cc1200->getTXFIFOLen());
    printf("  RX FIFO: %u bytes\r\n", (unsigned int)cc1200->getRXFIFOLen());
    printf("\r\n");
}

/**
 * @brief Command handler: radio_stream_rx - Start receiving stream (Usage: radio_stream_rx <num_bytes> <timeout_ms>)
 */
void VCPMenu::cmdRadioStreamRX(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    // Check arguments
    if (argc < 3) {
        printf("Usage: radio_stream_rx <num_bytes> <timeout_ms>\r\n");
        return;
    }
    
    // Parse arguments
    size_t numBytes = strtoul(argv[1], NULL, 10);
    uint32_t timeout = strtoul(argv[2], NULL, 10);
    
    if (numBytes == 0 || numBytes > VCP_RX_BUFFER_SIZE - 1) {
        printf("Invalid number of bytes (1-%d)\r\n", VCP_RX_BUFFER_SIZE - 1);
        return;
    }
    
    if (timeout == 0) {
        printf("Invalid timeout value\r\n");
        return;
    }
    
    printf("Starting stream receive mode for %u bytes with %lu ms timeout...\r\n", (unsigned int)numBytes, timeout);
    
    // Start receiving
    cc1200->sendCommand(CC1200::Command::RX);
    
    // Turn on RX LED for visual feedback
    this->globals->setRxLED(1);
    
    // Receive buffer
    char rxBuffer[VCP_RX_BUFFER_SIZE];
    uint32_t startTime = HAL_GetTick();
    size_t totalReceived = 0;
    
    // Receive loop
    while (totalReceived < numBytes && (HAL_GetTick() - startTime < timeout)) {
        // Get CC1200 instance
            
        // Read stream data directly
        size_t bytesToRead = numBytes - totalReceived;
        if (bytesToRead > VCP_RX_BUFFER_SIZE - 1) {
            bytesToRead = VCP_RX_BUFFER_SIZE - 1;
        }
        
        size_t rxLen = cc1200->readStream(rxBuffer, bytesToRead);
        
        if (rxLen > 0) {
            // Display received data as hex
            printf("Received %u bytes: ", (unsigned int)rxLen);
            for (size_t i = 0; i < rxLen; i++) {
                printf("%02X ", (uint8_t)rxBuffer[i]);
            }
            printf("\r\n");
            
            totalReceived += rxLen;
        }
        
        // Give other tasks a chance to run
        HAL_Delay(10);
    }
    
    // Stop receiving
    cc1200->sendCommand(CC1200::Command::IDLE);
    
    // Turn off RX LED
    this->globals->setRxLED(0);
    
    printf("Stream receive complete: %u of %u bytes received\r\n", (unsigned int)totalReceived, (unsigned int)numBytes);
}

/**
 * @brief Command handler: radio_stream_tx - Start transmitting stream (Usage: radio_stream_tx <hex_data>)
 */
void VCPMenu::cmdRadioStreamTX(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    // Check arguments
    if (argc < 2) {
        printf("Usage: radio_stream_tx <hex_data>\r\n");
        return;
    }
    
    // Parse hex data
    char txBuffer[VCP_TX_BUFFER_SIZE];
    size_t txLen = 0;
    
    const char* hexData = argv[1];
    size_t hexLen = strlen(hexData);
    
    // Convert hex string to binary
    for (size_t i = 0; i < hexLen; i += 2) {
        // Check if we have a complete byte (2 hex chars)
        if (i + 1 >= hexLen) {
            printf("Error: Incomplete hex byte\r\n");
            return;
        }
        
        // Parse the hex byte
        char byteStr[3] = {hexData[i], hexData[i+1], '\0'};
        char* endPtr;
        uint8_t byte = (uint8_t)strtoul(byteStr, &endPtr, 16);
        
        // Check for parsing errors
        if (*endPtr != '\0') {
            printf("Error: Invalid hex character at position %zu\r\n", i);
            return;
        }
        
        // Add the byte to the buffer
        txBuffer[txLen++] = (char)byte;
        
        // Check buffer overflow
        if (txLen >= VCP_TX_BUFFER_SIZE) {
            printf("Error: Data too long\r\n");
            return;
        }
    }
    
    printf("Transmitting %u bytes as stream...\r\n", (unsigned int)txLen);
    
    // Start transmitting
    cc1200->sendCommand(CC1200::Command::TX);
    
    // Turn on TX LED for visual feedback
    this->globals->setTxLED(1);
    
    // Get CC1200 instance
    
    // Write stream data directly
    size_t bytesWritten = cc1200->writeStream(txBuffer, txLen);
    
    // Wait a bit for transmission to complete
    HAL_Delay(100);
    
    // Stop transmitting
    cc1200->sendCommand(CC1200::Command::IDLE);
    
    // Turn off TX LED
    this->globals->setTxLED(0);
    
    printf("Stream transmission complete: %u of %u bytes sent\r\n", (unsigned int)bytesWritten, (unsigned int)txLen);
}

/**
 * @brief Command handler: radio_tx - Transmit data (Usage: radio_tx <hex_data>)
 */
void VCPMenu::cmdRadioTX(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    // Check arguments
    if (argc < 2) {
        printf("Usage: radio_tx <hex_data>\r\n");
        return;
    }
    
    // Parse hex data
    char txBuffer[VCP_TX_BUFFER_SIZE];
    size_t txLen = 0;
    
    const char* hexData = argv[1];
    size_t hexLen = strlen(hexData);
    
    // Convert hex string to binary
    for (size_t i = 0; i < hexLen; i += 2) {
        // Check if we have a complete byte (2 hex chars)
        if (i + 1 >= hexLen) {
            printf("Error: Incomplete hex byte\r\n");
            return;
        }
        
        // Parse the hex byte
        char byteStr[3] = {hexData[i], hexData[i+1], '\0'};
        char* endPtr;
        uint8_t byte = (uint8_t)strtoul(byteStr, &endPtr, 16);
        
        // Check for parsing errors
        if (*endPtr != '\0') {
            printf("Error: Invalid hex character at position %zu\r\n", i);
            return;
        }
        
        // Add the byte to the buffer
        txBuffer[txLen++] = (char)byte;
        
        // Check buffer overflow
        if (txLen >= VCP_TX_BUFFER_SIZE) {
            printf("Error: Data too long\r\n");
            return;
        }
    }
    
    printf("Transmitting %u bytes...\r\n", (unsigned int)txLen);
    
    // Transmit the data
    if (cc1200->enqueuePacket(reinterpret_cast<const char*>(txBuffer), txLen)) {
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
 * @brief Command handler: radio_version - Get CC1200 part version
 */
void VCPMenu::cmdRadioVersion(int argc, char* argv[]) {
    CC1200* cc1200 = this->globals->getCC1200();
    if (cc1200 == nullptr) {
        printf("Error: CC1200 not initialized\r\n");
        return;
    }
    
    
    // Read part number and version
    uint8_t partNumber = cc1200->readRegister(CC1200::ExtRegister::PARTNUMBER);
    uint8_t partVersion = cc1200->readRegister(CC1200::ExtRegister::PARTVERSION);
    
    printf("CC1200 Part Number: 0x%02X\r\n", partNumber);
    printf("CC1200 Part Version: 0x%02X\r\n", partVersion);
}

/**
 * @brief Command handler: restart - Restart the STM32
 */
void VCPMenu::cmdRestart(int argc, char* argv[]) {
    printf("Restarting system...\r\n");
    
    // Small delay to allow message to be sent
    HAL_Delay(100);
    
    // Reset the system
    NVIC_SystemReset();
}

/**
 * @brief Command handler: sysinfo - Display system information
 */
void VCPMenu::cmdSysInfo(int argc, char* argv[]) {
    printf("\r\nSystem Information:\r\n");
    printf("  MCU: STM32F4xx\r\n");
    printf("  Radio: CC1200\r\n");
    printf("  HAL Version: %u.%u.%u\r\n", HAL_GetHalVersion() >> 24, 
           (HAL_GetHalVersion() >> 16) & 0xFF, 
           (HAL_GetHalVersion() >> 8) & 0xFF);
    printf("  System Clock: %lu MHz\r\n", HAL_RCC_GetSysClockFreq() / 1000000);
    printf("  HCLK: %lu MHz\r\n", HAL_RCC_GetHCLKFreq() / 1000000);
    printf("  Uptime: %lu ms\r\n", HAL_GetTick());
    printf("\r\n");
}

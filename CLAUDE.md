# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is an STM32 project using the STM32CubeIDE build system with GNU ARM toolchain:

- **Build command**: Use STM32CubeIDE's build system or manually run `make` in the Debug/ directory
- **Target**: STM32F411CEU6 microcontroller
- **Toolchain**: GNU Tools for STM32 (arm-none-eabi-gcc/g++)
- **Output**: cc1200-hat-STM32Firmware.elf binary

The project uses auto-generated makefiles and should be built through STM32CubeIDE or by running `make` in the Debug directory.

## Architecture Overview

This firmware implements a CC1200 radio HAT controller with the following key components:

### Core Architecture
- **Radio Class** (`Core/Inc/Radio.h`, `Core/Src/Radio.cpp`): High-level interface for CC1200 radio operations including 4FSK modulation, infinite read/write modes, and DMA support
- **CC1200 HAL** (`Core/Inc/CC1200_HAL.h`, `Core/Src/CC1200_HAL.cpp`): Low-level driver for CC1200 radio chip (adapted from USC Rocket Propulsion Lab)
- **VCP Menu** (`Core/Inc/VCPMenu.h`, `Core/Src/VCPMenu.cpp`): USB CDC command-line interface for radio control
- **Globals** (`Core/Inc/globals.h`, `Core/Src/globals.cpp`): Global objects and utilities

### FreeRTOS Task Structure
The firmware uses FreeRTOS with the following tasks (defined in `Core/Src/freertos.cpp`):
- **defaultTask**: Main system task
- **myTask02-04**: Radio transmission, reception, and processing tasks
- **VCP Menu Task**: USB command processing

### Hardware Interfaces
- **SPI1**: Communication with CC1200 radio chip
- **USB CDC**: Virtual COM port for command interface
- **GPIO**: CC1200 control pins (reset, GPIO0-3) and LED indicators (service, TX, RX)
- **DMA**: Used for SPI and UART data transfers

## Development Commands

### Building
```bash
# Build the project (run from Debug directory)
make

# Clean build artifacts
make clean

# View binary size
arm-none-eabi-size cc1200-hat-STM32Firmware.elf
```

### Hardware Configuration
- **Target MCU**: STM32F411CEU6
- **Radio**: CC1200 (915 MHz ISM band)
- **Modulation**: 4FSK with 50 kBaud symbol rate
- **USB**: CDC class for VCP interface

## Key Code Patterns

### Radio Operations
The Radio class provides a high-level interface with methods like:
- `init()`: Initialize radio hardware
- `configure4FSK()`: Configure 4FSK modulation
- `transmit()`: Send data
- `receive()`: Receive data
- `setFrequency()`: Set operating frequency
- `setSymbolRate()`: Set symbol rate

### VCP Command Interface
USB commands are processed through the VCPMenu class:
- `help`: Display available commands
- `status`: Show radio status
- `tx <data>`: Transmit data
- `rx`: Start receiving
- `freq <hz>`: Set frequency
- `rate <hz>`: Set symbol rate

### Thread Safety
The firmware uses FreeRTOS with proper synchronization:
- Binary semaphores for task coordination
- Message queues for inter-task communication
- Thread-safe newlib implementation in Core/ThreadSafe/

## Important Notes

- The CC1200 driver files are adapted from USC Rocket Propulsion Lab under Apache License 2.0
- Default operating frequency is 915 MHz (ISM band)
- Default symbol rate is 50 kBaud with 25 kHz deviation
- The project uses STM32CubeIDE configuration (.ioc file) for hardware setup
- GPIO interrupts are handled in `Core/Src/gpio_interrupts.cpp`

## Current Status: WORKING ✅

The project now has **working CC1200 SPI communication** with comprehensive debug output. Basic radio functionality is operational and ready for advanced development.

### ✅ Successfully Implemented

- **SPI Communication**: CC1200 responds correctly to all register reads/writes
- **USB VCP Interface**: Complete command-line interface with 15+ radio commands
- **Debug Output**: Real-time SPI transaction logging (TX/RX bytes) over USB
- **Hardware Verification**: CC1200 part number confirmed (0x20), chip in RX mode
- **Code Architecture**: Clean direct CC1200 access without abstraction overhead

### Available Commands

**Basic Radio Operations:**
- `radio_debug_on/off` - Enable/disable SPI transaction debug output
- `radio_init` - Initialize CC1200 with default settings  
- `radio_version` - Read part number (0x20) and version (0x11)
- `radio_status` - Detailed status (RSSI, LQI, state, FIFO levels)
- `freq <Hz>` - Set frequency (915MHz ISM band supported)
- `rate <Hz>` - Set symbol rate (default 50kHz)

**Polling-Mode Data Operations:**
- `radio_tx <hex_data>` - Transmit packet data (hex format)
- `radio_rx <timeout_ms>` - Receive packets with timeout
- `radio_stream_tx/rx` - Stream mode for continuous data
- `tx/rx <data>` - High-level transmit/receive commands

**DMA-Enabled Data Operations:**
- `radio_tx_dma <hex_data>` - Transmit data using DMA
- `radio_rx_dma <timeout_ms>` - Receive data using DMA
- `radio_stream_tx_dma <hex_data>` - Stream transmit using DMA
- `radio_stream_rx_dma <bytes> <timeout_ms>` - Stream receive using DMA

**Continuous Streaming (DMA-Based):**
- `radio_stream_start_tx <hex_pattern>` - Start continuous TX streaming
- `radio_stream_start_rx` - Start continuous RX streaming
- `radio_stream_stop` - Stop all continuous streaming
- `radio_stream_stats` - Show streaming statistics

**System Commands:**
- `help` - Complete command reference
- `sysinfo` - System information and diagnostics
- `restart` - System restart

### SPI Debug Output Example

When `radio_debug_on` is enabled, you see exact protocol details:
```
> radio_status
SPI: TX=0xAF RX=0x00    # Read extended register command
SPI: TX=0x73 RX=0x00    # MARCSTATE register address  
SPI: TX=0x00 RX=0x41    # Read state value (0x41 = RX mode)
State: RX
```

### Hardware Configuration Verified

- **SPI1**: PA4 (CS), PA5 (SCK), PA6 (MISO), PA7 (MOSI) 
- **SPI Clock**: 6 MHz (prescaler /8 from 48MHz APB2, optimized for CC1200 timing)
- **CC1200 Control**: PB0 (Reset), PB12-14 (GPIO0,2,3)
- **Status LEDs**: PB3 (Service), PB4 (RX), PB5 (TX)
- **USB VCP**: Full CDC interface for commands and debug output

## Next Steps: DMA & Advanced Features

### Phase 1: DMA Implementation
- Re-enable SPI DMA for high-speed data transfer
- Implement DMA-based streaming for large data payloads
- Add DMA completion callbacks and error handling

### Phase 2: Advanced CC1200 Features  
- Implement 4FSK modulation optimization
- Add FIFO threshold interrupts for real-time data
- Implement packet mode with automatic CRC
- Add frequency hopping and channel management

### Phase 3: Digital Data Protocols
- High-speed data streaming protocols
- Custom packet formats for specific applications
- Error correction and retransmission
- Advanced modulation schemes beyond basic 4FSK

### Reference Documentation
- CC1200 datasheet available in `Documents/swru346b.pdf`
- Current implementation based on USC Rocket Propulsion Lab driver
- Ready for extension beyond basic HAL functionality

## File Structure Context

- `Core/Inc/`: All header files including hardware abstraction and class definitions
- `Core/Src/`: Implementation files for application logic
- `Core/Startup/`: STM32 startup assembly code
- `Core/ThreadSafe/`: Thread-safe implementations for FreeRTOS
- `USB_DEVICE/`: USB CDC implementation for VCP interface
- `Drivers/`: STM32 HAL drivers and CMSIS
- `Middlewares/`: FreeRTOS and USB device library
- `Debug/`: Build output directory with makefiles and binaries
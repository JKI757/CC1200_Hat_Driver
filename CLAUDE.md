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

## Current Status & Debugging

The project currently boots and provides a USB VCP command interface, but CC1200 radio functions are not working properly. The issue is likely in the DMA configuration or basic SPI communication.

### Available Debug Commands

The VCP menu provides comprehensive debugging tools:
- `radio_debug_on` - Enable SPI debug output to UART for low-level troubleshooting
- `radio_debug_off` - Disable SPI debug output
- `radio_status` - Get detailed CC1200 status (RSSI, LQI, state, FIFO)
- `radio_version` - Get CC1200 part number and version (should return 0x20)
- `radio_init` - Initialize radio with default settings
- `sysinfo` - Display system information

### Basic CC1200 Test Strategy

For debugging CC1200 communication issues:

1. **Enable debug output**: Use `radio_debug_on` to see SPI transactions
2. **Test basic communication**: Use `radio_version` to verify part number (should be 0x20)
3. **Check initialization**: Use `radio_init` and monitor debug output
4. **Test register access**: Use `radio_status` to verify register reads
5. **Verify SPI configuration**: Check that SPI1 is properly configured for CC1200

### Key Test Registers

- `ExtRegister::PARTNUMBER` (0x8F) - Should read 0x20 for CC1200
- `ExtRegister::PARTVERSION` (0x90) - Hardware version
- `ExtRegister::MARCSTATE` (0x73) - Current state machine state
- `Register::PKT_LEN` (0x2E) - Simple read/write test register

### Common Issues to Check

- SPI timing and clock configuration
- GPIO pin assignments for CS and RST
- DMA configuration conflicts
- Power supply stability
- Hardware reset sequence

## File Structure Context

- `Core/Inc/`: All header files including hardware abstraction and class definitions
- `Core/Src/`: Implementation files for application logic
- `Core/Startup/`: STM32 startup assembly code
- `Core/ThreadSafe/`: Thread-safe implementations for FreeRTOS
- `USB_DEVICE/`: USB CDC implementation for VCP interface
- `Drivers/`: STM32 HAL drivers and CMSIS
- `Middlewares/`: FreeRTOS and USB device library
- `Debug/`: Build output directory with makefiles and binaries
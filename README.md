# CC1200 Radio HAT STM32 Firmware

This project implements firmware for an STM32-based CC1200 radio HAT. The firmware provides a complete solution for controlling the CC1200 RF front end with 4FSK modulation, infinite read/write capabilities, and DMA support.

## Features

- **CC1200 Radio Driver**: Complete driver for the CC1200 radio chip with support for:
  - 4FSK modulation
  - Infinite read/write modes
  - DMA-based data transfer
  - Configurable frequency and symbol rate

- **USB VCP Interface**: Command-line interface over USB CDC for:
  - Transmitting and receiving data
  - Configuring radio parameters
  - Monitoring radio status

- **FreeRTOS Integration**: Multi-threaded operation with tasks for:
  - Watchdog management
  - Radio transmission
  - Radio reception
  - USB command processing

- **LED Indicators**:
  - Service LED: System status
  - TX LED: Transmission activity
  - RX LED: Reception activity

## Hardware Requirements

- STM32F4 series microcontroller
- CC1200 radio chip
- SPI interface for CC1200 communication
- GPIO pins for CC1200 control
- USB connection for VCP interface

## Pin Configuration

The firmware is configured to use the following pins:

- **CC1200 Interface**:
  - SPI1 for communication
  - _CC_RST_Pin (PB0): Reset pin
  - CC_GPIO0_Pin (PB12): GPIO0 for interrupt/status
  - CC_GPIO2_Pin (PB13): GPIO2 for interrupt/status
  - CC_GPIO3_Pin (PB14): GPIO3 for interrupt/status

- **LED Indicators**:
  - SVC_LED_Pin (PB3): Service LED
  - RX_LED_Pin (PB4): RX activity LED
  - TX_LED_Pin (PB5): TX activity LED

## USB VCP Commands

The firmware provides a command-line interface over USB CDC with the following commands:

- `help`: Display help menu
- `status`: Display radio status
- `tx <data>`: Transmit data
- `rx`: Start receiving data
- `freq <frequency>`: Set radio frequency in Hz
- `rate <symbol_rate>`: Set symbol rate in Hz
- `reset`: Reset the radio

## Project Structure

- **Core/Inc**: Header files
  - `Radio.h`: Radio class for CC1200 control
  - `VCPMenu.h`: VCP menu interface
  - `CC1200_HAL.h`: CC1200 driver
  - `globals.h`: Global objects and utilities

- **Core/Src**: Source files
  - `Radio.cpp`: Radio class implementation
  - `VCPMenu.cpp`: VCP menu implementation
  - `CC1200_HAL.cpp`: CC1200 driver implementation
  - `freertos.cpp`: FreeRTOS task implementations
  - `gpio_interrupts.cpp`: GPIO interrupt handlers

- **USB_DEVICE**: USB CDC implementation

## Building and Flashing

The project is configured for the STM32CubeIDE. To build and flash:

1. Open the project in STM32CubeIDE
2. Build the project
3. Connect the STM32 board via ST-Link
4. Flash the firmware

## Usage

1. Connect to the device using a USB serial terminal (115200 baud)
2. The welcome message and command prompt will appear
3. Use the available commands to control the radio

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

# To-fu Device Firmware

This repository contains the firmware for the To-fu translation and AI assistant device system. The To-fu device is a smart translation and communication assistant that can work in leader-follower mode to facilitate multilingual conversations.

## Project Overview

The To-fu device is designed to provide real-time translation services with both online and offline capabilities. It features voice recognition, expression capabilities, and seamless communication between leader and follower devices.

### Key Features

- Real-time translation between multiple languages
- Leader-follower device synchronization
- Voice recognition and processing
- Expression capabilities through display and audio
- Cloud connectivity for enhanced features
- Offline mode for basic functionality without internet
- Over-the-air (OTA) updates

## Repository Structure

```
root/
├── firmware_config.h       # Main configuration settings
├── hal.h                   # Hardware Abstraction Layer interface
├── system_manager.h        # System management interface
├── feature_manager.h       # Feature management interface
├── comm_manager.h          # Communication management interface
├── main.c                  # Main application entry point
├── README.md               # This file
└── project_structure.md    # Detailed project structure documentation
```

## Architecture

The firmware is organized into several key components:

1. **Hardware Abstraction Layer (HAL)**: Provides a unified interface to interact with hardware components while abstracting hardware-specific details.

2. **System Manager**: Responsible for initializing and coordinating all system components, managing tasks, and handling system events.

3. **Feature Manager**: Manages the various features of the device, including voice processing, expression handling, and interaction capabilities.

4. **Communication Manager**: Handles all communication interfaces, including WiFi, BLE, and protocol handling for cloud services and device-to-device communication.

## Getting Started

### Prerequisites

- ESP-IDF development environment (v4.4 or later)
- CMake (3.16 or later)
- C/C++ compiler compatible with ESP32
- Python 3.6 or later (for build scripts)

### Building the Firmware

1. Clone this repository:
   ```
   git clone https://github.com/your-organization/tofu-firmware.git
   cd tofu-firmware
   ```

2. Configure the project:
   ```
   idf.py menuconfig
   ```

3. Build the project:
   ```
   idf.py build
   ```

4. Flash to the device:
   ```
   idf.py -p [PORT] flash
   ```

5. Monitor the output:
   ```
   idf.py -p [PORT] monitor
   ```

### Configuration

The main configuration settings are in `firmware_config.h`. You should modify this file to match your hardware setup and requirements.

Key configuration areas:
- Hardware pin definitions
- System parameters
- Communication settings
- Feature flags

## Development Workflow

Please refer to `project_structure.md` for the detailed development workflow and project organization.

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- The ESP-IDF team for their excellent development framework
- Contributors to the open-source libraries used in this project

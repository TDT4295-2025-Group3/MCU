
# Upwards - MCU Firmware

This repository contains the firmware for the Upwards game created for [TDT4295 - Computer Design Project](https://www.ntnu.edu/studies/courses/TDT4295/2025) course
at Norwegian University of Science and Technology (NTNU).

The firmware is designed to run on a microcontroller unit (MCU) and is responsible for handling game logic, user input, and display output.

The code supports running on a STM32U595 microcontroller or on a local simulator for development and testing purposes.

## Code Structure

The code is structured into several components to decouple game logic from hardware-specific implementations:

- `app` - Contains the main game logic and application flow. `app/platform` includes the required interfaces for platform-specific implementations.
- `hid_input` - USB descriptor and high level drivers for HID devices (Keyboard and DualShock 4).
- `device` - Implementation of the interfaces for the STM32U595 microcontroller platform.
- `sim` - Implementation of the interfaces for a local simulator platform, using SDL3, small3dlib and hidapi.

## Using the code

### Requirements - SIMULATOR
For running simulator code, the following dependencies are required:

- [SDL3](https://www.libsdl.org/)
- [hidapi](https://github.com/libusb/hidapi)
- small3dlib (included in `sim/small3dlib.h`)

### Requirements - MCU
For building the MCU firmware, the following external tools are required:

- TinyUSB (included as a git submodule)
- FatFs (included in the repo)

### Running
To build and run the firmware, follow these steps:
1. **Clone the repository**:
   ```bash
   git clone git@github.com:TDT4295-2025-Group3/MCU.git
   
2. **Navigate to the project director and initialize submodules**:
   ```bash
    cd MCU
    git submodule update --init --recursive
   ```

You should then be able to either select the `STMU595VJT6` or `simulator` target in your IDE.
Nib OS


A lightweight operating system for the Raspberry Pi 2/3 with micro SD card support and MicroPython capability.

Overview:
Nib OS is a bare-metal operating system that runs directly on Raspberry Pi hardware without any underlying OS. It features:
SD Card Support - Reads files from FAT32 formatted SD and micro SD cards.
Serial Console - Nib OS features an interactive shell vie UART.
MicroPython Ready - Architecture prepared for Python script execution.
Memory Management - Basic heap allocation system.
Extensible - Easy to add new features and commands.

System Requirements:
Hardware:
Raspberry Pi 2 or 3
SD card (4GB+, FAT32 formatted)
USB-to-TTL serial cable (for console access)
Power supply

Software (for building):
ARM cross-compiler toolchain (gcc-arm-none-eabi)
Make utility
Linux, macOS, or Windows with WSL

Quick Start:

1. Install Build Tools:

Ubuntu/Debian:
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi make

macOS:
brew install --cask gcc-arm-embedded

Windows:
Use WSL with Ubuntu and follow the Ubuntu instructions.

2. Build Nib OS:
cd nib-os
make

3. Prepare the micro SD card:
Format the micro SD card as FAT32, then copy these files to the root:
Required Raspberry Pi firmware files:

bootcode.bin
start.elf
fixup.dat
(Download the above files from: https://github.com/raspberrypi/firmware/tree/master/boot)

Your kernel:
kernel.img (the file you just built)

Optional: The default Python script:
hello.py

Final micro SD card structure:
/
|- bootcode.bin
|- start.elf
|- fixup.dat
|- kernel.img
|- hello.py

4. Connect The Serial Console:

Wiring (USB-to-TTL cable to Raspberry Pi GPIO):
GND (Black) -> Pin 6 (Ground)
TX (White) -> Pin 10 (GPIO 15, RXD)
RX (Green) -> Pin 8 (GPIO 14, TXD)
DO NOT connect the power wire (Red)!

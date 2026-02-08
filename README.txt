# Nib OS

A lightweight operating system for Raspberry Pi 2/3 with SD card support and MicroPython capability.

## Overview

Nib OS is a bare-metal operating system that runs directly on Raspberry Pi hardware without any underlying OS. It features:

- **Bare Metal Execution** - Runs directly on hardware
- **SD Card Support** - Read files from FAT32 formatted SD cards
- **Serial Console** - Interactive shell via UART
- **MicroPython Ready** - Architecture prepared for Python script execution
- **Memory Management** - Basic heap allocation system
- **Extensible** - Easy to add new features and commands

## System Requirements

### Hardware
- Raspberry Pi 2 or 3
- SD card (4GB+, FAT32 formatted)
- USB-to-TTL serial cable (for console access)
- Power supply

### Software (for building)
- ARM cross-compiler toolchain (`gcc-arm-none-eabi`)
- Make utility
- Linux, macOS, or Windows with WSL

## Quick Start

### 1. Install Build Tools

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi make
```

**macOS:**
```bash
brew install --cask gcc-arm-embedded
```

**Windows:**
Use WSL with Ubuntu and follow Ubuntu instructions.

### 2. Build Nib OS

```bash
# Clone or download the repository
cd nib-os

# Build the kernel
make

# This creates kernel.img (~50KB)
```

### 3. Prepare SD Card

**Format SD card as FAT32**, then copy these files to the root:

**Required Raspberry Pi firmware files:**
- `bootcode.bin`
- `start.elf`
- `fixup.dat`

Download from: https://github.com/raspberrypi/firmware/tree/master/boot

**Your kernel:**
- `kernel.img` (the file you just built)

**Optional - Your Python scripts:**
- `hello.py`
- `test.py`
- etc.

**Final SD card structure:**
```
/
├── bootcode.bin
├── start.elf
├── fixup.dat
├── kernel.img
├── hello.py          (your Python scripts)
└── test.py
```

### 4. Connect Serial Console

**Wiring (USB-to-TTL cable to Raspberry Pi GPIO):**
- GND (Black) → Pin 6 (Ground)
- TX (White) → Pin 10 (GPIO 15, RXD)
- RX (Green) → Pin 8 (GPIO 14, TXD)

**⚠️ DO NOT connect the power wire (Red)!**

**Open serial terminal:**
```bash
# Linux
screen /dev/ttyUSB0 115200

# macOS
screen /dev/cu.usbserial 115200

# Windows (use PuTTY)
# Set: COM port, 115200 baud, 8N1
```

### 5. Boot

Insert SD card into Raspberry Pi and power on. You should see:

```
========================================
            Nib OS v1.0                
========================================
Lightweight OS with Python support

Initializing SD card...
SD card initialized successfully
Initializing FAT32 file system...
FAT32: Initialized successfully

Type 'help' for available commands.

Nib>
```

## Available Commands

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Display all available commands | `help` |
| `ls` | List files on SD card | `ls` |
| `cat <file>` | Display contents of a file | `cat hello.py` |
| `run <file>` | Run a Python script (requires MicroPython) | `run test.py` |
| `echo <text>` | Echo text back to console | `echo Hello World` |
| `clear` | Clear the screen | `clear` |
| `info` | Show system information | `info` |
| `mem` | Display memory usage statistics | `mem` |
| `reboot` | Reboot the system | `reboot` |

## Using Nib OS

### Listing Files

```
Nib> ls

Files in root directory:
========================
HELLO.PY  (249 bytes)
========================
```

### Viewing File Contents

```
Nib> cat hello.py

--- File contents ---
print("Hello from Nib OS!")
print("Running on bare metal!")
--- End of file ---
```

### Checking Memory

```
Nib> mem
Memory usage:
  Used: 2048 bytes
  Available: 16775168 bytes
```

### System Information

```
Nib> info
Nib OS v1.0
Architecture: ARM
Platform: Raspberry Pi 2/3
Features:
  - SD card support (FAT32)
  - MicroPython interpreter
  - File system access
```

## Installing MicroPython

By default, the `run` command displays Python code but doesn't execute it. To enable Python script execution, follow these steps:

### Step 1: Download MicroPython

```bash
# In your workspace directory
git clone https://github.com/micropython/micropython.git
cd micropython
git submodule update --init
```

### Step 2: Build MicroPython Cross Compiler

```bash
cd mpy-cross
make
cd ..
```

### Step 3: Build MicroPython for ARM

```bash
cd ports/bare-arm
make CROSS_COMPILE=arm-none-eabi-
cd ../..
```

### Step 4: Integrate with Nib OS

See the **MICROPYTHON_GUIDE.md** file for detailed integration instructions, including:
- Creating the MicroPython port files
- Updating the Makefile
- Implementing platform-specific functions
- Building the combined kernel

**Quick summary:**
1. Copy MicroPython source files to your Nib OS directory
2. Add MicroPython sources to the Makefile
3. Implement the MicroPython port interface
4. Rebuild: `make clean && make`

After integration, your kernel will be ~350KB and Python scripts will execute when you use `run`.

### Step 5: Test Python Execution

Create `hello.py` on your SD card:
```python
print("Hello from MicroPython!")
for i in range(5):
    print("Count:", i)
```

Then in Nib OS:
```
Nib> run hello.py
Loading Python script: hello.py
Executing Python code...
--- Output ---
Hello from MicroPython!
Count: 0
Count: 1
Count: 2
Count: 3
Count: 4
```

## Creating Python Scripts

### File Requirements
- Maximum file size: 10KB (recommended)
- File format: Plain text, UTF-8
- File extension: `.py`
- Location: SD card root directory

### Example Scripts

**hello.py** - Simple greeting
```python
print("Hello from Nib OS!")
print("This is MicroPython running on bare metal!")
```

**fibonacci.py** - Calculate Fibonacci numbers
```python
def fib(n):
    if n <= 1:
        return n
    return fib(n-1) + fib(n-2)

for i in range(10):
    print("fib({}) = {}".format(i, fib(i)))
```

**math.py** - Basic calculations
```python
a = 42
b = 58
print("Sum:", a + b)
print("Product:", a * b)
print("Division:", a / 2)
```

## File System Notes

### FAT32 Requirements
- SD card must be formatted as FAT32
- Files in root directory only (no subdirectories currently)
- File names follow 8.3 format (converted automatically)

### File Name Handling
When referencing files, use lowercase:
```
Nib> cat hello.py
```

Nib OS will automatically convert to uppercase for FAT32 compatibility.

## Memory Layout

```
0x0000 - 0x8000      Reserved (interrupt vectors, etc.)
0x8000 - ?           Kernel code and data
0x1000000 - 0x2000000  Heap (16MB)
0x8000000            Stack (grows downward)
```

## Troubleshooting

### SD Card Not Detected
- Verify SD card is formatted as FAT32
- Try reformatting the card
- Ensure card is properly inserted before powering on
- Check that firmware files (bootcode.bin, start.elf, fixup.dat) are present

### No Serial Output
- Verify wiring connections
- Check baud rate is set to 115200
- Try different serial terminal software
- Ensure TX/RX are not swapped

### Files Not Found
- File names are case-insensitive (use lowercase in commands)
- Ensure files are in root directory, not subdirectories
- Check file is not hidden or system file
- Verify FAT32 format is correct

### Build Errors
- Ensure ARM toolchain is properly installed
- Run `make clean` before `make`
- Check all source files are present
- Verify Makefile paths are correct

### System Hangs or Crashes
- Memory allocations can fail if heap is exhausted
- Use `mem` command to check available memory
- Reboot to reset the system
- Keep Python scripts under 10KB

## Project Structure

```
nib-os/
├── boot.S              Assembly bootloader
├── kernel.c            Main kernel and shell
├── uart.c/h            Serial communication driver
├── memory.c/h          Memory management
├── sd.c/h              SD card driver
├── fat32.c/h           FAT32 file system
├── linker.ld           Linker script
├── Makefile            Build system
├── README.md           This file
├── MICROPYTHON_GUIDE.md  Detailed MicroPython integration
└── examples/           Example Python scripts
    └── hello.py
```

## Build Targets

```bash
make              # Build kernel.img
make clean        # Remove build artifacts
make disasm       # Create disassembly listing
make install SDCARD=/path/to/boot  # Install to SD card
```

## Technical Specifications

- **Architecture:** ARM v7-A (Cortex-A)
- **Kernel Size:** ~50KB (without MicroPython), ~350KB (with MicroPython)
- **Memory:** 16MB heap
- **Storage:** FAT32 via SD card
- **I/O:** UART0 (115200 baud, 8N1)
- **Boot Address:** 0x8000

## Limitations

- Read-only file system (cannot write to SD card)
- Root directory only (no subdirectories)
- No USB support
- No HDMI/graphics output (serial only)
- No networking
- Single-threaded (no multitasking)
- Limited Python standard library (with MicroPython)

## Extending Nib OS

### Adding New Commands

Edit `kernel.c`:

```c
void cmd_mycommand(char* args) {
    uart_puts("My custom command!\n");
}

// In parse_command():
else if (strcmp(cmd, "mycommand") == 0) {
    cmd_mycommand(args);
}
```

### Adding Hardware Support

Create new driver files (e.g., `gpio.c/h`, `spi.c/h`) and add to Makefile.

### Exposing to Python

After integrating MicroPython, create modules in `modmachine.c` to expose hardware functions to Python scripts.

## Resources

- [MicroPython Documentation](https://docs.micropython.org/)
- [Raspberry Pi Bare Metal](https://github.com/dwelch67/raspberrypi)
- [ARM Architecture Reference](https://developer.arm.com/documentation/)
- [BCM2835 Peripherals](https://www.raspberrypi.org/app/uploads/2012/02/BCM2835-ARM-Peripherals.pdf)

## License

This project is licensed under the MIT License - see LICENSE file for details.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Credits

Created by Matteo Roscioli

Copy

# Makefile for Nib OS

# Compiler and tools
ARMGNU ?= arm-none-eabi

CC = $(ARMGNU)-gcc
AS = $(ARMGNU)-as
LD = $(ARMGNU)-ld
OBJCOPY = $(ARMGNU)-objcopy
OBJDUMP = $(ARMGNU)-objdump

# Compiler flags
CFLAGS = -Wall -Wextra -O2 -nostdlib -nostartfiles -ffreestanding \
         -mfpu=vfp -mfloat-abi=hard -march=armv7-a -mtune=cortex-a53

ASFLAGS = -march=armv7-a -mfpu=vfp -mfloat-abi=hard

# Source files
C_SOURCES = kernel.c uart.c memory.c sd.c fat32.c
ASM_SOURCES = boot.S

# Object files
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_OBJECTS = $(ASM_SOURCES:.S=.o)
OBJECTS = $(ASM_OBJECTS) $(C_OBJECTS)

# Output files
TARGET = kernel.elf
IMG = kernel.img

# Default target
all: $(IMG)

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble assembly files
%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

# Link object files
$(TARGET): $(OBJECTS)
	$(LD) -T linker.ld $(OBJECTS) -o $(TARGET)

# Create binary image
$(IMG): $(TARGET)
	$(OBJCOPY) $(TARGET) -O binary $(IMG)
	@echo ""
	@echo "======================================"
	@echo "  Kernel image created: $(IMG)"
	@echo "======================================"
	@ls -lh $(IMG)
	@echo ""

# Create disassembly
disasm: $(TARGET)
	$(OBJDUMP) -D $(TARGET) > kernel.list

# Clean build artifacts
clean:
	rm -f *.o *.elf *.img *.list

# Install to SD card
install: $(IMG)
	@echo "This will copy kernel.img to your SD card boot partition"
	@echo "Usage: make install SDCARD=/path/to/boot/partition"
ifdef SDCARD
	cp $(IMG) $(SDCARD)/kernel.img
	sync
	@echo "Kernel installed to $(SDCARD)"
else
	@echo "Please specify SDCARD=/path/to/boot/partition"
endif

.PHONY: all clean disasm install

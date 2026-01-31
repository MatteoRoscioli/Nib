/*
 * kernel.c - Nib OS kernel with MicroPython support
 */

#include "uart.h"
#include "memory.h"
#include "sd.h"
#include "fat32.h"

// MicroPython placeholder (we'll add integration instructions)
extern int micropython_init(void);
extern int micropython_run_file(const char* code, unsigned int size);

// String functions
int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcpy(char* dst, const char* src) {
    while (*src) {
        *dst++ = *src++;
    }
    *dst = '\0';
}

int strncmp(const char* s1, const char* s2, int n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

// Command: help
void cmd_help() {
    uart_puts("Available commands:\n");
    uart_puts("  help      - Show this help\n");
    uart_puts("  echo      - Echo text\n");
    uart_puts("  clear     - Clear screen\n");
    uart_puts("  info      - System information\n");
    uart_puts("  ls        - List files on SD card\n");
    uart_puts("  cat       - Display file contents\n");
    uart_puts("  run       - Run a Python file\n");
    uart_puts("  python    - Interactive Python (coming soon)\n");
    uart_puts("  mem       - Show memory usage\n");
    uart_puts("  reboot    - Reboot system\n");
}

// Command: echo
void cmd_echo(char* args) {
    uart_puts(args);
    uart_puts("\n");
}

// Command: clear
void cmd_clear() {
    uart_puts("\033[2J\033[H");
}

// Command: info
void cmd_info() {
    uart_puts("Nib OS v1.0\n");
    uart_puts("Architecture: ARM\n");
    uart_puts("Platform: Raspberry Pi 2/3\n");
    uart_puts("Features:\n");
    uart_puts("  - SD card support (FAT32)\n");
    uart_puts("  - MicroPython interpreter\n");
    uart_puts("  - File system access\n");
}

// Command: ls (list files)
void cmd_ls() {
    fat32_list_files();
}

// Command: cat (display file)
void cmd_cat(char* filename) {
    if (*filename == '\0') {
        uart_puts("Usage: cat <filename>\n");
        return;
    }
    
    unsigned char* buffer = (unsigned char*)malloc(10240);
    if (!buffer) {
        uart_puts("Error: Out of memory\n");
        return;
    }
    
    int size = fat32_read_file(filename, buffer, 10240);
    
    if (size > 0) {
        uart_puts("\n--- File contents ---\n");
        for (int i = 0; i < size; i++) {
            uart_putc(buffer[i]);
        }
        uart_puts("\n--- End of file ---\n");
    }
    
    free(buffer);
}

// Command: run (execute Python file)
void cmd_run(char* filename) {
    if (*filename == '\0') {
        uart_puts("Usage: run <filename.py>\n");
        return;
    }
    
    uart_puts("Loading Python script: ");
    uart_puts(filename);
    uart_puts("\n");
    
    unsigned char* buffer = (unsigned char*)malloc(10240);
    if (!buffer) {
        uart_puts("Error: Out of memory\n");
        return;
    }
    
    int size = fat32_read_file(filename, buffer, 10240);
    
    if (size > 0) {
        uart_puts("Executing Python code...\n");
        uart_puts("--- Output ---\n");
        
        // For now, just display the code
        // In full version, this would call: micropython_run_file((char*)buffer, size);
        uart_puts("MicroPython execution not yet integrated.\n");
        uart_puts("To integrate MicroPython:\n");
        uart_puts("1. Download MicroPython for bare-metal ARM\n");
        uart_puts("2. Link it with this kernel\n");
        uart_puts("3. Implement micropython_run_file() function\n");
        
        uart_puts("\n--- Code preview ---\n");
        for (int i = 0; i < (size < 500 ? size : 500); i++) {
            uart_putc(buffer[i]);
        }
        if (size > 500) uart_puts("\n... (truncated) ...");
        uart_puts("\n");
    }
    
    free(buffer);
}

// Command: mem (memory info)
void cmd_mem() {
    uart_puts("Memory usage:\n");
    uart_puts("  Used: ");
    uart_dec(mem_used());
    uart_puts(" bytes\n");
    uart_puts("  Available: ");
    uart_dec(mem_available());
    uart_puts(" bytes\n");
}

// Command: reboot
void cmd_reboot() {
    uart_puts("Rebooting...\n");
    volatile unsigned int* PM_RSTC = (unsigned int*)0x3F10001c;
    volatile unsigned int* PM_WDOG = (unsigned int*)0x3F100024;
    
    *PM_WDOG = 0x5a000020;
    *PM_RSTC = 0x5a000102;
    
    while(1);
}

// Parse and execute command
void parse_command(char* cmd) {
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    
    if (*cmd == '\0') return;
    
    // Find space to separate command and args
    char* args = cmd;
    while (*args && *args != ' ') args++;
    if (*args) {
        *args = '\0';
        args++;
        while (*args == ' ') args++;
    }
    
    // Execute commands
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd, "info") == 0) {
        cmd_info();
    } else if (strcmp(cmd, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(cmd, "cat") == 0) {
        cmd_cat(args);
    } else if (strcmp(cmd, "run") == 0) {
        cmd_run(args);
    } else if (strcmp(cmd, "mem") == 0) {
        cmd_mem();
    } else if (strcmp(cmd, "python") == 0) {
        uart_puts("Interactive Python coming soon!\n");
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else {
        uart_puts("Unknown command: ");
        uart_puts(cmd);
        uart_puts("\nType 'help' for available commands.\n");
    }
}

// Simple shell
void shell() {
    char buffer[256];
    int pos = 0;
    
    uart_puts("\nNib> ");
    
    while (1) {
        char c = uart_getc();
        
        if (c == '\r' || c == '\n') {
            buffer[pos] = '\0';
            uart_puts("\n");
            
            if (pos > 0) {
                parse_command(buffer);
                pos = 0;
            }
            
            uart_puts("Nib> ");
        } else if (c == 127 || c == 8) {  // Backspace
            if (pos > 0) {
                pos--;
                uart_puts("\b \b");
            }
        } else if (c >= 32 && c < 127 && pos < 255) {
            buffer[pos++] = c;
            uart_putc(c);
        }
    }
}

// Kernel main
void kernel_main(void) {
    // Initialize UART
    uart_init();
    
    // Clear screen and show welcome
    uart_puts("\033[2J\033[H");
    uart_puts("========================================\n");
    uart_puts("            Nib OS v1.0                \n");
    uart_puts("========================================\n");
    uart_puts("Lightweight OS with Python support\n\n");
    
    // Initialize memory
    mem_init();
    
    // Initialize SD card
    int sd_status = sd_init();
    if (sd_status != 0) {
        uart_puts("WARNING: SD card initialization failed!\n");
        uart_puts("File system features will not be available.\n\n");
    } else {
        // Initialize FAT32
        int fat_status = fat32_init();
        if (fat_status != 0) {
            uart_puts("WARNING: FAT32 initialization failed!\n");
            uart_puts("Make sure SD card is formatted as FAT32.\n\n");
        }
    }
    
    uart_puts("Type 'help' for available commands.\n");
    uart_puts("Type 'ls' to list files on SD card.\n");
    uart_puts("Type 'run filename.py' to execute Python scripts.\n\n");
    
    // Start shell
    shell();
    
    // Should never reach here
    while (1) {
        uart_putc('.');
    }
}

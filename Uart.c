/*
 * uart.c - UART driver for Raspberry Pi serial communication
 */

#include "uart.h"

// GPIO registers (Raspberry Pi 3)
#define GPIO_BASE       0x3F200000
#define GPFSEL1         ((volatile unsigned int*)(GPIO_BASE + 0x04))
#define GPPUD           ((volatile unsigned int*)(GPIO_BASE + 0x94))
#define GPPUDCLK0       ((volatile unsigned int*)(GPIO_BASE + 0x98))

// UART registers
#define UART0_BASE      0x3F201000
#define UART0_DR        ((volatile unsigned int*)(UART0_BASE + 0x00))
#define UART0_FR        ((volatile unsigned int*)(UART0_BASE + 0x18))
#define UART0_IBRD      ((volatile unsigned int*)(UART0_BASE + 0x24))
#define UART0_FBRD      ((volatile unsigned int*)(UART0_BASE + 0x28))
#define UART0_LCRH      ((volatile unsigned int*)(UART0_BASE + 0x2C))
#define UART0_CR        ((volatile unsigned int*)(UART0_BASE + 0x30))
#define UART0_ICR       ((volatile unsigned int*)(UART0_BASE + 0x44))

// Simple delay function
static void delay(int count) {
    volatile int i;
    for (i = 0; i < count; i++) {
        asm volatile("nop");
    }
}

void uart_init(void) {
    // Disable UART0
    *UART0_CR = 0x00000000;
    
    // Setup GPIO pins 14 and 15
    unsigned int ra = *GPFSEL1;
    ra &= ~(7 << 12);  // Clear GPIO 14
    ra |= 4 << 12;     // Set GPIO 14 to alt0
    ra &= ~(7 << 15);  // Clear GPIO 15
    ra |= 4 << 15;     // Set GPIO 15 to alt0
    *GPFSEL1 = ra;
    
    // Disable pull up/down for pins 14 and 15
    *GPPUD = 0;
    delay(150);
    *GPPUDCLK0 = (1 << 14) | (1 << 15);
    delay(150);
    *GPPUDCLK0 = 0;
    
    // Clear pending interrupts
    *UART0_ICR = 0x7FF;
    
    // Set baud rate to 115200
    // Assuming 3MHz clock: divisor = 3000000 / (16 * 115200) = 1.627
    *UART0_IBRD = 1;
    *UART0_FBRD = 40;
    
    // Enable FIFO, 8-bit data transmission
    *UART0_LCRH = (1 << 4) | (1 << 5) | (1 << 6);
    
    // Enable UART0, receive, and transmit
    *UART0_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(char c) {
    // Wait for UART to be ready to transmit
    while (*UART0_FR & (1 << 5)) { }
    *UART0_DR = c;
}

char uart_getc(void) {
    // Wait for UART to have received something
    while (*UART0_FR & (1 << 4)) { }
    return (char)(*UART0_DR);
}

void uart_puts(const char* str) {
    while (*str) {
        if (*str == '\n') {
            uart_putc('\r');
        }
        uart_putc(*str++);
    }
}

void uart_hex(unsigned int num) {
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        uart_putc(hex[(num >> i) & 0xF]);
    }
}

void uart_dec(unsigned int num) {
    if (num == 0) {
        uart_putc('0');
        return;
    }
    
    char buffer[16];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        uart_putc(buffer[--i]);
    }
}

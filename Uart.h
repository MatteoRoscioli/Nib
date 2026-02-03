/*
 * uart.h - UART driver header
 */

#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(char c);
char uart_getc(void);
void uart_puts(const char* str);
void uart_hex(unsigned int num);
void uart_dec(unsigned int num);

#endif

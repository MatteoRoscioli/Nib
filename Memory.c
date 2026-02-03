/*
 * memory.c - Simple memory management
 */

#include "memory.h"
#include "uart.h"

// Heap starts at 16MB
#define HEAP_START 0x1000000
#define HEAP_SIZE  0x1000000  // 16MB heap

static unsigned char* heap_current = (unsigned char*)HEAP_START;
static unsigned char* heap_end = (unsigned char*)(HEAP_START + HEAP_SIZE);

void mem_init(void) {
    heap_current = (unsigned char*)HEAP_START;
    uart_puts("Memory initialized: ");
    uart_hex(HEAP_START);
    uart_puts(" - ");
    uart_hex((unsigned int)heap_end);
    uart_puts("\n");
}

void* malloc(unsigned int size) {
    // Align to 4-byte boundary
    size = (size + 3) & ~3;
    
    if (heap_current + size > heap_end) {
        uart_puts("ERROR: Out of memory!\n");
        return 0;
    }
    
    void* ptr = heap_current;
    heap_current += size;
    
    return ptr;
}

void free(void* ptr) {
    // Simple allocator doesn't support free
    // In a real OS, you'd implement a proper allocator
    (void)ptr;
}

void* memset(void* dest, int val, unsigned int len) {
    unsigned char* ptr = (unsigned char*)dest;
    while (len-- > 0) {
        *ptr++ = (unsigned char)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, unsigned int len) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (len-- > 0) {
        *d++ = *s++;
    }
    return dest;
}

unsigned int mem_used(void) {
    return (unsigned int)(heap_current - (unsigned char*)HEAP_START);
}

unsigned int mem_available(void) {
    return (unsigned int)(heap_end - heap_current);
}

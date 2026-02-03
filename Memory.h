/*
 * memory.h - Memory management header
 */

#ifndef MEMORY_H
#define MEMORY_H

void mem_init(void);
void* malloc(unsigned int size);
void free(void* ptr);
void* memset(void* dest, int val, unsigned int len);
void* memcpy(void* dest, const void* src, unsigned int len);
unsigned int mem_used(void);
unsigned int mem_available(void);

#endif

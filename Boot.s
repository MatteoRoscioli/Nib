/*
 * boot.S - Nib OS bootloader and initial setup for ARM
 */

.section ".text.boot"
.global _start

_start:
    // Get CPU ID - only CPU 0 should continue
    mrc p15, 0, r0, c0, c0, 5
    ands r0, r0, #3
    bne halt

    // Set stack pointer to 16MB (increased for MicroPython)
    ldr sp, =0x1000000

    // Clear BSS section
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
clear_bss:
    cmp r0, r1
    bge clear_done
    str r2, [r0], #4
    b clear_bss

clear_done:
    // Enable VFP (Vector Floating Point)
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #0x300000
    orr r0, r0, #0xC00000
    mcr p15, 0, r0, c1, c0, 2
    mov r0, #0x40000000
    vmsr fpexc, r0

    // Enable L1 Cache
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #(1 << 2)   // Data cache
    orr r0, r0, #(1 << 12)  // Instruction cache
    mcr p15, 0, r0, c1, c0, 0

    // Jump to kernel main
    bl kernel_main

halt:
    // If we return from kernel_main or we're not CPU 0, halt
    wfe
    b halt

.section ".data"
    // Data section placeholder

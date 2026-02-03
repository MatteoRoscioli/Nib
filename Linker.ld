/*
 * linker.ld - Linker script for SimpleOS
 */

ENTRY(_start)

SECTIONS
{
    /* Kernel starts at 0x8000 for Raspberry Pi */
    . = 0x8000;
    
    /* Code section */
    .text : {
        KEEP(*(.text.boot))
        *(.text)
    }
    
    /* Read-only data */
    .rodata : {
        *(.rodata)
    }
    
    /* Initialized data */
    .data : {
        *(.data)
    }
    
    /* Uninitialized data (BSS) */
    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    
    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
    }
}

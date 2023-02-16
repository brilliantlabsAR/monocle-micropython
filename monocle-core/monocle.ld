/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Inc (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
 
/* This must be updated whenever the bootloader size changes */
bl_flash_end = 0;
bl_flash_settings_page_size = 0;

/* These must be updated whenever the softdevice is upgraded */
sd_flash_end = bl_flash_end + 0x26000;

/* This must be updated whenever softdevice settings are changed */
sd_ram_end = 0x2640;

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx) : ORIGIN = sd_flash_end, LENGTH = 512K - sd_flash_end - bl_flash_settings_page_size
    RAM (rwx) :  ORIGIN = 0x20000000 + sd_ram_end, LENGTH = 64K - sd_ram_end
}

SECTIONS
{
    /* Program code and other data goes into FLASH */

    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* Startup code */
        *(.text)             /* .text sections (code) */
        *(.text*)            /* .text* sections (code) */
        *(.rodata)           /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)          /* .rodata* sections (constants, strings, etc.) */
        
        . = ALIGN(4);
        _etext = .;          /* define a global symbol at end of code */
    } > FLASH

    /* This is used by the startup to initialize data */
    
    _sidata = LOADADDR(.data);

    /* This is the initialized data section. The program executes knowing that 
       the data is in the RAM but the loader puts the initial values in the 
       FLASH (inidata). It is one task of the startup to copy the initial values 
       from FLASH to RAM. */
    
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start; used by startup code in order to initialize the .data section in RAM */
        _ram_start = .;    /* create a global symbol at ram start for garbage collector */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end; used by startup code in order to initialize the .data section in RAM */
    } > RAM AT> FLASH

    /* Uninitialized data section */

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* define a global symbol at bss start; used by startup code */
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end; used by startup code and GC */

        PROVIDE(end = .);

    } > RAM

    .ARM.attributes 0 : 
    { 
        *(.ARM.attributes) 
    }
}

/* Define the stack area */

_stack_top = ORIGIN(RAM) + LENGTH(RAM);
_stack_bot = _stack_top - 4K;

/* Heap goes from end of bss ram to the bottom of the stack */

_heap_start = _ebss;
_heap_end = _stack_bot;

/* Throw an error if the heap becomes too small */

ASSERT(_heap_end - _heap_start >= 4K, "Heap has become too small")
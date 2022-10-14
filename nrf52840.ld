/* GNU linker script for NRF52840 */

_flash_size = 1M;
_ram_size   = 256K;

/* produce a link error if there is not this amount of RAM for these sections */
_stack_size = 8K;
_minimum_heap_size = 128K;

/* GNU linker script for s140 SoftDevice version 6.1.1 */

_sd_size = 0x00026000;
_sd_ram  = 0x000039c0;

/* Flash layout: bootloader_head | softdevice      | application     | filesystem    | bootloader_tail */
/* RAM layout:   bootloader RAM  | softdevice RAM  | application RAM */
_bootloader_head_size = DEFINED(_bootloader_head_size) ? _bootloader_head_size : 0;
_bootloader_tail_size = DEFINED(_bootloader_tail_size) ? _bootloader_tail_size : 0;
_bootloader_head_ram_size = DEFINED(_bootloader_head_ram_size) ? _bootloader_head_ram_size : 0;
_head_size     = DEFINED(_sd_size) ? _sd_size : _bootloader_head_size;
_head_ram      = DEFINED(_sd_ram)  ? _sd_ram : _bootloader_head_ram_size;
_sd_size       = DEFINED(_sd_size) ? _sd_size : 0;
_sd_ram        = DEFINED(_sd_ram)  ? _sd_ram : 0;
_fs_size       = DEFINED(_fs_size) ? _fs_size : 64K; /* TODO: set to 0 if not using the filesystem */
_app_size      = _flash_size - _head_size - _fs_size  - _bootloader_tail_size;
_app_start     = _head_size;
_fs_start      = _head_size + _app_size;
_fs_end        = _fs_start + _fs_size;
_app_ram_start = 0x20000000 + _head_ram;
_app_ram_size  = _ram_size - _head_ram;
_heap_start    = _ebss;
_heap_end      = _ram_end - _stack_size;
_heap_size     = _heap_end - _heap_start;

ASSERT(_heap_size >= _minimum_heap_size, "not enough RAM left for heap")

/* Specify the memory areas */
MEMORY
{
    FLASH_TEXT (rx) : ORIGIN = _app_start,     LENGTH = _app_size     /* app */
    RAM (xrw)       : ORIGIN = _app_ram_start, LENGTH = _app_ram_size
}
ENTRY(Reset_Handler)

/* define output sections */
SECTIONS
{
    /* The program code and other data goes into FLASH */
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* Startup code */
        *(.text)             /* .text sections (code) */
        *(.text*)            /* .text* sections (code) */
        *(.rodata)           /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)          /* .rodata* sections (constants, strings, etc.) */
    /*  *(.glue_7)   */      /* glue arm to thumb code */
    /*  *(.glue_7t)  */      /* glue thumb to arm code */

        . = ALIGN(4);
        _etext = .;          /* define a global symbol at end of code */
    } >FLASH_TEXT

    /*
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH
    */

    /* used by the startup to initialize data */
    _sidata = LOADADDR(.data);

    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH (inidata).
    It is one task of the startup to copy the initial values from FLASH to RAM. */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start; used by startup code in order to initialise the .data section in RAM */
        _ram_start = .;    /* create a global symbol at ram start for garbage collector */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end; used by startup code in order to initialise the .data section in RAM */
    } >RAM AT>FLASH_TEXT

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
    } >RAM

    /* Remove information from the standard libraries */
    /*
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
    */

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

/* Define heap and stack areas */
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
_estack = ORIGIN(RAM) + LENGTH(RAM);
_stack_top = _ram_end;
_stack_bot = _stack_top - _stack_size;
_unused_flash_start = (_sidata + (_edata - _sdata));
_unused_flash_len = (ORIGIN(FLASH_TEXT) + LENGTH(FLASH_TEXT)) - _unused_flash_start;

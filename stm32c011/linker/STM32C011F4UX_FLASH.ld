/* ============================================================================
 * Linker script STM32C011F4U6TR
 * Flash: 32KB (0x08000000 - 0x08007FFF)
 *   - Cod:    pages 0-14 (0x08000000 - 0x080077FF) = 30KB
 *   - Config: page  15  (0x08007800 - 0x08007FFF) = 2KB (rezervata)
 * SRAM:  6KB  (0x20000000 - 0x20001800)
 * ============================================================================ */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 30K   /* 30KB cod */
    CONFIG (r)      : ORIGIN = 0x08007800, LENGTH = 2K    /* 2KB configuratie */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 6K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* Top of RAM = top of stack */

_Min_Heap_Size  = 0x100;   /* Minim 256 bytes heap */
_Min_Stack_Size = 0x400;   /* Minim 1KB stack */

SECTIONS
{
    /* Tabela de vectori de intrerupere - trebuie sa fie la inceputul Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Cod si date read-only */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* Date initializate: copiate din Flash in RAM la startup */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT>FLASH

    /* Date neinitializate: zero la startup */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    /* Verificare overflow RAM */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}

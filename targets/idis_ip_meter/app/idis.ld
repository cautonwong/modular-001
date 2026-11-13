ENTRY(Reset_Handler)

MEMORY
{
    ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    .isr_vector : {
        KEEP(*(.isr_vector))
    } > ROM

    .text : {
        *(.text*)
        *(.rodata*)
    } > ROM

    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > ROM
    _sidata = LOADADDR(.data);

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    _estack = ORIGIN(RAM) + LENGTH(RAM);
    ASSERT(_ebss + 0x2000 <= _estack, "RAM overflow: bss plus 8 KiB stack")
}

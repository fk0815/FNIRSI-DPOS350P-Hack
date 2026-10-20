ENTRY(_start)
SECTIONS
{
    . = 0x40000000;
    .text : {
        startup.o(.text.start)
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }
}

MEMORY
{
    flash (rx)  : ORIGIN = 0x08000000, LENGTH = 128K
    ram   (rwx) : ORIGIN = 0x20000000, LENGTH = 36K
}

ENTRY(Reset_Handler)

_top_of_stack = ORIGIN(ram) + LENGTH(ram);

_Min_Heap_Size = 0x200; /* required amount of heap */
_Min_Stack_Size = 0x400; /* required amount of stack */


SECTIONS
{
    .text : 
    {
        . = ALIGN(4);
      /*  *(.isr_vector) */     /* Interrupt vector table */
		KEEP(*(.isr_vector)) /* Startup code */
        *(.text*)           /* Program code */
        *(.rodata*)         /* Read-only data */
        . = ALIGN(4);
        _etext = .;         /* Symbol to mark the end of .text */
    } > flash

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;         /* Create a symbol for the start of .data */
        *(.data*)           /* Initialized data */
        . = ALIGN(4);
        _edata = .;         /* Create a symbol for the end of .data */
    } > ram AT > flash

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;          /* Create a symbol for the start of .bss */
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;          /* Create a symbol for the end of .bss */
    } > ram
	
	/* This section can be used by the C library for heap memory */
    ._user_heap_stack :
    {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
    } > ram
}
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
  . = 0x100000;

  .text : {
    *(.text.boot)
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(COMMON)
    *(.bss*)
  }

  /DISCARD/ : { *(.eh_frame*) *(.comment*) }
}

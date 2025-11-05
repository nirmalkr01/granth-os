OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

/* Link high, load low */
KERNEL_VMA = 0xFFFFFFFF80000000;
KERNEL_LMA = 0x00100000;

PHDRS {
  text   PT_LOAD FLAGS(5); /* R | X */
  rodata PT_LOAD FLAGS(4); /* R     */
  data   PT_LOAD FLAGS(6); /* R | W */
}

SECTIONS {
  . = KERNEL_VMA;

  /* Export boundaries for PMM reservations */
  __kernel_start = .;

  .text ALIGN(0x1000) : AT (ADDR(.text) - KERNEL_VMA + KERNEL_LMA) {
    __text_start = .;
    *(.text.boot)
    *(.text*)
    __text_end = .;
  } :text
  . = ALIGN(0x1000);

  .rodata ALIGN(0x1000) : AT (ADDR(.rodata) - KERNEL_VMA + KERNEL_LMA) {
    __rodata_start = .;
    *(.rodata*)
    __rodata_end = .;
  } :rodata
  . = ALIGN(0x1000);

  .data ALIGN(0x1000) : AT (ADDR(.data) - KERNEL_VMA + KERNEL_LMA) {
    __data_start = .;
    *(.data*)
    *(.init_array*) *(.fini_array*)
    *(.ctors*) *(.dtors*)
    __data_end = .;
  } :data

  .bss ALIGN(0x1000) : AT (ADDR(.bss) - KERNEL_VMA + KERNEL_LMA) {
    __bss_start = .;
    *(COMMON)
    *(.bss*)
    __bss_end = .;
  } :data

  __kernel_end = .;

  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}

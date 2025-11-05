bits 64
default rel
global _start
extern kmain

section .text.boot
_start:
    lea     rsp, [rel stack_top]
    call    kmain
.hang:
    hlt
    jmp     .hang

section .bss
align 16
stack:      resb 65536
stack_top:

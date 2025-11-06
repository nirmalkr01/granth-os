/* src/kernel/arch/isr.S */
/* 64-bit IDT stubs (GAS, Intel syntax) */
    .intel_syntax noprefix
    .section .text
    .global isr_stub_table
    .extern isr_handler_c

/* ---------- Macros ---------- */
/* No-error-code exceptions */
.macro ISR_NOERR n
    .global isr\n
isr\n:
    push 0                /* fake error code */
    push \n               /* push vector */
    /* stack (top→bottom): vec, err, then CPU-pushed RIP, CS, RFLAGS, RSP, SS */
    mov rdi, [rsp]        /* vec */
    mov rsi, [rsp+8]      /* err */
    mov rdx, [rsp+16]     /* rip */
    call isr_handler_c
    add rsp, 16           /* pop vec + err */
    iretq
.endm

/* With-error-code exceptions (CPU already pushed err) */
.macro ISR_ERR n
    .global isr\n
isr\n:
    push \n               /* push vector (err already on stack) */
    mov rdi, [rsp]        /* vec */
    mov rsi, [rsp+8]      /* err */
    mov rdx, [rsp+16]     /* rip */
    call isr_handler_c
    add rsp, 8            /* pop vec (err stays for iretq to consume) */
    iretq
.endm

/* IRQ (no error code) */
.macro IRQ n
    .global irq\n
irq\n:
    push 0
    push \n
    mov rdi, [rsp]        /* vec */
    mov rsi, [rsp+8]      /* err=0 */
    mov rdx, [rsp+16]     /* rip */
    call isr_handler_c
    add rsp, 16
    iretq
.endm

/* ---------- Exceptions 0..31 ---------- */
ISR_NOERR 0   /* DE */
ISR_NOERR 1   /* DB */
ISR_NOERR 2   /* NMI */
ISR_NOERR 3   /* BP */
ISR_NOERR 4   /* OF */
ISR_NOERR 5   /* BR */
ISR_NOERR 6   /* UD */
ISR_NOERR 7   /* NM */
ISR_ERR   8   /* DF */
ISR_NOERR 9
ISR_ERR   10  /* TS */
ISR_ERR   11  /* NP */
ISR_ERR   12  /* SS */
ISR_ERR   13  /* GP */
ISR_ERR   14  /* PF */
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17  /* AC */
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

/* ---------- IRQ0 @ 0x20 ---------- */
IRQ 32

/* ---------- Stub table ---------- */
    .p2align 3
isr_stub_table:
    .quad isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    .quad isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    .quad isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    .quad isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    /* pad up to vector 0x20, then IRQ32 */
    .set _pad, 32
    .rept (0x20 - _pad)
      .quad 0
    .endr
    .quad irq32

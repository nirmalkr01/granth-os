#include <stdint.h>
#include <stddef.h>
#include "granth/arch/gdt.h"
#include "granth/arch/tss.h"

struct __attribute__((packed)) gdtr {
    uint16_t limit;
    uint64_t base;
};

static uint64_t gdt[6]; // 0:null, 1:kernel code, 2:kernel data, 3-4:TSS
extern struct tss64 _tss;

static void lgdt(void *gdtrp){
    __asm__ volatile ("lgdt (%0)" :: "r"(gdtrp));
    // Reload segments
    __asm__ volatile (
        "pushq $0x08 \n"
        "leaq 1f(%%rip), %%rax \n"
        "pushq %%rax \n"
        "lretq \n"
        "1:\n"
        "mov $0x10, %%ax \n"
        "mov %%ax, %%ds \n"
        "mov %%ax, %%es \n"
        "mov %%ax, %%ss \n"
        "mov %%ax, %%fs \n"
        "mov %%ax, %%gs \n"
        :
        :
        : "rax","memory"
    );
}

void gdt_init(void){
    // Null
    gdt[0] = 0;

    // Kernel code: base=0, limit=0, flags: 0xA09B? In long mode, use 0x00AF9A000000FFFF? But minimal:
    // 64-bit code descriptor: 0x00AF9A000000FFFF (Granular, 64-bit)
    gdt[1] = 0x00AF9A000000FFFFULL;
    // Kernel data: 0x00AF92000000FFFF
    gdt[2] = 0x00AF92000000FFFFULL;

    // TSS descriptor: 16 bytes (two entries)
    uint64_t tss_base = (uint64_t)&_tss;
    uint64_t tss_limit = sizeof(struct tss64)-1;

    uint64_t tss_lo =
        ((tss_limit & 0xFFFFULL)) |
        ((tss_base & 0xFFFFFFULL) << 16) |
        (0x89ULL << 40) |                 /* type=0x9 (AVL 64-bit TSS), present */
        ((tss_limit & 0xF0000ULL) << 32) |
        (((tss_base >> 24) & 0xFFULL) << 56);

    uint64_t tss_hi = (tss_base >> 32);

    gdt[3] = tss_lo;
    gdt[4] = tss_hi;

    struct gdtr gdtr = {
        .limit = sizeof(gdt)-1,
        .base  = (uint64_t)&gdt[0],
    };
    lgdt(&gdtr);

    // Load TR selector = 0x18 (gdt[3])
    __asm__ volatile ("ltr %0" :: "r"((uint16_t)0x18));
}

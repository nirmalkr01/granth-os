#include <stdint.h>
#include <stddef.h>
#include "granth/arch/idt.h"

/* --------- tiny serial logger (duplicates to keep M0 code decoupled) -------- */
#define COM1_PORT 0x3F8
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static void serial_init_once(void){
    static int inited=0; if (inited) return; inited=1;
    outb(COM1_PORT+1,0x00); outb(COM1_PORT+3,0x80);
    outb(COM1_PORT+0,0x03); outb(COM1_PORT+1,0x00);
    outb(COM1_PORT+3,0x03); outb(COM1_PORT+2,0xC7);
    outb(COM1_PORT+4,0x0B);
}
static void puts(const char* s){ serial_init_once(); while(*s){ while(!(inb(COM1_PORT+5)&0x20)){} outb(COM1_PORT, *s++);} }
static void puthex64(uint64_t v){
    static const char H[]="0123456789ABCDEF";
    puts("0x");
    for(int i=60;i>=0;i-=4) { char c=H[(v>>i)&0xF]; while(!(inb(COM1_PORT+5)&0x20)){} outb(COM1_PORT,c); }
}

/* ------------------------- IDT structures ---------------------------------- */
struct __attribute__((packed)) idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;       // 3 bits
    uint8_t  type_attr; // present, DPL, type
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
};
struct __attribute__((packed)) idtr {
    uint16_t limit;
    uint64_t base;
};

#define IDT_MAX 256
static struct idt_entry idt[IDT_MAX];

extern void* isr_stub_table[]; // from isr.S
static inline void lidt(void* idtrp){ __asm__ volatile("lidt (%0)"::"r"(idtrp):"memory"); }

/* Vector names for nice dumps */
static const char* exc_name[32] = {
    "DE Divide Error","DB Debug","NMI","BP Breakpoint","OF Overflow","BR BOUND","UD InvOpcode","NM NoMath",
    "DF DoubleFault","CSO CoprocSegOver","TS InvalidTSS","NP SegmentNP","SS StackSegFlt","GP GPF","PF PageFault",
    "RES","MF x87 FP","AC AlignCheck","MC MCE","XM SIMD FP","VE Virt","CP CPProtect","RES","RES","RES","RES","RES","RES","RES","RES","RES","RES"
};

static void set_gate(int vec, void* isr, int ist, uint8_t type_attr){
    uint64_t addr = (uint64_t)isr;
    idt[vec].offset_low  = addr & 0xFFFF;
    idt[vec].selector    = 0x08;      // kernel code selector
    idt[vec].ist         = ist & 0x7;
    idt[vec].type_attr   = type_attr; // 0x8E = present, DPL=0, type=14 (interrupt)
    idt[vec].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vec].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vec].zero        = 0;
}

/* C handler called from assembly stubs */
void isr_handler_c(uint64_t vec, uint64_t err, uint64_t rip){
    if (vec < 32){
        puts("[EXC] "); puts(exc_name[vec]); puts(" vec="); puthex64(vec);
        puts(" err="); puthex64(err); puts(" rip="); puthex64(rip); puts("\n");
        // hang on fault
        for(;;) __asm__ volatile("hlt");
    } else if (vec == 0x20){
        extern void pit_on_irq0(void);
        pit_on_irq0();
    } else {
        puts("[INT] vec="); puthex64(vec); puts("\n");
    }
}

void idt_init(void){
    // Clear IDT
    for (int i=0;i<IDT_MAX;i++){
        idt[i] = (struct idt_entry){0};
    }

    // Exceptions 0..31 on IST1 for safety (double-fault etc.)
    for (int vec=0; vec<=31; vec++){
        set_gate(vec, isr_stub_table[vec], 1, 0x8E);
    }

    // IRQ0 (PIT) at 0x20
    set_gate(0x20, isr_stub_table[0x20], 0, 0x8E);

    struct idtr idtr = { .limit = sizeof(idt)-1, .base = (uint64_t)&idt[0] };
    lidt(&idtr);
    puts("[IDT] ready\n");
}

void idt_test_div0(void){
    volatile int a = 1, b = 0;
    (void)(a/b);
}

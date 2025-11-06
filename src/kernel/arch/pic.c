#include <stdint.h>

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1
#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }

void pic_remap_and_mask(uint8_t off1, uint8_t off2){
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    outb(PIC1_DATA, off1);
    outb(PIC2_DATA, off2);

    outb(PIC1_DATA, 4); // tell master about slave at IRQ2
    outb(PIC2_DATA, 2); // tell slave its cascade id

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // mask all except IRQ0 (timer) on master; mask all on slave
    outb(PIC1_DATA, 0xFE); // 1111 1110 -> only IRQ0 enabled
    outb(PIC2_DATA, 0xFF);

    (void)a1; (void)a2;
}

void pic_eoi_master(void){
    outb(PIC1_CMD, 0x20);
}

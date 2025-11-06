// src/kernel/arch/pit.c
#include <stdint.h>
#include "granth/arch/timer.h"
#include "granth/arch/pic.h"

#define PIT_CH0       0x40
#define PIT_CMD       0x43
#define PIT_INPUT_HZ  1193182

static volatile uint64_t g_ticks = 0;
static volatile uint32_t g_hz    = 100;

static inline void outb(uint16_t p, uint8_t v){
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}

void pit_on_irq0(void){
    g_ticks++;
    pic_eoi_master();
}

uint64_t timer_ticks(void){
    return g_ticks;
}

void timer_init(uint32_t hz){
    if (hz < 20)   hz = 20;
    if (hz > 1000) hz = 1000;
    g_hz = hz;

    // Remap PIC to 0x20..0x2F and unmask IRQ0
    pic_remap_and_mask(0x20, 0x28);

    // Program PIT channel 0 in mode 3 (square wave)
    uint32_t divisor = PIT_INPUT_HZ / hz;
    outb(PIT_CMD, 0x36);                  // ch0, lobyte/hibyte, mode 3, binary
    outb(PIT_CH0, divisor & 0xFF);
    outb(PIT_CH0, (divisor >> 8) & 0xFF);

    g_ticks = 0;
}

void sleep_ms(uint64_t ms){
    // Busy-wait using HLT until enough ticks have elapsed
    uint64_t start = timer_ticks();
    uint64_t need  = (ms * g_hz + 999) / 1000;  // ceil(ms*hz/1000)
    while (timer_ticks() - start < need) {
        __asm__ volatile("hlt");
    }
}

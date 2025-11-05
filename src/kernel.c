#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "granth/hal.h"
#include "granth/boot_info.h"
#include "granth/mm/pmm.h"
#include "granth/mm/vmm.h"
#include "granth/mm/heap.h"

/* --- tiny serial logger (COM1) --- */
#define COM1_PORT 0x3F8
static inline void outb(uint16_t p, uint8_t v){ __asm__ volatile("outb %0,%1": :"a"(v),"Nd"(p)); }
static inline uint8_t inb(uint16_t p){ uint8_t r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static void serial_init(void){
    outb(COM1_PORT+1,0x00); outb(COM1_PORT+3,0x80);
    outb(COM1_PORT+0,0x03); outb(COM1_PORT+1,0x00);
    outb(COM1_PORT+3,0x03); outb(COM1_PORT+2,0xC7);
    outb(COM1_PORT+4,0x0B);
}
static int serial_ready(void){ return inb(COM1_PORT+5) & 0x20; }
static void serial_write(char c){ while(!serial_ready()){} outb(COM1_PORT, c); }
static void serial_print(const char *s){ while(*s) serial_write(*s++); }
static void serial_print_u64(const char* pfx, uint64_t v){
    static const char* H="0123456789ABCDEF";
    serial_print(pfx); serial_print("0x");
    for (int i=60;i>=0;i-=4){ serial_write(H[(v>>i)&0xF]); }
    serial_write('\n');
}

/* --- framebuffer helpers --- */
static struct limine_framebuffer *fb;
static uint8_t *fb_addr; static uint64_t fb_pitch, fb_w, fb_h;

static void fill(uint32_t color){
    for(uint64_t y=0;y<fb_h;++y){
        uint32_t *row=(uint32_t*)(fb_addr + y*fb_pitch);
        for(uint64_t x=0;x<fb_w;++x) row[x]=color;
    }
}
static void stripe(uint32_t color, uint64_t y0, uint64_t h){
    if (y0>=fb_h) return;
    uint64_t y1 = (y0+h>fb_h)? fb_h : y0+h;
    for (uint64_t y=y0;y<y1;y++){
        uint32_t *row=(uint32_t*)(fb_addr + y*fb_pitch);
        for(uint64_t x=0;x<fb_w;++x) row[x]=color;
    }
}

void kmain(void){
    serial_init();            serial_print("[BOOT] kmain\n");
    hal_init();               serial_print("[HAL] init ok\n");

    if(!hal.fb){ serial_print("[HAL][ERR] no FB\n"); for(;;)__asm__("hlt"); }
    fb = hal.fb;

    /* Use HHDM pointer for FB */
    fb_addr  = (uint8_t*)((uint64_t)fb->address + hal.hhdm_offset);
    fb_pitch = fb->pitch; fb_w = fb->width; fb_h = fb->height;

    serial_print("[FB] clear\n");
    fill(0x001E1E3A); /* dark bluish */

    /* PMM bring-up */
    pmm_init();
    struct pmm_stats ps = pmm_stats();
    serial_print_u64("[PMM] total_pages=", ps.total_pages);
    serial_print_u64("[PMM] free_pages =", ps.free_pages);

    /* Heap + smoke alloc */
    heap_init();
    void* t = kmalloc(256); (void)t;
    serial_print("[HEAP] ok\n");

    /* VMM: build our own PML4 and switch CR3 (keeps HHDM FB mapped) */
    vmm_init();
    serial_print("[VMM] CR3 switched\n");

    /* Visual acceptance: green stripe if all good */
    stripe(0x0000FF00, 32, 16); /* green bar */

    serial_print("[OK] halt\n");
    for(;;) __asm__ volatile("hlt");
}

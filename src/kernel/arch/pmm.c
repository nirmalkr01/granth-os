#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "granth/boot_info.h"
#include "granth/mm/pmm.h"

#define PAGE_SIZE 4096ULL
#define MAX_PHYS  (4ULL*1024*1024*1024) // support up to 4 GiB for now
#define PAGES_MAX (MAX_PHYS / PAGE_SIZE)
#define BITMAP_BYTES (PAGES_MAX / 8)

static uint8_t bitmap[BITMAP_BYTES];
static uint64_t total_pages, free_pages;

static inline void bset(uint64_t idx){ bitmap[idx>>3] |=  (1u << (idx&7)); }
static inline void bclr(uint64_t idx){ bitmap[idx>>3] &= ~(1u << (idx&7)); }
static inline int   bget(uint64_t idx){ return (bitmap[idx>>3] >> (idx&7)) & 1u; }

static inline uint64_t pa_to_idx(uint64_t pa){ return pa / PAGE_SIZE; }

static void reserve_range(uint64_t pa, uint64_t len) {
    uint64_t start = pa & ~(PAGE_SIZE-1);
    uint64_t end   = (pa + len + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    for(uint64_t p = start; p < end; p += PAGE_SIZE){
        if (p >= MAX_PHYS) break;
        uint64_t i = pa_to_idx(p);
        if (!bget(i)) { bset(i); if (free_pages) free_pages--; }
    }
}

static void free_range(uint64_t pa, uint64_t len) {
    uint64_t start = pa & ~(PAGE_SIZE-1);
    uint64_t end   = (pa + len + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    for(uint64_t p = start; p < end; p += PAGE_SIZE){
        if (p >= MAX_PHYS) break;
        uint64_t i = pa_to_idx(p);
        if (bget(i)) { bclr(i); free_pages++; }
    }
}

extern uint8_t __kernel_start, __kernel_end;
extern uint8_t __text_start, __text_end, __rodata_start, __rodata_end;
extern uint8_t __data_start, __data_end, __bss_start, __bss_end;

// Convert a linked VMA to physical using V2P: pa = va - VMA + LMA
#define KERNEL_VMA 0xFFFFFFFF80000000ULL
#define KERNEL_LMA 0x0000000000100000ULL
static inline uint64_t v2p(const void* va){
    return (uint64_t)va - KERNEL_VMA + KERNEL_LMA;
}

void pmm_init(void){
    // Mark everything reserved
    for (size_t i=0;i<BITMAP_BYTES;i++) bitmap[i]=0xFF;

    total_pages = PAGES_MAX;
    free_pages  = 0;

    // Free all USABLE memory reported by Limine (within our MAX_PHYS window)
    if (g_boot.memmap){
        for (uint64_t i=0;i<g_boot.memmap->entry_count;i++){
            struct limine_memmap_entry* e = g_boot.memmap->entries[i];
            if (e->type == LIMINE_MEMMAP_USABLE){
                uint64_t base = e->base;
                uint64_t len  = e->length;
                if (base >= MAX_PHYS) continue;
                if (base + len > MAX_PHYS) len = MAX_PHYS - base;
                free_range(base, len);
            }
        }
    }

    // Reserve low 1 MiB
    reserve_range(0, 0x100000);

    // Reserve kernel image
    reserve_range(v2p(&__kernel_start), (uint64_t)(&__kernel_end) - (uint64_t)(&__kernel_start));

    // Reserve framebuffer if present
    if (g_boot.fb_phys && g_boot.fb_size){
        reserve_range(g_boot.fb_phys, g_boot.fb_size);
    }
}

uint64_t pmm_alloc_page(void){
    for (uint64_t i=0;i<PAGES_MAX;i++){
        if (!bget(i)){ bset(i); if (free_pages) free_pages--; return i * PAGE_SIZE; }
    }
    return 0;
}

void pmm_free_page(uint64_t pa){
    if (pa >= MAX_PHYS || (pa & (PAGE_SIZE-1))) return;
    uint64_t i = pa_to_idx(pa);
    if (bget(i)){ bclr(i); free_pages++; }
}

struct pmm_stats pmm_stats(void){
    struct pmm_stats s;
    s.total_pages = total_pages;
    s.free_pages  = free_pages;
    s.used_pages  = total_pages - free_pages;
    return s;
}

#include <stdint.h>
#include <stddef.h>
#include "granth/mm/vmm.h"
#include "granth/mm/pmm.h"
#include "granth/boot_info.h"

#define PAGE_SIZE 4096ULL
#define PML_SHIFT 39
#define PDP_SHIFT 30
#define PD_SHIFT  21
#define PT_SHIFT  12
#define IDX(va,shift) (((va) >> (shift)) & 0x1FF)

#define PTE_P   (1ULL<<0)
#define PTE_W   (1ULL<<1)
#define PTE_UC  ((1ULL<<3) | (1ULL<<4)) /* PWT|PCD */
#define PTE_XD  (1ULL<<63)

static inline void invlpg(uint64_t va){ __asm__ volatile("invlpg (%0)"::"r"(va):"memory"); }

/* Ownership: all page tables come from PMM; we zero them on alloc */
static uint64_t new_page_zero(void){
    uint64_t pa = pmm_alloc_page();
    if (!pa) return 0;
    volatile uint64_t *p = (volatile uint64_t *)(g_boot.hhdm_offset + pa);
    for (size_t i=0;i<PAGE_SIZE/8;i++) p[i]=0;
    return pa;
}

static uint64_t pml4_pa = 0;

/* Map one 4K page */
static int map4k(uint64_t va, uint64_t pa, uint64_t flags){
    if (!pml4_pa) return -1;
    uint64_t pml4_va = g_boot.hhdm_offset + pml4_pa;

    uint64_t *pml4 = (uint64_t*)pml4_va;
    uint64_t i4 = IDX(va, PML_SHIFT);
    uint64_t i3 = IDX(va, PDP_SHIFT);
    uint64_t i2 = IDX(va, PD_SHIFT);
    uint64_t i1 = IDX(va, PT_SHIFT);

    if (!(pml4[i4] & PTE_P)){
        uint64_t pa3 = new_page_zero(); if (!pa3) return -1;
        pml4[i4] = pa3 | PTE_P | PTE_W;
    }
    uint64_t *pdp = (uint64_t*)(g_boot.hhdm_offset + (pml4[i4] & ~0xFFFULL));

    if (!(pdp[i3] & PTE_P)){
        uint64_t pa2 = new_page_zero(); if (!pa2) return -1;
        pdp[i3] = pa2 | PTE_P | PTE_W;
    }
    uint64_t *pd = (uint64_t*)(g_boot.hhdm_offset + (pdp[i3] & ~0xFFFULL));

    if (!(pd[i2] & PTE_P)){
        uint64_t pa1 = new_page_zero(); if (!pa1) return -1;
        pd[i2] = pa1 | PTE_P | PTE_W;
    }
    uint64_t *pt = (uint64_t*)(g_boot.hhdm_offset + (pd[i2] & ~0xFFFULL));

    uint64_t pte = pa | PTE_P;
    if (flags & VMM_WRITE)  pte |= PTE_W;
    if (flags & VMM_UC)     pte |= PTE_UC;
    if (flags & VMM_NOEXEC) pte |= PTE_XD;

    pt[i1] = pte;
    invlpg(va);
    return 0;
}

int vmm_map_range(uint64_t va, uint64_t pa, uint64_t len, uint64_t flags){
    for (uint64_t off = 0; off < len; off += PAGE_SIZE){
        if (map4k(va+off, pa+off, flags) != 0) return -1;
    }
    return 0;
}

void vmm_init(void){
    /* Build brand new PML4 */
    pml4_pa = new_page_zero();

    /* 1) Identity map first 2 MiB (UEFI services / early DMA) RW */
    vmm_map_range(0x0, 0x0, 0x200000, VMM_WRITE | VMM_NOEXEC);

    /* 2) Map kernel higher-half: [__kernel_start .. __kernel_end] */
    extern uint8_t __kernel_start, __kernel_end;
    extern uint8_t __text_start, __text_end, __rodata_start, __rodata_end;
    extern uint8_t __data_start, __data_end, __bss_start, __bss_end;

    const uint64_t KERNEL_VMA = 0xFFFFFFFF80000000ULL;
    const uint64_t KERNEL_LMA = 0x0000000000100000ULL;

    uint64_t k_va  = (uint64_t)&__kernel_start;
    uint64_t k_end = (uint64_t)&__kernel_end;
    uint64_t k_len = (k_end - k_va + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
    uint64_t k_pa  = k_va - KERNEL_VMA + KERNEL_LMA;

    /* Map everything RWX for now (keep it simple for M1). W^X later. */
    vmm_map_range(k_va, k_pa, k_len, VMM_WRITE);

    /* 3) Map HHDM subsection for framebuffer only (uncached) */
    if (g_boot.fb_phys && g_boot.fb_size){
        uint64_t fb_pa = g_boot.fb_phys & ~(PAGE_SIZE-1);
        uint64_t fb_va = fb_pa + g_boot.hhdm_offset;
        uint64_t fb_len= ((g_boot.fb_size + (g_boot.fb_phys & (PAGE_SIZE-1))) + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
        vmm_map_range(fb_va, fb_pa, fb_len, VMM_WRITE | VMM_UC | VMM_NOEXEC);
    }

    /* Load CR3 with the new PML4 physical address */
    __asm__ volatile("mov %0, %%cr3"::"r"(pml4_pa):"memory");
}

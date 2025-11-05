#pragma once
#include <stdint.h>
#include <stddef.h>

void vmm_init(void);

/* map a VA->PA range with flags; length in bytes (multiple of 4K) */
enum vmm_flags {
    VMM_WRITE = 1 << 0,
    VMM_NOEXEC= 1 << 1,
    VMM_UC    = 1 << 2,   // Uncached (PWT|PCD)
};
int  vmm_map_range(uint64_t va, uint64_t pa, uint64_t len, uint64_t flags);

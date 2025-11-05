#pragma once
#include <stdint.h>
#include <stddef.h>

void     pmm_init(void);
uint64_t pmm_alloc_page(void);        // returns physical address (4K-aligned) or 0
void     pmm_free_page(uint64_t pa);  // pa must be 4K-aligned

struct pmm_stats {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t used_pages;
};
struct pmm_stats pmm_stats(void);

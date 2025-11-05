#pragma once
#include <stdint.h>
#include "limine.h"

struct boot_info {
    const struct limine_memmap_response *memmap;
    uint64_t hhdm_offset;

    struct limine_framebuffer *fb;
    uint64_t fb_phys;
    uint64_t fb_size;

    void *rsdp;
    uint64_t acpi_rev;
};

extern struct boot_info g_boot;

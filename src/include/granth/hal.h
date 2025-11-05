#pragma once
#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "granth/boot_info.h"

struct hal_info {
    const struct limine_memmap_response *memmap;
    struct limine_framebuffer *fb;
    uint64_t hhdm_offset;
    void *rsdp;
    uint64_t acpi_rev;
};

extern struct hal_info hal;

void hal_init(void);

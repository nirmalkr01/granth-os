#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "granth/hal.h"
#include "granth/boot_info.h"

LIMINE_REQUESTS_START_MARKER
volatile struct limine_memmap_request       memmap_request      = { .id = LIMINE_MEMMAP_REQUEST,      .revision = 0 };
volatile struct limine_framebuffer_request  framebuffer_request = { .id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0 };
volatile struct limine_hhdm_request         hhdm_request        = { .id = LIMINE_HHDM_REQUEST,        .revision = 0 };
volatile struct limine_rsdp_request         rsdp_request        = { .id = LIMINE_RSDP_REQUEST,        .revision = 0 };
LIMINE_REQUESTS_END_MARKER

struct hal_info hal;
struct boot_info g_boot;

void hal_init(void) {
    hal.memmap = memmap_request.response;
    hal.fb = (framebuffer_request.response && framebuffer_request.response->framebuffer_count)
           ? framebuffer_request.response->framebuffers[0] : NULL;
    hal.hhdm_offset = hhdm_request.response ? hhdm_request.response->offset : 0;
    hal.rsdp = rsdp_request.response ? rsdp_request.response->address : NULL;
    hal.acpi_rev = rsdp_request.response ? rsdp_request.response->revision : 0;

    if (!hal.memmap || hal.memmap->entry_count == 0) for(;;) __asm__ volatile("hlt");

    // Fill boot_info
    g_boot.memmap = hal.memmap;
    g_boot.hhdm_offset = hal.hhdm_offset;
    g_boot.fb = hal.fb;
    if (hal.fb) {
        g_boot.fb_phys = (uint64_t)hal.fb->address;
        g_boot.fb_size = hal.fb->height * hal.fb->pitch;
    } else {
        g_boot.fb_phys = 0; g_boot.fb_size = 0;
    }
    g_boot.rsdp = hal.rsdp;
    g_boot.acpi_rev = hal.acpi_rev;
}

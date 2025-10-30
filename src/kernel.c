#include <stdint.h>
#include <stddef.h>
#include "limine.h"

/* ----- Limine request list markers (required by modern limine.h) ----- */
LIMINE_REQUESTS_START_MARKER

/* Request a framebuffer from Limine */
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

LIMINE_REQUESTS_END_MARKER
/* -------------------------------------------------------------------- */

static inline void putpixel(uint8_t *fb, uint64_t pitch, uint64_t x, uint64_t y, uint32_t rgb) {
    uint32_t *p = (uint32_t *)(fb + y * pitch + x * 4);
    *p = rgb;
}

static void draw_string(uint8_t *fb, uint64_t pitch, uint64_t w, uint64_t h,
                        const char *s, uint64_t x, uint64_t y) {
    // Super-minimal: draw each character as a 6x9 block (placeholder)
    uint32_t color = 0x00ffffff; // white
    for (const char *c = s; *c; ++c) {
        (void)c; // no font yet
        for (uint64_t yy = 0; yy < 9; ++yy)
            for (uint64_t xx = 0; xx < 6; ++xx)
                if (x+xx < w && y+yy < h)
                    putpixel(fb, pitch, x+xx, y+yy, color);
        x += 8;
    }
}

void kmain(void) {
    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        for (;;){ __asm__ volatile("hlt"); }
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    uint8_t *base = fb->address;
    uint64_t pitch = fb->pitch;
    uint64_t w = fb->width;
    uint64_t h = fb->height;

    // Fill background
    for (uint64_t yy = 0; yy < h; ++yy) {
        uint32_t *row = (uint32_t *)(base + yy * pitch);
        for (uint64_t xx = 0; xx < w; ++xx) row[xx] = 0x00222222; // dark gray
    }

    draw_string(base, pitch, w, h,
                "Granth OS v0 (UEFI + Limine) : Hello, World!", 50, 50);

    for (;;){ __asm__ volatile("hlt"); }
}

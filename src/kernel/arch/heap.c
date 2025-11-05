#include <stdint.h>
#include <stddef.h>
#include "granth/mm/heap.h"

static uint8_t  heap_area[64 * 1024];
static size_t   heap_off = 0;

void heap_init(void) { heap_off = 0; }
void* kmalloc(size_t n) {
    if (!n) return NULL;
    n = (n + 15) & ~((size_t)15);
    if (heap_off + n > sizeof(heap_area)) return NULL;
    void* p = &heap_area[heap_off];
    heap_off += n;
    return p;
}
void kfree(void* p){ (void)p; } // bump allocator has no free

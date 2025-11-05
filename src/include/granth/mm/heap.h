#pragma once
#include <stddef.h>
void  heap_init(void);
void* kmalloc(size_t n);
void  kfree(void* p); // no-op in bump impl

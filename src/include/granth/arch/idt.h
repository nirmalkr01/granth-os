#pragma once
#include <stdint.h>

void idt_init(void);

/* Trigger a test fault, e.g., divide-by-zero, for diagnostics */
void idt_test_div0(void);

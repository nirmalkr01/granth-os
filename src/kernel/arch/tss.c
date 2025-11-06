#include <stdint.h>
#include "granth/arch/tss.h"

struct tss64 _tss __attribute__((aligned(16)));

// 2 separate stacks: RSP0 for interrupts in kernel; IST1 for double-fault
static uint8_t ist1_stack[16*1024] __attribute__((aligned(16)));
static uint8_t rsp0_stack[16*1024] __attribute__((aligned(16)));

void tss_init(void){
    // Set kernel RSP0 and IST1
    _tss.rsp0 = (uint64_t)(rsp0_stack + sizeof(rsp0_stack));
    _tss.ist1 = (uint64_t)(ist1_stack + sizeof(ist1_stack));
    _tss.iopb_offset = sizeof(struct tss64); // no IO bitmap
}

#pragma once
#include <stdint.h>

void     timer_init(uint32_t hz);     // init periodic timer (PIT) at hz
uint64_t timer_ticks(void);           // monotonically increasing tick count
void     sleep_ms(uint64_t ms);       // busy-wait using ticks (simple)

#pragma once
#include <stdint.h>
void pic_remap_and_mask(uint8_t master_offset, uint8_t slave_offset);
void pic_eoi_master(void);

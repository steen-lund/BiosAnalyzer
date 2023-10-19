#pragma once

#include <stdint.h>

uintptr_t allocateIn4GBRange(uint32_t size);
void freeIn4GBRange(void* addr, uint32_t size);

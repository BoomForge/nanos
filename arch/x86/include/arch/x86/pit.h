#ifndef ARCH_X86_PIT_H
#define ARCH_X86_PIT_H

#include <kernel/types.h>

void pit_init(uint32_t frequency);
uint32_t pit_ticks(void);

#endif

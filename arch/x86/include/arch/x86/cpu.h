#ifndef ARCH_X86_CPU_H
#define ARCH_X86_CPU_H

#include <kernel/types.h>

void x86_enable_interrupts(void);
void x86_disable_interrupts(void);
void x86_halt(void);
uint32_t x86_read_cr2(void);
void x86_load_cr3(uint32_t addr);
void x86_enable_paging(void);

#endif

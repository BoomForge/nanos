#ifndef ARCH_X86_PORTS_H
#define ARCH_X86_PORTS_H

#include <kernel/types.h>

uint8_t x86_inb(uint16_t port);
void x86_outb(uint16_t port, uint8_t value);

#endif

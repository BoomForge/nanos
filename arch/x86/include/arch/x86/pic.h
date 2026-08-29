#ifndef ARCH_X86_PIC_H
#define ARCH_X86_PIC_H

#include <kernel/types.h>

#define PIC_IRQ_BASE 32u

void pic_remap(void);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);
void pic_send_eoi(uint8_t irq);

#endif

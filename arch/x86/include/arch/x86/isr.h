#ifndef ARCH_X86_ISR_H
#define ARCH_X86_ISR_H

#include <kernel/types.h>

struct interrupt_frame {
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t saved_esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t user_ss;
};

typedef void (*interrupt_handler_t)(struct interrupt_frame *frame);

void isr_init(void);
void isr_set_handler(uint8_t vector, interrupt_handler_t handler);
void x86_interrupt_dispatch(struct interrupt_frame *frame);

#endif

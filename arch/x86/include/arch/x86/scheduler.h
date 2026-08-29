#ifndef ARCH_X86_SCHEDULER_H
#define ARCH_X86_SCHEDULER_H

#include <arch/x86/isr.h>
#include <kernel/types.h>

void x86_scheduler_preempt(struct interrupt_frame *frame);

#endif

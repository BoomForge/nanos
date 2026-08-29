#ifndef ARCH_PROCESS_CONTEXT_H
#define ARCH_PROCESS_CONTEXT_H

#include <kernel/types.h>

#define PLATFORM_PROCESS_CONTEXT_SIZE 40u
#define PLATFORM_PROCESS_CONTEXT_ALIGN 4u

struct platform_process_context {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t esp;
};

#endif

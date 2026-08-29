#include <arch/x86/isr.h>
#include <arch/x86/syscall.h>
#include <kernel/syscall.h>
#include <kernel/types.h>

static void syscall_interrupt(struct interrupt_frame *frame)
{
    frame->eax = syscall_dispatch(frame->eax, frame->ebx, frame->ecx,
        frame->edx, frame->esi);
}

void x86_syscall_init(void)
{
    isr_set_handler(0x80u, syscall_interrupt);
}

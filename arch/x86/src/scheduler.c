#include <arch/x86/isr.h>
#include <arch/x86/scheduler.h>
#include <nanos/syscall.h>
#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/types.h>

static int frame_from_user(const struct interrupt_frame *frame)
{
    return (frame->cs & 0x3u) == 0x3u;
}

static struct platform_process_context *process_context(struct process *process)
{
    return (struct platform_process_context *)process->context;
}

static const struct platform_process_context *const_process_context(
    const struct process *process)
{
    return (const struct platform_process_context *)process->context;
}

static void save_context(struct process *process,
    const struct interrupt_frame *frame)
{
    struct platform_process_context *context;

    context = process_context(process);
    context->eax = frame->eax;
    context->ebx = frame->ebx;
    context->ecx = frame->ecx;
    context->edx = frame->edx;
    context->esi = frame->esi;
    context->edi = frame->edi;
    context->ebp = frame->ebp;
    context->eip = frame->eip;
    context->eflags = frame->eflags;
    context->esp = frame->user_esp;
    process->context_started = 1;
}

static void load_context(struct interrupt_frame *frame,
    const struct process *process)
{
    const struct platform_process_context *context;

    context = const_process_context(process);
    frame->eax = context->eax;
    frame->ebx = context->ebx;
    frame->ecx = context->ecx;
    frame->edx = context->edx;
    frame->esi = context->esi;
    frame->edi = context->edi;
    frame->ebp = context->ebp;
    frame->eip = context->eip;
    frame->eflags = context->eflags;
    frame->user_esp = context->esp;
}

static void switch_from_frame(struct interrupt_frame *frame,
    struct process *(*pick_next)(void))
{
    struct process *current;
    struct process *next;

    if (!frame_from_user(frame)) {
        return;
    }

    current = process_current();
    if (current == NULL) {
        return;
    }

    save_context(current, frame);
    next = pick_next();
    if (next == NULL || next == current) {
        return;
    }

    platform_user_switch(next);
    load_context(frame, next);
}

void x86_scheduler_preempt(struct interrupt_frame *frame)
{
    switch_from_frame(frame, process_on_preempt);
}

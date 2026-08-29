#include <arch/x86/isr.h>
#include <arch/x86/pic.h>
#include <arch/x86/pit.h>
#include <arch/x86/ports.h>
#include <arch/x86/scheduler.h>
#include <kernel/types.h>

#define PIT_FREQUENCY 1193182u
#define PIT_COMMAND 0x43u
#define PIT_CHANNEL0 0x40u

static volatile uint32_t tick_count;

static void pit_irq(struct interrupt_frame *frame)
{
    ++tick_count;
    x86_scheduler_preempt(frame);
}

void pit_init(uint32_t frequency)
{
    uint32_t divisor;

    if (frequency == 0u) {
        frequency = 100u;
    }

    divisor = PIT_FREQUENCY / frequency;
    if (divisor == 0u) {
        divisor = 1u;
    }
    if (divisor > 65535u) {
        divisor = 65535u;
    }

    isr_set_handler(PIC_IRQ_BASE + 0u, pit_irq);

    x86_outb(PIT_COMMAND, 0x36u);
    x86_outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xffu));
    x86_outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8u) & 0xffu));
    pic_clear_mask(0u);
}

uint32_t pit_ticks(void)
{
    return tick_count;
}

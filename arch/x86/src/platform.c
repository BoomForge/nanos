#include <arch/x86/cpu.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <arch/x86/isr.h>
#include <arch/x86/keyboard.h>
#include <arch/x86/mouse.h>
#include <arch/x86/paging.h>
#include <arch/x86/pic.h>
#include <arch/x86/pit.h>
#include <arch/x86/serial.h>
#include <arch/x86/syscall.h>
#include <arch/x86/vga_text.h>
#include <kernel/platform.h>
#include <kernel/types.h>

static void mask_all_irqs(void)
{
    uint8_t irq;

    for (irq = 0u; irq < 16u; ++irq) {
        pic_set_mask(irq);
    }
}

void platform_disable_interrupts(void)
{
    x86_disable_interrupts();
}

void platform_enable_interrupts(void)
{
    x86_enable_interrupts();
}

void platform_halt(void)
{
    x86_halt();
}

void platform_console_init(void)
{
    serial_init();
    vga_text_clear();
}

void platform_console_putc(char ch)
{
    if (ch == '\n') {
        serial_putc('\r');
    }
    serial_putc(ch);
    vga_text_putc(ch);
}

void platform_cpu_init(void)
{
    gdt_init();
}

void platform_interrupts_init(void)
{
    idt_init();
    isr_init();
    idt_load();
    pic_remap();
    mask_all_irqs();
    x86_syscall_init();
}

void platform_paging_init(void)
{
    paging_init();
}

void platform_input_init(void)
{
    keyboard_init();
    mouse_init();
}

void platform_timer_init(uint32_t ticks_per_second)
{
    pit_init(ticks_per_second);
}

uint32_t platform_timer_ticks(void)
{
    return pit_ticks();
}

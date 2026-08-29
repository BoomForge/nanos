#include <arch/x86/idt.h>
#include <arch/x86/isr.h>
#include <arch/x86/pic.h>
#include <kernel/panic.h>
#include <kernel/print.h>
#include <kernel/types.h>

#define IDT_KERNEL_CODE 0x08u
#define IDT_INTERRUPT_GATE 0x8eu
#define IDT_USER_INTERRUPT_GATE 0xeeu
#define ISR_COUNT 48u

static interrupt_handler_t handlers[256];

void isr0(void);
void isr1(void);
void isr2(void);
void isr3(void);
void isr4(void);
void isr5(void);
void isr6(void);
void isr7(void);
void isr8(void);
void isr9(void);
void isr10(void);
void isr11(void);
void isr12(void);
void isr13(void);
void isr14(void);
void isr15(void);
void isr16(void);
void isr17(void);
void isr18(void);
void isr19(void);
void isr20(void);
void isr21(void);
void isr22(void);
void isr23(void);
void isr24(void);
void isr25(void);
void isr26(void);
void isr27(void);
void isr28(void);
void isr29(void);
void isr30(void);
void isr31(void);
void isr32(void);
void isr33(void);
void isr34(void);
void isr35(void);
void isr36(void);
void isr37(void);
void isr38(void);
void isr39(void);
void isr40(void);
void isr41(void);
void isr42(void);
void isr43(void);
void isr44(void);
void isr45(void);
void isr46(void);
void isr47(void);
void isr128(void);

static void (*const isr_stubs[ISR_COUNT])(void) = {
    isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
    isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
    isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
};

void isr_set_handler(uint8_t vector, interrupt_handler_t handler)
{
    handlers[vector] = handler;
}

void isr_init(void)
{
    uint32_t i;

    for (i = 0u; i < ISR_COUNT; ++i) {
        idt_set_gate((uint8_t)i, (uint32_t)(uintptr_t)isr_stubs[i],
            IDT_KERNEL_CODE, IDT_INTERRUPT_GATE);
    }
    idt_set_gate(0x80u, (uint32_t)(uintptr_t)isr128,
        IDT_KERNEL_CODE, IDT_USER_INTERRUPT_GATE);
}

void x86_interrupt_dispatch(struct interrupt_frame *frame)
{
    interrupt_handler_t handler;

    handler = handlers[frame->vector];
    if (handler != NULL) {
        handler(frame);
    } else if (frame->vector < 32u) {
        print_write("cpu exception ");
        print_uint(frame->vector);
        print_write(" error ");
        print_hex32(frame->error_code);
        print_write(" eip ");
        print_hex32(frame->eip);
        print_putc('\n');
        panic("unhandled CPU exception");
    }

    if (frame->vector >= PIC_IRQ_BASE && frame->vector < PIC_IRQ_BASE + 16u) {
        pic_send_eoi((uint8_t)(frame->vector - PIC_IRQ_BASE));
    }
}

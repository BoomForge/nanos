#include <arch/x86/pic.h>
#include <arch/x86/ports.h>
#include <kernel/types.h>

#define PIC1_COMMAND 0x20u
#define PIC1_DATA 0x21u
#define PIC2_COMMAND 0xa0u
#define PIC2_DATA 0xa1u
#define PIC_EOI 0x20u

static void io_wait(void)
{
    x86_outb(0x80u, 0u);
}

void pic_remap(void)
{
    uint8_t master_mask;
    uint8_t slave_mask;

    master_mask = x86_inb(PIC1_DATA);
    slave_mask = x86_inb(PIC2_DATA);

    x86_outb(PIC1_COMMAND, 0x11u);
    io_wait();
    x86_outb(PIC2_COMMAND, 0x11u);
    io_wait();

    x86_outb(PIC1_DATA, PIC_IRQ_BASE);
    io_wait();
    x86_outb(PIC2_DATA, PIC_IRQ_BASE + 8u);
    io_wait();

    x86_outb(PIC1_DATA, 0x04u);
    io_wait();
    x86_outb(PIC2_DATA, 0x02u);
    io_wait();

    x86_outb(PIC1_DATA, 0x01u);
    io_wait();
    x86_outb(PIC2_DATA, 0x01u);
    io_wait();

    x86_outb(PIC1_DATA, master_mask);
    x86_outb(PIC2_DATA, slave_mask);
}

void pic_set_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    port = (irq < 8u) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8u) {
        irq = (uint8_t)(irq - 8u);
    }

    value = (uint8_t)(x86_inb(port) | (uint8_t)(1u << irq));
    x86_outb(port, value);
}

void pic_clear_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t value;

    port = (irq < 8u) ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8u) {
        irq = (uint8_t)(irq - 8u);
    }

    value = (uint8_t)(x86_inb(port) & (uint8_t)~(1u << irq));
    x86_outb(port, value);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8u) {
        x86_outb(PIC2_COMMAND, PIC_EOI);
    }
    x86_outb(PIC1_COMMAND, PIC_EOI);
}

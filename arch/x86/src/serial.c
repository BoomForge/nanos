#include <arch/x86/ports.h>
#include <arch/x86/serial.h>

#define COM1_PORT 0x3f8u

static int serial_ready;

int serial_init(void)
{
    x86_outb((uint16_t)(COM1_PORT + 1u), 0x00u);
    x86_outb((uint16_t)(COM1_PORT + 3u), 0x80u);
    x86_outb((uint16_t)(COM1_PORT + 0u), 0x03u);
    x86_outb((uint16_t)(COM1_PORT + 1u), 0x00u);
    x86_outb((uint16_t)(COM1_PORT + 3u), 0x03u);
    x86_outb((uint16_t)(COM1_PORT + 2u), 0xc7u);
    x86_outb((uint16_t)(COM1_PORT + 4u), 0x0bu);

    serial_ready = 1;
    return 0;
}

static int serial_transmit_empty(void)
{
    return (x86_inb((uint16_t)(COM1_PORT + 5u)) & 0x20u) != 0u;
}

void serial_putc(char ch)
{
    if (!serial_ready) {
        return;
    }

    while (!serial_transmit_empty()) {
    }

    x86_outb(COM1_PORT, (uint8_t)ch);
}

void serial_write(const char *text)
{
    while (*text != '\0') {
        if (*text == '\n') {
            serial_putc('\r');
        }
        serial_putc(*text);
        ++text;
    }
}

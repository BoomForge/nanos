#include <kernel/platform.h>
#include <kernel/print.h>

void print_init(void)
{
    platform_console_init();
}

void print_putc(char ch)
{
    platform_console_putc(ch);
}

void print_write(const char *text)
{
    while (*text != '\0') {
        print_putc(*text);
        ++text;
    }
}

void print_writeln(const char *text)
{
    print_write(text);
    print_putc('\n');
}

void print_hex32(uint32_t value)
{
    static const char digits[] = "0123456789abcdef";
    int shift;

    print_write("0x");
    for (shift = 28; shift >= 0; shift -= 4) {
        print_putc(digits[(value >> (uint32_t)shift) & 0x0fu]);
    }
}

void print_uint(uint32_t value)
{
    char buffer[11];
    uint32_t i;

    if (value == 0u) {
        print_putc('0');
        return;
    }

    i = 0u;
    while (value > 0u && i < sizeof(buffer)) {
        buffer[i] = (char)('0' + (value % 10u));
        value /= 10u;
        ++i;
    }

    while (i > 0u) {
        --i;
        print_putc(buffer[i]);
    }
}

#include <kernel/panic.h>
#include <kernel/print.h>

void panic(const char *message)
{
    print_write("panic: ");
    print_writeln(message);

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}

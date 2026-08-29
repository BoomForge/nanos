#include <kernel/init.h>
#include <kernel/print.h>
#include <kernel/process.h>

#define INIT_FIRST_APP "guidemo.nx"

void kernel_init_start_first_app(void)
{
    print_writeln("starting userspace gui demo");
    if (process_start_app(INIT_FIRST_APP, "") >= 0) {
        (void)process_schedule();
    } else {
        print_writeln("userspace gui demo not found");
    }
}

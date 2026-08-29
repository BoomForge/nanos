#include <kernel/fb_console.h>
#include <kernel/block.h>
#include <kernel/debug/selftest.h>
#include <kernel/desktop/desktop.h>
#include <kernel/framebuffer.h>
#include <kernel/heap.h>
#include <kernel/init.h>
#include <kernel/input.h>
#include <kernel/debug/monitor.h>
#include <kernel/multiboot2.h>
#include <kernel/panic.h>
#include <kernel/platform.h>
#include <kernel/pmm.h>
#include <kernel/print.h>
#include <kernel/process.h>
#include <kernel/ramdisk.h>
#include <kernel/tarfs.h>
#include <kernel/time.h>
#include <kernel/types.h>
#include <kernel/vfs.h>

void kmain(uint32_t magic, uint32_t multiboot_info)
{
    uint32_t ticks;
    uint32_t second;
    uint32_t last_second;
    int timer_announced;

    last_second = 0u;
    timer_announced = 0;

    platform_disable_interrupts();

    print_init();
    print_writeln("NanOS booting");

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        print_write("bad boot magic ");
        print_hex32(magic);
        print_putc('\n');
        panic("kernel was not started by a Multiboot2 loader");
    }

    print_write("multiboot info ");
    print_hex32(multiboot_info);
    print_putc('\n');

    if (framebuffer_init_from_multiboot(multiboot_info) == 0) {
        print_writeln("framebuffer initialized");
        fb_console_init(0u, 0u, framebuffer_width(), framebuffer_height());
    } else {
        print_writeln("no 32-bit framebuffer, using VGA text");
    }

    platform_cpu_init();
    print_writeln("platform cpu initialized");

    platform_interrupts_init();
    print_writeln("platform interrupts initialized");

    pmm_init(multiboot_info);
    pmm_print_summary();

    platform_paging_init();

    heap_init(16u);
    heap_print_summary();
#if NANOS_BOOT_SELFTESTS
    debug_selftest_run_boot_checks();
#endif

    block_init();
    ramdisk_init_from_multiboot(multiboot_info);
    vfs_init();
    if (tarfs_mount(block_first()) == 0) {
        print_writeln("vfs root mounted");
    } else {
        print_writeln("vfs root mount failed");
    }

    kernel_time_init(100u);
    process_init();
    platform_input_init();
    kernel_input_init();
    desktop_init();

    platform_enable_interrupts();
    print_writeln("interrupts enabled");

    if (!desktop_is_ready()) {
        print_writeln("desktop unavailable, starting monitor");
        monitor_start();
    } else {
        kernel_init_start_first_app();
    }

    print_writeln("halt loop entered");
    for (;;) {
        platform_halt();
        ticks = kernel_time_ticks();
        second = ticks / kernel_time_ticks_per_second();

        if (second != last_second) {
            last_second = second;
            if (!timer_announced) {
                print_writeln("timer ticking");
                timer_announced = 1;
            }
        }
    }
}

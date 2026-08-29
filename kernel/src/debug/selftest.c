#include <kernel/debug/selftest.h>
#include <kernel/fb_console.h>
#include <kernel/heap.h>
#include <kernel/pmm.h>
#include <kernel/print.h>
#include <kernel/types.h>

static void test_page_allocator(void)
{
    uint32_t page;

    page = pmm_alloc_page();
    if (page == 0u) {
        print_writeln("pmm test allocation failed");
        fb_console_writeln("PMM alloc failed");
        return;
    }

    print_write("pmm test page ");
    print_hex32(page);
    print_putc('\n');

    fb_console_write("PMM page ");
    fb_console_write("ok");
    fb_console_putc('\n');

    pmm_free_page(page);
}

static void test_heap(void)
{
    char *text;
    char *again;
    void *first;
    void *second;

    text = (char *)heap_alloc(32u);
    if (text == NULL) {
        print_writeln("heap test allocation failed");
        fb_console_writeln("HEAP alloc failed");
        return;
    }

    text[0] = 'o';
    text[1] = 'k';
    text[2] = '\0';

    print_write("heap test ");
    print_writeln(text);
    fb_console_write("HEAP ");
    fb_console_writeln(text);

    first = heap_alloc(64u);
    second = heap_alloc(64u);
    heap_free(first);
    again = (char *)heap_alloc(32u);
    if (again == NULL || again != first) {
        print_writeln("heap reuse test failed");
        fb_console_writeln("HEAP reuse failed");
        return;
    }

    heap_free(second);
    heap_free(again);
    print_writeln("heap free test ok");
    fb_console_writeln("HEAP free ok");
    heap_print_summary();
}

void debug_selftest_run_boot_checks(void)
{
    test_page_allocator();
    test_heap();
}

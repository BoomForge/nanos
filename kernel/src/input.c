#include <kernel/input.h>
#include <kernel/desktop/desktop.h>
#include <kernel/debug/monitor.h>
#include <kernel/print.h>
#include <kernel/process.h>
#include <nanos/syscall.h>

void kernel_input_init(void)
{
    print_writeln("keyboard ready");
}

void kernel_input_on_key(char ch)
{
    kernel_input_on_key_event((uint32_t)(uint8_t)ch);
}

void kernel_input_on_key_event(uint32_t key)
{
    char ch;

    ch = (char)(key & NANOS_KEY_CHAR_MASK);
    if (process_current() != NULL) {
        process_post_key(process_current(), key);
    } else if (desktop_is_ready()) {
        desktop_on_key(key);
    } else if ((key & NANOS_KEY_FLAG_RELEASE) == 0u && ch != 0) {
        monitor_on_key(ch);
    }
}

void kernel_input_on_mouse(int32_t dx, int32_t dy, uint32_t buttons,
    int32_t wheel)
{
    if (desktop_is_ready()) {
        desktop_on_mouse(dx, dy, buttons, wheel);
    }
}

#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/string.h>
#include <kernel/syscall/internal.h>
#include <kernel/time.h>
#include <kernel/types.h>

uint32_t syscall_poll_event(void)
{
    return process_poll_event_current();
}

uint32_t syscall_get_key(void)
{
    return process_get_key_current();
}

uint32_t syscall_get_button(void)
{
    return process_get_button_current();
}

uint32_t syscall_get_mouse(struct nanos_mouse_event *user_event)
{
    struct nanos_mouse_event event;

    if (!platform_user_range_is_valid(user_event, NANOS_MOUSE_EVENT_SIZE) ||
        process_get_mouse_current(&event) != 0) {
        return NANOS_ERROR_INVALID;
    }

    memcpy(user_event, &event, sizeof(event));
    return 0u;
}

uint32_t syscall_wait_event_timeout(uint32_t timeout_ticks)
{
    uint32_t start;
    uint32_t event;

    if (process_current() == NULL) {
        return NANOS_ERROR_INVALID;
    }

    start = kernel_time_ticks();
    for (;;) {
        event = process_poll_event_current();
        if (event != NANOS_EVENT_NONE) {
            return event;
        }
        if (timeout_ticks != 0xffffffffu &&
            kernel_time_ticks() - start >= timeout_ticks) {
            return NANOS_EVENT_NONE;
        }
        if (process_has_ready()) {
            return NANOS_EVENT_NONE;
        }
        platform_enable_interrupts();
        platform_halt();
    }
}

uint32_t syscall_wait_event(void)
{
    return syscall_wait_event_timeout(0xffffffffu);
}

uint32_t syscall_set_event_mask(uint32_t mask)
{
    process_set_event_mask_current(mask);
    return 0u;
}

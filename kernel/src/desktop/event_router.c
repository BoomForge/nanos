#include <kernel/desktop/event_router.h>
#include <kernel/gui/app_button.h>
#include <kernel/gui/window.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <nanos/syscall.h>

void desktop_route_window_mouse(struct window *window, uint32_t type,
    int32_t x, int32_t y, uint32_t buttons, int32_t wheel)
{
    struct nanos_mouse_event user_event;
    int32_t client_x;
    int32_t client_y;
    uint32_t button_id;

    if (window == NULL || window->owner_pid == 0u) {
        return;
    }

    client_x = x - window->x - 1;
    client_y = y - window->y - (int32_t)WINDOW_TITLE_HEIGHT;
    if (type == WINDOW_EVENT_MOUSE_DOWN) {
        button_id = gui_app_button_hit(window->owner_pid, client_x, client_y);
        if (button_id != 0u) {
            process_post_button_to_pid(window->owner_pid, button_id);
            return;
        }
    }

    if (type == WINDOW_EVENT_MOUSE_DOWN) {
        user_event.type = NANOS_MOUSE_TYPE_DOWN;
    } else if (type == WINDOW_EVENT_MOUSE_UP) {
        user_event.type = NANOS_MOUSE_TYPE_UP;
    } else if (type == WINDOW_EVENT_MOUSE_WHEEL) {
        user_event.type = NANOS_MOUSE_TYPE_WHEEL;
    } else {
        user_event.type = NANOS_MOUSE_TYPE_MOVE;
    }
    user_event.x = client_x;
    user_event.y = client_y;
    user_event.screen_x = x;
    user_event.screen_y = y;
    user_event.buttons = buttons;
    user_event.wheel = wheel;
    process_post_mouse_to_pid(window->owner_pid, &user_event);
}

void desktop_route_window_key(struct window *window, uint32_t key)
{
    if (window == NULL || window->owner_pid == 0u) {
        return;
    }

    process_post_key_to_pid(window->owner_pid, key);
}

void desktop_route_window_event(struct window *window, uint32_t type)
{
    if (window == NULL || window->owner_pid == 0u) {
        return;
    }

    if (type == WINDOW_EVENT_CLOSE) {
        process_post_event_to_pid(window->owner_pid, NANOS_EVENT_EXIT);
        return;
    }
    if (type == WINDOW_EVENT_MAXIMIZE) {
        process_post_event_to_pid(window->owner_pid, NANOS_EVENT_REDRAW);
        return;
    }
}

void desktop_route_window_redraw(struct window *window)
{
    if (window == NULL || window->owner_pid == 0u) {
        return;
    }

    process_post_event_to_pid(window->owner_pid, NANOS_EVENT_REDRAW);
}

#ifndef KERNEL_DESKTOP_EVENT_ROUTER_H
#define KERNEL_DESKTOP_EVENT_ROUTER_H

#include <kernel/types.h>

struct window;

void desktop_route_window_mouse(struct window *window, uint32_t type,
    int32_t x, int32_t y, uint32_t buttons, int32_t wheel);
void desktop_route_window_key(struct window *window, uint32_t key);
void desktop_route_window_event(struct window *window, uint32_t type);
void desktop_route_window_redraw(struct window *window);

#endif

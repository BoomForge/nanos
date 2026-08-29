#ifndef KERNEL_DESKTOP_DESKTOP_H
#define KERNEL_DESKTOP_DESKTOP_H

#include <kernel/types.h>

struct window;

void desktop_init(void);
int desktop_is_ready(void);
struct window *desktop_define_user_window(uint32_t owner_pid, int32_t x,
    int32_t y, uint32_t width, uint32_t height, uint32_t color,
    const char *title);
void desktop_on_mouse(int32_t dx, int32_t dy, uint32_t buttons, int32_t wheel);
void desktop_on_key(uint32_t key);
void desktop_redraw(void);

#endif

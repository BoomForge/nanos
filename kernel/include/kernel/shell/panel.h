#ifndef KERNEL_SHELL_PANEL_H
#define KERNEL_SHELL_PANEL_H

#include <kernel/gui/window.h>
#include <kernel/types.h>

#define SHELL_PANEL_HEIGHT 36u
#define SHELL_PANEL_ACTION_WINDOW 1
#define SHELL_PANEL_ACTION_PANEL 2

void shell_panel_init(void);
uint32_t shell_panel_top(void);
void shell_panel_draw_area(int32_t x, int32_t y, uint32_t width, uint32_t height);
int shell_panel_handle_mouse_down(int32_t x, int32_t y, struct window **window);

#endif

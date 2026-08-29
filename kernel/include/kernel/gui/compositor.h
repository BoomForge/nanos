#ifndef KERNEL_GUI_COMPOSITOR_H
#define KERNEL_GUI_COMPOSITOR_H

#include <kernel/rect.h>
#include <kernel/gui/window.h>
#include <kernel/types.h>

typedef void (*gui_background_draw_fn)(int32_t x, int32_t y, uint32_t width, uint32_t height);

void gui_compositor_init(gui_background_draw_fn background_draw);
void gui_compositor_set_focused_window(struct window *focused);
void gui_compositor_redraw_all(struct window *focused);
void gui_compositor_redraw_area(int32_t x, int32_t y, uint32_t width, uint32_t height,
    struct window *focused);
void gui_compositor_invalidate_window_rect(struct window *window, struct gui_rect rect);

#endif

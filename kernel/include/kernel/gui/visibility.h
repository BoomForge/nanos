#ifndef KERNEL_GUI_VISIBILITY_H
#define KERNEL_GUI_VISIBILITY_H

#include <kernel/framebuffer.h>
#include <kernel/gui/window.h>
#include <kernel/types.h>

#define GUI_VISIBLE_REGION_RECTS 32u

struct gui_visible_rect {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

struct gui_visible_region {
    struct framebuffer_clip old_clip;
    struct gui_visible_rect rects[GUI_VISIBLE_REGION_RECTS];
    uint32_t count;
};

int gui_visible_region_for_window_client(struct window *window, int32_t x,
    int32_t y, uint32_t width, uint32_t height,
    struct gui_visible_region *region);
void gui_visible_region_begin_clip(struct gui_visible_region *region,
    uint32_t index);
void gui_visible_region_end_clip(struct gui_visible_region *region);

#endif

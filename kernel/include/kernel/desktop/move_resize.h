#ifndef KERNEL_DESKTOP_MOVE_RESIZE_H
#define KERNEL_DESKTOP_MOVE_RESIZE_H

#include <kernel/types.h>

struct window;

struct desktop_drag_preview {
    struct window *window;
    int32_t x;
    int32_t y;
    int visible;
};

struct desktop_resize_preview {
    struct window *window;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    int visible;
};

void desktop_drag_preview_begin(struct desktop_drag_preview *preview,
    struct window *window);
void desktop_drag_preview_clear(struct desktop_drag_preview *preview);
void desktop_drag_preview_update(struct desktop_drag_preview *preview,
    int32_t x, int32_t y);

void desktop_resize_preview_begin(struct desktop_resize_preview *preview,
    struct window *window);
void desktop_resize_preview_clear(struct desktop_resize_preview *preview);
void desktop_resize_preview_update(struct desktop_resize_preview *preview,
    int32_t mouse_x, int32_t mouse_y, int32_t start_mouse_x,
    int32_t start_mouse_y, int32_t start_x, int32_t start_y,
    uint32_t start_width, uint32_t start_height, uint32_t edges,
    uint32_t min_width, uint32_t min_height, uint32_t work_width,
    uint32_t work_height);

#endif

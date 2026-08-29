#include <kernel/cursor.h>
#include <kernel/desktop/move_resize.h>
#include <kernel/framebuffer.h>
#include <kernel/gui/window.h>
#include <kernel/types.h>

static void xor_pixel(uint32_t x, uint32_t y)
{
    framebuffer_put_pixel(x, y, framebuffer_get_pixel(x, y) ^ 0x00ffffffu);
}

static void xor_hline(uint32_t x, uint32_t y, uint32_t width)
{
    uint32_t i;

    for (i = 0u; i < width; ++i) {
        xor_pixel(x + i, y);
    }
}

static void xor_vline(uint32_t x, uint32_t y, uint32_t height)
{
    uint32_t i;

    for (i = 0u; i < height; ++i) {
        xor_pixel(x, y + i);
    }
}

static void xor_outline(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    if (x < 0 || y < 0 || width == 0u || height == 0u) {
        return;
    }

    xor_hline((uint32_t)x, (uint32_t)y, width);
    xor_hline((uint32_t)x, (uint32_t)y + height - 1u, width);
    xor_vline((uint32_t)x, (uint32_t)y, height);
    xor_vline((uint32_t)x + width - 1u, (uint32_t)y, height);
}

static void clamp_resize_preview(int32_t *x, int32_t *y, uint32_t *width,
    uint32_t *height, uint32_t edges, uint32_t min_width,
    uint32_t min_height, uint32_t work_width, uint32_t work_height)
{
    int32_t right;
    int32_t bottom;
    int32_t min_right;
    int32_t min_bottom;

    right = *x + (int32_t)*width;
    bottom = *y + (int32_t)*height;

    if (*x < 0) {
        *x = 0;
    }
    if (*y < 0) {
        *y = 0;
    }
    if (right > (int32_t)work_width) {
        right = (int32_t)work_width;
    }
    if (bottom > (int32_t)work_height) {
        bottom = (int32_t)work_height;
    }

    min_right = *x + (int32_t)min_width;
    min_bottom = *y + (int32_t)min_height;
    if (right < min_right) {
        if ((edges & WINDOW_RESIZE_LEFT) != 0u) {
            *x = right - (int32_t)min_width;
            if (*x < 0) {
                *x = 0;
                right = (int32_t)min_width;
            }
        } else {
            right = min_right;
        }
    }
    if (bottom < min_bottom) {
        if ((edges & WINDOW_RESIZE_TOP) != 0u) {
            *y = bottom - (int32_t)min_height;
            if (*y < 0) {
                *y = 0;
                bottom = (int32_t)min_height;
            }
        } else {
            bottom = min_bottom;
        }
    }
    if (right > (int32_t)work_width) {
        right = (int32_t)work_width;
    }
    if (bottom > (int32_t)work_height) {
        bottom = (int32_t)work_height;
    }

    *width = (uint32_t)(right - *x);
    *height = (uint32_t)(bottom - *y);
}

void desktop_drag_preview_begin(struct desktop_drag_preview *preview,
    struct window *window)
{
    if (preview == NULL) {
        return;
    }

    preview->window = window;
    preview->visible = 0;
    if (window != NULL) {
        preview->x = window->x;
        preview->y = window->y;
    }
}

void desktop_drag_preview_clear(struct desktop_drag_preview *preview)
{
    if (preview == NULL || !preview->visible || preview->window == NULL) {
        return;
    }

    xor_outline(preview->x, preview->y, preview->window->width,
        preview->window->height);
    preview->visible = 0;
}

void desktop_drag_preview_update(struct desktop_drag_preview *preview,
    int32_t x, int32_t y)
{
    if (preview == NULL || preview->window == NULL) {
        return;
    }

    cursor_hide();
    desktop_drag_preview_clear(preview);
    preview->x = x;
    preview->y = y;
    xor_outline(preview->x, preview->y, preview->window->width,
        preview->window->height);
    preview->visible = 1;
    cursor_show();
}

void desktop_resize_preview_begin(struct desktop_resize_preview *preview,
    struct window *window)
{
    if (preview == NULL) {
        return;
    }

    preview->window = window;
    preview->visible = 0;
    if (window != NULL) {
        preview->x = window->x;
        preview->y = window->y;
        preview->width = window->width;
        preview->height = window->height;
    }
}

void desktop_resize_preview_clear(struct desktop_resize_preview *preview)
{
    if (preview == NULL || !preview->visible || preview->window == NULL) {
        return;
    }

    xor_outline(preview->x, preview->y, preview->width, preview->height);
    preview->visible = 0;
}

void desktop_resize_preview_update(struct desktop_resize_preview *preview,
    int32_t mouse_x, int32_t mouse_y, int32_t start_mouse_x,
    int32_t start_mouse_y, int32_t start_x, int32_t start_y,
    uint32_t start_width, uint32_t start_height, uint32_t edges,
    uint32_t min_width, uint32_t min_height, uint32_t work_width,
    uint32_t work_height)
{
    int32_t x;
    int32_t y;
    int32_t right;
    int32_t bottom;
    int32_t dx;
    int32_t dy;
    uint32_t width;
    uint32_t height;

    if (preview == NULL || preview->window == NULL) {
        return;
    }

    dx = mouse_x - start_mouse_x;
    dy = mouse_y - start_mouse_y;
    x = start_x;
    y = start_y;
    right = start_x + (int32_t)start_width;
    bottom = start_y + (int32_t)start_height;

    if ((edges & WINDOW_RESIZE_LEFT) != 0u) {
        x = start_x + dx;
    }
    if ((edges & WINDOW_RESIZE_RIGHT) != 0u) {
        right = start_x + (int32_t)start_width + dx;
    }
    if ((edges & WINDOW_RESIZE_TOP) != 0u) {
        y = start_y + dy;
    }
    if ((edges & WINDOW_RESIZE_BOTTOM) != 0u) {
        bottom = start_y + (int32_t)start_height + dy;
    }

    if (right < x) {
        x = right;
    }
    if (bottom < y) {
        y = bottom;
    }

    width = (uint32_t)(right - x);
    height = (uint32_t)(bottom - y);
    clamp_resize_preview(&x, &y, &width, &height, edges, min_width,
        min_height, work_width, work_height);

    if (preview->visible && x == preview->x && y == preview->y &&
        width == preview->width && height == preview->height) {
        return;
    }

    cursor_hide();
    desktop_resize_preview_clear(preview);
    preview->x = x;
    preview->y = y;
    preview->width = width;
    preview->height = height;
    xor_outline(preview->x, preview->y, preview->width, preview->height);
    preview->visible = 1;
    cursor_show();
}

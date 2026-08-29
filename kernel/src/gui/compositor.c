#include <kernel/cursor.h>
#include <kernel/framebuffer.h>
#include <kernel/gui/compositor.h>
#include <kernel/rect.h>
#include <kernel/gui/window.h>
#include <kernel/types.h>

#define COMPOSITOR_TILE_WIDTH 256u
#define COMPOSITOR_TILE_HEIGHT 96u

static gui_background_draw_fn draw_background;
static struct window *focused_window;
static uint32_t redraw_tile[COMPOSITOR_TILE_WIDTH * COMPOSITOR_TILE_HEIGHT];

static int clip_to_screen(int32_t *x, int32_t *y, uint32_t *width, uint32_t *height)
{
    uint32_t trim;

    if (*width == 0u || *height == 0u) {
        return 0;
    }

    if (*x < 0) {
        trim = (uint32_t)(0 - *x);
        if (trim >= *width) {
            return 0;
        }
        *width -= trim;
        *x = 0;
    }
    if (*y < 0) {
        trim = (uint32_t)(0 - *y);
        if (trim >= *height) {
            return 0;
        }
        *height -= trim;
        *y = 0;
    }
    if ((uint32_t)*x >= framebuffer_width() || (uint32_t)*y >= framebuffer_height()) {
        return 0;
    }
    if ((uint32_t)*x + *width > framebuffer_width()) {
        *width = framebuffer_width() - (uint32_t)*x;
    }
    if ((uint32_t)*y + *height > framebuffer_height()) {
        *height = framebuffer_height() - (uint32_t)*y;
    }

    return *width != 0u && *height != 0u;
}

static int window_is_area_covered(struct window *window, int32_t x, int32_t y,
    uint32_t width, uint32_t height)
{
    struct window *above;

    if (window == NULL) {
        return 1;
    }

    above = window->next;
    while (above != NULL) {
        if (gui_window_intersects(above, x, y, width, height)) {
            return 1;
        }
        above = above->next;
    }

    return 0;
}

static void draw_area_contents(int32_t x, int32_t y, uint32_t width, uint32_t height,
    struct window *focused, struct window *only_window)
{
    struct window *window;

    if (only_window != NULL) {
        gui_window_draw(only_window, focused);
        return;
    }

    if (draw_background != NULL) {
        draw_background(x, y, width, height);
    }

    window = gui_window_first();
    while (window != NULL) {
        if (gui_window_intersects(window, x, y, width, height)) {
            gui_window_draw(window, focused);
        }
        window = window->next;
    }
}

static void redraw_area_tiled(int32_t x, int32_t y, uint32_t width, uint32_t height,
    struct window *focused, struct window *only_window)
{
    struct framebuffer_clip old_clip;
    uint32_t tile_x;
    uint32_t tile_y;
    uint32_t tile_w;
    uint32_t tile_h;
    uint32_t right;
    uint32_t bottom;

    if (!clip_to_screen(&x, &y, &width, &height)) {
        return;
    }

    right = (uint32_t)x + width;
    bottom = (uint32_t)y + height;
    tile_y = (uint32_t)y;
    while (tile_y < bottom) {
        tile_h = bottom - tile_y;
        if (tile_h > COMPOSITOR_TILE_HEIGHT) {
            tile_h = COMPOSITOR_TILE_HEIGHT;
        }

        tile_x = (uint32_t)x;
        while (tile_x < right) {
            tile_w = right - tile_x;
            if (tile_w > COMPOSITOR_TILE_WIDTH) {
                tile_w = COMPOSITOR_TILE_WIDTH;
            }

            framebuffer_get_clip(&old_clip);
            framebuffer_begin_buffer(redraw_tile, tile_x, tile_y, tile_w, tile_h);
            framebuffer_intersect_clip((int32_t)tile_x, (int32_t)tile_y, tile_w, tile_h);
            draw_area_contents((int32_t)tile_x, (int32_t)tile_y, tile_w, tile_h,
                focused, only_window);
            framebuffer_restore_clip(&old_clip);
            framebuffer_end_buffer();
            framebuffer_copy_buffer_to_screen(redraw_tile, tile_x, tile_y, tile_w, tile_h);

            tile_x += tile_w;
        }

        tile_y += tile_h;
    }
}

void gui_compositor_init(gui_background_draw_fn background_draw)
{
    draw_background = background_draw;
    focused_window = NULL;
}

void gui_compositor_set_focused_window(struct window *focused)
{
    focused_window = focused;
}

void gui_compositor_redraw_all(struct window *focused)
{
    gui_compositor_set_focused_window(focused);
    gui_compositor_redraw_area(0, 0, framebuffer_width(), framebuffer_height(), focused);
}

void gui_compositor_redraw_area(int32_t x, int32_t y, uint32_t width, uint32_t height,
    struct window *focused)
{
    gui_compositor_set_focused_window(focused);
    redraw_area_tiled(x, y, width, height, focused, NULL);
}

void gui_compositor_invalidate_window_rect(struct window *window, struct gui_rect rect)
{
    int32_t screen_x;
    int32_t screen_y;

    if (window == NULL || rect.width == 0u || rect.height == 0u) {
        return;
    }

    screen_x = window->x + 1 + rect.x;
    screen_y = window->y + (int32_t)WINDOW_TITLE_HEIGHT + rect.y;

    cursor_hide();
    if (!window_is_area_covered(window, screen_x, screen_y, rect.width, rect.height)) {
        redraw_area_tiled(screen_x, screen_y, rect.width, rect.height, focused_window, window);
    } else {
        redraw_area_tiled(screen_x, screen_y, rect.width, rect.height, focused_window, NULL);
    }
    cursor_show();
}

#include <kernel/framebuffer.h>
#include <kernel/gui/font.h>
#include <kernel/gui/window.h>
#include <kernel/shell/panel.h>
#include <kernel/types.h>

#define PANEL_BG 0x001c2428u
#define PANEL_EDGE 0x006ac0b8u
#define PANEL_BUTTON 0x00324148u
#define PANEL_BUTTON_BORDER 0x000d1416u
#define PANEL_TEXT 0x00e8ece8u
#define PANEL_PAD 8u
#define PANEL_BUTTON_H 24u
#define PANEL_BUTTON_GAP 6u
#define PANEL_WINDOW_MIN_W 92u
#define PANEL_WINDOW_MAX_W 150u
#define PANEL_WINDOW 0x004c5558u
#define PANEL_WINDOW_MINIMIZED 0x002b3539u

static uint32_t panel_top;

static uint32_t text_width(const char *text)
{
    uint32_t width;

    width = 0u;
    while (text != NULL && *text != '\0') {
        width += GUI_FONT_CELL_WIDTH;
        ++text;
    }

    return width;
}

static uint32_t window_button_width(struct window *window)
{
    uint32_t width;

    width = PANEL_PAD * 2u + text_width(window->title);
    if (width < PANEL_WINDOW_MIN_W) {
        width = PANEL_WINDOW_MIN_W;
    }
    if (width > PANEL_WINDOW_MAX_W) {
        width = PANEL_WINDOW_MAX_W;
    }

    return width;
}

static void window_button_rect(struct window *target, uint32_t *x, uint32_t *y,
    uint32_t *width, uint32_t *height)
{
    struct window *window;

    *x = PANEL_PAD;
    *y = panel_top + (SHELL_PANEL_HEIGHT - PANEL_BUTTON_H) / 2u;
    *width = 0u;
    *height = PANEL_BUTTON_H;

    window = gui_window_first();
    while (window != NULL) {
        *width = window_button_width(window);
        if (window == target) {
            return;
        }
        *x += *width + PANEL_BUTTON_GAP;
        window = window->next;
    }

    *width = 0u;
}

void shell_panel_init(void)
{
    panel_top = 0u;
    if (framebuffer_height() > SHELL_PANEL_HEIGHT) {
        panel_top = framebuffer_height() - SHELL_PANEL_HEIGHT;
    }
}

uint32_t shell_panel_top(void)
{
    return panel_top;
}

void shell_panel_draw_area(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    uint32_t bx;
    uint32_t by;
    uint32_t bw;
    uint32_t bh;
    uint32_t text_y;
    uint32_t color;
    struct window *window;

    (void)x;
    (void)y;
    (void)width;
    (void)height;

    framebuffer_fill_rect(0u, panel_top, framebuffer_width(), SHELL_PANEL_HEIGHT, PANEL_BG);
    framebuffer_fill_rect(0u, panel_top, framebuffer_width(), 2u, PANEL_EDGE);

    window = gui_window_first();
    while (window != NULL) {
        window_button_rect(window, &bx, &by, &bw, &bh);
        if (bw == 0u || bx >= framebuffer_width()) {
            return;
        }
        if (bx + bw > framebuffer_width()) {
            bw = framebuffer_width() - bx;
        }

        color = gui_window_is_minimized(window) ? PANEL_WINDOW_MINIMIZED : PANEL_WINDOW;
        framebuffer_fill_rect(bx, by, bw, bh, color);
        framebuffer_draw_rect(bx, by, bw, bh, PANEL_BUTTON_BORDER);
        text_y = by + (bh - GUI_FONT_CELL_HEIGHT) / 2u;
        gui_draw_text(bx + PANEL_PAD, text_y, PANEL_TEXT, color, window->title);
        window = window->next;
    }
}

int shell_panel_handle_mouse_down(int32_t x, int32_t y, struct window **clicked_window)
{
    uint32_t bx;
    uint32_t by;
    uint32_t bw;
    uint32_t bh;
    struct window *window;

    if (clicked_window != NULL) {
        *clicked_window = NULL;
    }

    if (y < (int32_t)panel_top) {
        return 0;
    }

    window = gui_window_first();
    while (window != NULL) {
        window_button_rect(window, &bx, &by, &bw, &bh);
        if (x >= (int32_t)bx && y >= (int32_t)by &&
                x < (int32_t)(bx + bw) && y < (int32_t)(by + bh)) {
            if (clicked_window != NULL) {
                *clicked_window = window;
            }
            return SHELL_PANEL_ACTION_WINDOW;
        }
        window = window->next;
    }

    return SHELL_PANEL_ACTION_PANEL;
}

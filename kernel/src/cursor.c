#include <kernel/cursor.h>
#include <kernel/framebuffer.h>
#include <kernel/types.h>

#define CURSOR_W 16u
#define CURSOR_H 16u

static uint32_t saved_pixels[CURSOR_W * CURSOR_H];
static uint32_t pos_x;
static uint32_t pos_y;
static uint8_t button_state;
static uint32_t active_shape;
static int cursor_ready;
static int background_saved;
static uint32_t hide_depth;

static const char *arrow_bitmap[CURSOR_H] = {
    "X               ",
    "XX              ",
    "X.X             ",
    "X..X            ",
    "X...X           ",
    "X....X          ",
    "X.....X         ",
    "X......X        ",
    "X.......X       ",
    "X.....XXXX      ",
    "X..X..X         ",
    "X.X X..X        ",
    "XX  X..X        ",
    "X    X.X        ",
    "     XX         ",
    "                ",
};

static const char *text_bitmap[CURSOR_H] = {
    "                ",
    "   XXXXXXXX     ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "      ..        ",
    "   XXXXXXXX     ",
    "                ",
    "                ",
};

static const char *resize_bitmap[CURSOR_H] = {
    "       ..       ",
    "      ....      ",
    "     ..XX..     ",
    "                ",
    "                ",
    "                ",
    "  ..        ..  ",
    "....X      X....",
    "....X      X....",
    "  ..        ..  ",
    "                ",
    "                ",
    "                ",
    "     ..XX..     ",
    "      ....      ",
    "       ..       ",
};

static const char **active_bitmap(void)
{
    if (active_shape == CURSOR_SHAPE_TEXT) {
        return text_bitmap;
    }
    if (active_shape == CURSOR_SHAPE_RESIZE) {
        return resize_bitmap;
    }

    return arrow_bitmap;
}

static int cursor_pixel(uint32_t x, uint32_t y)
{
    const char **bitmap;

    bitmap = active_bitmap();
    if (bitmap[y][x] == 'X') {
        return 2;
    }
    if (bitmap[y][x] == '.') {
        return 1;
    }

    return 0;
}

static void save_background(void)
{
    uint32_t x;
    uint32_t y;
    uint32_t sx;
    uint32_t sy;

    if (!cursor_ready) {
        return;
    }

    for (y = 0u; y < CURSOR_H; ++y) {
        for (x = 0u; x < CURSOR_W; ++x) {
            sx = pos_x + x;
            sy = pos_y + y;
            saved_pixels[y * CURSOR_W + x] = framebuffer_get_pixel(sx, sy);
        }
    }

    background_saved = 1;
}

static void restore_background(void)
{
    uint32_t x;
    uint32_t y;
    uint32_t sx;
    uint32_t sy;

    if (!cursor_ready || !background_saved) {
        return;
    }

    for (y = 0u; y < CURSOR_H; ++y) {
        for (x = 0u; x < CURSOR_W; ++x) {
            sx = pos_x + x;
            sy = pos_y + y;
            if (sx < framebuffer_width() && sy < framebuffer_height()) {
                framebuffer_put_pixel(sx, sy, saved_pixels[y * CURSOR_W + x]);
            }
        }
    }

    background_saved = 0;
}

static void draw_cursor(void)
{
    uint32_t x;
    uint32_t y;
    uint32_t sx;
    uint32_t sy;
    int shape;
    uint32_t color;

    if (!cursor_ready) {
        return;
    }

    save_background();

    for (y = 0u; y < CURSOR_H; ++y) {
        for (x = 0u; x < CURSOR_W; ++x) {
            shape = cursor_pixel(x, y);
            if (shape != 0) {
                sx = pos_x + x;
                sy = pos_y + y;
                color = (shape == 2) ? 0x00000000u : 0x00ffffffu;
                framebuffer_put_pixel(sx, sy, color);
            }
        }
    }
}

void cursor_init(void)
{
    if (!framebuffer_is_available()) {
        return;
    }

    pos_x = framebuffer_width() / 2u;
    pos_y = framebuffer_height() / 2u;
    button_state = 0u;
    active_shape = CURSOR_SHAPE_ARROW;
    cursor_ready = 1;
    background_saved = 0;
    hide_depth = 0u;
    draw_cursor();
}

void cursor_move(int32_t dx, int32_t dy, uint8_t buttons)
{
    int32_t next_x;
    int32_t next_y;
    int32_t max_x;
    int32_t max_y;

    if (!cursor_ready) {
        return;
    }

    if (hide_depth == 0u) {
        restore_background();
    }

    next_x = (int32_t)pos_x + dx;
    next_y = (int32_t)pos_y + dy;
    max_x = (int32_t)(framebuffer_width() - CURSOR_W);
    max_y = (int32_t)(framebuffer_height() - CURSOR_H);

    if (next_x < 0) {
        next_x = 0;
    }
    if (next_y < 0) {
        next_y = 0;
    }
    if (next_x > max_x) {
        next_x = max_x;
    }
    if (next_y > max_y) {
        next_y = max_y;
    }

    pos_x = (uint32_t)next_x;
    pos_y = (uint32_t)next_y;
    button_state = buttons;
    if (hide_depth == 0u) {
        draw_cursor();
    }
}

void cursor_redraw(void)
{
    background_saved = 0;
    if (hide_depth == 0u) {
        draw_cursor();
    }
}

void cursor_hide(void)
{
    if (hide_depth == 0u) {
        restore_background();
    }
    ++hide_depth;
}

void cursor_show(void)
{
    if (hide_depth == 0u) {
        return;
    }
    --hide_depth;
    if (hide_depth == 0u) {
        draw_cursor();
    }
}

void cursor_set_shape(uint32_t shape)
{
    if (shape > CURSOR_SHAPE_RESIZE) {
        shape = CURSOR_SHAPE_ARROW;
    }
    if (shape == active_shape) {
        return;
    }

    if (hide_depth == 0u) {
        restore_background();
    }
    active_shape = shape;
    if (hide_depth == 0u) {
        draw_cursor();
    }
}

uint32_t cursor_x(void)
{
    return pos_x;
}

uint32_t cursor_y(void)
{
    return pos_y;
}

uint8_t cursor_buttons(void)
{
    return button_state;
}

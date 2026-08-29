#include <kernel/fb_console.h>
#include <kernel/framebuffer.h>
#include <kernel/gui/font.h>
#include <kernel/types.h>

#define CELL_W GUI_FONT_CELL_WIDTH
#define CELL_H GUI_FONT_CELL_HEIGHT
#define CONSOLE_BG 0x0010181cu
#define CONSOLE_FG 0x00e8ece8u

static uint32_t console_x;
static uint32_t console_y;
static uint32_t console_w;
static uint32_t console_h;
static uint32_t cursor_col;
static uint32_t cursor_row;
static uint32_t cols;
static uint32_t rows;
static int console_ready;

static void draw_cell(uint32_t col, uint32_t row, char ch)
{
    uint32_t px;
    uint32_t py;

    px = console_x + col * CELL_W;
    py = console_y + row * CELL_H;
    gui_draw_char(px, py, CONSOLE_FG, CONSOLE_BG, ch);
}

static void newline(void)
{
    cursor_col = 0u;
    if (cursor_row + 1u < rows) {
        ++cursor_row;
    } else {
        fb_console_clear();
    }
}

void fb_console_init(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    console_x = x;
    console_y = y;
    console_w = width;
    console_h = height;
    cols = console_w / CELL_W;
    rows = console_h / CELL_H;
    cursor_col = 0u;
    cursor_row = 0u;
    console_ready = framebuffer_is_available() && cols > 0u && rows > 0u;

    if (console_ready) {
        fb_console_clear();
    }
}

void fb_console_clear(void)
{
    if (!console_ready) {
        return;
    }

    framebuffer_fill_rect(console_x, console_y, console_w, console_h, CONSOLE_BG);
    cursor_col = 0u;
    cursor_row = 0u;
}

void fb_console_putc(char ch)
{
    if (!console_ready) {
        return;
    }

    if (ch == '\n') {
        newline();
        return;
    }

    if (ch == '\b') {
        if (cursor_col > 0u) {
            --cursor_col;
            draw_cell(cursor_col, cursor_row, ' ');
        }
        return;
    }

    draw_cell(cursor_col, cursor_row, ch);
    ++cursor_col;
    if (cursor_col >= cols) {
        newline();
    }
}

void fb_console_write(const char *text)
{
    while (*text != '\0') {
        fb_console_putc(*text);
        ++text;
    }
}

void fb_console_writeln(const char *text)
{
    fb_console_write(text);
    fb_console_putc('\n');
}

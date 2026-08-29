#ifndef KERNEL_GUI_FONT_H
#define KERNEL_GUI_FONT_H

#include <kernel/types.h>

#define GUI_FONT_WIDTH 5u
#define GUI_FONT_HEIGHT 7u
#define GUI_FONT_SCALE 2u
#define GUI_FONT_CELL_WIDTH ((GUI_FONT_WIDTH + 1u) * GUI_FONT_SCALE)
#define GUI_FONT_CELL_HEIGHT ((GUI_FONT_HEIGHT + 1u) * GUI_FONT_SCALE)

void gui_draw_char(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, char ch);
void gui_draw_text(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, const char *text);

#endif

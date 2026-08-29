#ifndef KERNEL_CURSOR_H
#define KERNEL_CURSOR_H

#include <kernel/types.h>

#define CURSOR_SHAPE_ARROW 0u
#define CURSOR_SHAPE_TEXT 1u
#define CURSOR_SHAPE_RESIZE 2u

void cursor_init(void);
void cursor_move(int32_t dx, int32_t dy, uint8_t buttons);
void cursor_redraw(void);
void cursor_hide(void);
void cursor_show(void);
void cursor_set_shape(uint32_t shape);
uint32_t cursor_x(void);
uint32_t cursor_y(void);
uint8_t cursor_buttons(void);

#endif

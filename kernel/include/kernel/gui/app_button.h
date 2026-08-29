#ifndef KERNEL_GUI_APP_BUTTON_H
#define KERNEL_GUI_APP_BUTTON_H

#include <kernel/types.h>

void gui_app_buttons_clear(uint32_t owner_pid);
int gui_app_button_define(uint32_t owner_pid, uint32_t id, int32_t x,
    int32_t y, uint32_t width, uint32_t height);
uint32_t gui_app_button_hit(uint32_t owner_pid, int32_t x, int32_t y);

#endif

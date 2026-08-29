#ifndef KERNEL_RECT_H
#define KERNEL_RECT_H

#include <kernel/types.h>

struct gui_rect {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

#endif

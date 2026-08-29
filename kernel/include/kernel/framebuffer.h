#ifndef KERNEL_FRAMEBUFFER_H
#define KERNEL_FRAMEBUFFER_H

#include <kernel/types.h>

struct framebuffer {
    uint8_t *addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
};

struct framebuffer_clip {
    uint32_t x1;
    uint32_t y1;
    uint32_t x2;
    uint32_t y2;
};

int framebuffer_init_from_multiboot(uint32_t info_addr);
int framebuffer_is_available(void);
uint32_t framebuffer_addr(void);
uint32_t framebuffer_size(void);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);
uint32_t framebuffer_get_pixel(uint32_t x, uint32_t y);
void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void framebuffer_set_clip(int32_t x, int32_t y, uint32_t w, uint32_t h);
void framebuffer_intersect_clip(int32_t x, int32_t y, uint32_t w, uint32_t h);
void framebuffer_get_clip(struct framebuffer_clip *clip);
void framebuffer_restore_clip(const struct framebuffer_clip *clip);
void framebuffer_reset_clip(void);
void framebuffer_begin_buffer(uint32_t *buffer, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height);
void framebuffer_end_buffer(void);
void framebuffer_copy_buffer_to_screen(const uint32_t *buffer, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height);
void framebuffer_clear(uint32_t color);
void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void framebuffer_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    uint32_t color);

#endif

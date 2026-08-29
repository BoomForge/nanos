#include <kernel/framebuffer.h>
#include <kernel/multiboot2.h>
#include <kernel/print.h>
#include <kernel/types.h>

static struct framebuffer fb;
static int fb_ready;
static uint32_t clip_x1;
static uint32_t clip_y1;
static uint32_t clip_x2;
static uint32_t clip_y2;
static uint32_t *draw_buffer;
static uint32_t draw_buffer_x;
static uint32_t draw_buffer_y;
static uint32_t draw_buffer_width;
static uint32_t draw_buffer_height;

int framebuffer_init_from_multiboot(uint32_t info_addr)
{
    const struct multiboot2_tag *tag;
    const struct multiboot2_tag_framebuffer *mb_fb;

    tag = multiboot2_find_tag(info_addr, MULTIBOOT2_TAG_TYPE_FRAMEBUFFER);
    if (tag == NULL) {
        return -1;
    }

    mb_fb = (const struct multiboot2_tag_framebuffer *)tag;
    if (mb_fb->framebuffer_bpp != 32u || mb_fb->framebuffer_addr_high != 0u) {
        return -1;
    }

    fb.addr = (uint8_t *)(uintptr_t)mb_fb->framebuffer_addr_low;
    fb.width = mb_fb->framebuffer_width;
    fb.height = mb_fb->framebuffer_height;
    fb.pitch = mb_fb->framebuffer_pitch;
    fb.bpp = mb_fb->framebuffer_bpp;
    fb_ready = 1;
    framebuffer_reset_clip();

    print_write("framebuffer ");
    print_uint(fb.width);
    print_write("x");
    print_uint(fb.height);
    print_write(" pitch ");
    print_uint(fb.pitch);
    print_putc('\n');

    return 0;
}

int framebuffer_is_available(void)
{
    return fb_ready;
}

uint32_t framebuffer_addr(void)
{
    return (uint32_t)(uintptr_t)fb.addr;
}

uint32_t framebuffer_size(void)
{
    return fb.pitch * fb.height;
}

uint32_t framebuffer_width(void)
{
    return fb.width;
}

uint32_t framebuffer_height(void)
{
    return fb.height;
}

uint32_t framebuffer_get_pixel(uint32_t x, uint32_t y)
{
    uint32_t *pixel;

    if (!fb_ready || x >= fb.width || y >= fb.height) {
        return 0u;
    }

    pixel = (uint32_t *)(void *)(fb.addr + y * fb.pitch + x * 4u);
    return *pixel;
}

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t *pixel;
    uint32_t local_x;
    uint32_t local_y;

    if (!fb_ready || x >= fb.width || y >= fb.height) {
        return;
    }
    if (x < clip_x1 || x >= clip_x2 || y < clip_y1 || y >= clip_y2) {
        return;
    }

    if (draw_buffer != NULL) {
        if (x < draw_buffer_x || y < draw_buffer_y) {
            return;
        }
        local_x = x - draw_buffer_x;
        local_y = y - draw_buffer_y;
        if (local_x >= draw_buffer_width || local_y >= draw_buffer_height) {
            return;
        }
        draw_buffer[local_y * draw_buffer_width + local_x] = color;
        return;
    }

    pixel = (uint32_t *)(void *)(fb.addr + y * fb.pitch + x * 4u);
    *pixel = color;
}

void framebuffer_set_clip(int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    uint32_t trim;

    if (!fb_ready) {
        return;
    }
    if (w == 0u || h == 0u) {
        clip_x1 = 0u;
        clip_y1 = 0u;
        clip_x2 = 0u;
        clip_y2 = 0u;
        return;
    }

    if (x < 0) {
        trim = (uint32_t)(0 - x);
        if (trim >= w) {
            clip_x1 = 0u;
            clip_y1 = 0u;
            clip_x2 = 0u;
            clip_y2 = 0u;
            return;
        }
        w -= trim;
        x = 0;
    }
    if (y < 0) {
        trim = (uint32_t)(0 - y);
        if (trim >= h) {
            clip_x1 = 0u;
            clip_y1 = 0u;
            clip_x2 = 0u;
            clip_y2 = 0u;
            return;
        }
        h -= trim;
        y = 0;
    }

    if ((uint32_t)x >= fb.width || (uint32_t)y >= fb.height) {
        clip_x1 = 0u;
        clip_y1 = 0u;
        clip_x2 = 0u;
        clip_y2 = 0u;
        return;
    }

    clip_x1 = (uint32_t)x;
    clip_y1 = (uint32_t)y;
    clip_x2 = clip_x1 + w;
    clip_y2 = clip_y1 + h;

    if (clip_x2 > fb.width || clip_x2 < clip_x1) {
        clip_x2 = fb.width;
    }
    if (clip_y2 > fb.height || clip_y2 < clip_y1) {
        clip_y2 = fb.height;
    }
}

void framebuffer_intersect_clip(int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    struct framebuffer_clip old_clip;

    if (!fb_ready) {
        return;
    }

    framebuffer_get_clip(&old_clip);
    framebuffer_set_clip(x, y, w, h);

    if (clip_x1 < old_clip.x1) {
        clip_x1 = old_clip.x1;
    }
    if (clip_y1 < old_clip.y1) {
        clip_y1 = old_clip.y1;
    }
    if (clip_x2 > old_clip.x2) {
        clip_x2 = old_clip.x2;
    }
    if (clip_y2 > old_clip.y2) {
        clip_y2 = old_clip.y2;
    }
    if (clip_x1 >= clip_x2 || clip_y1 >= clip_y2) {
        clip_x1 = 0u;
        clip_y1 = 0u;
        clip_x2 = 0u;
        clip_y2 = 0u;
    }
}

void framebuffer_get_clip(struct framebuffer_clip *clip)
{
    if (clip == NULL) {
        return;
    }

    clip->x1 = clip_x1;
    clip->y1 = clip_y1;
    clip->x2 = clip_x2;
    clip->y2 = clip_y2;
}

void framebuffer_restore_clip(const struct framebuffer_clip *clip)
{
    if (clip == NULL || !fb_ready) {
        return;
    }

    clip_x1 = clip->x1;
    clip_y1 = clip->y1;
    clip_x2 = clip->x2;
    clip_y2 = clip->y2;
}

void framebuffer_reset_clip(void)
{
    if (!fb_ready) {
        return;
    }

    clip_x1 = 0u;
    clip_y1 = 0u;
    clip_x2 = fb.width;
    clip_y2 = fb.height;
}

void framebuffer_begin_buffer(uint32_t *buffer, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height)
{
    if (!fb_ready || buffer == NULL || width == 0u || height == 0u) {
        return;
    }

    draw_buffer = buffer;
    draw_buffer_x = x;
    draw_buffer_y = y;
    draw_buffer_width = width;
    draw_buffer_height = height;
}

void framebuffer_end_buffer(void)
{
    draw_buffer = NULL;
    draw_buffer_x = 0u;
    draw_buffer_y = 0u;
    draw_buffer_width = 0u;
    draw_buffer_height = 0u;
}

void framebuffer_copy_buffer_to_screen(const uint32_t *buffer, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height)
{
    uint32_t xx;
    uint32_t yy;
    uint32_t source_width;
    uint32_t *dest;
    const uint32_t *src;

    if (!fb_ready || buffer == NULL || width == 0u || height == 0u) {
        return;
    }
    if (x >= fb.width || y >= fb.height) {
        return;
    }
    source_width = width;
    if (x + width > fb.width || x + width < x) {
        width = fb.width - x;
    }
    if (y + height > fb.height || y + height < y) {
        height = fb.height - y;
    }

    for (yy = 0u; yy < height; ++yy) {
        dest = (uint32_t *)(void *)(fb.addr + (y + yy) * fb.pitch + x * 4u);
        src = buffer + yy * source_width;
        for (xx = 0u; xx < width; ++xx) {
            dest[xx] = src[xx];
        }
    }
}

void framebuffer_clear(uint32_t color)
{
    framebuffer_fill_rect(0u, 0u, fb.width, fb.height, color);
}

void framebuffer_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    uint32_t yy;
    uint32_t xx;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t local_x;
    uint32_t local_y;
    uint32_t local_max_x;
    uint32_t local_max_y;
    uint32_t *row;

    if (!fb_ready) {
        return;
    }

    max_x = x + w;
    max_y = y + h;
    if (max_x < x) {
        max_x = fb.width;
    }
    if (max_y < y) {
        max_y = fb.height;
    }

    if (x < clip_x1) {
        x = clip_x1;
    }
    if (y < clip_y1) {
        y = clip_y1;
    }
    if (max_x > clip_x2) {
        max_x = clip_x2;
    }
    if (max_y > clip_y2) {
        max_y = clip_y2;
    }
    if (max_x > fb.width) {
        max_x = fb.width;
    }
    if (max_y > fb.height) {
        max_y = fb.height;
    }
    if (x >= max_x || y >= max_y) {
        return;
    }

    if (draw_buffer != NULL) {
        if (max_x <= draw_buffer_x || max_y <= draw_buffer_y ||
            x >= draw_buffer_x + draw_buffer_width ||
            y >= draw_buffer_y + draw_buffer_height) {
            return;
        }
        if (x < draw_buffer_x) {
            x = draw_buffer_x;
        }
        if (y < draw_buffer_y) {
            y = draw_buffer_y;
        }

        local_x = x - draw_buffer_x;
        local_y = y - draw_buffer_y;
        local_max_x = max_x - draw_buffer_x;
        local_max_y = max_y - draw_buffer_y;
        if (local_max_x > draw_buffer_width) {
            local_max_x = draw_buffer_width;
        }
        if (local_max_y > draw_buffer_height) {
            local_max_y = draw_buffer_height;
        }

        for (yy = local_y; yy < local_max_y; ++yy) {
            row = draw_buffer + yy * draw_buffer_width;
            for (xx = local_x; xx < local_max_x; ++xx) {
                row[xx] = color;
            }
        }
        return;
    }

    for (yy = y; yy < max_y; ++yy) {
        row = (uint32_t *)(void *)(fb.addr + yy * fb.pitch);
        for (xx = x; xx < max_x; ++xx) {
            row[xx] = color;
        }
    }
}

void framebuffer_draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (w == 0u || h == 0u) {
        return;
    }

    framebuffer_fill_rect(x, y, w, 1u, color);
    framebuffer_fill_rect(x, y + h - 1u, w, 1u, color);
    framebuffer_fill_rect(x, y, 1u, h, color);
    framebuffer_fill_rect(x + w - 1u, y, 1u, h, color);
}

void framebuffer_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    uint32_t color)
{
    int32_t dx;
    int32_t dy;
    int32_t sx;
    int32_t sy;
    int32_t err;
    int32_t twice_err;

    if (!fb_ready) {
        return;
    }

    dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    dy = y1 >= y0 ? y0 - y1 : y1 - y0;
    sx = x0 < x1 ? 1 : -1;
    sy = y0 < y1 ? 1 : -1;
    err = dx + dy;

    for (;;) {
        if (x0 >= 0 && y0 >= 0) {
            framebuffer_put_pixel((uint32_t)x0, (uint32_t)y0, color);
        }
        if (x0 == x1 && y0 == y1) {
            break;
        }

        twice_err = err * 2;
        if (twice_err >= dy) {
            err += dy;
            x0 += sx;
        }
        if (twice_err <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

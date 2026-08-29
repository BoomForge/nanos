#include <kernel/framebuffer.h>
#include <kernel/gui/visibility.h>
#include <kernel/gui/window.h>
#include <kernel/string.h>
#include <kernel/types.h>

static int rect_intersect(struct gui_visible_rect a, struct gui_visible_rect b,
    struct gui_visible_rect *out)
{
    int32_t ax2;
    int32_t ay2;
    int32_t bx2;
    int32_t by2;
    int32_t x1;
    int32_t y1;
    int32_t x2;
    int32_t y2;

    if (out == NULL || a.width == 0u || a.height == 0u ||
        b.width == 0u || b.height == 0u) {
        return 0;
    }

    ax2 = a.x + (int32_t)a.width;
    ay2 = a.y + (int32_t)a.height;
    bx2 = b.x + (int32_t)b.width;
    by2 = b.y + (int32_t)b.height;
    x1 = a.x > b.x ? a.x : b.x;
    y1 = a.y > b.y ? a.y : b.y;
    x2 = ax2 < bx2 ? ax2 : bx2;
    y2 = ay2 < by2 ? ay2 : by2;
    if (x1 >= x2 || y1 >= y2) {
        return 0;
    }

    out->x = x1;
    out->y = y1;
    out->width = (uint32_t)(x2 - x1);
    out->height = (uint32_t)(y2 - y1);
    return 1;
}

static void add_rect(struct gui_visible_rect *rects, uint32_t *count,
    struct gui_visible_rect rect)
{
    if (rect.width == 0u || rect.height == 0u ||
        *count >= GUI_VISIBLE_REGION_RECTS) {
        return;
    }

    rects[*count] = rect;
    ++(*count);
}

static void subtract_rect_from_list(struct gui_visible_rect *rects,
    uint32_t *count, struct gui_visible_rect cut)
{
    struct gui_visible_rect source[GUI_VISIBLE_REGION_RECTS];
    struct gui_visible_rect inter;
    struct gui_visible_rect part;
    uint32_t source_count;
    uint32_t i;
    int32_t rx2;
    int32_t ry2;
    int32_t ix2;
    int32_t iy2;

    source_count = *count;
    memcpy(source, rects, sizeof(source));
    *count = 0u;

    for (i = 0u; i < source_count; ++i) {
        if (!rect_intersect(source[i], cut, &inter)) {
            add_rect(rects, count, source[i]);
            continue;
        }

        rx2 = source[i].x + (int32_t)source[i].width;
        ry2 = source[i].y + (int32_t)source[i].height;
        ix2 = inter.x + (int32_t)inter.width;
        iy2 = inter.y + (int32_t)inter.height;

        part.x = source[i].x;
        part.y = source[i].y;
        part.width = source[i].width;
        part.height = (uint32_t)(inter.y - source[i].y);
        add_rect(rects, count, part);

        part.x = source[i].x;
        part.y = iy2;
        part.width = source[i].width;
        part.height = (uint32_t)(ry2 - iy2);
        add_rect(rects, count, part);

        part.x = source[i].x;
        part.y = inter.y;
        part.width = (uint32_t)(inter.x - source[i].x);
        part.height = inter.height;
        add_rect(rects, count, part);

        part.x = ix2;
        part.y = inter.y;
        part.width = (uint32_t)(rx2 - ix2);
        part.height = inter.height;
        add_rect(rects, count, part);
    }
}

int gui_visible_region_for_window_client(struct window *window, int32_t x,
    int32_t y, uint32_t width, uint32_t height,
    struct gui_visible_region *region)
{
    struct gui_visible_rect client;
    struct gui_visible_rect requested;
    struct gui_visible_rect visible;
    struct gui_visible_rect cover;
    struct window *above;

    if (window == NULL || region == NULL || gui_window_is_minimized(window) ||
        window->width <= 2u || window->height <= WINDOW_TITLE_HEIGHT + 1u) {
        return -1;
    }

    client.x = window->x + 1;
    client.y = window->y + (int32_t)WINDOW_TITLE_HEIGHT;
    client.width = window->width - 2u;
    client.height = window->height - WINDOW_TITLE_HEIGHT - 1u;
    requested.x = x;
    requested.y = y;
    requested.width = width;
    requested.height = height;

    framebuffer_get_clip(&region->old_clip);
    region->count = 0u;
    if (!rect_intersect(client, requested, &visible)) {
        return 0;
    }

    add_rect(region->rects, &region->count, visible);
    above = window->next;
    while (above != NULL && region->count != 0u) {
        if (!gui_window_is_minimized(above)) {
            cover.x = above->x;
            cover.y = above->y;
            cover.width = above->width;
            cover.height = above->height;
            subtract_rect_from_list(region->rects, &region->count, cover);
        }
        above = above->next;
    }

    return 0;
}

void gui_visible_region_begin_clip(struct gui_visible_region *region,
    uint32_t index)
{
    if (region == NULL || index >= region->count) {
        return;
    }

    framebuffer_restore_clip(&region->old_clip);
    framebuffer_intersect_clip(region->rects[index].x, region->rects[index].y,
        region->rects[index].width, region->rects[index].height);
}

void gui_visible_region_end_clip(struct gui_visible_region *region)
{
    if (region != NULL) {
        framebuffer_restore_clip(&region->old_clip);
    }
}

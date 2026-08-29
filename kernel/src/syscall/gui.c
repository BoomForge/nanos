#include <kernel/desktop/desktop.h>
#include <kernel/cursor.h>
#include <kernel/framebuffer.h>
#include <kernel/gui/app_button.h>
#include <kernel/gui/font.h>
#include <kernel/gui/visibility.h>
#include <kernel/gui/window.h>
#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/string.h>
#include <kernel/syscall/internal.h>
#include <kernel/types.h>

static char user_window_title[PROCESS_MAX][NANOS_PATH_MAX];
static uint32_t user_window_draw_depth[PROCESS_MAX];

static struct window *current_user_window(void)
{
    return gui_window_find_by_owner(process_current_app_pid());
}

uint32_t syscall_draw_window(uint32_t packed_x, uint32_t packed_y,
    uint32_t color, const char *user_title)
{
    struct process *process;
    uint32_t slot;
    uint32_t owner_pid;
    uint32_t width;
    uint32_t height;
    int32_t x;
    int32_t y;

    process = process_current();
    if (process == NULL ||
        syscall_unpack_position_size(packed_x, &x, &width) != 0 ||
        syscall_unpack_position_size(packed_y, &y, &height) != 0) {
        return NANOS_ERROR_INVALID;
    }

    owner_pid = process_current_app_pid();
    slot = process_current_app_slot();
    if (slot >= PROCESS_MAX ||
        syscall_copy_user_string(user_title, user_window_title[slot],
        sizeof(user_window_title[slot])) != 0) {
        return NANOS_ERROR_INVALID;
    }

    if (desktop_define_user_window(owner_pid, x, y, width, height,
        color & 0x00ffffffu, user_window_title[slot]) == NULL) {
        return NANOS_ERROR_INVALID;
    }
    gui_app_buttons_clear(owner_pid);

    return 0u;
}

uint32_t syscall_window_draw(uint32_t phase)
{
    struct process *process;
    uint32_t slot;

    process = process_current();
    if (process == NULL) {
        return NANOS_ERROR_INVALID;
    }

    slot = process_current_app_slot();
    if (slot >= PROCESS_MAX) {
        return NANOS_ERROR_INVALID;
    }

    if (phase == NANOS_WINDOW_DRAW_BEGIN) {
        if (user_window_draw_depth[slot] == 0u) {
            cursor_hide();
        }
        ++user_window_draw_depth[slot];
        return 0u;
    }
    if (phase == NANOS_WINDOW_DRAW_END) {
        if (user_window_draw_depth[slot] == 0u) {
            return NANOS_ERROR_INVALID;
        }
        --user_window_draw_depth[slot];
        if (user_window_draw_depth[slot] == 0u) {
            cursor_show();
        }
        return 0u;
    }

    return NANOS_ERROR_INVALID;
}

static void finish_window_draw_for_process(struct process *process)
{
    uint32_t slot;

    if (process == NULL) {
        return;
    }

    slot = process_app_slot_for_pid(process->pid);
    if (slot >= PROCESS_MAX) {
        return;
    }

    while (user_window_draw_depth[slot] != 0u) {
        --user_window_draw_depth[slot];
        cursor_show();
    }
}

void syscall_gui_finish_current_draw(void)
{
    finish_window_draw_for_process(process_current());
}

static int32_t packed_x(uint32_t packed)
{
    return (int32_t)(packed >> 16);
}

static int32_t packed_y(uint32_t packed)
{
    return (int32_t)(packed & 0xffffu);
}

static uint32_t draw_text_in_window(struct window *window, uint32_t x,
    uint32_t y, uint32_t color, const char *text)
{
    struct gui_visible_region region;
    uint32_t width;
    uint32_t i;
    uint32_t screen_x;
    uint32_t screen_y;

    if (window == NULL || text == NULL) {
        return NANOS_ERROR_INVALID;
    }

    width = (uint32_t)strlen(text) * GUI_FONT_CELL_WIDTH;
    screen_x = (uint32_t)window->x + 1u + x;
    screen_y = (uint32_t)window->y + WINDOW_TITLE_HEIGHT + y;
    if (gui_visible_region_for_window_client(window, (int32_t)screen_x,
        (int32_t)screen_y, width, GUI_FONT_CELL_HEIGHT, &region) != 0) {
        return NANOS_ERROR_INVALID;
    }

    for (i = 0u; i < region.count; ++i) {
        gui_visible_region_begin_clip(&region, i);
        gui_draw_text(screen_x, screen_y, color & 0x00ffffffu,
            window->color, text);
    }
    gui_visible_region_end_clip(&region);
    return 0u;
}

uint32_t syscall_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    struct gui_visible_region region;
    struct window *window;
    int32_t screen_x;
    int32_t screen_y;
    uint32_t i;

    window = current_user_window();
    if (window == NULL || x > 0xffffu || y > 0xffffu) {
        return NANOS_ERROR_INVALID;
    }

    screen_x = window->x + 1 + (int32_t)x;
    screen_y = window->y + (int32_t)WINDOW_TITLE_HEIGHT + (int32_t)y;
    if (gui_visible_region_for_window_client(window, screen_x, screen_y,
        1u, 1u, &region) != 0) {
        return NANOS_ERROR_INVALID;
    }

    for (i = 0u; i < region.count; ++i) {
        gui_visible_region_begin_clip(&region, i);
        framebuffer_put_pixel((uint32_t)screen_x, (uint32_t)screen_y,
            color & 0x00ffffffu);
    }
    gui_visible_region_end_clip(&region);
    return 0u;
}

uint32_t syscall_write_text(uint32_t packed_xy, uint32_t color,
    const char *user_text)
{
    struct window *window;
    char text[128];
    uint32_t x;
    uint32_t y;

    window = current_user_window();
    if (window == NULL ||
        syscall_copy_user_string(user_text, text, sizeof(text)) != 0) {
        return NANOS_ERROR_INVALID;
    }

    x = packed_xy >> 16;
    y = packed_xy & 0xffffu;
    return draw_text_in_window(window, x, y, color, text);
}

uint32_t syscall_draw_rect(uint32_t packed_x, uint32_t packed_y,
    uint32_t color)
{
    struct gui_visible_region region;
    struct window *window;
    uint32_t width;
    uint32_t height;
    uint32_t i;
    uint32_t screen_x;
    uint32_t screen_y;
    int32_t x;
    int32_t y;

    window = current_user_window();
    if (window == NULL ||
        syscall_unpack_position_size(packed_x, &x, &width) != 0 ||
        syscall_unpack_position_size(packed_y, &y, &height) != 0) {
        return NANOS_ERROR_INVALID;
    }

    screen_x = (uint32_t)(window->x + 1 + x);
    screen_y = (uint32_t)(window->y + (int32_t)WINDOW_TITLE_HEIGHT + y);
    if (gui_visible_region_for_window_client(window, (int32_t)screen_x,
        (int32_t)screen_y, width, height, &region) != 0) {
        return NANOS_ERROR_INVALID;
    }

    for (i = 0u; i < region.count; ++i) {
        gui_visible_region_begin_clip(&region, i);
        framebuffer_fill_rect(screen_x, screen_y, width, height,
            color & 0x00ffffffu);
    }
    gui_visible_region_end_clip(&region);
    return 0u;
}

static int image_byte_count(uint32_t width, uint32_t height, uint32_t *out)
{
    uint32_t pixels;

    if (out == NULL || width == 0u || height == 0u) {
        return -1;
    }
    if (height > 0xffffffffu / width) {
        return -1;
    }

    pixels = width * height;
    if (pixels > 0xffffffffu / sizeof(uint32_t)) {
        return -1;
    }

    *out = pixels * (uint32_t)sizeof(uint32_t);
    return 0;
}

uint32_t syscall_put_image(uint32_t packed_x, uint32_t packed_y,
    const uint32_t *user_pixels)
{
    struct gui_visible_region region;
    struct gui_visible_rect clip;
    struct window *window;
    uint32_t width;
    uint32_t height;
    uint32_t byte_count;
    uint32_t i;
    uint32_t sx;
    uint32_t sy;
    uint32_t start_x;
    uint32_t end_x;
    uint32_t start_y;
    uint32_t end_y;
    int32_t x;
    int32_t y;
    int32_t screen_x;
    int32_t screen_y;

    window = current_user_window();
    if (window == NULL || user_pixels == NULL ||
        syscall_unpack_position_size(packed_x, &x, &width) != 0 ||
        syscall_unpack_position_size(packed_y, &y, &height) != 0) {
        return NANOS_ERROR_INVALID;
    }
    if (width == 0u || height == 0u) {
        return 0u;
    }
    if (image_byte_count(width, height, &byte_count) != 0 ||
        !platform_user_range_is_valid(user_pixels, byte_count)) {
        return NANOS_ERROR_INVALID;
    }

    screen_x = window->x + 1 + x;
    screen_y = window->y + (int32_t)WINDOW_TITLE_HEIGHT + y;
    if (gui_visible_region_for_window_client(window, screen_x, screen_y,
        width, height, &region) != 0) {
        return NANOS_ERROR_INVALID;
    }

    for (i = 0u; i < region.count; ++i) {
        clip = region.rects[i];
        start_x = (uint32_t)(clip.x - screen_x);
        start_y = (uint32_t)(clip.y - screen_y);
        end_x = start_x + clip.width;
        end_y = start_y + clip.height;

        gui_visible_region_begin_clip(&region, i);
        for (sy = start_y; sy < end_y; ++sy) {
            for (sx = start_x; sx < end_x; ++sx) {
                framebuffer_put_pixel((uint32_t)(screen_x + (int32_t)sx),
                    (uint32_t)(screen_y + (int32_t)sy),
                    user_pixels[sy * width + sx] & 0x00ffffffu);
            }
        }
    }
    gui_visible_region_end_clip(&region);
    return 0u;
}

uint32_t syscall_define_button(uint32_t packed_x, uint32_t packed_y,
    uint32_t id, uint32_t color)
{
    struct gui_visible_region region;
    struct window *window;
    uint32_t width;
    uint32_t height;
    uint32_t fill_color;
    uint32_t i;
    uint32_t screen_x;
    uint32_t screen_y;
    int32_t x;
    int32_t y;

    window = current_user_window();
    if (window == NULL || id == 0u ||
        syscall_unpack_position_size(packed_x, &x, &width) != 0 ||
        syscall_unpack_position_size(packed_y, &y, &height) != 0 ||
        gui_app_button_define(window->owner_pid, id, x, y, width, height) != 0) {
        return NANOS_ERROR_INVALID;
    }

    fill_color = color & 0x00ffffffu;
    screen_x = (uint32_t)(window->x + 1 + x);
    screen_y = (uint32_t)(window->y + (int32_t)WINDOW_TITLE_HEIGHT + y);
    if (gui_visible_region_for_window_client(window, (int32_t)screen_x,
        (int32_t)screen_y, width, height, &region) != 0) {
        return NANOS_ERROR_INVALID;
    }

    for (i = 0u; i < region.count; ++i) {
        gui_visible_region_begin_clip(&region, i);
        framebuffer_fill_rect(screen_x, screen_y, width, height, fill_color);
        framebuffer_draw_rect(screen_x, screen_y, width, height, 0x00101416u);
    }
    gui_visible_region_end_clip(&region);
    return 0u;
}

uint32_t syscall_get_screen_size(void)
{
    uint32_t width;
    uint32_t height;

    width = framebuffer_width();
    height = framebuffer_height();
    if (width == 0u || height == 0u) {
        return NANOS_ERROR_INVALID;
    }

    return ((width - 1u) & 0xffffu) << 16 | ((height - 1u) & 0xffffu);
}

uint32_t syscall_draw_line(uint32_t packed_start, uint32_t packed_end,
    uint32_t color)
{
    struct gui_visible_region region;
    struct window *window;
    int32_t base_x;
    int32_t base_y;
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
    int32_t min_x;
    int32_t min_y;
    int32_t max_x;
    int32_t max_y;
    uint32_t i;

    window = current_user_window();
    if (window == NULL) {
        return NANOS_ERROR_INVALID;
    }

    base_x = window->x + 1;
    base_y = window->y + (int32_t)WINDOW_TITLE_HEIGHT;
    x0 = base_x + packed_x(packed_start);
    y0 = base_y + packed_y(packed_start);
    x1 = base_x + packed_x(packed_end);
    y1 = base_y + packed_y(packed_end);
    min_x = x0 < x1 ? x0 : x1;
    min_y = y0 < y1 ? y0 : y1;
    max_x = x0 > x1 ? x0 : x1;
    max_y = y0 > y1 ? y0 : y1;
    if (gui_visible_region_for_window_client(window, min_x, min_y,
        (uint32_t)(max_x - min_x + 1), (uint32_t)(max_y - min_y + 1),
        &region) != 0) {
        return NANOS_ERROR_INVALID;
    }

    for (i = 0u; i < region.count; ++i) {
        gui_visible_region_begin_clip(&region, i);
        framebuffer_draw_line(x0, y0, x1, y1, color & 0x00ffffffu);
    }
    gui_visible_region_end_clip(&region);
    return 0u;
}

static uint32_t number_base_from_format(uint32_t format)
{
    uint32_t base;

    base = (format >> 16) & 0xffffu;
    if (base == 0u) {
        base = 10u;
    }
    return base;
}

static int number_to_text(uint32_t value, uint32_t format, char *out,
    uint32_t out_size)
{
    char reversed[32];
    uint32_t base;
    uint32_t min_digits;
    uint32_t count;
    uint32_t i;
    uint32_t digit;

    if (out == NULL || out_size == 0u) {
        return -1;
    }

    base = number_base_from_format(format);
    if (base != 10u && base != 16u) {
        return -1;
    }

    min_digits = format & 0xffu;
    if (min_digits >= sizeof(reversed)) {
        min_digits = sizeof(reversed) - 1u;
    }

    count = 0u;
    do {
        digit = value % base;
        if (digit < 10u) {
            reversed[count] = (char)('0' + digit);
        } else {
            reversed[count] = (char)('A' + digit - 10u);
        }
        ++count;
        value /= base;
    } while (value != 0u && count < sizeof(reversed));

    while (count < min_digits) {
        reversed[count] = '0';
        ++count;
    }
    if (count + 1u > out_size) {
        return -1;
    }

    for (i = 0u; i < count; ++i) {
        out[i] = reversed[count - i - 1u];
    }
    out[count] = '\0';
    return 0;
}

uint32_t syscall_write_number(uint32_t format, uint32_t value,
    uint32_t packed_xy, uint32_t color)
{
    struct window *window;
    char text[32];

    window = current_user_window();
    if (window == NULL ||
        number_to_text(value, format, text, sizeof(text)) != 0) {
        return NANOS_ERROR_INVALID;
    }

    return draw_text_in_window(window, packed_xy >> 16, packed_xy & 0xffffu,
        color, text);
}

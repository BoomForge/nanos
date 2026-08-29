#include <kernel/desktop/event_router.h>
#include <kernel/framebuffer.h>
#include <kernel/gui/font.h>
#include <kernel/gui/window.h>
#include <kernel/heap.h>
#include <kernel/types.h>

#define WINDOW_BUTTON_SIZE 12u
#define WINDOW_BUTTON_MARGIN 8u
#define WINDOW_BUTTON_GAP 8u
#define WINDOW_BUTTON_Y_PAD 7u
#define WINDOW_BUTTON_CLOSE 0u
#define WINDOW_BUTTON_MAXIMIZE 1u
#define WINDOW_BUTTON_MINIMIZE 2u
#define WINDOW_RESIZE_BORDER 5

static struct window *first_window;
static struct window *last_window;

static int point_in_window(struct window *window, int32_t x, int32_t y)
{
    if (window == NULL || window->minimized) {
        return 0;
    }

    return x >= window->x && y >= window->y &&
        x < window->x + (int32_t)window->width &&
        y < window->y + (int32_t)window->height;
}

static uint32_t title_button_right_offset(uint32_t index)
{
    return WINDOW_BUTTON_MARGIN + WINDOW_BUTTON_SIZE +
        index * (WINDOW_BUTTON_SIZE + WINDOW_BUTTON_GAP);
}

static int title_button_rect(struct window *window, uint32_t index,
    int32_t *x, int32_t *y)
{
    uint32_t right_offset;

    if (window == NULL) {
        return 0;
    }

    right_offset = title_button_right_offset(index);
    if (window->width <= right_offset) {
        return 0;
    }

    *x = window->x + (int32_t)window->width - (int32_t)right_offset;
    *y = window->y + (int32_t)WINDOW_BUTTON_Y_PAD;
    return 1;
}

static void draw_title_button(struct window *window, uint32_t index, uint32_t color)
{
    int32_t button_x;
    int32_t button_y;

    if (!title_button_rect(window, index, &button_x, &button_y)) {
        return;
    }

    if (button_x < 0 || button_y < 0) {
        return;
    }

    framebuffer_fill_rect((uint32_t)button_x, (uint32_t)button_y,
        WINDOW_BUTTON_SIZE, WINDOW_BUTTON_SIZE, color);
}

static int title_button_hit(struct window *window, uint32_t index, int32_t x, int32_t y)
{
    int32_t button_x;
    int32_t button_y;

    if (!point_in_window(window, x, y) ||
            y >= window->y + (int32_t)WINDOW_TITLE_HEIGHT) {
        return 0;
    }
    if (!title_button_rect(window, index, &button_x, &button_y)) {
        return 0;
    }

    return x >= button_x && y >= button_y &&
        x < button_x + (int32_t)WINDOW_BUTTON_SIZE &&
        y < button_y + (int32_t)WINDOW_BUTTON_SIZE;
}

void gui_window_draw(struct window *window, struct window *focused)
{
    struct framebuffer_clip old_clip;
    uint32_t title_color;
    uint32_t content_height;
    uint32_t content_width;
    uint32_t x;
    uint32_t y;

    if (window == NULL || window->minimized) {
        return;
    }

    if (window->x < 0 || window->y < 0) {
        return;
    }

    x = (uint32_t)window->x;
    y = (uint32_t)window->y;
    title_color = (window == focused) ? 0x00324148u : 0x004c5558u;

    framebuffer_fill_rect(x, y, window->width, window->height, window->color);
    framebuffer_fill_rect(x, y, window->width, WINDOW_TITLE_HEIGHT, title_color);
    framebuffer_draw_rect(x, y, window->width, window->height, 0x000d1416u);
    draw_title_button(window, WINDOW_BUTTON_MINIMIZE, 0x00e8ece8u);
    draw_title_button(window, WINDOW_BUTTON_MAXIMIZE, 0x006ac0b8u);
    draw_title_button(window, WINDOW_BUTTON_CLOSE, 0x00d45a4cu);
    gui_draw_text(x + 14u, y + 8u, 0x00e8ece8u, title_color, window->title);

    if (window->draw != NULL) {
        content_width = 0u;
        content_height = 0u;
        if (window->width > 2u) {
            content_width = window->width - 2u;
        }
        if (window->height > WINDOW_TITLE_HEIGHT + 1u) {
            content_height = window->height - WINDOW_TITLE_HEIGHT - 1u;
        }
        framebuffer_get_clip(&old_clip);
        framebuffer_intersect_clip((int32_t)(x + 1u), (int32_t)(y + WINDOW_TITLE_HEIGHT),
            content_width, content_height);
        window->draw(window);
        framebuffer_restore_clip(&old_clip);
    }
}

void gui_window_system_init(void)
{
    first_window = NULL;
    last_window = NULL;
}

struct window *gui_window_create(int32_t x, int32_t y, uint32_t width, uint32_t height,
    uint32_t color, const char *title)
{
    return gui_window_create_owned(x, y, width, height, color, title, 0u);
}

struct window *gui_window_create_owned(int32_t x, int32_t y, uint32_t width,
    uint32_t height, uint32_t color, const char *title, uint32_t owner_pid)
{
    struct window *window;

    window = (struct window *)heap_alloc(sizeof(*window));
    if (window == NULL) {
        return NULL;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->color = color;
    window->title = title;
    window->minimized = 0;
    window->maximized = 0;
    window->restore_x = x;
    window->restore_y = y;
    window->restore_width = width;
    window->restore_height = height;
    window->cursor_shape = CURSOR_SHAPE_ARROW;
    window->owner_pid = owner_pid;
    window->redraw_pending = 0;
    window->draw = NULL;
    window->event = NULL;
    window->next = NULL;
    window->prev = last_window;

    if (last_window != NULL) {
        last_window->next = window;
    } else {
        first_window = window;
    }

    last_window = window;
    return window;
}

struct window *gui_window_find_by_owner(uint32_t owner_pid)
{
    struct window *window;

    if (owner_pid == 0u) {
        return NULL;
    }

    window = first_window;
    while (window != NULL) {
        if (window->owner_pid == owner_pid) {
            return window;
        }
        window = window->next;
    }

    return NULL;
}

void gui_window_set_draw(struct window *window, window_draw_fn draw)
{
    if (window == NULL) {
        return;
    }

    window->draw = draw;
}

void gui_window_set_event(struct window *window, window_event_fn event)
{
    if (window == NULL) {
        return;
    }

    window->event = event;
}

void gui_window_set_cursor(struct window *window, uint32_t cursor_shape)
{
    if (window == NULL) {
        return;
    }
    if (cursor_shape > CURSOR_SHAPE_RESIZE) {
        cursor_shape = CURSOR_SHAPE_ARROW;
    }

    window->cursor_shape = cursor_shape;
}

uint32_t gui_window_cursor(struct window *window)
{
    if (window == NULL) {
        return CURSOR_SHAPE_ARROW;
    }

    return window->cursor_shape;
}

void gui_window_draw_all(struct window *focused)
{
    struct window *window;

    window = first_window;
    while (window != NULL) {
        gui_window_draw(window, focused);
        window = window->next;
    }
}

int gui_window_intersects(struct window *window, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    int32_t right;
    int32_t bottom;
    int32_t window_right;
    int32_t window_bottom;

    if (window == NULL || window->minimized) {
        return 0;
    }

    right = x + (int32_t)width;
    bottom = y + (int32_t)height;
    window_right = window->x + (int32_t)window->width;
    window_bottom = window->y + (int32_t)window->height;

    return x < window_right && right > window->x &&
        y < window_bottom && bottom > window->y;
}

struct window *gui_window_at(int32_t x, int32_t y)
{
    struct window *window;

    window = last_window;
    while (window != NULL) {
        if (point_in_window(window, x, y)) {
            return window;
        }
        window = window->prev;
    }

    return NULL;
}

int gui_window_title_hit(struct window *window, int32_t x, int32_t y)
{
    if (!point_in_window(window, x, y)) {
        return 0;
    }

    return y < window->y + (int32_t)WINDOW_TITLE_HEIGHT;
}

int gui_window_minimize_hit(struct window *window, int32_t x, int32_t y)
{
    return title_button_hit(window, WINDOW_BUTTON_MINIMIZE, x, y);
}

int gui_window_maximize_hit(struct window *window, int32_t x, int32_t y)
{
    return title_button_hit(window, WINDOW_BUTTON_MAXIMIZE, x, y);
}

int gui_window_close_hit(struct window *window, int32_t x, int32_t y)
{
    return title_button_hit(window, WINDOW_BUTTON_CLOSE, x, y);
}

int gui_window_client_hit(struct window *window, int32_t x, int32_t y)
{
    if (!point_in_window(window, x, y)) {
        return 0;
    }

    return x >= window->x + 1 &&
        x < window->x + (int32_t)window->width - 1 &&
        y >= window->y + (int32_t)WINDOW_TITLE_HEIGHT &&
        y < window->y + (int32_t)window->height - 1;
}

uint32_t gui_window_resize_hit(struct window *window, int32_t x, int32_t y)
{
    uint32_t edges;
    int32_t right;
    int32_t bottom;

    if (!point_in_window(window, x, y) || window->maximized) {
        return 0u;
    }

    right = window->x + (int32_t)window->width;
    bottom = window->y + (int32_t)window->height;
    edges = 0u;

    if (x < window->x + WINDOW_RESIZE_BORDER) {
        edges |= WINDOW_RESIZE_LEFT;
    } else if (x >= right - WINDOW_RESIZE_BORDER) {
        edges |= WINDOW_RESIZE_RIGHT;
    }

    if (y < window->y + WINDOW_RESIZE_BORDER) {
        edges |= WINDOW_RESIZE_TOP;
    } else if (y >= bottom - WINDOW_RESIZE_BORDER) {
        edges |= WINDOW_RESIZE_BOTTOM;
    }

    return edges;
}

void gui_window_dispatch_mouse_event(struct window *window, uint32_t type,
    int32_t x, int32_t y, uint32_t buttons, int32_t wheel)
{
    struct window_event event;

    if (window == NULL) {
        return;
    }
    if (window->owner_pid != 0u) {
        desktop_route_window_mouse(window, type, x, y, buttons, wheel);
        return;
    }
    if (window->event == NULL) {
        return;
    }

    event.type = type;
    event.x = x - window->x - 1;
    event.y = y - window->y - (int32_t)WINDOW_TITLE_HEIGHT;
    event.screen_x = x;
    event.screen_y = y;
    event.buttons = buttons;
    event.wheel = wheel;
    event.ch = 0;
    event.key = 0u;
    window->event(window, &event);
}

void gui_window_dispatch_key_event(struct window *window, uint32_t key)
{
    struct window_event event;

    if (window == NULL) {
        return;
    }
    if (window->owner_pid != 0u) {
        desktop_route_window_key(window, key);
        return;
    }
    if (window->event == NULL) {
        return;
    }

    event.type = WINDOW_EVENT_KEY;
    event.x = 0;
    event.y = 0;
    event.screen_x = 0;
    event.screen_y = 0;
    event.buttons = 0u;
    event.wheel = 0;
    event.ch = (char)(key & 0xffu);
    event.key = key;
    window->event(window, &event);
}

void gui_window_dispatch_event(struct window *window, uint32_t type)
{
    struct window_event event;

    if (window == NULL) {
        return;
    }
    if (window->owner_pid != 0u) {
        desktop_route_window_event(window, type);
        return;
    }
    if (window->event == NULL) {
        return;
    }

    event.type = type;
    event.x = 0;
    event.y = 0;
    event.screen_x = 0;
    event.screen_y = 0;
    event.buttons = 0u;
    event.wheel = 0;
    event.ch = 0;
    event.key = 0u;
    window->event(window, &event);
}

void gui_window_request_redraw(struct window *window)
{
    if (window == NULL || window->owner_pid == 0u) {
        return;
    }

    desktop_route_window_redraw(window);
}

void gui_window_mark_redraw(struct window *window)
{
    if (window == NULL || window->owner_pid == 0u || window->minimized) {
        return;
    }

    window->redraw_pending = 1;
}

int gui_window_take_redraw(struct window *window)
{
    if (window == NULL || window->redraw_pending == 0) {
        return 0;
    }

    window->redraw_pending = 0;
    return 1;
}

void gui_window_move(struct window *window, int32_t x, int32_t y)
{
    if (window == NULL) {
        return;
    }

    window->x = x;
    window->y = y;
}

void gui_window_resize(struct window *window, int32_t x, int32_t y,
    uint32_t width, uint32_t height)
{
    if (window == NULL) {
        return;
    }

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
}

void gui_window_bring_to_front(struct window *window)
{
    if (window == NULL || window == last_window) {
        return;
    }

    if (window->prev != NULL) {
        window->prev->next = window->next;
    } else {
        first_window = window->next;
    }

    if (window->next != NULL) {
        window->next->prev = window->prev;
    }

    window->prev = last_window;
    window->next = NULL;
    if (last_window != NULL) {
        last_window->next = window;
    }
    last_window = window;
}

void gui_window_set_minimized(struct window *window, int minimized)
{
    if (window == NULL) {
        return;
    }

    window->minimized = minimized ? 1 : 0;
}

int gui_window_is_minimized(struct window *window)
{
    if (window == NULL) {
        return 0;
    }

    return window->minimized;
}

void gui_window_save_restore_geometry(struct window *window)
{
    if (window == NULL) {
        return;
    }

    window->restore_x = window->x;
    window->restore_y = window->y;
    window->restore_width = window->width;
    window->restore_height = window->height;
}

void gui_window_set_maximized(struct window *window, int maximized)
{
    if (window == NULL) {
        return;
    }

    window->maximized = maximized ? 1 : 0;
}

int gui_window_is_maximized(struct window *window)
{
    if (window == NULL) {
        return 0;
    }

    return window->maximized;
}

uint32_t gui_window_owner(struct window *window)
{
    if (window == NULL) {
        return 0u;
    }

    return window->owner_pid;
}

void gui_window_destroy(struct window *window)
{
    if (window == NULL) {
        return;
    }

    if (window->prev != NULL) {
        window->prev->next = window->next;
    } else {
        first_window = window->next;
    }

    if (window->next != NULL) {
        window->next->prev = window->prev;
    } else {
        last_window = window->prev;
    }

    window->next = NULL;
    window->prev = NULL;
    heap_free(window);
}

struct window *gui_window_first(void)
{
    return first_window;
}

struct window *gui_window_last(void)
{
    return last_window;
}

struct window *gui_window_last_visible(void)
{
    struct window *window;

    window = last_window;
    while (window != NULL) {
        if (!window->minimized) {
            return window;
        }
        window = window->prev;
    }

    return NULL;
}

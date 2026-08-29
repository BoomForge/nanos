#ifndef KERNEL_GUI_WINDOW_H
#define KERNEL_GUI_WINDOW_H

#include <kernel/cursor.h>
#include <kernel/types.h>

#define WINDOW_TITLE_HEIGHT 28u
#define WINDOW_EVENT_MOUSE_MOVE 1u
#define WINDOW_EVENT_MOUSE_DOWN 2u
#define WINDOW_EVENT_MOUSE_UP 3u
#define WINDOW_EVENT_KEY 4u
#define WINDOW_EVENT_CLOSE 5u
#define WINDOW_EVENT_MINIMIZE 6u
#define WINDOW_EVENT_MAXIMIZE 7u
#define WINDOW_EVENT_MOUSE_WHEEL 8u

#define WINDOW_RESIZE_LEFT 1u
#define WINDOW_RESIZE_RIGHT 2u
#define WINDOW_RESIZE_TOP 4u
#define WINDOW_RESIZE_BOTTOM 8u

struct window;
typedef void (*window_draw_fn)(struct window *window);

struct window_event {
    uint32_t type;
    int32_t x;
    int32_t y;
    int32_t screen_x;
    int32_t screen_y;
    uint32_t buttons;
    int32_t wheel;
    char ch;
    uint32_t key;
};

typedef void (*window_event_fn)(struct window *window, const struct window_event *event);

struct window {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    uint32_t color;
    const char *title;
    int minimized;
    int maximized;
    int32_t restore_x;
    int32_t restore_y;
    uint32_t restore_width;
    uint32_t restore_height;
    uint32_t cursor_shape;
    uint32_t owner_pid;
    int redraw_pending;
    window_draw_fn draw;
    window_event_fn event;
    struct window *next;
    struct window *prev;
};

void gui_window_system_init(void);
struct window *gui_window_create(int32_t x, int32_t y, uint32_t width, uint32_t height,
    uint32_t color, const char *title);
struct window *gui_window_create_owned(int32_t x, int32_t y, uint32_t width,
    uint32_t height, uint32_t color, const char *title, uint32_t owner_pid);
struct window *gui_window_find_by_owner(uint32_t owner_pid);
void gui_window_set_draw(struct window *window, window_draw_fn draw);
void gui_window_set_event(struct window *window, window_event_fn event);
void gui_window_set_cursor(struct window *window, uint32_t cursor_shape);
uint32_t gui_window_cursor(struct window *window);
void gui_window_draw(struct window *window, struct window *focused);
void gui_window_draw_all(struct window *focused);
int gui_window_intersects(struct window *window, int32_t x, int32_t y, uint32_t width, uint32_t height);
struct window *gui_window_at(int32_t x, int32_t y);
int gui_window_client_hit(struct window *window, int32_t x, int32_t y);
void gui_window_dispatch_mouse_event(struct window *window, uint32_t type,
    int32_t x, int32_t y, uint32_t buttons, int32_t wheel);
void gui_window_dispatch_key_event(struct window *window, uint32_t key);
void gui_window_dispatch_event(struct window *window, uint32_t type);
int gui_window_title_hit(struct window *window, int32_t x, int32_t y);
int gui_window_minimize_hit(struct window *window, int32_t x, int32_t y);
int gui_window_maximize_hit(struct window *window, int32_t x, int32_t y);
int gui_window_close_hit(struct window *window, int32_t x, int32_t y);
uint32_t gui_window_resize_hit(struct window *window, int32_t x, int32_t y);
void gui_window_request_redraw(struct window *window);
void gui_window_mark_redraw(struct window *window);
int gui_window_take_redraw(struct window *window);
void gui_window_move(struct window *window, int32_t x, int32_t y);
void gui_window_bring_to_front(struct window *window);
void gui_window_set_minimized(struct window *window, int minimized);
int gui_window_is_minimized(struct window *window);
void gui_window_save_restore_geometry(struct window *window);
void gui_window_resize(struct window *window, int32_t x, int32_t y,
    uint32_t width, uint32_t height);
void gui_window_set_maximized(struct window *window, int maximized);
int gui_window_is_maximized(struct window *window);
uint32_t gui_window_owner(struct window *window);
void gui_window_destroy(struct window *window);
struct window *gui_window_first(void);
struct window *gui_window_last(void);
struct window *gui_window_last_visible(void);

#endif

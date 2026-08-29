#include <kernel/cursor.h>
#include <kernel/desktop/desktop.h>
#include <kernel/desktop/move_resize.h>
#include <kernel/framebuffer.h>
#include <kernel/gui/app_button.h>
#include <kernel/gui/compositor.h>
#include <kernel/gui/window.h>
#include <kernel/print.h>
#include <kernel/process.h>
#include <kernel/shell/panel.h>
#include <kernel/types.h>
#include <nanos/syscall.h>

static struct window *focused_window;
static struct window *event_window;
static struct window *drag_window;
static struct window *resize_window;
static int32_t drag_offset_x;
static int32_t drag_offset_y;
static int32_t resize_start_mouse_x;
static int32_t resize_start_mouse_y;
static int32_t resize_start_x;
static int32_t resize_start_y;
static uint32_t resize_start_w;
static uint32_t resize_start_h;
static uint32_t resize_edges;
static uint32_t last_buttons;
static struct desktop_drag_preview drag_preview;
static struct desktop_resize_preview resize_preview;
static int desktop_ready;

#define DESKTOP_MIN_WINDOW_W 140u
#define DESKTOP_MIN_WINDOW_H 80u

static void desktop_on_process_exit(uint32_t pid, int status);
static void flush_user_window_redraws(void);
static void queue_visible_user_window_redraws(void);

static void repaint_area(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    gui_compositor_redraw_area(x, y, width, height, focused_window);
}

static void expose_area(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    repaint_area(x, y, width, height);
    queue_visible_user_window_redraws();
}

static void repaint_window_area(struct window *window)
{
    if (window == NULL || gui_window_is_minimized(window)) {
        return;
    }

    repaint_area(window->x - 2, window->y - 2, window->width + 4u,
        window->height + 4u);
}

static void expose_window_area(struct window *window)
{
    if (window == NULL || gui_window_is_minimized(window)) {
        return;
    }

    expose_area(window->x - 2, window->y - 2, window->width + 4u,
        window->height + 4u);
}

static void expose_window_change(int32_t old_x, int32_t old_y,
    uint32_t old_width, uint32_t old_height, struct window *window)
{
    repaint_area(old_x - 2, old_y - 2, old_width + 4u, old_height + 4u);
    repaint_window_area(window);
    queue_visible_user_window_redraws();
}

static void redraw_panel(void)
{
    repaint_area(0, (int32_t)shell_panel_top(), framebuffer_width(),
        SHELL_PANEL_HEIGHT);
}

static void desktop_draw_background_area(int32_t x, int32_t y, uint32_t width, uint32_t height)
{
    uint32_t area_y;
    uint32_t area_bottom;
    uint32_t panel_y;
    uint32_t fill_x;
    uint32_t fill_w;
    uint32_t fill_h;
    uint32_t trim;

    if (width == 0u || height == 0u) {
        return;
    }
    if (x < 0) {
        trim = (uint32_t)(0 - x);
        if (trim >= width) {
            return;
        }
        width -= trim;
        x = 0;
    }
    if (y < 0) {
        trim = (uint32_t)(0 - y);
        if (trim >= height) {
            return;
        }
        height -= trim;
        y = 0;
    }

    fill_x = (uint32_t)x;
    fill_w = width;
    area_y = (uint32_t)y;
    area_bottom = area_y + height;
    panel_y = shell_panel_top();

    if (area_y < panel_y) {
        fill_h = height;
        if (area_bottom > panel_y) {
            fill_h = panel_y - area_y;
        }
        framebuffer_fill_rect(fill_x, area_y, fill_w, fill_h, 0x0023322fu);
    }

    if (area_bottom > panel_y) {
        if (area_y < panel_y) {
            fill_h = area_bottom - panel_y;
            framebuffer_fill_rect(fill_x, panel_y, fill_w, fill_h, 0x001c2428u);
        } else {
            framebuffer_fill_rect(fill_x, area_y, fill_w, height, 0x001c2428u);
        }
        shell_panel_draw_area(x, y, width, height);
    }
}

static int32_t clamp_window_x(struct window *window, int32_t x)
{
    int32_t max_x;

    max_x = (int32_t)framebuffer_width() - (int32_t)window->width;
    if (x < 0) {
        return 0;
    }
    if (x > max_x) {
        return max_x;
    }
    return x;
}

static int32_t clamp_window_y(struct window *window, int32_t y)
{
    int32_t max_y;

    max_y = (int32_t)shell_panel_top() - (int32_t)window->height;
    if (max_y < 0) {
        max_y = 0;
    }
    if (y < 0) {
        return 0;
    }
    if (y > max_y) {
        return max_y;
    }
    return y;
}

static void redraw_window_titlebar(struct window *window)
{
    if (window == NULL) {
        return;
    }

    repaint_area(window->x, window->y, window->width, WINDOW_TITLE_HEIGHT);
}

static void focus_window(struct window *window)
{
    struct window *old_focused;
    int changed_order;

    old_focused = focused_window;
    changed_order = (window != NULL && window != gui_window_last());
    focused_window = window;
    if (focused_window != NULL) {
        gui_window_bring_to_front(focused_window);
    }
    gui_compositor_set_focused_window(focused_window);

    if (changed_order) {
        redraw_window_titlebar(old_focused);
        if (focused_window != NULL) {
            repaint_window_area(focused_window);
            gui_window_mark_redraw(focused_window);
        }
    } else {
        redraw_window_titlebar(old_focused);
        redraw_window_titlebar(focused_window);
    }
}

static void close_window(struct window *window)
{
    int32_t redraw_x;
    int32_t redraw_y;
    uint32_t redraw_w;
    uint32_t redraw_h;

    if (window == NULL) {
        return;
    }

    redraw_x = window->x - 2;
    redraw_y = window->y - 2;
    redraw_w = window->width + 4u;
    redraw_h = window->height + 4u;

    if (event_window == window) {
        event_window = NULL;
    }
    if (drag_window == window) {
        desktop_drag_preview_clear(&drag_preview);
        drag_window = NULL;
        drag_preview.window = NULL;
    }
    if (resize_window == window) {
        desktop_resize_preview_clear(&resize_preview);
        resize_window = NULL;
        resize_preview.window = NULL;
    }

    gui_window_destroy(window);
    focused_window = gui_window_last_visible();
    gui_compositor_set_focused_window(focused_window);
    repaint_area(redraw_x, redraw_y, redraw_w, redraw_h);
    redraw_window_titlebar(focused_window);
    redraw_panel();
    queue_visible_user_window_redraws();
    flush_user_window_redraws();
}

static void minimize_window(struct window *window)
{
    int32_t redraw_x;
    int32_t redraw_y;
    uint32_t redraw_w;
    uint32_t redraw_h;

    if (window == NULL) {
        return;
    }

    redraw_x = window->x - 2;
    redraw_y = window->y - 2;
    redraw_w = window->width + 4u;
    redraw_h = window->height + 4u;

    if (event_window == window) {
        event_window = NULL;
    }
    if (drag_window == window) {
        desktop_drag_preview_clear(&drag_preview);
        drag_window = NULL;
        drag_preview.window = NULL;
    }
    if (resize_window == window) {
        desktop_resize_preview_clear(&resize_preview);
        resize_window = NULL;
        resize_preview.window = NULL;
    }

    gui_window_set_minimized(window, 1);
    if (focused_window == window) {
        focused_window = gui_window_last_visible();
        gui_compositor_set_focused_window(focused_window);
    }

    repaint_area(redraw_x, redraw_y, redraw_w, redraw_h);
    redraw_window_titlebar(focused_window);
    redraw_panel();
    queue_visible_user_window_redraws();
    flush_user_window_redraws();
}

static void toggle_maximize_window(struct window *window)
{
    int32_t old_x;
    int32_t old_y;
    uint32_t old_width;
    uint32_t old_height;
    uint32_t work_height;

    if (window == NULL) {
        return;
    }

    old_x = window->x;
    old_y = window->y;
    old_width = window->width;
    old_height = window->height;

    if (gui_window_is_maximized(window)) {
        gui_window_resize(window, window->restore_x, window->restore_y,
            window->restore_width, window->restore_height);
        gui_window_set_maximized(window, 0);
    } else {
        gui_window_save_restore_geometry(window);
        work_height = shell_panel_top();
        if (work_height < WINDOW_TITLE_HEIGHT) {
            work_height = WINDOW_TITLE_HEIGHT;
        }
        gui_window_resize(window, 0, 0, framebuffer_width(), work_height);
        gui_window_set_maximized(window, 1);
    }

    gui_window_set_minimized(window, 0);
    focus_window(window);
    expose_window_change(old_x, old_y, old_width, old_height, window);
    redraw_panel();
    flush_user_window_redraws();
}

static void restore_window(struct window *window)
{
    if (window == NULL) {
        return;
    }

    gui_window_set_minimized(window, 0);
    focus_window(window);
    expose_window_area(window);
    redraw_panel();
    flush_user_window_redraws();
}

static void update_resize_outline(int32_t mouse_x, int32_t mouse_y)
{
    desktop_resize_preview_update(&resize_preview, mouse_x, mouse_y,
        resize_start_mouse_x, resize_start_mouse_y, resize_start_x,
        resize_start_y, resize_start_w, resize_start_h, resize_edges,
        DESKTOP_MIN_WINDOW_W, DESKTOP_MIN_WINDOW_H, framebuffer_width(),
        shell_panel_top());
}

static void update_cursor_shape(int32_t mouse_x, int32_t mouse_y)
{
    struct window *hovered;

    if (resize_window != NULL) {
        cursor_set_shape(CURSOR_SHAPE_RESIZE);
        return;
    }
    if (drag_window != NULL || mouse_y >= (int32_t)shell_panel_top()) {
        cursor_set_shape(CURSOR_SHAPE_ARROW);
        return;
    }

    hovered = gui_window_at(mouse_x, mouse_y);
    if (hovered == NULL) {
        cursor_set_shape(CURSOR_SHAPE_ARROW);
        return;
    }
    if (gui_window_resize_hit(hovered, mouse_x, mouse_y) != 0u) {
        cursor_set_shape(CURSOR_SHAPE_RESIZE);
        return;
    }
    if (gui_window_client_hit(hovered, mouse_x, mouse_y)) {
        cursor_set_shape(gui_window_cursor(hovered));
        return;
    }

    cursor_set_shape(CURSOR_SHAPE_ARROW);
}

static void queue_visible_user_window_redraws(void)
{
    struct window *window;

    window = gui_window_first();
    while (window != NULL) {
        if (gui_window_owner(window) != 0u && !gui_window_is_minimized(window)) {
            gui_window_mark_redraw(window);
        }
        window = window->next;
    }
}

static void flush_user_window_redraws(void)
{
    struct window *window;

    window = gui_window_first();
    while (window != NULL) {
        if (!gui_window_is_minimized(window) && gui_window_take_redraw(window)) {
            gui_window_request_redraw(window);
        }
        window = window->next;
    }
}

void desktop_redraw(void)
{
    if (!desktop_ready) {
        return;
    }

    gui_compositor_redraw_all(focused_window);
    cursor_redraw();
    queue_visible_user_window_redraws();
    flush_user_window_redraws();
}

void desktop_init(void)
{
    if (!framebuffer_is_available()) {
        return;
    }

    gui_window_system_init();
    gui_compositor_init(desktop_draw_background_area);
    shell_panel_init();

    focused_window = gui_window_last_visible();
    if (focused_window != NULL) {
        gui_window_bring_to_front(focused_window);
    }
    gui_compositor_set_focused_window(focused_window);
    desktop_ready = 1;
    process_set_exit_hook(desktop_on_process_exit);

    cursor_init();
    desktop_redraw();
    print_writeln("desktop initialized");
}

int desktop_is_ready(void)
{
    return desktop_ready;
}

struct window *desktop_define_user_window(uint32_t owner_pid, int32_t x,
    int32_t y, uint32_t width, uint32_t height, uint32_t color,
    const char *title)
{
    struct window *window;
    int new_window;

    if (!desktop_ready || owner_pid == 0u || title == NULL ||
        width < DESKTOP_MIN_WINDOW_W || height < DESKTOP_MIN_WINDOW_H) {
        return NULL;
    }

    window = gui_window_find_by_owner(owner_pid);
    new_window = 0;
    if (window == NULL) {
        window = gui_window_create_owned(x, y, width, height, color, title,
            owner_pid);
        if (window == NULL) {
            return NULL;
        }
        new_window = 1;
    } else {
        window->color = color;
        window->title = title;
    }

    if (new_window) {
        focus_window(window);
    }
    repaint_window_area(window);
    redraw_panel();
    return window;
}

void desktop_close_user_window(uint32_t owner_pid)
{
    struct window *window;

    if (!desktop_ready || owner_pid == 0u) {
        return;
    }

    window = gui_window_find_by_owner(owner_pid);
    if (window != NULL) {
        gui_app_buttons_clear(owner_pid);
        close_window(window);
    }
}

static void desktop_on_process_exit(uint32_t pid, int status)
{
    (void)status;
    desktop_close_user_window(pid);
}

void desktop_on_mouse(int32_t dx, int32_t dy, uint32_t buttons, int32_t wheel)
{
    uint32_t changed_buttons;
    uint32_t left_now;
    uint32_t left_was;
    int panel_action;
    struct window *hit;
    struct window *panel_window;
    int32_t mouse_x;
    int32_t mouse_y;
    int32_t new_x;
    int32_t new_y;

    if (!desktop_ready) {
        cursor_move(dx, dy, (uint8_t)buttons);
        return;
    }

    cursor_move(dx, dy, (uint8_t)buttons);
    mouse_x = (int32_t)cursor_x();
    mouse_y = (int32_t)cursor_y();
    left_now = buttons & NANOS_MOUSE_BUTTON_LEFT;
    left_was = last_buttons & NANOS_MOUSE_BUTTON_LEFT;
    changed_buttons = buttons ^ last_buttons;
    update_cursor_shape(mouse_x, mouse_y);

    if (wheel != 0 && drag_window == NULL && resize_window == NULL) {
        hit = gui_window_at(mouse_x, mouse_y);
        if (hit != NULL && gui_window_client_hit(hit, mouse_x, mouse_y)) {
            gui_window_dispatch_mouse_event(hit, WINDOW_EVENT_MOUSE_WHEEL,
                mouse_x, mouse_y, buttons, wheel);
        } else if (focused_window != NULL &&
                gui_window_client_hit(focused_window, mouse_x, mouse_y)) {
            gui_window_dispatch_mouse_event(focused_window,
                WINDOW_EVENT_MOUSE_WHEEL, mouse_x, mouse_y, buttons, wheel);
        }
        flush_user_window_redraws();
        last_buttons = buttons;
        return;
    }

    if (left_now != 0u && left_was == 0u) {
        event_window = NULL;
        drag_window = NULL;
        resize_window = NULL;
        panel_window = NULL;
        panel_action = shell_panel_handle_mouse_down(mouse_x, mouse_y, &panel_window);
        if (panel_action != 0) {
            cursor_hide();
            if (panel_action == SHELL_PANEL_ACTION_WINDOW) {
                restore_window(panel_window);
                cursor_show();
            } else {
                redraw_panel();
                cursor_show();
            }
            update_cursor_shape(mouse_x, mouse_y);
            last_buttons = buttons;
            return;
        }
        hit = gui_window_at(mouse_x, mouse_y);
        if (hit != NULL) {
            resize_edges = gui_window_resize_hit(hit, mouse_x, mouse_y);
            if (gui_window_close_hit(hit, mouse_x, mouse_y)) {
                cursor_hide();
                gui_window_dispatch_event(hit, WINDOW_EVENT_CLOSE);
                close_window(hit);
                cursor_show();
                update_cursor_shape(mouse_x, mouse_y);
                last_buttons = buttons;
                return;
            }
            if (gui_window_maximize_hit(hit, mouse_x, mouse_y)) {
                cursor_hide();
                gui_window_dispatch_event(hit, WINDOW_EVENT_MAXIMIZE);
                toggle_maximize_window(hit);
                cursor_show();
                update_cursor_shape(mouse_x, mouse_y);
                last_buttons = buttons;
                return;
            }
            if (gui_window_minimize_hit(hit, mouse_x, mouse_y)) {
                cursor_hide();
                gui_window_dispatch_event(hit, WINDOW_EVENT_MINIMIZE);
                minimize_window(hit);
                cursor_show();
                update_cursor_shape(mouse_x, mouse_y);
                last_buttons = buttons;
                return;
            }
            if (resize_edges != 0u && !gui_window_is_maximized(hit)) {
                cursor_hide();
                focus_window(hit);
                cursor_show();
                event_window = NULL;
                resize_window = hit;
                resize_start_mouse_x = mouse_x;
                resize_start_mouse_y = mouse_y;
                resize_start_x = hit->x;
                resize_start_y = hit->y;
                resize_start_w = hit->width;
                resize_start_h = hit->height;
                desktop_resize_preview_begin(&resize_preview, hit);
                update_cursor_shape(mouse_x, mouse_y);
                flush_user_window_redraws();
                last_buttons = buttons;
                return;
            }
            if (focused_window != hit || hit != gui_window_last()) {
                cursor_hide();
                focus_window(hit);
                cursor_show();
            }
            if (gui_window_title_hit(hit, mouse_x, mouse_y) &&
                    !gui_window_is_maximized(hit)) {
                event_window = NULL;
                drag_window = hit;
                drag_offset_x = mouse_x - hit->x;
                drag_offset_y = mouse_y - hit->y;
                desktop_drag_preview_begin(&drag_preview, hit);
            } else if (gui_window_client_hit(hit, mouse_x, mouse_y)) {
                event_window = hit;
                gui_window_dispatch_mouse_event(hit, WINDOW_EVENT_MOUSE_DOWN,
                    mouse_x, mouse_y, buttons, 0);
            }
        }
    } else if (left_now != 0u && resize_window != NULL) {
        update_resize_outline(mouse_x, mouse_y);
    } else if (left_now != 0u && drag_window != NULL) {
        new_x = clamp_window_x(drag_window, mouse_x - drag_offset_x);
        new_y = clamp_window_y(drag_window, mouse_y - drag_offset_y);
        if (!drag_preview.visible || new_x != drag_preview.x ||
                new_y != drag_preview.y) {
            desktop_drag_preview_update(&drag_preview, new_x, new_y);
        }
    } else if (left_now != 0u && event_window != NULL) {
        gui_window_dispatch_mouse_event(event_window, WINDOW_EVENT_MOUSE_MOVE,
            mouse_x, mouse_y, buttons, 0);
    } else if (left_now == 0u && left_was != 0u) {
        if (resize_window != NULL) {
            struct window *released_window;
            int32_t old_x;
            int32_t old_y;
            uint32_t old_w;
            uint32_t old_h;

            cursor_hide();
            desktop_resize_preview_clear(&resize_preview);
            released_window = resize_window;
            old_x = released_window->x;
            old_y = released_window->y;
            old_w = released_window->width;
            old_h = released_window->height;
            gui_window_resize(released_window, resize_preview.x, resize_preview.y,
                resize_preview.width, resize_preview.height);
            gui_window_set_maximized(released_window, 0);
            resize_window = NULL;
            resize_preview.window = NULL;
            expose_window_change(old_x, old_y, old_w, old_h, released_window);
            flush_user_window_redraws();
            cursor_show();
        } else if (drag_window != NULL) {
            struct window *released_window;
            int32_t old_x;
            int32_t old_y;

            cursor_hide();
            desktop_drag_preview_clear(&drag_preview);
            released_window = drag_window;
            old_x = drag_window->x;
            old_y = drag_window->y;
            gui_window_move(drag_window, drag_preview.x, drag_preview.y);
            drag_window = NULL;
            drag_preview.window = NULL;
            expose_window_change(old_x, old_y, released_window->width,
                released_window->height, released_window);
            flush_user_window_redraws();
            cursor_show();
        } else if (event_window != NULL) {
            gui_window_dispatch_mouse_event(event_window, WINDOW_EVENT_MOUSE_UP,
                mouse_x, mouse_y, buttons, 0);
        }
        drag_window = NULL;
        resize_window = NULL;
        event_window = NULL;
        update_cursor_shape(mouse_x, mouse_y);
    } else if (changed_buttons != 0u) {
        hit = event_window;
        if (hit == NULL) {
            hit = gui_window_at(mouse_x, mouse_y);
        }
        if (hit != NULL && gui_window_client_hit(hit, mouse_x, mouse_y)) {
            event_window = hit;
            if ((buttons & changed_buttons) != 0u) {
                gui_window_dispatch_mouse_event(hit, WINDOW_EVENT_MOUSE_DOWN,
                    mouse_x, mouse_y, buttons, 0);
            } else {
                gui_window_dispatch_mouse_event(hit, WINDOW_EVENT_MOUSE_UP,
                    mouse_x, mouse_y, buttons, 0);
            }
        }
        if (buttons == 0u) {
            event_window = NULL;
        }
    } else if (buttons != 0u && event_window != NULL) {
        gui_window_dispatch_mouse_event(event_window, WINDOW_EVENT_MOUSE_MOVE,
            mouse_x, mouse_y, buttons, 0);
    } else if (focused_window != NULL && gui_window_client_hit(focused_window, mouse_x, mouse_y)) {
        gui_window_dispatch_mouse_event(focused_window, WINDOW_EVENT_MOUSE_MOVE,
            mouse_x, mouse_y, buttons, 0);
    }

    flush_user_window_redraws();
    last_buttons = buttons;
}

void desktop_on_key(uint32_t key)
{
    if (!desktop_ready || focused_window == NULL) {
        return;
    }

    gui_window_dispatch_key_event(focused_window, key);
}

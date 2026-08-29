#include <kernel/gui/app_button.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <nanos/syscall.h>

struct app_button {
    int used;
    uint32_t owner_pid;
    uint32_t id;
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

#define APP_BUTTON_CAPACITY (PROCESS_MAX * NANOS_BUTTON_MAX)

static struct app_button buttons[APP_BUTTON_CAPACITY];

void gui_app_buttons_clear(uint32_t owner_pid)
{
    uint32_t index;

    for (index = 0u; index < APP_BUTTON_CAPACITY; ++index) {
        if (buttons[index].owner_pid == owner_pid) {
            buttons[index].used = 0;
            buttons[index].owner_pid = 0u;
        }
    }
}

int gui_app_button_define(uint32_t owner_pid, uint32_t id, int32_t x,
    int32_t y, uint32_t width, uint32_t height)
{
    uint32_t slot;
    uint32_t free_index;
    struct app_button *button;

    if (owner_pid == 0u || id == 0u || width == 0u || height == 0u) {
        return -1;
    }

    slot = APP_BUTTON_CAPACITY;
    free_index = APP_BUTTON_CAPACITY;
    for (slot = 0u; slot < APP_BUTTON_CAPACITY; ++slot) {
        button = &buttons[slot];
        if (button->used && button->owner_pid == owner_pid &&
            button->id == id) {
            break;
        }
        if (!button->used && free_index == APP_BUTTON_CAPACITY) {
            free_index = slot;
        }
    }

    if (slot == APP_BUTTON_CAPACITY) {
        if (free_index == APP_BUTTON_CAPACITY) {
            return -1;
        }
        slot = free_index;
    }

    button = &buttons[slot];
    button->used = 1;
    button->owner_pid = owner_pid;
    button->id = id;
    button->x = x;
    button->y = y;
    button->width = width;
    button->height = height;
    return 0;
}

uint32_t gui_app_button_hit(uint32_t owner_pid, int32_t x, int32_t y)
{
    uint32_t index;
    struct app_button *button;

    if (owner_pid == 0u) {
        return 0u;
    }

    for (index = 0u; index < APP_BUTTON_CAPACITY; ++index) {
        button = &buttons[index];
        if (button->used && button->owner_pid == owner_pid &&
            x >= button->x && y >= button->y &&
            x < button->x + (int32_t)button->width &&
            y < button->y + (int32_t)button->height) {
            return button->id;
        }
    }

    return 0u;
}

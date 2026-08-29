#include <kernel/process.h>
#include <kernel/syscall.h>
#include <kernel/syscall/internal.h>
#include <kernel/types.h>

uint32_t syscall_dispatch(uint32_t number, uint32_t arg0, uint32_t arg1,
    uint32_t arg2, uint32_t arg3)
{
    if (number == SYSCALL_DRAW_WINDOW) {
        return syscall_draw_window(arg0, arg1, arg2,
            (const char *)(uintptr_t)arg3);
    }
    if (number == SYSCALL_GET_KEY) {
        return syscall_get_key();
    }
    if (number == SYSCALL_GET_BUTTON) {
        return syscall_get_button();
    }
    if (number == SYSCALL_GET_MOUSE) {
        return syscall_get_mouse((struct nanos_mouse_event *)(uintptr_t)arg0);
    }
    if (number == SYSCALL_PUT_PIXEL) {
        return syscall_put_pixel(arg0, arg1, arg2);
    }
    if (number == SYSCALL_WRITE_TEXT) {
        return syscall_write_text(arg0, arg1, (const char *)(uintptr_t)arg2);
    }
    if (number == SYSCALL_PUT_IMAGE) {
        return syscall_put_image(arg0, arg1,
            (const uint32_t *)(uintptr_t)arg2);
    }
    if (number == SYSCALL_DEFINE_BUTTON) {
        return syscall_define_button(arg0, arg1, arg2, arg3);
    }
    if (number == SYSCALL_PROCESS_INFO) {
        return syscall_process_info(arg1,
            (struct nanos_process_info *)(uintptr_t)arg0);
    }
    if (number == SYSCALL_EXIT) {
        syscall_gui_finish_current_draw();
        process_exit((int)arg0);
        return 0u;
    }
    if (number == SYSCALL_WAIT_EVENT) {
        return syscall_wait_event();
    }
    if (number == SYSCALL_POLL_EVENT) {
        return syscall_poll_event();
    }
    if (number == SYSCALL_WINDOW_DRAW) {
        return syscall_window_draw(arg0);
    }
    if (number == SYSCALL_DRAW_RECT) {
        return syscall_draw_rect(arg0, arg1, arg2);
    }
    if (number == SYSCALL_GET_SCREEN_SIZE) {
        return syscall_get_screen_size();
    }
    if (number == SYSCALL_APP_CONTROL) {
        return syscall_app_control(arg0, arg1);
    }
    if (number == SYSCALL_START_APP) {
        return syscall_start_app_from_user((const char *)(uintptr_t)arg0,
            (const char *)(uintptr_t)arg1);
    }
    if (number == SYSCALL_WAIT_EVENT_TIMEOUT) {
        return syscall_wait_event_timeout(arg0);
    }
    if (number == SYSCALL_DRAW_LINE) {
        return syscall_draw_line(arg0, arg1, arg2);
    }
    if (number == SYSCALL_SET_EVENT_MASK) {
        return syscall_set_event_mask(arg0);
    }
    if (number == SYSCALL_WRITE_NUMBER) {
        return syscall_write_number(arg0, arg1, arg2, arg3);
    }
    if (number == SYSCALL_THREAD) {
        return syscall_thread(arg0, arg1, arg2);
    }
    if (number == SYSCALL_FILE) {
        return syscall_file_from_user(
            (const struct nanos_file_request *)(uintptr_t)arg0);
    }
    if (number == SYSCALL_IPC) {
        return syscall_ipc(arg0, arg1,
            (struct nanos_ipc_message *)(uintptr_t)arg2);
    }
    return 0xffffffffu;
}

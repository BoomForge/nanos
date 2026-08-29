#ifndef KERNEL_SYSCALL_INTERNAL_H
#define KERNEL_SYSCALL_INTERNAL_H

#include <kernel/types.h>
#include <nanos/syscall.h>

int syscall_copy_user_string(const char *user_text, char *out,
    uint32_t out_size);
int syscall_unpack_position_size(uint32_t packed, int32_t *pos,
    uint32_t *size);

uint32_t syscall_draw_window(uint32_t packed_x, uint32_t packed_y,
    uint32_t color, const char *user_title);
uint32_t syscall_window_draw(uint32_t phase);
uint32_t syscall_put_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t syscall_write_text(uint32_t packed_xy, uint32_t color,
    const char *user_text);
uint32_t syscall_put_image(uint32_t packed_x, uint32_t packed_y,
    const uint32_t *user_pixels);
uint32_t syscall_define_button(uint32_t packed_x, uint32_t packed_y,
    uint32_t id, uint32_t color);
uint32_t syscall_draw_rect(uint32_t packed_x, uint32_t packed_y,
    uint32_t color);
uint32_t syscall_get_screen_size(void);
uint32_t syscall_draw_line(uint32_t packed_start, uint32_t packed_end,
    uint32_t color);
uint32_t syscall_write_number(uint32_t format, uint32_t value,
    uint32_t packed_xy, uint32_t color);

uint32_t syscall_poll_event(void);
uint32_t syscall_get_key(void);
uint32_t syscall_get_button(void);
uint32_t syscall_get_mouse(struct nanos_mouse_event *user_event);
uint32_t syscall_wait_event(void);
uint32_t syscall_wait_event_timeout(uint32_t timeout_ticks);
uint32_t syscall_set_event_mask(uint32_t mask);

uint32_t syscall_start_app_from_user(const char *user_path,
    const char *user_param);
uint32_t syscall_thread(uint32_t operation, uint32_t entry,
    uint32_t stack_top);
uint32_t syscall_process_info(uint32_t selector,
    struct nanos_process_info *user_info);
uint32_t syscall_app_control(uint32_t operation, uint32_t target_pid);
uint32_t syscall_file_from_user(const struct nanos_file_request *user_request);
uint32_t syscall_ipc(uint32_t operation, uint32_t target_pid,
    struct nanos_ipc_message *user_message);
void syscall_gui_finish_current_draw(void);

#endif

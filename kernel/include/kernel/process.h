#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <kernel/compiler.h>
#include <kernel/nx_loader.h>
#include <kernel/platform_context.h>
#include <nanos/app.h>
#include <nanos/syscall.h>
#include <kernel/pmm.h>
#include <kernel/types.h>

#define PROCESS_MAX 8u
#define PROCESS_STACK_SIZE 4096u
#define PROCESS_KEY_QUEUE_SIZE 16u
#define PROCESS_IPC_QUEUE_SIZE 4u

typedef void (*process_exit_hook_fn)(uint32_t pid, int status);

enum process_state {
    PROCESS_EMPTY = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_EXITED
};

struct process {
    uint32_t pid;
    uint32_t app_pid;
    char name[64];
    enum process_state state;
    int exit_status;
    uint32_t address_space;
    uint32_t image_size;
    uint32_t stack_size;
    uint32_t stack_top;
    uint32_t entry_offset;
    uint32_t icon_offset;
    uint32_t service_id;
    uint8_t *image_memory;
    uint32_t preempt_count;
    uint32_t event_mask;
    uint32_t pending_events;
    uint32_t key_queue[PROCESS_KEY_QUEUE_SIZE];
    uint32_t key_read_pos;
    uint32_t key_write_pos;
    uint32_t key_count;
    struct nanos_ipc_message ipc_queue[PROCESS_IPC_QUEUE_SIZE];
    uint32_t ipc_read_pos;
    uint32_t ipc_write_pos;
    uint32_t ipc_count;
    struct nanos_mouse_event mouse_event;
    uint32_t button_state;
    int context_started;
    uint8_t context[PLATFORM_PROCESS_CONTEXT_SIZE]
        ALIGNED(PLATFORM_PROCESS_CONTEXT_ALIGN);
    uint8_t image[NX_IMAGE_MAX] ALIGNED(PMM_PAGE_SIZE);
    uint8_t stack[PROCESS_STACK_SIZE] ALIGNED(PMM_PAGE_SIZE);
};

void process_init(void);
void process_set_exit_hook(process_exit_hook_fn hook);
int process_start_app(const char *path, const char *param);
int process_create_thread_current(uint32_t entry, uint32_t stack_top);
int process_has_ready(void);
void process_set_event_mask_current(uint32_t mask);
uint32_t process_poll_event_current(void);
uint32_t process_get_key_current(void);
uint32_t process_get_button_current(void);
int process_get_mouse_current(struct nanos_mouse_event *event);
int process_get_ipc_current(struct nanos_ipc_message *message);
int process_peek_ipc_current(struct nanos_ipc_message *message);
uint32_t process_ipc_pending_current(void);
void process_post_event(struct process *process, uint32_t event);
void process_post_key(struct process *process, uint32_t key);
void process_post_button(struct process *process, uint32_t id);
int process_post_ipc_to_pid(uint32_t target_pid,
    const struct nanos_ipc_message *message);
void process_post_event_to_pid(uint32_t pid, uint32_t event);
void process_post_key_to_pid(uint32_t pid, uint32_t key);
void process_post_button_to_pid(uint32_t pid, uint32_t id);
void process_post_mouse_to_pid(uint32_t pid, const struct nanos_mouse_event *event);
int process_schedule(void);
void process_exit(int status);
int process_kill_app(uint32_t target_pid, int status);
uint32_t process_active_count(void);
int process_get_info(uint32_t selector, struct nanos_process_info *info);
int process_bind_service_current(uint32_t service_id);
uint32_t process_resolve_service(uint32_t service_id);
int process_unbind_service_current(uint32_t service_id);
struct process *process_current(void);
uint32_t process_current_pid(void);
uint32_t process_current_app_pid(void);
uint32_t process_current_app_slot(void);
uint32_t process_app_pid_for_pid(uint32_t pid);
uint32_t process_app_slot_for_pid(uint32_t pid);
struct process *process_on_preempt(void);

#endif

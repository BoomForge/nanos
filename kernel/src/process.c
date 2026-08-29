#include <kernel/nx_loader.h>
#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/process/internal.h>
#include <kernel/string.h>
#include <kernel/types.h>

static struct process processes[PROCESS_MAX];
static struct process *current_process;
static struct nx_image loader_image;
static uint32_t next_pid;
static uint32_t last_schedule_index;
static process_exit_hook_fn exit_hook;

uint32_t process_event_to_mask_internal(uint32_t event)
{
    if (event < NANOS_EVENT_REDRAW || event > NANOS_EVENT_STACK) {
        return 0u;
    }

    return 1u << (event - 1u);
}

static void clear_process_slot(struct process *process, uint32_t index)
{
    process->pid = 0u;
    process->app_pid = 0u;
    process->name[0] = '\0';
    process->state = PROCESS_EMPTY;
    process->exit_status = 0;
    process->address_space = index;
    process->image_size = 0u;
    process->stack_size = 0u;
    process->stack_top = 0u;
    process->entry_offset = 0u;
    process->icon_offset = NANOS_APP_ICON_NONE;
    process->service_id = 0u;
    process->image_memory = process->image;
    process->preempt_count = 0u;
    process->event_mask = NANOS_EVENT_MASK_DEFAULT;
    process->pending_events = 0u;
    process->key_read_pos = 0u;
    process->key_write_pos = 0u;
    process->key_count = 0u;
    process->ipc_read_pos = 0u;
    process->ipc_write_pos = 0u;
    process->ipc_count = 0u;
    memset(process->ipc_queue, 0, sizeof(process->ipc_queue));
    memset(&process->mouse_event, 0, sizeof(process->mouse_event));
    process->button_state = 0u;
    process->context_started = 0;
    memset(&process->context, 0, sizeof(process->context));
}

static void copy_process_name(struct process *process, const char *name)
{
    uint32_t i;

    i = 0u;
    while (name[i] != '\0' && i + 1u < sizeof(process->name)) {
        process->name[i] = name[i];
        ++i;
    }
    process->name[i] = '\0';
}

static int copy_param_value(char *dest, const char *src)
{
    uint32_t i;

    if (dest == NULL || src == NULL) {
        return -1;
    }

    for (i = 0u; i < NANOS_APP_PARAM_MAX; ++i) {
        dest[i] = src[i];
        if (dest[i] == '\0') {
            return 0;
        }
    }

    dest[NANOS_APP_PARAM_MAX - 1u] = '\0';
    return -1;
}

static void apply_loaded_image(struct process *process,
    const struct nx_image *image)
{
    copy_process_name(process, image->name);
    memset(process->image, 0, sizeof(process->image));
    memcpy(process->image, image->data, image->file_size);
    memset(process->stack, 0, sizeof(process->stack));
    process->app_pid = process->pid;
    process->image_size = image->memory_size;
    process->stack_size = image->stack_size;
    process->stack_top = image->stack_top;
    process->entry_offset = image->entry_offset;
    process->icon_offset = image->icon_offset;
    process->service_id = 0u;
    process->image_memory = process->image;
    process->preempt_count = 0u;
    process->pending_events = 0u;
    process->key_read_pos = 0u;
    process->key_write_pos = 0u;
    process->key_count = 0u;
    process->ipc_read_pos = 0u;
    process->ipc_write_pos = 0u;
    process->ipc_count = 0u;
    memset(process->ipc_queue, 0, sizeof(process->ipc_queue));
    memset(&process->mouse_event, 0, sizeof(process->mouse_event));
    process->button_state = 0u;
    process->context_started = 0;
    process->exit_status = 0;
    memset(&process->context, 0, sizeof(process->context));
}

void process_init(void)
{
    uint32_t i;

    for (i = 0u; i < PROCESS_MAX; ++i) {
        clear_process_slot(&processes[i], i);
    }

    current_process = NULL;
    next_pid = 1u;
    last_schedule_index = 0u;
    exit_hook = NULL;
}

void process_set_exit_hook(process_exit_hook_fn hook)
{
    exit_hook = hook;
}

static struct process *alloc_process(void)
{
    uint32_t i;

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (processes[i].state == PROCESS_EMPTY) {
            return &processes[i];
        }
    }

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (processes[i].state == PROCESS_EXITED) {
            return &processes[i];
        }
    }

    return NULL;
}

struct process *process_find_internal(uint32_t pid)
{
    uint32_t i;

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (processes[i].state != PROCESS_EMPTY &&
            processes[i].pid == pid) {
            return &processes[i];
        }
    }

    return NULL;
}

struct process *process_find_app_owner_internal(uint32_t app_pid)
{
    struct process *process;

    process = process_find_internal(app_pid);
    if (process == NULL || process->app_pid != app_pid) {
        return NULL;
    }

    return process;
}

struct process *process_next_ready_internal(void)
{
    uint32_t offset;
    uint32_t index;

    for (offset = 0u; offset < PROCESS_MAX; ++offset) {
        index = (last_schedule_index + offset) % PROCESS_MAX;
        if (processes[index].state == PROCESS_READY) {
            last_schedule_index = (index + 1u) % PROCESS_MAX;
            return &processes[index];
        }
    }

    return NULL;
}

int process_has_ready_internal(void)
{
    uint32_t i;

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (processes[i].state == PROCESS_READY) {
            return 1;
        }
    }

    return 0;
}

int process_has_ready(void)
{
    return process_has_ready_internal();
}

void process_set_current_internal(struct process *process)
{
    current_process = process;
}

void process_reset_slot_internal(struct process *process)
{
    uint32_t index;

    if (process == NULL) {
        return;
    }

    index = (uint32_t)(process - processes);
    if (index >= PROCESS_MAX) {
        return;
    }

    clear_process_slot(process, index);
}

uint32_t process_slot_count_internal(void)
{
    return PROCESS_MAX;
}

struct process *process_slot_internal(uint32_t index)
{
    if (index >= PROCESS_MAX) {
        return NULL;
    }

    return &processes[index];
}

uint32_t process_active_count(void)
{
    uint32_t i;
    uint32_t count;

    count = 0u;
    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (processes[i].state != PROCESS_EMPTY &&
            processes[i].state != PROCESS_EXITED) {
            ++count;
        }
    }

    return count;
}

static struct process *process_by_active_index(uint32_t active_index)
{
    uint32_t i;
    uint32_t seen;

    seen = 0u;
    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (processes[i].state == PROCESS_EMPTY ||
            processes[i].state == PROCESS_EXITED) {
            continue;
        }
        if (seen == active_index) {
            return &processes[i];
        }
        ++seen;
    }

    return NULL;
}

static void fill_process_info(struct process *process,
    struct nanos_process_info *info)
{
    uint32_t i;

    memset(info, 0, sizeof(*info));
    info->pid = process->pid;
    info->app_pid = process->app_pid;
    info->state = (uint32_t)process->state;
    info->slot = process->address_space;
    info->preempts = process->preempt_count;
    info->image_size = process->image_size;
    info->stack_size = process->stack_size;
    info->entry_offset = process->entry_offset;
    info->stack_top = process->stack_top;
    info->icon_offset = process->icon_offset;
    info->service_id = process->service_id;

    for (i = 0u; i + 1u < NANOS_PROCESS_INFO_NAME_MAX &&
        process->name[i] != '\0'; ++i) {
        info->name[i] = process->name[i];
    }
    info->name[i] = '\0';
}

int process_get_info(uint32_t selector, struct nanos_process_info *info)
{
    struct process *process;

    if (info == NULL) {
        return -1;
    }

    if (selector == NANOS_PROCESS_INFO_CURRENT) {
        process = process_current();
    } else {
        process = process_by_active_index(selector);
    }
    if (process == NULL) {
        return -1;
    }

    fill_process_info(process, info);
    return 0;
}

int process_start_app(const char *path, const char *param)
{
    struct process *process;
    char app_param[NANOS_APP_PARAM_MAX];

    if (path == NULL) {
        return -1;
    }
    if (param == NULL) {
        param = "";
    }
    if (copy_param_value(app_param, param) != 0) {
        return -1;
    }
    if (nx_load(path, &loader_image) != 0) {
        return -1;
    }

    process = alloc_process();
    if (process == NULL) {
        return -1;
    }

    process->pid = next_pid;
    ++next_pid;
    apply_loaded_image(process, &loader_image);

    if (platform_user_prepare(process) != 0) {
        process->state = PROCESS_EMPTY;
        return -1;
    }
    if (platform_user_setup_param(process, app_param) != 0) {
        process->state = PROCESS_EMPTY;
        return -1;
    }

    process->state = PROCESS_READY;
    process_post_event(process, NANOS_EVENT_REDRAW);
    return (int)process->pid;
}

static int virtual_range_contains(uint32_t base, uint32_t size,
    uint32_t value)
{
    if (size == 0u || value < base) {
        return 0;
    }
    if (value >= base + size || base + size < base) {
        return 0;
    }
    return 1;
}

int process_create_thread_current(uint32_t entry, uint32_t stack_top)
{
    struct process *parent;
    struct process *thread;

    parent = process_current();
    if (parent == NULL || parent->state != PROCESS_RUNNING ||
        parent->app_pid == 0u ||
        !virtual_range_contains(NANOS_APP_LOAD_BASE, parent->image_size,
        entry) ||
        stack_top < NANOS_APP_STACK_BASE ||
        stack_top > NANOS_APP_STACK_BASE + parent->stack_size ||
        ((stack_top - NANOS_APP_STACK_BASE) & 3u) != 0u) {
        return -1;
    }

    thread = alloc_process();
    if (thread == NULL) {
        return -1;
    }

    thread->pid = next_pid;
    ++next_pid;
    thread->app_pid = parent->app_pid;
    copy_process_name(thread, parent->name);
    memset(thread->stack, 0, sizeof(thread->stack));
    thread->image_size = parent->image_size;
    thread->stack_size = parent->stack_size;
    thread->stack_top = stack_top;
    thread->entry_offset = entry - NANOS_APP_LOAD_BASE;
    thread->icon_offset = parent->icon_offset;
    thread->service_id = 0u;
    thread->image_memory = parent->image_memory;
    thread->preempt_count = 0u;
    thread->event_mask = NANOS_EVENT_MASK_DEFAULT;
    thread->pending_events = 0u;
    thread->key_read_pos = 0u;
    thread->key_write_pos = 0u;
    thread->key_count = 0u;
    thread->ipc_read_pos = 0u;
    thread->ipc_write_pos = 0u;
    thread->ipc_count = 0u;
    memset(thread->ipc_queue, 0, sizeof(thread->ipc_queue));
    memset(&thread->mouse_event, 0, sizeof(thread->mouse_event));
    thread->button_state = 0u;
    thread->context_started = 0;
    thread->exit_status = 0;
    memset(&thread->context, 0, sizeof(thread->context));

    if (platform_user_prepare(thread) != 0) {
        thread->state = PROCESS_EMPTY;
        return -1;
    }

    thread->state = PROCESS_READY;
    return (int)thread->pid;
}

static void exit_app_threads(uint32_t app_pid, struct process *owner)
{
    uint32_t i;

    if (app_pid == 0u) {
        return;
    }

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (&processes[i] != owner &&
            processes[i].state != PROCESS_EMPTY &&
            processes[i].state != PROCESS_EXITED &&
            processes[i].app_pid == app_pid) {
            processes[i].state = PROCESS_EXITED;
            processes[i].exit_status = 0;
            processes[i].service_id = 0u;
        }
    }
}

static int mark_app_exited(uint32_t app_pid, int status)
{
    struct process *process;
    struct process *owner;
    uint32_t i;
    int current_killed;
    int found;

    owner = process_find_app_owner_internal(app_pid);
    if (owner == NULL || owner->state == PROCESS_EMPTY ||
        owner->state == PROCESS_EXITED) {
        return -1;
    }

    current_killed = 0;
    found = 0;
    for (i = 0u; i < PROCESS_MAX; ++i) {
        process = &processes[i];
        if (process->state == PROCESS_EMPTY ||
            process->state == PROCESS_EXITED ||
            process->app_pid != app_pid) {
            continue;
        }

        if (process == current_process) {
            current_killed = 1;
        }
        process->state = PROCESS_EXITED;
        process->exit_status = status;
        process->pending_events = 0u;
        process->service_id = 0u;
        found = 1;
    }

    if (!found) {
        return -1;
    }
    if (current_killed) {
        current_process = NULL;
    }
    if (exit_hook != NULL) {
        exit_hook(app_pid, status);
    }

    return current_killed;
}

void process_exit(int status)
{
    struct process *exited;

    if (current_process != NULL) {
        exited = current_process;
        exited->state = PROCESS_EXITED;
        exited->exit_status = status;
        exited->service_id = 0u;
        current_process = NULL;
        if (exited->app_pid == exited->pid) {
            exit_app_threads(exited->app_pid, exited);
        }
        if (exit_hook != NULL) {
            exit_hook(exited->pid, status);
        }
    }

    platform_user_exit(status);
}

int process_kill_app(uint32_t target_pid, int status)
{
    struct process *target;

    target = process_find_internal(target_pid);
    if (target == NULL || target->app_pid == 0u ||
        target->state == PROCESS_EMPTY || target->state == PROCESS_EXITED) {
        return -1;
    }

    return mark_app_exited(target->app_pid, status);
}

static int process_is_live_owner(struct process *process)
{
    return process != NULL &&
        process->state != PROCESS_EMPTY &&
        process->state != PROCESS_EXITED &&
        process->app_pid == process->pid;
}

static struct process *process_current_app_owner(void)
{
    struct process *current;

    current = process_current();
    if (current == NULL || current->app_pid == 0u) {
        return NULL;
    }

    return process_find_app_owner_internal(current->app_pid);
}

int process_bind_service_current(uint32_t service_id)
{
    struct process *owner;
    uint32_t i;

    if (service_id == 0u) {
        return -1;
    }

    owner = process_current_app_owner();
    if (!process_is_live_owner(owner)) {
        return -1;
    }

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (process_is_live_owner(&processes[i]) &&
            processes[i].service_id == service_id &&
            processes[i].app_pid != owner->app_pid) {
            return -1;
        }
    }

    owner->service_id = service_id;
    return 0;
}

uint32_t process_resolve_service(uint32_t service_id)
{
    uint32_t i;

    if (service_id == 0u) {
        return 0u;
    }

    for (i = 0u; i < PROCESS_MAX; ++i) {
        if (process_is_live_owner(&processes[i]) &&
            processes[i].service_id == service_id) {
            return processes[i].pid;
        }
    }

    return 0u;
}

int process_unbind_service_current(uint32_t service_id)
{
    struct process *owner;

    if (service_id == 0u) {
        return -1;
    }

    owner = process_current_app_owner();
    if (!process_is_live_owner(owner) || owner->service_id != service_id) {
        return -1;
    }

    owner->service_id = 0u;
    return 0;
}

struct process *process_current(void)
{
    return current_process;
}

uint32_t process_current_pid(void)
{
    if (current_process == NULL) {
        return 0u;
    }

    return current_process->pid;
}

uint32_t process_current_app_pid(void)
{
    if (current_process == NULL) {
        return 0u;
    }

    return current_process->app_pid;
}

uint32_t process_current_app_slot(void)
{
    if (current_process == NULL || current_process->app_pid == 0u) {
        return 0xffffffffu;
    }

    return process_app_slot_for_pid(current_process->pid);
}

uint32_t process_app_pid_for_pid(uint32_t pid)
{
    struct process *process;

    process = process_find_internal(pid);
    if (process == NULL) {
        return 0u;
    }

    return process->app_pid;
}

uint32_t process_app_slot_for_pid(uint32_t pid)
{
    struct process *process;
    struct process *owner;

    process = process_find_internal(pid);
    if (process == NULL || process->app_pid == 0u) {
        return 0xffffffffu;
    }

    owner = process_find_app_owner_internal(process->app_pid);
    if (owner == NULL) {
        return 0xffffffffu;
    }

    return owner->address_space;
}

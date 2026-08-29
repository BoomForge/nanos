#include <kernel/process.h>
#include <kernel/process/internal.h>
#include <kernel/string.h>
#include <kernel/types.h>

void process_set_event_mask_current(uint32_t mask)
{
    struct process *current;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING) {
        return;
    }

    current->event_mask = mask;
    if ((mask & NANOS_EVENT_MASK_REDRAW) != 0u) {
        process_post_event(current, NANOS_EVENT_REDRAW);
    }
    if ((mask & NANOS_EVENT_MASK_KEY) != 0u &&
        current->key_count != 0u) {
        process_post_event(current, NANOS_EVENT_KEY);
    }
    if ((mask & NANOS_EVENT_MASK_BUTTON) != 0u &&
        current->button_state != 0u) {
        process_post_event(current, NANOS_EVENT_BUTTON);
    }
    if ((mask & NANOS_EVENT_MASK_IPC) != 0u &&
        current->ipc_count != 0u) {
        process_post_event(current, NANOS_EVENT_IPC);
    }
}

uint32_t process_poll_event_current(void)
{
    struct process *current;
    uint32_t event;
    uint32_t mask;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING) {
        return NANOS_EVENT_NONE;
    }

    for (event = NANOS_EVENT_REDRAW; event <= NANOS_EVENT_STACK; ++event) {
        mask = process_event_to_mask_internal(event);
        if ((current->pending_events & mask) != 0u) {
            current->pending_events &= ~mask;
            return event;
        }
    }

    return NANOS_EVENT_NONE;
}

uint32_t process_get_key_current(void)
{
    struct process *current;
    uint32_t key;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING ||
        current->key_count == 0u) {
        return 0u;
    }

    key = current->key_queue[current->key_read_pos];
    current->key_read_pos =
        (current->key_read_pos + 1u) % PROCESS_KEY_QUEUE_SIZE;
    --current->key_count;
    if (current->key_count != 0u) {
        process_post_event(current, NANOS_EVENT_KEY);
    }
    return key;
}

uint32_t process_get_button_current(void)
{
    struct process *current;
    uint32_t id;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING) {
        return 0u;
    }

    id = current->button_state;
    current->button_state = 0u;
    return id;
}

int process_get_mouse_current(struct nanos_mouse_event *event)
{
    struct process *current;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING ||
        event == NULL) {
        return -1;
    }

    memcpy(event, &current->mouse_event, sizeof(current->mouse_event));
    return 0;
}

int process_get_ipc_current(struct nanos_ipc_message *message)
{
    struct process *current;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING ||
        message == NULL || current->ipc_count == 0u) {
        return -1;
    }

    memcpy(message, &current->ipc_queue[current->ipc_read_pos],
        sizeof(message[0]));
    current->ipc_read_pos =
        (current->ipc_read_pos + 1u) % PROCESS_IPC_QUEUE_SIZE;
    --current->ipc_count;
    if (current->ipc_count != 0u) {
        process_post_event(current, NANOS_EVENT_IPC);
    }

    return 0;
}

int process_peek_ipc_current(struct nanos_ipc_message *message)
{
    struct process *current;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING ||
        message == NULL || current->ipc_count == 0u) {
        return -1;
    }

    memcpy(message, &current->ipc_queue[current->ipc_read_pos],
        sizeof(message[0]));
    return 0;
}

uint32_t process_ipc_pending_current(void)
{
    struct process *current;

    current = process_current();
    if (current == NULL || current->state != PROCESS_RUNNING) {
        return 0u;
    }

    return current->ipc_count;
}

void process_post_event(struct process *process, uint32_t event)
{
    uint32_t mask;

    if (process == NULL || process->state == PROCESS_EMPTY ||
        process->state == PROCESS_EXITED) {
        return;
    }

    mask = process_event_to_mask_internal(event);
    if (mask == 0u || (process->event_mask & mask) == 0u) {
        return;
    }

    process->pending_events |= mask;
}

void process_post_key(struct process *process, uint32_t key)
{
    if (process == NULL || process->state == PROCESS_EMPTY ||
        process->state == PROCESS_EXITED || process->key_count >=
        PROCESS_KEY_QUEUE_SIZE) {
        return;
    }

    process->key_queue[process->key_write_pos] = key;
    process->key_write_pos =
        (process->key_write_pos + 1u) % PROCESS_KEY_QUEUE_SIZE;
    ++process->key_count;
    process_post_event(process, NANOS_EVENT_KEY);
}

void process_post_button(struct process *process, uint32_t id)
{
    if (process == NULL || process->state == PROCESS_EMPTY ||
        process->state == PROCESS_EXITED || id == 0u) {
        return;
    }

    process->button_state = id;
    process_post_event(process, NANOS_EVENT_BUTTON);
}

int process_post_ipc_to_pid(uint32_t target_pid,
    const struct nanos_ipc_message *message)
{
    struct process *target;
    struct process *owner;

    target = process_find_internal(target_pid);
    if (target == NULL || message == NULL || target->state == PROCESS_EMPTY ||
        target->state == PROCESS_EXITED ||
        message->size == 0u || message->size > NANOS_IPC_MESSAGE_DATA_MAX) {
        return -1;
    }
    owner = process_find_app_owner_internal(target->app_pid);
    if (owner != NULL) {
        target = owner;
    }
    if (target->ipc_count >= PROCESS_IPC_QUEUE_SIZE) {
        return -1;
    }

    memcpy(&target->ipc_queue[target->ipc_write_pos], message,
        sizeof(message[0]));
    target->ipc_write_pos =
        (target->ipc_write_pos + 1u) % PROCESS_IPC_QUEUE_SIZE;
    ++target->ipc_count;
    process_post_event(target, NANOS_EVENT_IPC);
    return 0;
}

void process_post_event_to_pid(uint32_t pid, uint32_t event)
{
    process_post_event(process_find_internal(pid), event);
}

void process_post_key_to_pid(uint32_t pid, uint32_t key)
{
    process_post_key(process_find_internal(pid), key);
}

void process_post_button_to_pid(uint32_t pid, uint32_t id)
{
    process_post_button(process_find_internal(pid), id);
}

void process_post_mouse_to_pid(uint32_t pid,
    const struct nanos_mouse_event *event)
{
    struct process *process;

    process = process_find_internal(pid);
    if (process == NULL || event == NULL ||
        process->state == PROCESS_EMPTY ||
        process->state == PROCESS_EXITED) {
        return;
    }

    memcpy(&process->mouse_event, event, sizeof(process->mouse_event));
    process_post_event(process, NANOS_EVENT_MOUSE);
}

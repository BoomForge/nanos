#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/string.h>
#include <kernel/syscall/internal.h>
#include <kernel/types.h>

static uint32_t syscall_ipc_send(uint32_t target_pid,
    struct nanos_ipc_message *user_message)
{
    struct nanos_ipc_message message;
    struct process *current;

    current = process_current();
    if (current == NULL ||
        !platform_user_range_is_valid(user_message, NANOS_IPC_MESSAGE_SIZE)) {
        return NANOS_ERROR_INVALID;
    }

    memcpy(&message, user_message, sizeof(message));
    if (message.size == 0u || message.size > NANOS_IPC_MESSAGE_DATA_MAX) {
        return NANOS_ERROR_INVALID;
    }

    message.source_pid = current->app_pid;
    if (process_post_ipc_to_pid(target_pid, &message) != 0) {
        return NANOS_ERROR_INVALID;
    }

    return 0u;
}

static uint32_t syscall_ipc_recv(struct nanos_ipc_message *user_message)
{
    struct nanos_ipc_message message;

    if (!platform_user_range_is_valid(user_message, NANOS_IPC_MESSAGE_SIZE)) {
        return NANOS_ERROR_INVALID;
    }
    if (process_get_ipc_current(&message) != 0) {
        return 0u;
    }

    memcpy(user_message, &message, sizeof(message));
    return message.size;
}

static uint32_t syscall_ipc_peek(struct nanos_ipc_message *user_message)
{
    struct nanos_ipc_message message;

    if (!platform_user_range_is_valid(user_message, NANOS_IPC_MESSAGE_SIZE)) {
        return NANOS_ERROR_INVALID;
    }
    if (process_peek_ipc_current(&message) != 0) {
        return 0u;
    }

    memcpy(user_message, &message, sizeof(message));
    return message.size;
}

uint32_t syscall_ipc(uint32_t operation, uint32_t target_pid,
    struct nanos_ipc_message *user_message)
{
    if (operation == NANOS_IPC_SEND) {
        return syscall_ipc_send(target_pid, user_message);
    }
    if (operation == NANOS_IPC_RECV) {
        return syscall_ipc_recv(user_message);
    }
    if (operation == NANOS_IPC_BIND_SERVICE) {
        if (process_bind_service_current(target_pid) != 0) {
            return NANOS_ERROR_INVALID;
        }
        return 0u;
    }
    if (operation == NANOS_IPC_RESOLVE_SERVICE) {
        return process_resolve_service(target_pid);
    }
    if (operation == NANOS_IPC_UNBIND_SERVICE) {
        if (process_unbind_service_current(target_pid) != 0) {
            return NANOS_ERROR_INVALID;
        }
        return 0u;
    }
    if (operation == NANOS_IPC_PENDING) {
        return process_ipc_pending_current();
    }
    if (operation == NANOS_IPC_PEEK) {
        return syscall_ipc_peek(user_message);
    }

    return NANOS_ERROR_INVALID;
}

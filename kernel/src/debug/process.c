#include <kernel/debug/process.h>
#include <kernel/process.h>
#include <kernel/process/internal.h>
#include <kernel/types.h>

int debug_process_reap(uint32_t pid)
{
    struct process *process;

    process = process_find_internal(pid);
    if (process == NULL || process->state != PROCESS_EXITED) {
        return -1;
    }

    process_reset_slot_internal(process);
    return 0;
}

const struct process *debug_process_find(uint32_t pid)
{
    return process_find_internal(pid);
}

uint32_t debug_process_count(void)
{
    return process_slot_count_internal();
}

const struct process *debug_process_get(uint32_t index)
{
    return process_slot_internal(index);
}

const char *debug_process_state_name(enum process_state state)
{
    if (state == PROCESS_RUNNING) {
        return "running";
    }
    if (state == PROCESS_READY) {
        return "ready";
    }
    if (state == PROCESS_EXITED) {
        return "exited";
    }

    return "empty";
}

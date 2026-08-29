#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/process/internal.h>

int process_schedule(void)
{
    struct process *process;
    int entered;

    if (process_current() != NULL) {
        return -1;
    }

    entered = 0;
    while (process_has_ready_internal()) {
        process = process_next_ready_internal();
        if (process == NULL) {
            break;
        }

        process_set_current_internal(process);
        process->state = PROCESS_RUNNING;
        entered = 1;
        (void)platform_user_enter(process);
    }

    return entered ? 0 : -1;
}

struct process *process_on_preempt(void)
{
    struct process *previous;
    struct process *next;

    previous = process_current();
    if (previous == NULL || previous->state != PROCESS_RUNNING) {
        return previous;
    }

    next = process_next_ready_internal();
    if (next == NULL) {
        return previous;
    }

    ++previous->preempt_count;
    previous->state = PROCESS_READY;
    process_set_current_internal(next);
    next->state = PROCESS_RUNNING;
    return next;
}

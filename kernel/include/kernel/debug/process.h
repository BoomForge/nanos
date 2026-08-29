#ifndef KERNEL_DEBUG_PROCESS_H
#define KERNEL_DEBUG_PROCESS_H

#include <kernel/process.h>
#include <kernel/types.h>

int debug_process_reap(uint32_t pid);
const struct process *debug_process_find(uint32_t pid);
uint32_t debug_process_count(void);
const struct process *debug_process_get(uint32_t index);
const char *debug_process_state_name(enum process_state state);

#endif

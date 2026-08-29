#ifndef KERNEL_PROCESS_INTERNAL_H
#define KERNEL_PROCESS_INTERNAL_H

#include <kernel/process.h>
#include <kernel/types.h>

uint32_t process_event_to_mask_internal(uint32_t event);
struct process *process_find_internal(uint32_t pid);
struct process *process_find_app_owner_internal(uint32_t app_pid);
struct process *process_next_ready_internal(void);
int process_has_ready_internal(void);
void process_set_current_internal(struct process *process);
void process_reset_slot_internal(struct process *process);
uint32_t process_slot_count_internal(void);
struct process *process_slot_internal(uint32_t index);

#endif

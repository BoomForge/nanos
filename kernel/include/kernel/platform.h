#ifndef KERNEL_PLATFORM_H
#define KERNEL_PLATFORM_H

#include <kernel/types.h>

struct process;

void platform_disable_interrupts(void);
void platform_enable_interrupts(void);
void platform_halt(void);
void platform_console_init(void);
void platform_console_putc(char ch);
void platform_cpu_init(void);
void platform_interrupts_init(void);
void platform_paging_init(void);
void platform_input_init(void);
void platform_timer_init(uint32_t ticks_per_second);
uint32_t platform_timer_ticks(void);
int platform_user_prepare(struct process *process);
int platform_user_setup_param(struct process *process, const char *param);
void platform_user_set_return_value(struct process *process, uint32_t value);
int platform_user_enter(struct process *process);
void platform_user_switch(struct process *process);
int platform_user_range_is_valid(const void *addr, uint32_t size);
void platform_user_exit(int status);

#endif

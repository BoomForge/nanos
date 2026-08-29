#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <kernel/types.h>

void kernel_time_init(uint32_t ticks_per_second);
uint32_t kernel_time_ticks(void);
uint32_t kernel_time_ticks_per_second(void);
uint32_t kernel_time_seconds(void);

#endif

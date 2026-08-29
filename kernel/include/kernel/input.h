#ifndef KERNEL_INPUT_H
#define KERNEL_INPUT_H

#include <kernel/types.h>

void kernel_input_init(void);
void kernel_input_on_key(char ch);
void kernel_input_on_key_event(uint32_t key);
void kernel_input_on_mouse(int32_t dx, int32_t dy, uint32_t buttons,
    int32_t wheel);

#endif

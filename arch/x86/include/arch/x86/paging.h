#ifndef ARCH_X86_PAGING_H
#define ARCH_X86_PAGING_H

#include <kernel/types.h>

void paging_init(void);
void paging_map_range(uint32_t start, uint32_t size);
void paging_map_user_readonly_range(uint32_t virtual_start,
    uint32_t physical_start, uint32_t size);
void paging_map_user_writable_range(uint32_t virtual_start,
    uint32_t physical_start, uint32_t size);
int paging_user_space_init(uint32_t slot, uint32_t image_physical,
    uint32_t image_size, uint32_t stack_physical, uint32_t stack_size);
void paging_switch_kernel_space(void);
void paging_switch_user_space(uint32_t slot);
int paging_is_enabled(void);

#endif

#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <kernel/types.h>

struct heap_stats {
    uint32_t total_bytes;
    uint32_t used_bytes;
    uint32_t free_bytes;
    uint32_t blocks;
};

void heap_init(uint32_t initial_pages);
void *heap_alloc(size_t size);
void heap_free(void *ptr);
struct heap_stats heap_get_stats(void);
void heap_print_summary(void);

#endif

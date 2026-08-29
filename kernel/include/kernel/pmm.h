#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <kernel/types.h>

#define PMM_PAGE_SIZE 4096u

struct pmm_stats {
    uint32_t total_pages;
    uint32_t free_pages;
    uint32_t used_pages;
    uint32_t highest_page;
};

void pmm_init(uint32_t multiboot_info);
uint32_t pmm_alloc_page(void);
uint32_t pmm_alloc_pages(uint32_t count);
void pmm_free_page(uint32_t addr);
struct pmm_stats pmm_get_stats(void);
void pmm_print_summary(void);

#endif

#include <kernel/framebuffer.h>
#include <kernel/multiboot2.h>
#include <kernel/pmm.h>
#include <kernel/print.h>
#include <kernel/types.h>

#define PMM_MAX_PAGES 1048576u
#define PMM_MAX_MEMORY_END 0xffffffffu
#define PMM_BITMAP_BYTES (PMM_MAX_PAGES / 8u)
#define PMM_LOW_RESERVED_END 0x00100000u

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

static uint8_t page_bitmap[PMM_BITMAP_BYTES];
static uint32_t highest_page_seen;
static uint32_t free_page_count;

static uint32_t align_down(uint32_t value, uint32_t alignment)
{
    return value & ~(alignment - 1u);
}

static uint32_t align_up(uint32_t value, uint32_t alignment)
{
    return align_down(value + alignment - 1u, alignment);
}

static int page_is_used(uint32_t page)
{
    return (page_bitmap[page / 8u] & (uint8_t)(1u << (page % 8u))) != 0u;
}

static void mark_page_used(uint32_t page)
{
    if (page >= PMM_MAX_PAGES) {
        return;
    }

    if (!page_is_used(page)) {
        page_bitmap[page / 8u] = (uint8_t)(page_bitmap[page / 8u] | (uint8_t)(1u << (page % 8u)));
        if (free_page_count > 0u) {
            --free_page_count;
        }
    }
}

static void mark_page_free(uint32_t page)
{
    if (page >= PMM_MAX_PAGES) {
        return;
    }

    if (page_is_used(page)) {
        page_bitmap[page / 8u] = (uint8_t)(page_bitmap[page / 8u] & (uint8_t)~(1u << (page % 8u)));
        ++free_page_count;
    }
}

static void mark_range_used(uint32_t start, uint32_t end)
{
    uint32_t page;
    uint32_t first_page;
    uint32_t last_page;

    if (end <= start) {
        return;
    }

    first_page = align_down(start, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;
    last_page = align_up(end, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;

    for (page = first_page; page < last_page && page < PMM_MAX_PAGES; ++page) {
        mark_page_used(page);
    }
}

static void mark_range_free(uint32_t start, uint32_t end)
{
    uint32_t page;
    uint32_t first_page;
    uint32_t last_page;

    if (end <= start) {
        return;
    }

    first_page = align_up(start, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;
    last_page = align_down(end, PMM_PAGE_SIZE) / PMM_PAGE_SIZE;

    for (page = first_page; page < last_page && page < PMM_MAX_PAGES; ++page) {
        mark_page_free(page);
        if (page > highest_page_seen) {
            highest_page_seen = page;
        }
    }
}

static uint32_t clamp_range_end(uint32_t start, uint32_t length)
{
    if (length > PMM_MAX_MEMORY_END - start) {
        return PMM_MAX_MEMORY_END;
    }
    return start + length;
}

static void free_available_memory(uint32_t multiboot_info)
{
    const struct multiboot2_tag *tag;
    const struct multiboot2_tag_mmap *mmap;
    const struct multiboot2_mmap_entry *entry;
    const uint8_t *entry_ptr;
    const uint8_t *entries_end;
    uint32_t start;
    uint32_t end;

    tag = multiboot2_find_tag(multiboot_info, MULTIBOOT2_TAG_TYPE_MMAP);
    if (tag == NULL) {
        print_writeln("memory map missing");
        return;
    }

    mmap = (const struct multiboot2_tag_mmap *)tag;
    if (mmap->entry_size < sizeof(*entry)) {
        print_writeln("memory map entry size invalid");
        return;
    }

    entry_ptr = (const uint8_t *)(mmap + 1);
    entries_end = ((const uint8_t *)tag) + tag->size;

    while (entry_ptr + sizeof(*entry) <= entries_end) {
        entry = (const struct multiboot2_mmap_entry *)(const void *)entry_ptr;
        if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE &&
                entry->base_addr_high == 0u && entry->length_high == 0u &&
                entry->base_addr_low < PMM_MAX_MEMORY_END) {
            start = entry->base_addr_low;
            end = clamp_range_end(start, entry->length_low);
            mark_range_free(start, end);
        }
        entry_ptr += mmap->entry_size;
    }
}

static void reserve_framebuffer(uint32_t multiboot_info)
{
    const struct multiboot2_tag *tag;
    const struct multiboot2_tag_framebuffer *fb;
    uint32_t bytes;

    tag = multiboot2_find_tag(multiboot_info, MULTIBOOT2_TAG_TYPE_FRAMEBUFFER);
    if (tag == NULL) {
        return;
    }

    fb = (const struct multiboot2_tag_framebuffer *)tag;
    if (fb->framebuffer_addr_high != 0u || fb->framebuffer_bpp != 32u) {
        return;
    }

    bytes = fb->framebuffer_pitch * fb->framebuffer_height;
    mark_range_used(fb->framebuffer_addr_low, fb->framebuffer_addr_low + bytes);
}

static void reserve_modules(uint32_t multiboot_info)
{
    const struct multiboot2_info *info;
    const struct multiboot2_tag *tag;
    const struct multiboot2_tag_module *module;
    uintptr_t end;

    info = (const struct multiboot2_info *)(uintptr_t)multiboot_info;
    tag = multiboot2_first_tag(multiboot_info);
    end = (uintptr_t)info + info->total_size;

    while ((uintptr_t)tag < end && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
            module = (const struct multiboot2_tag_module *)tag;
            mark_range_used(module->mod_start, module->mod_end);
        }
        tag = multiboot2_next_tag(tag);
    }
}

void pmm_init(uint32_t multiboot_info)
{
    uint32_t i;
    const struct multiboot2_info *info;
    uint32_t kernel_start;
    uint32_t kernel_end;
    uint32_t mb_start;
    uint32_t mb_end;

    for (i = 0u; i < PMM_BITMAP_BYTES; ++i) {
        page_bitmap[i] = 0xffu;
    }

    highest_page_seen = 0u;
    free_page_count = 0u;

    free_available_memory(multiboot_info);

    info = (const struct multiboot2_info *)(uintptr_t)multiboot_info;
    kernel_start = (uint32_t)(uintptr_t)&__kernel_start;
    kernel_end = (uint32_t)(uintptr_t)&__kernel_end;
    mb_start = multiboot_info;
    mb_end = multiboot_info + info->total_size;

    mark_range_used(0u, PMM_LOW_RESERVED_END);
    mark_range_used(kernel_start, kernel_end);
    mark_range_used(mb_start, mb_end);
    reserve_framebuffer(multiboot_info);
    reserve_modules(multiboot_info);
}

uint32_t pmm_alloc_page(void)
{
    return pmm_alloc_pages(1u);
}

uint32_t pmm_alloc_pages(uint32_t count)
{
    uint32_t page;
    uint32_t run;
    uint32_t i;

    if (count == 0u) {
        return 0u;
    }

    for (page = PMM_LOW_RESERVED_END / PMM_PAGE_SIZE; page <= highest_page_seen; ++page) {
        run = 0u;
        while (page + run <= highest_page_seen && run < count && !page_is_used(page + run)) {
            ++run;
        }

        if (run == count) {
            for (i = 0u; i < count; ++i) {
                mark_page_used(page + i);
            }
            return page * PMM_PAGE_SIZE;
        }

        page += run;
    }

    return 0u;
}

void pmm_free_page(uint32_t addr)
{
    if ((addr % PMM_PAGE_SIZE) != 0u || addr < PMM_LOW_RESERVED_END) {
        return;
    }

    mark_page_free(addr / PMM_PAGE_SIZE);
}

struct pmm_stats pmm_get_stats(void)
{
    struct pmm_stats stats;

    stats.highest_page = highest_page_seen;
    stats.total_pages = highest_page_seen + 1u;
    stats.free_pages = free_page_count;
    stats.used_pages = stats.total_pages - stats.free_pages;
    return stats;
}

void pmm_print_summary(void)
{
    struct pmm_stats stats;
    uint32_t total_kib;
    uint32_t free_kib;

    stats = pmm_get_stats();
    total_kib = stats.total_pages * (PMM_PAGE_SIZE / 1024u);
    free_kib = stats.free_pages * (PMM_PAGE_SIZE / 1024u);

    print_write("memory total ");
    print_uint(total_kib);
    print_write(" KiB, free ");
    print_uint(free_kib);
    print_write(" KiB, pages ");
    print_uint(stats.free_pages);
    print_write("/");
    print_uint(stats.total_pages);
    print_putc('\n');
}

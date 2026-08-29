#include <arch/x86/cpu.h>
#include <arch/x86/gdt.h>
#include <arch/x86/isr.h>
#include <arch/x86/paging.h>
#include <kernel/compiler.h>
#include <kernel/framebuffer.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/print.h>
#include <kernel/process.h>
#include <kernel/string.h>
#include <kernel/types.h>

#define PAGE_PRESENT 0x001u
#define PAGE_WRITABLE 0x002u
#define PAGE_USER 0x004u
#define KERNEL_PAGE_FLAGS (PAGE_PRESENT | PAGE_WRITABLE)
#define USER_READONLY_PAGE_FLAGS (PAGE_PRESENT | PAGE_USER)
#define USER_WRITABLE_PAGE_FLAGS (PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER)
#define PAGE_ENTRIES 1024u
#define EARLY_IDENTITY_BYTES (128u * 1024u * 1024u)
#define EARLY_TABLES (EARLY_IDENTITY_BYTES / (PAGE_ENTRIES * PMM_PAGE_SIZE))
#define EXTRA_TABLES 16u
#define USER_SPACE_COUNT PROCESS_MAX
#define USER_BASE_DIRECTORY 256u
#define USER_IMAGE_BASE 0x40000000u
#define USER_STACK_BASE 0x40010000u

static uint32_t page_directory[PAGE_ENTRIES] ALIGNED(PMM_PAGE_SIZE);
static uint32_t early_tables[EARLY_TABLES][PAGE_ENTRIES] ALIGNED(PMM_PAGE_SIZE);
static uint32_t extra_tables[EXTRA_TABLES][PAGE_ENTRIES] ALIGNED(PMM_PAGE_SIZE);
static uint32_t user_directories[USER_SPACE_COUNT][PAGE_ENTRIES]
    ALIGNED(PMM_PAGE_SIZE);
static uint32_t user_tables[USER_SPACE_COUNT][PAGE_ENTRIES]
    ALIGNED(PMM_PAGE_SIZE);
static uint32_t extra_tables_used;
static int paging_ready;

static uint32_t align_down(uint32_t value)
{
    return value & ~(PMM_PAGE_SIZE - 1u);
}

static uint32_t align_up(uint32_t value)
{
    return align_down(value + PMM_PAGE_SIZE - 1u);
}

static uint32_t *alloc_extra_table(void)
{
    uint32_t *table;

    if (extra_tables_used >= EXTRA_TABLES) {
        return NULL;
    }

    table = extra_tables[extra_tables_used];
    ++extra_tables_used;
    memset(table, 0, PMM_PAGE_SIZE);
    return table;
}

static uint32_t *get_page_table(uint32_t directory_index)
{
    uint32_t *table;

    if ((page_directory[directory_index] & PAGE_PRESENT) != 0u) {
        return (uint32_t *)(uintptr_t)(page_directory[directory_index] & 0xfffff000u);
    }

    table = alloc_extra_table();
    if (table == NULL) {
        panic("out of early page tables");
    }

    page_directory[directory_index] =
        ((uint32_t)(uintptr_t)table) | KERNEL_PAGE_FLAGS;
    return table;
}

static void map_page_in_directory(uint32_t *directory, uint32_t *user_table,
    uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags)
{
    uint32_t directory_index;
    uint32_t table_index;
    uint32_t *table;

    directory_index = virtual_addr >> 22u;
    table_index = (virtual_addr >> 12u) & 0x3ffu;
    if (directory == page_directory) {
        table = get_page_table(directory_index);
    } else {
        if (directory_index != USER_BASE_DIRECTORY) {
            panic("user mapping outside user table");
        }
        table = user_table;
    }
    if ((flags & PAGE_USER) != 0u) {
        directory[directory_index] |= PAGE_USER;
    }
    table[table_index] = (physical_addr & 0xfffff000u) | flags;
}

static void map_range_in_directory(uint32_t *directory, uint32_t *user_table,
    uint32_t virtual_start, uint32_t physical_start, uint32_t size,
    uint32_t flags)
{
    uint32_t virtual_addr;
    uint32_t physical_addr;
    uint32_t virtual_end;

    if (size == 0u) {
        return;
    }

    virtual_addr = align_down(virtual_start);
    physical_addr = align_down(physical_start);
    if (size > 0xffffffffu - virtual_start) {
        virtual_end = 0xffffffffu;
    } else {
        virtual_end = align_up(virtual_start + size);
    }

    while (virtual_addr < virtual_end) {
        map_page_in_directory(directory, user_table, virtual_addr,
            physical_addr, flags);
        virtual_addr += PMM_PAGE_SIZE;
        physical_addr += PMM_PAGE_SIZE;
    }
}

static void paging_map_range_flags(uint32_t virtual_start,
    uint32_t physical_start, uint32_t size, uint32_t flags)
{
    map_range_in_directory(page_directory, NULL, virtual_start, physical_start,
        size, flags);
}

void paging_map_range(uint32_t start, uint32_t size)
{
    paging_map_range_flags(start, start, size, KERNEL_PAGE_FLAGS);
}

void paging_map_user_readonly_range(uint32_t virtual_start,
    uint32_t physical_start, uint32_t size)
{
    paging_map_range_flags(virtual_start, physical_start, size,
        USER_READONLY_PAGE_FLAGS);
}

void paging_map_user_writable_range(uint32_t virtual_start,
    uint32_t physical_start, uint32_t size)
{
    paging_map_range_flags(virtual_start, physical_start, size,
        USER_WRITABLE_PAGE_FLAGS);
}

int paging_user_space_init(uint32_t slot, uint32_t image_physical,
    uint32_t image_size, uint32_t stack_physical, uint32_t stack_size)
{
    if (slot >= USER_SPACE_COUNT || image_size == 0u || stack_size == 0u) {
        return -1;
    }

    memcpy(user_directories[slot], page_directory, sizeof(page_directory));
    memset(user_tables[slot], 0, sizeof(user_tables[slot]));
    user_directories[slot][USER_BASE_DIRECTORY] =
        ((uint32_t)(uintptr_t)user_tables[slot]) | KERNEL_PAGE_FLAGS |
        PAGE_USER;

    map_range_in_directory(user_directories[slot], user_tables[slot],
        USER_IMAGE_BASE, image_physical, image_size,
        USER_WRITABLE_PAGE_FLAGS);
    map_range_in_directory(user_directories[slot], user_tables[slot],
        USER_STACK_BASE, stack_physical, stack_size,
        USER_WRITABLE_PAGE_FLAGS);

    return 0;
}

void paging_switch_kernel_space(void)
{
    x86_load_cr3((uint32_t)(uintptr_t)page_directory);
}

void paging_switch_user_space(uint32_t slot)
{
    if (slot >= USER_SPACE_COUNT) {
        panic("bad user address space");
    }

    x86_load_cr3((uint32_t)(uintptr_t)user_directories[slot]);
}

static void page_fault_handler(struct interrupt_frame *frame)
{
    if ((frame->cs & 0x3u) == 0x3u) {
        print_write("user page fault addr ");
        print_hex32(x86_read_cr2());
        print_write(" eip ");
        print_hex32(frame->eip);
        print_write(" error ");
        print_hex32(frame->error_code);
        print_putc('\n');
        process_exit(139);
        return;
    }

    print_write("page fault addr ");
    print_hex32(x86_read_cr2());
    print_write(" eip ");
    print_hex32(frame->eip);
    print_write(" error ");
    print_hex32(frame->error_code);
    print_putc('\n');
    panic("page fault");
}

void paging_init(void)
{
    uint32_t i;
    uint32_t j;
    uint32_t addr;

    memset(page_directory, 0, sizeof(page_directory));
    memset(early_tables, 0, sizeof(early_tables));
    memset(extra_tables, 0, sizeof(extra_tables));
    extra_tables_used = 0u;

    addr = 0u;
    for (i = 0u; i < EARLY_TABLES; ++i) {
        for (j = 0u; j < PAGE_ENTRIES; ++j) {
            early_tables[i][j] = addr | KERNEL_PAGE_FLAGS;
            addr += PMM_PAGE_SIZE;
        }
        page_directory[i] =
            ((uint32_t)(uintptr_t)early_tables[i]) | KERNEL_PAGE_FLAGS;
    }

    if (framebuffer_is_available()) {
        paging_map_range(framebuffer_addr(), framebuffer_size());
    }

    isr_set_handler(14u, page_fault_handler);
    x86_load_cr3((uint32_t)(uintptr_t)page_directory);
    x86_enable_paging();
    paging_ready = 1;

    print_writeln("paging enabled");
}

int paging_is_enabled(void)
{
    return paging_ready;
}

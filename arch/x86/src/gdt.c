#include <arch/x86/gdt.h>
#include <kernel/compiler.h>
#include <kernel/string.h>
#include <kernel/types.h>

#define GDT_ENTRIES 6u

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} PACKED;

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} PACKED;

struct gdt_pointer {
    uint16_t limit;
    uint32_t base;
} PACKED;

static struct gdt_entry gdt[GDT_ENTRIES];
static struct gdt_pointer gdt_ptr;
static struct tss_entry tss;

void gdt_flush(uint32_t gdt_pointer_addr);

static void gdt_set_entry(uint32_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
    gdt[index].base_low = (uint16_t)(base & 0xffffu);
    gdt[index].base_middle = (uint8_t)((base >> 16u) & 0xffu);
    gdt[index].base_high = (uint8_t)((base >> 24u) & 0xffu);
    gdt[index].limit_low = (uint16_t)(limit & 0xffffu);
    gdt[index].granularity = (uint8_t)((limit >> 16u) & 0x0fu);
    gdt[index].granularity = (uint8_t)(gdt[index].granularity | (granularity & 0xf0u));
    gdt[index].access = access;
}

static void tss_flush(void)
{
    uint16_t selector;

    selector = GDT_TSS_SELECTOR;
    __asm__ volatile ("ltr %0" : : "r" (selector));
}

void gdt_set_kernel_stack(uint32_t stack_top)
{
    tss.esp0 = stack_top;
}

void gdt_init(void)
{
    uint32_t tss_base;

    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1u);
    gdt_ptr.base = (uint32_t)(uintptr_t)&gdt[0];
    memset(&tss, 0, sizeof(tss));

    tss.ss0 = GDT_KERNEL_DATA_SELECTOR;
    tss.iomap_base = sizeof(tss);
    tss_base = (uint32_t)(uintptr_t)&tss;

    gdt_set_entry(0u, 0u, 0u, 0u, 0u);
    gdt_set_entry(1u, 0u, 0xffffffffu, 0x9au, 0xcfu);
    gdt_set_entry(2u, 0u, 0xffffffffu, 0x92u, 0xcfu);
    gdt_set_entry(3u, 0u, 0xffffffffu, 0xfau, 0xcfu);
    gdt_set_entry(4u, 0u, 0xffffffffu, 0xf2u, 0xcfu);
    gdt_set_entry(5u, tss_base, sizeof(tss) - 1u, 0x89u, 0x00u);

    gdt_flush((uint32_t)(uintptr_t)&gdt_ptr);
    tss_flush();
}

#include <arch/x86/idt.h>
#include <kernel/compiler.h>
#include <kernel/string.h>
#include <kernel/types.h>

#define IDT_ENTRIES 256u

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} PACKED;

struct idt_pointer {
    uint16_t limit;
    uint32_t base;
} PACKED;

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_pointer idt_ptr;

static void idt_load_pointer(uint32_t idt_pointer_addr)
{
    __asm__ volatile ("lidt (%0)" : : "r" (idt_pointer_addr));
}

void idt_set_gate(uint8_t vector, uint32_t handler, uint16_t selector, uint8_t flags)
{
    idt[vector].base_low = (uint16_t)(handler & 0xffffu);
    idt[vector].selector = selector;
    idt[vector].zero = 0u;
    idt[vector].flags = flags;
    idt[vector].base_high = (uint16_t)((handler >> 16u) & 0xffffu);
}

void idt_init(void)
{
    memset(idt, 0, sizeof(idt));

    idt_ptr.limit = (uint16_t)(sizeof(idt) - 1u);
    idt_ptr.base = (uint32_t)(uintptr_t)&idt[0];
}

void idt_load(void)
{
    idt_load_pointer((uint32_t)(uintptr_t)&idt_ptr);
}

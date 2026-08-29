#include <arch/x86/cpu.h>

void x86_enable_interrupts(void)
{
    __asm__ volatile ("sti");
}

void x86_disable_interrupts(void)
{
    __asm__ volatile ("cli");
}

void x86_halt(void)
{
    __asm__ volatile ("hlt");
}

uint32_t x86_read_cr2(void)
{
    uint32_t value;

    __asm__ volatile ("movl %%cr2, %0" : "=r" (value));
    return value;
}

void x86_load_cr3(uint32_t addr)
{
    __asm__ volatile ("movl %0, %%cr3" : : "r" (addr) : "memory");
}

void x86_enable_paging(void)
{
    uint32_t cr0;

    __asm__ volatile ("movl %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile ("movl %0, %%cr0" : : "r" (cr0) : "memory");
}

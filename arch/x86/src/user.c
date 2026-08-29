#include <arch/x86/gdt.h>
#include <arch/x86/paging.h>
#include <arch/x86/cpu.h>
#include <kernel/compiler.h>
#include <kernel/pmm.h>
#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/string.h>
#include <kernel/types.h>
#include <nanos/app.h>

#define USER_IMAGE_BASE ((uint32_t)NANOS_APP_LOAD_BASE)
#define USER_STACK_BASE ((uint32_t)NANOS_APP_STACK_BASE)

static uint8_t kernel_tss_stack[PROCESS_STACK_SIZE] ALIGNED(16);
static uint32_t kernel_return_esp;
static uint32_t kernel_return_ebp;
static uint32_t kernel_return_eip;
static uint32_t active_image_size;
static uint32_t active_stack_size;
static uint32_t active_address_space;
static int user_active;
static int user_exit_status;

static int range_inside(uint32_t addr, uint32_t size, uint32_t base,
    uint32_t limit)
{
    uint32_t end;

    if (size == 0u) {
        return addr >= base && addr <= limit;
    }
    if (addr > 0xffffffffu - size) {
        return 0;
    }

    end = addr + size;
    return addr >= base && end <= limit;
}

int x86_user_save_context(uint32_t *esp, uint32_t *ebp, uint32_t *eip);
void x86_user_restore_context(uint32_t esp, uint32_t ebp, uint32_t eip)
    NORETURN;
void x86_user_enter_context(const struct platform_process_context *context)
    NORETURN;

static struct platform_process_context *process_context(struct process *process)
{
    return (struct platform_process_context *)process->context;
}

static void set_active_process(struct process *process)
{
    active_image_size = process->image_size;
    active_stack_size = process->stack_size;
    active_address_space = process->address_space;
}

static void write_stack_u32(struct process *process, uint32_t offset,
    uint32_t value)
{
    process->stack[offset] = (uint8_t)(value & 0xffu);
    process->stack[offset + 1u] = (uint8_t)((value >> 8u) & 0xffu);
    process->stack[offset + 2u] = (uint8_t)((value >> 16u) & 0xffu);
    process->stack[offset + 3u] = (uint8_t)((value >> 24u) & 0xffu);
}

int platform_user_prepare(struct process *process)
{
    struct platform_process_context *context;

    if (process == NULL || process->image_size == 0u ||
        process->image_size > NX_IMAGE_MAX ||
        process->stack_size == 0u ||
        process->stack_size > PROCESS_STACK_SIZE ||
        process->entry_offset >= process->image_size) {
        return -1;
    }

    if (paging_user_space_init(process->address_space,
        (uint32_t)(uintptr_t)process->image_memory, process->image_size,
        (uint32_t)(uintptr_t)process->stack, process->stack_size) != 0) {
        return -1;
    }

    context = process_context(process);
    context->eax = 0u;
    context->ebx = 0u;
    context->ecx = 0u;
    context->edx = 0u;
    context->esi = 0u;
    context->edi = 0u;
    context->ebp = 0u;
    context->eip = USER_IMAGE_BASE + process->entry_offset;
    context->eflags = 0x202u;
    context->esp = process->stack_top;
    process->context_started = 1;

    return 0;
}

int platform_user_setup_param(struct process *process, const char *param)
{
    struct platform_process_context *context;
    uint32_t len;
    uint32_t offset;
    uint32_t param_ptr;

    if (process == NULL || param == NULL) {
        return -1;
    }

    len = (uint32_t)strlen(param) + 1u;
    if (len > NANOS_APP_PARAM_MAX) {
        return -1;
    }

    context = process_context(process);
    if (context->esp < USER_STACK_BASE ||
        context->esp > USER_STACK_BASE + process->stack_size) {
        return -1;
    }

    offset = context->esp - USER_STACK_BASE;
    if (len > offset) {
        return -1;
    }
    offset -= len;
    memcpy(process->stack + offset, param, len);
    param_ptr = USER_STACK_BASE + offset;

    offset &= 0xfffffffcu;
    if (offset < 8u) {
        return -1;
    }
    offset -= 8u;

    write_stack_u32(process, offset, param_ptr);
    write_stack_u32(process, offset + 4u, len - 1u);
    context->esp = USER_STACK_BASE + offset;
    return 0;
}

void platform_user_set_return_value(struct process *process, uint32_t value)
{
    if (process != NULL) {
        process_context(process)->eax = value;
    }
}

int platform_user_enter(struct process *process)
{
    if (process == NULL || !process->context_started) {
        return -1;
    }

    if (x86_user_save_context(&kernel_return_esp, &kernel_return_ebp,
        &kernel_return_eip) != 0) {
        user_active = 0;
        return user_exit_status;
    }

    user_exit_status = 0;
    set_active_process(process);
    user_active = 1;
    gdt_set_kernel_stack((uint32_t)(uintptr_t)(kernel_tss_stack +
        PROCESS_STACK_SIZE));
    paging_switch_user_space(active_address_space);
    x86_user_enter_context(process_context(process));
    return user_exit_status;
}

void platform_user_switch(struct process *process)
{
    set_active_process(process);
    paging_switch_user_space(active_address_space);
}

void platform_user_exit(int status)
{
    user_exit_status = status;
    if (!user_active) {
        for (;;) {
        }
    }

    paging_switch_kernel_space();
    x86_enable_interrupts();
    x86_user_restore_context(kernel_return_esp, kernel_return_ebp,
        kernel_return_eip);
}

int platform_user_range_is_valid(const void *addr, uint32_t size)
{
    uint32_t value;

    value = (uint32_t)(uintptr_t)addr;
    if (range_inside(value, size, USER_IMAGE_BASE,
        USER_IMAGE_BASE + active_image_size)) {
        return 1;
    }
    if (range_inside(value, size, USER_STACK_BASE,
        USER_STACK_BASE + active_stack_size)) {
        return 1;
    }

    return 0;
}

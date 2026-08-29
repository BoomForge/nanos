#include <kernel/multiboot2.h>
#include <kernel/types.h>

const struct multiboot2_tag *multiboot2_first_tag(uint32_t info_addr)
{
    return (const struct multiboot2_tag *)(uintptr_t)(info_addr + 8u);
}

const struct multiboot2_tag *multiboot2_next_tag(const struct multiboot2_tag *tag)
{
    uintptr_t next;

    next = (uintptr_t)tag + tag->size;
    next = (next + 7u) & ~((uintptr_t)7u);
    return (const struct multiboot2_tag *)next;
}

const struct multiboot2_tag *multiboot2_find_tag(uint32_t info_addr, uint32_t type)
{
    const struct multiboot2_info *info;
    const struct multiboot2_tag *tag;
    uintptr_t end;

    info = (const struct multiboot2_info *)(uintptr_t)info_addr;
    tag = multiboot2_first_tag(info_addr);
    end = (uintptr_t)info + info->total_size;

    while ((uintptr_t)tag < end && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == type) {
            return tag;
        }
        tag = multiboot2_next_tag(tag);
    }

    return NULL;
}

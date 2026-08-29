#ifndef KERNEL_MULTIBOOT2_H
#define KERNEL_MULTIBOOT2_H

#include <kernel/compiler.h>
#include <kernel/types.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289u
#define MULTIBOOT2_TAG_TYPE_END 0u
#define MULTIBOOT2_TAG_TYPE_MODULE 3u
#define MULTIBOOT2_TAG_TYPE_MMAP 6u
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER 8u
#define MULTIBOOT2_MEMORY_AVAILABLE 1u

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} PACKED;

struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
} PACKED;

struct multiboot2_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint32_t framebuffer_addr_low;
    uint32_t framebuffer_addr_high;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
} PACKED;

struct multiboot2_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} PACKED;

struct multiboot2_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[1];
} PACKED;

struct multiboot2_mmap_entry {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t reserved;
} PACKED;

const struct multiboot2_tag *multiboot2_first_tag(uint32_t info_addr);
const struct multiboot2_tag *multiboot2_next_tag(const struct multiboot2_tag *tag);
const struct multiboot2_tag *multiboot2_find_tag(uint32_t info_addr, uint32_t type);

#endif

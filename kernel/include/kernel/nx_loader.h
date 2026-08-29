#ifndef KERNEL_NX_LOADER_H
#define KERNEL_NX_LOADER_H

#include <kernel/types.h>

#define NX_IMAGE_MAX 8192u

struct nx_image {
    char name[64];
    uint8_t data[NX_IMAGE_MAX];
    uint32_t file_size;
    uint32_t memory_size;
    uint32_t stack_size;
    uint32_t stack_top;
    uint32_t entry_offset;
    uint32_t icon_offset;
};

int nx_load(const char *path, struct nx_image *image);

#endif

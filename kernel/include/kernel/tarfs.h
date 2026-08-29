#ifndef KERNEL_TARFS_H
#define KERNEL_TARFS_H

#include <kernel/block.h>

int tarfs_mount(struct block_device *device);

#endif

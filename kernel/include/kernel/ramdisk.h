#ifndef KERNEL_RAMDISK_H
#define KERNEL_RAMDISK_H

#include <kernel/types.h>

void ramdisk_init_from_multiboot(uint32_t multiboot_info);

#endif

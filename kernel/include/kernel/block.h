#ifndef KERNEL_BLOCK_H
#define KERNEL_BLOCK_H

#include <kernel/types.h>

#define BLOCK_SECTOR_SIZE 512u

struct block_device;

typedef int (*block_read_sector_fn)(struct block_device *device,
    uint32_t lba, void *buffer);

struct block_device {
    const char *name;
    uint32_t sector_size;
    uint32_t sector_count;
    block_read_sector_fn read_sector;
    void *context;
    struct block_device *next;
};

void block_init(void);
int block_register(struct block_device *device);
uint32_t block_device_count(void);
struct block_device *block_first(void);
struct block_device *block_next(struct block_device *device);
int block_read_sector(struct block_device *device, uint32_t lba, void *buffer);
int block_read(struct block_device *device, uint32_t offset, void *buffer, uint32_t byte_count);

#endif

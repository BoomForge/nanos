#include <kernel/block.h>
#include <kernel/string.h>
#include <kernel/types.h>

static struct block_device *first_device;
static struct block_device *last_device;
static uint32_t device_count;

void block_init(void)
{
    first_device = NULL;
    last_device = NULL;
    device_count = 0u;
}

int block_register(struct block_device *device)
{
    if (device == NULL || device->name == NULL || device->read_sector == NULL ||
            device->sector_size != BLOCK_SECTOR_SIZE || device->sector_count == 0u) {
        return -1;
    }

    device->next = NULL;
    if (last_device != NULL) {
        last_device->next = device;
    } else {
        first_device = device;
    }
    last_device = device;
    ++device_count;
    return 0;
}

uint32_t block_device_count(void)
{
    return device_count;
}

struct block_device *block_first(void)
{
    return first_device;
}

struct block_device *block_next(struct block_device *device)
{
    if (device == NULL) {
        return NULL;
    }

    return device->next;
}

int block_read_sector(struct block_device *device, uint32_t lba, void *buffer)
{
    if (device == NULL || buffer == NULL || device->read_sector == NULL ||
            lba >= device->sector_count) {
        return -1;
    }

    return device->read_sector(device, lba, buffer);
}

int block_read(struct block_device *device, uint32_t offset, void *buffer, uint32_t byte_count)
{
    uint8_t sector[BLOCK_SECTOR_SIZE];
    uint8_t *out;
    uint32_t copied;
    uint32_t lba;
    uint32_t sector_offset;
    uint32_t chunk;

    if (device == NULL || buffer == NULL || byte_count == 0u) {
        return -1;
    }

    out = (uint8_t *)buffer;
    copied = 0u;
    while (copied < byte_count) {
        lba = offset / device->sector_size;
        sector_offset = offset % device->sector_size;
        if (block_read_sector(device, lba, sector) != 0) {
            return -1;
        }

        chunk = device->sector_size - sector_offset;
        if (chunk > byte_count - copied) {
            chunk = byte_count - copied;
        }

        memcpy(out + copied, sector + sector_offset, chunk);
        copied += chunk;
        offset += chunk;
    }

    return 0;
}

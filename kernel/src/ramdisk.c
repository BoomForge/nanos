#include <kernel/block.h>
#include <kernel/multiboot2.h>
#include <kernel/print.h>
#include <kernel/ramdisk.h>
#include <kernel/string.h>
#include <kernel/types.h>

struct ramdisk_context {
    const uint8_t *data;
    uint32_t size;
};

static struct ramdisk_context ramdisk;
static struct block_device ramdisk_device;

static int ramdisk_read_sector(struct block_device *device, uint32_t lba, void *buffer)
{
    struct ramdisk_context *context;
    uint32_t offset;
    uint32_t available;

    context = (struct ramdisk_context *)device->context;
    offset = lba * BLOCK_SECTOR_SIZE;

    memset(buffer, 0, BLOCK_SECTOR_SIZE);
    if (offset >= context->size) {
        return 0;
    }

    available = context->size - offset;
    if (available > BLOCK_SECTOR_SIZE) {
        available = BLOCK_SECTOR_SIZE;
    }

    memcpy(buffer, context->data + offset, available);
    return 0;
}

void ramdisk_init_from_multiboot(uint32_t multiboot_info)
{
    const struct multiboot2_tag *tag;
    const struct multiboot2_tag_module *module;
    uint32_t size;

    tag = multiboot2_find_tag(multiboot_info, MULTIBOOT2_TAG_TYPE_MODULE);
    if (tag == NULL) {
        print_writeln("ramdisk module missing");
        return;
    }

    module = (const struct multiboot2_tag_module *)tag;
    if (module->mod_end <= module->mod_start) {
        print_writeln("ramdisk module empty");
        return;
    }

    size = module->mod_end - module->mod_start;
    ramdisk.data = (const uint8_t *)(uintptr_t)module->mod_start;
    ramdisk.size = size;

    ramdisk_device.name = "ram0";
    ramdisk_device.sector_size = BLOCK_SECTOR_SIZE;
    ramdisk_device.sector_count = (size + BLOCK_SECTOR_SIZE - 1u) / BLOCK_SECTOR_SIZE;
    ramdisk_device.read_sector = ramdisk_read_sector;
    ramdisk_device.context = &ramdisk;
    ramdisk_device.next = NULL;

    if (block_register(&ramdisk_device) == 0) {
        print_write("ramdisk ");
        print_uint(size);
        print_write(" bytes, sectors ");
        print_uint(ramdisk_device.sector_count);
        print_putc('\n');
    } else {
        print_writeln("ramdisk registration failed");
    }
}

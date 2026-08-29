#include <kernel/nx_loader.h>
#include <kernel/string.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <nanos/app.h>

#define NX_BIN_PREFIX "/bin/"

static int has_slash(const char *path)
{
    while (*path != '\0') {
        if (*path == '/') {
            return 1;
        }
        ++path;
    }

    return 0;
}

static int make_bin_path(const char *name, char *path, uint32_t path_size)
{
    const char *prefix;
    uint32_t i;

    prefix = NX_BIN_PREFIX;
    i = 0u;
    while (*prefix != '\0') {
        if (i + 1u >= path_size) {
            return -1;
        }
        path[i] = *prefix;
        ++i;
        ++prefix;
    }

    while (*name != '\0') {
        if (i + 1u >= path_size) {
            return -1;
        }
        path[i] = *name;
        ++i;
        ++name;
    }
    path[i] = '\0';

    return 0;
}

static uint32_t read_u32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
        ((uint32_t)data[1] << 8u) |
        ((uint32_t)data[2] << 16u) |
        ((uint32_t)data[3] << 24u);
}

static void copy_name(const char *path, char *name, uint32_t name_size)
{
    uint32_t i;

    i = 0u;
    while (path[i] != '\0' && i + 1u < name_size) {
        name[i] = path[i];
        ++i;
    }
    name[i] = '\0';
}

static int load_exact_path(const char *path, struct nx_image *image)
{
    struct vfs_file file;
    uint32_t header_size;
    uint32_t total;
    int bytes;

    if (path == NULL || image == NULL) {
        return -1;
    }

    memset(image, 0, sizeof(*image));
    if (vfs_open(path, &file) != 0) {
        return -1;
    }

    if (file.size < NANOS_APP_HEADER_SIZE || file.size > NX_IMAGE_MAX) {
        vfs_close(&file);
        return -1;
    }

    total = 0u;
    while (total < file.size) {
        bytes = vfs_read(&file, image->data + total, file.size - total);
        if (bytes <= 0) {
            vfs_close(&file);
            return -1;
        }
        total += (uint32_t)bytes;
    }
    vfs_close(&file);

    if (read_u32(image->data + NANOS_APP_HEADER_MAGIC_OFFSET) !=
        NANOS_APP_MAGIC) {
        return -1;
    }

    header_size = read_u32(image->data + NANOS_APP_HEADER_SIZE_OFFSET);
    image->entry_offset = read_u32(image->data + NANOS_APP_HEADER_ENTRY_OFFSET);
    image->file_size = read_u32(image->data +
        NANOS_APP_HEADER_IMAGE_SIZE_OFFSET);
    image->memory_size = read_u32(image->data +
        NANOS_APP_HEADER_MEMORY_SIZE_OFFSET);
    image->stack_size = read_u32(image->data +
        NANOS_APP_HEADER_STACK_SIZE_OFFSET);
    image->stack_top = read_u32(image->data +
        NANOS_APP_HEADER_STACK_TOP_OFFSET);
    image->icon_offset = read_u32(image->data +
        NANOS_APP_HEADER_ICON_OFFSET_OFFSET);

    if (header_size != NANOS_APP_HEADER_SIZE ||
        image->file_size != total ||
        image->file_size > image->memory_size ||
        image->memory_size > NX_IMAGE_MAX ||
        image->entry_offset < header_size ||
        image->entry_offset >= image->file_size ||
        image->stack_size == 0u ||
        image->stack_size > NANOS_APP_STACK_SIZE ||
        image->stack_top < NANOS_APP_STACK_BASE ||
        image->stack_top > NANOS_APP_STACK_BASE + image->stack_size ||
        ((image->stack_top - NANOS_APP_STACK_BASE) & 3u) != 0u) {
        return -1;
    }
    if (image->icon_offset != NANOS_APP_ICON_NONE &&
        image->icon_offset >= image->file_size) {
        return -1;
    }

    copy_name(path, image->name, sizeof(image->name));
    return 0;
}

int nx_load(const char *path, struct nx_image *image)
{
    char bin_path[64];

    if (path == NULL || image == NULL) {
        return -1;
    }

    if (has_slash(path)) {
        return load_exact_path(path, image);
    }

    if (make_bin_path(path, bin_path, sizeof(bin_path)) == 0 &&
        load_exact_path(bin_path, image) == 0) {
        return 0;
    }

    return load_exact_path(path, image);
}

#include <kernel/block.h>
#include <kernel/string.h>
#include <kernel/tarfs.h>
#include <kernel/types.h>
#include <kernel/vfs.h>

#define TAR_BLOCK_SIZE 512u
#define TAR_NAME_OFFSET 0u
#define TAR_NAME_SIZE 100u
#define TAR_SIZE_OFFSET 124u
#define TAR_SIZE_SIZE 12u
#define TAR_TYPE_OFFSET 156u
#define TARFS_PATH_MAX 128u

static struct block_device *tar_device;
static const struct vfs_ops tarfs_ops;

static uint32_t align_tar(uint32_t value)
{
    return (value + TAR_BLOCK_SIZE - 1u) & ~(TAR_BLOCK_SIZE - 1u);
}

static int header_is_empty(const uint8_t *header)
{
    uint32_t i;

    for (i = 0u; i < TAR_BLOCK_SIZE; ++i) {
        if (header[i] != 0u) {
            return 0;
        }
    }

    return 1;
}

static uint32_t parse_octal(const uint8_t *field, uint32_t size)
{
    uint32_t value;
    uint32_t i;

    value = 0u;
    for (i = 0u; i < size; ++i) {
        if (field[i] >= '0' && field[i] <= '7') {
            value = value * 8u + (uint32_t)(field[i] - '0');
        }
    }

    return value;
}

static void normalize_path(const char *input, char *output)
{
    uint32_t in_pos;
    uint32_t out_pos;
    char ch;

    in_pos = 0u;
    out_pos = 0u;

    while (input[in_pos] == '/') {
        ++in_pos;
    }
    if (input[in_pos] == '.' && input[in_pos + 1u] == '/') {
        in_pos += 2u;
    }

    while (input[in_pos] != '\0' && out_pos + 1u < TARFS_PATH_MAX) {
        ch = input[in_pos];
        if (ch >= 'A' && ch <= 'Z') {
            ch = (char)(ch + ('a' - 'A'));
        }
        output[out_pos] = ch;
        ++out_pos;
        ++in_pos;
    }

    while (out_pos > 0u && output[out_pos - 1u] == '/') {
        --out_pos;
    }

    output[out_pos] = '\0';
}

static void read_name(const uint8_t *header, char *name)
{
    uint32_t i;
    char raw[TARFS_PATH_MAX];

    for (i = 0u; i + 1u < TARFS_PATH_MAX && i < TAR_NAME_SIZE &&
            header[TAR_NAME_OFFSET + i] != 0u; ++i) {
        raw[i] = (char)header[TAR_NAME_OFFSET + i];
    }
    raw[i] = '\0';
    normalize_path(raw, name);
}

static int path_matches(const char *entry, const char *wanted)
{
    uint32_t i;

    i = 0u;
    while (entry[i] != '\0' && wanted[i] != '\0') {
        if (entry[i] != wanted[i]) {
            return 0;
        }
        ++i;
    }

    return entry[i] == '\0' && wanted[i] == '\0';
}

static int direct_child_name(const char *parent, const char *entry, char *child)
{
    uint32_t parent_len;
    uint32_t entry_pos;
    uint32_t child_pos;

    parent_len = strlen(parent);
    if (parent_len == 0u) {
        entry_pos = 0u;
    } else {
        if (!path_matches(parent, entry)) {
            entry_pos = 0u;
            while (entry[entry_pos] != '\0' && parent[entry_pos] != '\0' &&
                    entry[entry_pos] == parent[entry_pos]) {
                ++entry_pos;
            }
            if (entry_pos != parent_len || entry[entry_pos] != '/') {
                return 0;
            }
            ++entry_pos;
        } else {
            return 0;
        }
    }

    if (entry[entry_pos] == '\0') {
        return 0;
    }

    child_pos = 0u;
    while (entry[entry_pos] != '\0' && entry[entry_pos] != '/') {
        if (child_pos + 1u < VFS_NAME_MAX) {
            child[child_pos] = entry[entry_pos];
            ++child_pos;
        }
        ++entry_pos;
    }
    child[child_pos] = '\0';

    return entry[entry_pos] == '\0';
}

static int copy_path(char *dest, const char *src)
{
    uint32_t i;

    for (i = 0u; i + 1u < VFS_PATH_MAX && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return src[i] == '\0' ? 0 : -1;
}

static int tarfs_directory_exists(const char *path)
{
    uint8_t header[TAR_BLOCK_SIZE];
    char wanted[TARFS_PATH_MAX];
    char name[TARFS_PATH_MAX];
    char child[VFS_NAME_MAX];
    uint32_t offset;
    uint32_t size;
    char type;

    normalize_path(path, wanted);
    if (wanted[0] == '\0') {
        return 1;
    }

    offset = 0u;
    for (;;) {
        if (block_read(tar_device, offset, header, TAR_BLOCK_SIZE) != 0) {
            return 0;
        }
        if (header_is_empty(header)) {
            return 0;
        }

        read_name(header, name);
        size = parse_octal(header + TAR_SIZE_OFFSET, TAR_SIZE_SIZE);
        type = (char)header[TAR_TYPE_OFFSET];

        if (type == '5' && path_matches(name, wanted)) {
            return 1;
        }
        if (direct_child_name(wanted, name, child)) {
            return 1;
        }

        offset += TAR_BLOCK_SIZE + align_tar(size);
    }
}

static int tarfs_open(const char *path, struct vfs_file *file)
{
    uint8_t header[TAR_BLOCK_SIZE];
    char wanted[TARFS_PATH_MAX];
    char name[TARFS_PATH_MAX];
    uint32_t offset;
    uint32_t size;
    char type;

    normalize_path(path, wanted);
    offset = 0u;
    for (;;) {
        if (block_read(tar_device, offset, header, TAR_BLOCK_SIZE) != 0) {
            return -1;
        }
        if (header_is_empty(header)) {
            return -1;
        }

        read_name(header, name);
        size = parse_octal(header + TAR_SIZE_OFFSET, TAR_SIZE_SIZE);
        type = (char)header[TAR_TYPE_OFFSET];
        if ((type == '\0' || type == '0') && path_matches(name, wanted)) {
            file->ops = &tarfs_ops;
            file->context = (void *)(uintptr_t)(offset + TAR_BLOCK_SIZE);
            file->size = size;
            file->pos = 0u;
            return 0;
        }

        offset += TAR_BLOCK_SIZE + align_tar(size);
    }
}

static int tarfs_read(struct vfs_file *file, void *buffer, uint32_t byte_count)
{
    uint32_t data_offset;
    uint32_t remaining;

    if (file == NULL || buffer == NULL) {
        return -1;
    }
    if (file->pos >= file->size) {
        return 0;
    }

    remaining = file->size - file->pos;
    if (byte_count > remaining) {
        byte_count = remaining;
    }

    data_offset = (uint32_t)(uintptr_t)file->context;
    if (block_read(tar_device, data_offset + file->pos, buffer, byte_count) != 0) {
        return -1;
    }
    file->pos += byte_count;
    return (int)byte_count;
}

static void tarfs_close(struct vfs_file *file)
{
    (void)file;
}

static int tarfs_open_dir(const char *path, struct vfs_dir *dir)
{
    char normalized[TARFS_PATH_MAX];

    if (path == NULL || dir == NULL || !tarfs_directory_exists(path)) {
        return -1;
    }

    normalize_path(path, normalized);
    if (copy_path(dir->path, normalized) != 0) {
        return -1;
    }

    dir->ops = &tarfs_ops;
    dir->context = NULL;
    dir->pos = 0u;
    return 0;
}

static int tarfs_read_dir(struct vfs_dir *dir, struct vfs_dir_entry *entry)
{
    uint8_t header[TAR_BLOCK_SIZE];
    char name[TARFS_PATH_MAX];
    uint32_t offset;
    uint32_t size;
    char type;

    if (dir == NULL || entry == NULL) {
        return -1;
    }

    offset = dir->pos;
    for (;;) {
        if (block_read(tar_device, offset, header, TAR_BLOCK_SIZE) != 0) {
            return -1;
        }
        if (header_is_empty(header)) {
            dir->pos = offset;
            return 0;
        }

        read_name(header, name);
        size = parse_octal(header + TAR_SIZE_OFFSET, TAR_SIZE_SIZE);
        type = (char)header[TAR_TYPE_OFFSET];
        offset += TAR_BLOCK_SIZE + align_tar(size);

        if (direct_child_name(dir->path, name, entry->name)) {
            entry->type = (type == '5') ? VFS_TYPE_DIR : VFS_TYPE_FILE;
            entry->size = size;
            dir->pos = offset;
            return 1;
        }
    }
}

static void tarfs_close_dir(struct vfs_dir *dir)
{
    (void)dir;
}

static const struct vfs_ops tarfs_ops = {
    tarfs_open,
    tarfs_read,
    tarfs_close,
    tarfs_open_dir,
    tarfs_read_dir,
    tarfs_close_dir
};

int tarfs_mount(struct block_device *device)
{
    uint8_t header[TAR_BLOCK_SIZE];

    if (device == NULL) {
        return -1;
    }
    if (block_read(device, 0u, header, TAR_BLOCK_SIZE) != 0 || header_is_empty(header)) {
        return -1;
    }

    tar_device = device;
    return vfs_mount_root(&tarfs_ops);
}

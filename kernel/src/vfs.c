#include <kernel/string.h>
#include <kernel/types.h>
#include <kernel/vfs.h>

static const struct vfs_ops *root_ops;

void vfs_init(void)
{
    root_ops = NULL;
}

int vfs_mount_root(const struct vfs_ops *ops)
{
    if (ops == NULL || ops->open == NULL || ops->read == NULL ||
            ops->close == NULL || ops->open_dir == NULL ||
            ops->read_dir == NULL || ops->close_dir == NULL) {
        return -1;
    }

    root_ops = ops;
    return 0;
}

static uint32_t path_length(const char *path)
{
    uint32_t len;

    len = 0u;
    while (path[len] != '\0') {
        ++len;
    }
    return len;
}

static int path_copy(char *out, uint32_t out_size, const char *path)
{
    uint32_t i;

    if (out_size == 0u) {
        return -1;
    }

    for (i = 0u; i + 1u < out_size && path[i] != '\0'; ++i) {
        out[i] = path[i];
    }
    out[i] = '\0';
    return path[i] == '\0' ? 0 : -1;
}

static void path_pop(char *out)
{
    uint32_t len;

    len = path_length(out);
    if (len <= 1u) {
        out[0] = '/';
        out[1] = '\0';
        return;
    }

    while (len > 1u && out[len - 1u] != '/') {
        --len;
    }
    if (len <= 1u) {
        out[1] = '\0';
    } else {
        out[len - 1u] = '\0';
    }
}

static int path_append(char *out, uint32_t out_size, const char *segment,
    uint32_t segment_len)
{
    uint32_t out_len;
    uint32_t i;

    if (segment_len == 0u) {
        return 0;
    }

    out_len = path_length(out);
    if (out_len > 1u) {
        if (out_len + 1u >= out_size) {
            return -1;
        }
        out[out_len] = '/';
        ++out_len;
    }

    if (out_len + segment_len >= out_size) {
        return -1;
    }

    for (i = 0u; i < segment_len; ++i) {
        out[out_len + i] = segment[i];
    }
    out[out_len + segment_len] = '\0';
    return 0;
}

static int path_apply(const char *path, char *out, uint32_t out_size)
{
    uint32_t start;
    uint32_t len;

    start = 0u;
    while (path[start] != '\0') {
        while (path[start] == '/') {
            ++start;
        }
        if (path[start] == '\0') {
            break;
        }

        len = 0u;
        while (path[start + len] != '\0' && path[start + len] != '/') {
            ++len;
        }

        if (len == 1u && path[start] == '.') {
        } else if (len == 2u && path[start] == '.' &&
            path[start + 1u] == '.') {
            path_pop(out);
        } else if (path_append(out, out_size, path + start, len) != 0) {
            return -1;
        }

        start += len;
    }

    return 0;
}

int vfs_resolve_path(const char *cwd, const char *path, char *out,
    uint32_t out_size)
{
    if (path == NULL || out == NULL || out_size < 2u) {
        return -1;
    }

    if (path[0] == '/') {
        out[0] = '/';
        out[1] = '\0';
        return path_apply(path, out, out_size);
    }

    if (cwd == NULL || cwd[0] == '\0') {
        cwd = "/";
    }
    if (cwd[0] != '/' || path_copy(out, out_size, cwd) != 0) {
        return -1;
    }

    return path_apply(path, out, out_size);
}

int vfs_open(const char *path, struct vfs_file *file)
{
    if (root_ops == NULL || path == NULL || file == NULL) {
        return -1;
    }

    memset(file, 0, sizeof(*file));
    return root_ops->open(path, file);
}

int vfs_read(struct vfs_file *file, void *buffer, uint32_t byte_count)
{
    if (file == NULL || file->ops == NULL || file->ops->read == NULL ||
            buffer == NULL) {
        return -1;
    }

    return file->ops->read(file, buffer, byte_count);
}

void vfs_close(struct vfs_file *file)
{
    if (file == NULL || file->ops == NULL || file->ops->close == NULL) {
        return;
    }

    file->ops->close(file);
    memset(file, 0, sizeof(*file));
}

int vfs_open_dir(const char *path, struct vfs_dir *dir)
{
    if (root_ops == NULL || root_ops->open_dir == NULL ||
        path == NULL || dir == NULL) {
        return -1;
    }

    memset(dir, 0, sizeof(*dir));
    return root_ops->open_dir(path, dir);
}

int vfs_read_dir(struct vfs_dir *dir, struct vfs_dir_entry *entry)
{
    if (dir == NULL || dir->ops == NULL || dir->ops->read_dir == NULL ||
        entry == NULL) {
        return -1;
    }

    return dir->ops->read_dir(dir, entry);
}

void vfs_close_dir(struct vfs_dir *dir)
{
    if (dir == NULL || dir->ops == NULL || dir->ops->close_dir == NULL) {
        return;
    }

    dir->ops->close_dir(dir);
    memset(dir, 0, sizeof(*dir));
}

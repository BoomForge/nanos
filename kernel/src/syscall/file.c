#include <kernel/platform.h>
#include <kernel/string.h>
#include <kernel/syscall/internal.h>
#include <kernel/types.h>
#include <kernel/vfs.h>

static int resolve_user_file_path(const char *user_path, char *resolved,
    uint32_t resolved_size)
{
    char path[NANOS_PATH_MAX];

    if (syscall_copy_user_string(user_path, path, sizeof(path)) != 0 ||
        vfs_resolve_path("/", path, resolved, resolved_size) != 0) {
        return -1;
    }

    return 0;
}

static uint32_t syscall_file_read(const struct nanos_file_request *request)
{
    struct vfs_file file;
    char resolved[VFS_PATH_MAX];
    uint32_t skipped;
    uint32_t chunk;
    char scratch[64];
    int bytes;

    if (request->size == 0u) {
        return 0u;
    }
    if (request->size != 0u &&
        !platform_user_range_is_valid(request->buffer, request->size)) {
        return NANOS_ERROR_INVALID;
    }
    if (resolve_user_file_path(request->path, resolved, sizeof(resolved)) != 0 ||
        vfs_open(resolved, &file) != 0) {
        return NANOS_ERROR_INVALID;
    }

    skipped = 0u;
    while (skipped < request->offset) {
        chunk = request->offset - skipped;
        if (chunk > sizeof(scratch)) {
            chunk = sizeof(scratch);
        }
        bytes = vfs_read(&file, scratch, chunk);
        if (bytes < 0) {
            vfs_close(&file);
            return NANOS_ERROR_INVALID;
        }
        if (bytes == 0) {
            vfs_close(&file);
            return 0u;
        }
        skipped += (uint32_t)bytes;
    }

    bytes = vfs_read(&file, request->buffer, request->size);
    vfs_close(&file);
    if (bytes < 0) {
        return NANOS_ERROR_INVALID;
    }

    return (uint32_t)bytes;
}

static uint32_t syscall_file_read_dir(const struct nanos_file_request *request)
{
    struct vfs_dir dir;
    struct vfs_dir_entry entry;
    char resolved[VFS_PATH_MAX];
    uint32_t index;
    int result;

    if (request->size < NANOS_DIRENT_SIZE ||
        !platform_user_range_is_valid(request->buffer, NANOS_DIRENT_SIZE)) {
        return NANOS_ERROR_INVALID;
    }
    if (resolve_user_file_path(request->path, resolved, sizeof(resolved)) != 0 ||
        vfs_open_dir(resolved, &dir) != 0) {
        return NANOS_ERROR_INVALID;
    }

    index = 0u;
    for (;;) {
        result = vfs_read_dir(&dir, &entry);
        if (result < 0) {
            vfs_close_dir(&dir);
            return NANOS_ERROR_INVALID;
        }
        if (result == 0) {
            vfs_close_dir(&dir);
            return 0u;
        }
        if (index == request->offset) {
            break;
        }
        ++index;
    }

    memset(request->buffer, 0, NANOS_DIRENT_SIZE);
    memcpy(request->buffer, &entry, sizeof(entry));
    vfs_close_dir(&dir);
    return 1u;
}

uint32_t syscall_file_from_user(const struct nanos_file_request *user_request)
{
    struct nanos_file_request request;

    if (!platform_user_range_is_valid(user_request,
        (uint32_t)sizeof(user_request[0]))) {
        return NANOS_ERROR_INVALID;
    }

    memcpy(&request, user_request, sizeof(request));
    if (request.operation == NANOS_FILE_READ) {
        return syscall_file_read(&request);
    }
    if (request.operation == NANOS_FILE_READ_DIR) {
        return syscall_file_read_dir(&request);
    }

    return NANOS_ERROR_INVALID;
}

#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <kernel/types.h>

#define VFS_NAME_MAX 64u
#define VFS_PATH_MAX 128u
#define VFS_TYPE_FILE 1u
#define VFS_TYPE_DIR 2u

struct vfs_file;
struct vfs_dir;

struct vfs_dir_entry {
    char name[VFS_NAME_MAX];
    uint32_t type;
    uint32_t size;
};

struct vfs_ops {
    int (*open)(const char *path, struct vfs_file *file);
    int (*read)(struct vfs_file *file, void *buffer, uint32_t byte_count);
    void (*close)(struct vfs_file *file);
    int (*open_dir)(const char *path, struct vfs_dir *dir);
    int (*read_dir)(struct vfs_dir *dir, struct vfs_dir_entry *entry);
    void (*close_dir)(struct vfs_dir *dir);
};

struct vfs_file {
    const struct vfs_ops *ops;
    void *context;
    uint32_t size;
    uint32_t pos;
};

struct vfs_dir {
    const struct vfs_ops *ops;
    void *context;
    uint32_t pos;
    char path[VFS_PATH_MAX];
};

void vfs_init(void);
int vfs_mount_root(const struct vfs_ops *ops);
int vfs_resolve_path(const char *cwd, const char *path, char *out,
    uint32_t out_size);
int vfs_open(const char *path, struct vfs_file *file);
int vfs_read(struct vfs_file *file, void *buffer, uint32_t byte_count);
void vfs_close(struct vfs_file *file);
int vfs_open_dir(const char *path, struct vfs_dir *dir);
int vfs_read_dir(struct vfs_dir *dir, struct vfs_dir_entry *entry);
void vfs_close_dir(struct vfs_dir *dir);

#endif

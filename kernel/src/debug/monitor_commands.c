#include <kernel/block.h>
#include <kernel/debug/monitor_internal.h>
#include <kernel/debug/process.h>
#include <kernel/fb_console.h>
#include <kernel/heap.h>
#include <kernel/pmm.h>
#include <kernel/process.h>
#include <kernel/time.h>
#include <kernel/types.h>
#include <kernel/vfs.h>
#include <nanos/app.h>
#include <nanos/syscall.h>

static int parse_program_param(const char *text, char *path,
    uint32_t path_size, char *param, uint32_t param_size)
{
    uint32_t len;
    uint32_t i;

    if (text == NULL || path == NULL || path_size == 0u ||
        param == NULL || param_size == 0u) {
        return -1;
    }

    while (monitor_is_space(*text)) {
        ++text;
    }
    if (*text == '\0') {
        return -1;
    }

    len = 0u;
    while (text[len] != '\0' && !monitor_is_space(text[len])) {
        if (len + 1u >= path_size) {
            return -1;
        }
        path[len] = text[len];
        ++len;
    }
    path[len] = '\0';
    text += len;

    while (monitor_is_space(*text)) {
        ++text;
    }

    i = 0u;
    while (text[i] != '\0') {
        if (i + 1u >= param_size) {
            return -1;
        }
        param[i] = text[i];
        ++i;
    }
    param[i] = '\0';
    return 0;
}

static void command_help(void)
{
    monitor_writeln("commands:");
    monitor_writeln("  help   show this help");
    monitor_writeln("  clear  clear the screen");
    monitor_writeln("  mem    show physical memory");
    monitor_writeln("  heap   show kernel heap");
    monitor_writeln("  uptime show seconds since timer start");
    monitor_writeln("  blk    list block devices");
    monitor_writeln("  read0  print ram0 sector 0");
    monitor_writeln("  pwd    show current directory");
    monitor_writeln("  cd     change current directory");
    monitor_writeln("  ls     list files");
    monitor_writeln("  cat    print a file");
    monitor_writeln("  ps     list processes");
    monitor_writeln("  reap   remove an exited process");
    monitor_writeln("  run    run a user program");
    monitor_writeln("  start  queue a user program");
    monitor_writeln("  sched  run queued programs");
}

static void command_mem(void)
{
    struct pmm_stats stats;

    stats = pmm_get_stats();
    monitor_write("memory total ");
    monitor_write_uint((stats.total_pages * PMM_PAGE_SIZE) / 1024u);
    monitor_write(" KiB, free ");
    monitor_write_uint((stats.free_pages * PMM_PAGE_SIZE) / 1024u);
    monitor_write(" KiB, used pages ");
    monitor_write_uint(stats.used_pages);
    monitor_putc('/');
    monitor_write_uint(stats.total_pages);
    monitor_putc('\n');
}

static void command_heap(void)
{
    struct heap_stats stats;

    stats = heap_get_stats();
    if (stats.total_bytes == 0u) {
        monitor_writeln("heap not ready");
        return;
    }

    monitor_write("heap used ");
    monitor_write_uint(stats.used_bytes);
    monitor_putc('/');
    monitor_write_uint(stats.total_bytes);
    monitor_write(" bytes, free ");
    monitor_write_uint(stats.free_bytes);
    monitor_write(", blocks ");
    monitor_write_uint(stats.blocks);
    monitor_putc('\n');
}

static void command_uptime(void)
{
    monitor_write("uptime ");
    monitor_write_uint(kernel_time_seconds());
    monitor_writeln(" seconds");
}

static void command_blk(void)
{
    struct block_device *device;

    if (block_device_count() == 0u) {
        monitor_writeln("no block devices");
        return;
    }

    device = block_first();
    while (device != NULL) {
        monitor_write(device->name);
        monitor_write(": sectors ");
        monitor_write_uint(device->sector_count);
        monitor_write(", sector size ");
        monitor_write_uint(device->sector_size);
        monitor_putc('\n');
        device = block_next(device);
    }
}

static void command_read0(void)
{
    static uint8_t sector[BLOCK_SECTOR_SIZE];
    struct block_device *device;
    uint32_t i;
    char ch;

    device = block_first();
    if (device == NULL) {
        monitor_writeln("no block devices");
        return;
    }
    if (block_read_sector(device, 0u, sector) != 0) {
        monitor_writeln("read failed");
        return;
    }

    monitor_write(device->name);
    monitor_writeln(" sector 0:");
    for (i = 0u; i < 160u && i < BLOCK_SECTOR_SIZE; ++i) {
        if (sector[i] == 0u) {
            break;
        }
        ch = (char)sector[i];
        if (ch == '\n') {
            monitor_putc('\n');
        } else if (ch >= ' ' && ch <= '~') {
            monitor_putc(ch);
        } else {
            monitor_putc('.');
        }
    }
    monitor_putc('\n');
}

static int monitor_program_has_path(const char *path)
{
    while (*path != '\0') {
        if (*path == '/') {
            return 1;
        }
        ++path;
    }

    return 0;
}

static const char *resolve_monitor_program(const char *program,
    char *resolved, uint32_t resolved_size)
{
    if (program == NULL || program[0] == '\0') {
        return NULL;
    }

    if (program[0] == '.' || monitor_program_has_path(program)) {
        if (monitor_resolve_path(program, resolved, resolved_size) != 0) {
            return NULL;
        }
        return resolved;
    }

    return program;
}

static void command_pwd(void)
{
    monitor_writeln(monitor_cwd());
}

static void command_cd(const char *path)
{
    struct vfs_dir dir;
    char resolved[VFS_PATH_MAX];

    if (path == NULL || *path == '\0') {
        path = "/";
    }
    if (monitor_resolve_path(path, resolved, sizeof(resolved)) != 0 ||
        vfs_open_dir(resolved, &dir) != 0) {
        monitor_writeln("cd failed");
        return;
    }
    vfs_close_dir(&dir);

    if (monitor_set_cwd(resolved) != 0) {
        monitor_writeln("cd failed");
    }
}

static void command_ls(const char *path)
{
    struct vfs_dir dir;
    struct vfs_dir_entry entry;
    char resolved[VFS_PATH_MAX];
    int result;

    if (monitor_resolve_path(path, resolved, sizeof(resolved)) != 0 ||
        vfs_open_dir(resolved, &dir) != 0) {
        monitor_writeln("ls failed");
        return;
    }

    for (;;) {
        result = vfs_read_dir(&dir, &entry);
        if (result < 0) {
            vfs_close_dir(&dir);
            monitor_writeln("ls failed");
            return;
        }
        if (result == 0) {
            break;
        }

        monitor_write(entry.name);
        if (entry.type == VFS_TYPE_DIR) {
            monitor_putc('/');
        } else {
            monitor_write(" ");
            monitor_write_uint(entry.size);
            monitor_write(" bytes");
        }
        monitor_putc('\n');
    }

    vfs_close_dir(&dir);
}

static void command_cat(const char *path)
{
    struct vfs_file file;
    char resolved[VFS_PATH_MAX];
    uint8_t buffer[96];
    int bytes;
    int i;
    char ch;

    if (path == NULL || *path == '\0') {
        monitor_writeln("usage: cat PATH");
        return;
    }
    if (monitor_resolve_path(path, resolved, sizeof(resolved)) != 0 ||
        vfs_open(resolved, &file) != 0) {
        monitor_writeln("file not found");
        return;
    }

    for (;;) {
        bytes = vfs_read(&file, buffer, sizeof(buffer));
        if (bytes <= 0) {
            break;
        }
        for (i = 0; i < bytes; ++i) {
            ch = (char)buffer[i];
            if (ch == '\n') {
                monitor_putc('\n');
            } else if (ch >= ' ' && ch <= '~') {
                monitor_putc(ch);
            } else {
                monitor_putc('.');
            }
        }
    }
    monitor_putc('\n');
    vfs_close(&file);
}

static void command_ps(void)
{
    const struct process *process;
    uint32_t i;
    int printed;

    printed = 0;
    for (i = 0u; i < debug_process_count(); ++i) {
        process = debug_process_get(i);
        if (process == NULL || process->state == PROCESS_EMPTY) {
            continue;
        }

        monitor_write("pid ");
        monitor_write_uint(process->pid);
        monitor_write(" app ");
        monitor_write_uint(process->app_pid);
        monitor_write(" ");
        monitor_write(process->name);
        monitor_write(" ");
        monitor_write(debug_process_state_name(process->state));
        monitor_write(" status ");
        monitor_write_uint((uint32_t)process->exit_status);
        monitor_write(" preempts ");
        monitor_write_uint(process->preempt_count);
        monitor_putc('\n');
        printed = 1;
    }

    if (!printed) {
        monitor_writeln("no processes");
    }
}

static void command_run(const char *path)
{
    const struct process *process;
    char param[NANOS_APP_PARAM_MAX];
    char program[NANOS_PATH_MAX];
    char resolved[VFS_PATH_MAX];
    const char *app_path;
    int pid;

    if (parse_program_param(path, program, sizeof(program), param,
        sizeof(param)) != 0) {
        monitor_writeln("usage: run PROGRAM [PARAM]");
        return;
    }

    app_path = resolve_monitor_program(program, resolved, sizeof(resolved));
    if (app_path == NULL) {
        monitor_writeln("run failed");
        return;
    }

    pid = process_start_app(app_path, param);
    if (pid < 0) {
        monitor_writeln("run failed");
        return;
    }

    if (process_schedule() != 0) {
        monitor_writeln("run failed");
        return;
    }

    process = debug_process_find((uint32_t)pid);
    if (process == NULL || process->state != PROCESS_EXITED) {
        monitor_writeln("run queued");
        return;
    }

    monitor_write(program);
    monitor_write(" exited ");
    monitor_write_uint((uint32_t)process->exit_status);
    monitor_putc('\n');
}

static void command_start(const char *path)
{
    char param[NANOS_APP_PARAM_MAX];
    char program[NANOS_PATH_MAX];
    char resolved[VFS_PATH_MAX];
    const char *app_path;
    int pid;

    if (parse_program_param(path, program, sizeof(program), param,
        sizeof(param)) != 0) {
        monitor_writeln("usage: start PROGRAM [PARAM]");
        return;
    }

    app_path = resolve_monitor_program(program, resolved, sizeof(resolved));
    if (app_path == NULL) {
        monitor_writeln("start failed");
        return;
    }

    pid = process_start_app(app_path, param);
    if (pid < 0) {
        monitor_writeln("start failed");
        return;
    }

    monitor_write("started ");
    monitor_write(program);
    monitor_write(" pid ");
    monitor_write_uint((uint32_t)pid);
    monitor_putc('\n');
}

static void command_sched(void)
{
    if (process_schedule() != 0) {
        monitor_writeln("no runnable processes");
    }
}

static void command_reap(const char *pid_text)
{
    uint32_t pid;

    if (monitor_parse_uint(pid_text, &pid) != 0) {
        monitor_writeln("usage: reap PID");
        return;
    }

    if (debug_process_reap(pid) != 0) {
        monitor_writeln("reap failed");
        return;
    }

    monitor_write("reaped ");
    monitor_write_uint(pid);
    monitor_putc('\n');
}

void monitor_execute_line(const char *line)
{
    const char *arg;

    if (monitor_command_equals(line, "")) {
        return;
    }
    if (monitor_command_equals(line, "help")) {
        command_help();
    } else if (monitor_command_equals(line, "clear")) {
        fb_console_clear();
    } else if (monitor_command_equals(line, "mem")) {
        command_mem();
    } else if (monitor_command_equals(line, "heap")) {
        command_heap();
    } else if (monitor_command_equals(line, "uptime")) {
        command_uptime();
    } else if (monitor_command_equals(line, "blk")) {
        command_blk();
    } else if (monitor_command_equals(line, "read0")) {
        command_read0();
    } else if (monitor_command_equals(line, "pwd")) {
        command_pwd();
    } else if (monitor_command_equals(line, "ps")) {
        command_ps();
    } else {
        arg = monitor_command_argument(line, "cd");
        if (arg != NULL) {
            command_cd(arg);
        } else {
            arg = monitor_command_argument(line, "ls");
            if (arg != NULL) {
                command_ls(arg);
            } else {
                arg = monitor_command_argument(line, "cat");
                if (arg != NULL) {
                    command_cat(arg);
                } else {
                    arg = monitor_command_argument(line, "run");
                    if (arg != NULL) {
                        command_run(arg);
                    } else {
                        arg = monitor_command_argument(line, "start");
                        if (arg != NULL) {
                            command_start(arg);
                        } else {
                            arg = monitor_command_argument(line, "reap");
                            if (arg != NULL) {
                                command_reap(arg);
                            } else if (monitor_command_equals(line, "sched")) {
                                command_sched();
                            } else {
                                monitor_write("unknown command: ");
                                monitor_writeln(line);
                            }
                        }
                    }
                }
            }
        }
    }
}

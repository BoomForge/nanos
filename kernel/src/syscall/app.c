#include <kernel/platform.h>
#include <kernel/process.h>
#include <kernel/string.h>
#include <kernel/syscall/internal.h>
#include <kernel/types.h>

uint32_t syscall_start_app_from_user(const char *user_path,
    const char *user_param)
{
    char param[NANOS_APP_PARAM_MAX];
    char path[NANOS_PATH_MAX];
    int pid;

    if (syscall_copy_user_string(user_path, path, sizeof(path)) != 0) {
        return NANOS_ERROR_INVALID;
    }

    param[0] = '\0';
    if (user_param != NULL) {
        if (syscall_copy_user_string(user_param, param,
            sizeof(param)) != 0) {
            return NANOS_ERROR_INVALID;
        }
    }

    pid = process_start_app(path, param);
    if (pid < 0) {
        return NANOS_ERROR_INVALID;
    }

    return (uint32_t)pid;
}

uint32_t syscall_thread(uint32_t operation, uint32_t entry,
    uint32_t stack_top)
{
    int pid;

    if (operation != NANOS_THREAD_CREATE) {
        return NANOS_ERROR_INVALID;
    }

    pid = process_create_thread_current(entry, stack_top);
    if (pid < 0) {
        return NANOS_ERROR_INVALID;
    }

    return (uint32_t)pid;
}

uint32_t syscall_process_info(uint32_t selector,
    struct nanos_process_info *user_info)
{
    struct nanos_process_info info;

    if (selector == NANOS_PROCESS_INFO_COUNT) {
        return process_active_count();
    }
    if (!platform_user_range_is_valid(user_info, NANOS_PROCESS_INFO_SIZE)) {
        return NANOS_ERROR_INVALID;
    }
    if (process_get_info(selector, &info) != 0) {
        return NANOS_ERROR_INVALID;
    }

    memcpy(user_info, &info, sizeof(info));
    return info.pid;
}

uint32_t syscall_app_control(uint32_t operation, uint32_t target_pid)
{
    int result;

    if (operation != NANOS_APP_CONTROL_KILL || target_pid == 0u) {
        return NANOS_ERROR_INVALID;
    }

    if (process_app_pid_for_pid(target_pid) == process_current_app_pid()) {
        syscall_gui_finish_current_draw();
    }

    result = process_kill_app(target_pid, 0);
    if (result < 0) {
        return NANOS_ERROR_INVALID;
    }
    if (result != 0) {
        platform_user_exit(0);
    }

    return 0u;
}

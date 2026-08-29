#ifndef KERNEL_DEBUG_MONITOR_INTERNAL_H
#define KERNEL_DEBUG_MONITOR_INTERNAL_H

#include <kernel/types.h>
#include <kernel/vfs.h>

void monitor_putc(char ch);
void monitor_write(const char *text);
void monitor_writeln(const char *text);
void monitor_write_uint(uint32_t value);
int monitor_copy_text(char *dest, uint32_t dest_size, const char *src);
int monitor_is_space(char ch);
int monitor_command_equals(const char *input, const char *command);
const char *monitor_command_argument(const char *input, const char *command);
int monitor_parse_uint(const char *text, uint32_t *value);
const char *monitor_cwd(void);
int monitor_set_cwd(const char *path);
int monitor_resolve_path(const char *path, char *resolved,
    uint32_t resolved_size);
void monitor_execute_line(const char *line);

#endif

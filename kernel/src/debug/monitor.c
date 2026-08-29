#include <kernel/debug/monitor.h>
#include <kernel/debug/monitor_internal.h>
#include <kernel/fb_console.h>
#include <kernel/print.h>
#include <kernel/types.h>
#include <kernel/vfs.h>

#define MONITOR_LINE_MAX 128u

static char line[MONITOR_LINE_MAX];
static char cwd[VFS_PATH_MAX];
static uint32_t line_len;

int monitor_copy_text(char *dest, uint32_t dest_size, const char *src)
{
    uint32_t i;

    if (dest == NULL || src == NULL || dest_size == 0u) {
        return -1;
    }

    for (i = 0u; i + 1u < dest_size && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
    return src[i] == '\0' ? 0 : -1;
}

void monitor_putc(char ch)
{
    fb_console_putc(ch);
    print_putc(ch);
}

void monitor_write(const char *text)
{
    while (*text != '\0') {
        monitor_putc(*text);
        ++text;
    }
}

void monitor_writeln(const char *text)
{
    monitor_write(text);
    monitor_putc('\n');
}

void monitor_write_uint(uint32_t value)
{
    char digits[11];
    uint32_t count;

    if (value == 0u) {
        monitor_putc('0');
        return;
    }

    count = 0u;
    while (value != 0u && count < sizeof(digits)) {
        digits[count] = (char)('0' + (value % 10u));
        value /= 10u;
        ++count;
    }

    while (count > 0u) {
        --count;
        monitor_putc(digits[count]);
    }
}

static void prompt(void)
{
    monitor_write("nanos:");
    monitor_write(cwd);
    monitor_write("> ");
}

int monitor_is_space(char ch)
{
    return ch == ' ' || ch == '\t';
}

int monitor_command_equals(const char *input, const char *command)
{
    while (monitor_is_space(*input)) {
        ++input;
    }

    while (*command != '\0') {
        if (*input != *command) {
            return 0;
        }
        ++input;
        ++command;
    }

    while (monitor_is_space(*input)) {
        ++input;
    }

    return *input == '\0';
}

const char *monitor_command_argument(const char *input, const char *command)
{
    while (monitor_is_space(*input)) {
        ++input;
    }

    while (*command != '\0') {
        if (*input != *command) {
            return NULL;
        }
        ++input;
        ++command;
    }
    if (*input != '\0' && !monitor_is_space(*input)) {
        return NULL;
    }
    while (monitor_is_space(*input)) {
        ++input;
    }

    return input;
}

int monitor_parse_uint(const char *text, uint32_t *value)
{
    uint32_t digit;
    uint32_t result;

    if (text == NULL || value == NULL) {
        return -1;
    }

    while (monitor_is_space(*text)) {
        ++text;
    }
    if (*text < '0' || *text > '9') {
        return -1;
    }

    result = 0u;
    while (*text >= '0' && *text <= '9') {
        digit = (uint32_t)(*text - '0');
        if (result > (0xffffffffu - digit) / 10u) {
            return -1;
        }
        result = (result * 10u) + digit;
        ++text;
    }

    while (monitor_is_space(*text)) {
        ++text;
    }
    if (*text != '\0') {
        return -1;
    }

    *value = result;
    return 0;
}

const char *monitor_cwd(void)
{
    return cwd;
}

int monitor_set_cwd(const char *path)
{
    return monitor_copy_text(cwd, sizeof(cwd), path);
}

int monitor_resolve_path(const char *path, char *resolved,
    uint32_t resolved_size)
{
    if (path == NULL || *path == '\0') {
        path = ".";
    }

    return vfs_resolve_path(cwd, path, resolved, resolved_size);
}

void monitor_start(void)
{
    fb_console_clear();
    line_len = 0u;
    (void)monitor_set_cwd("/");

    monitor_writeln("NanOS text monitor");
    monitor_writeln("Type 'help' for commands.");
    prompt();
}

void monitor_on_key(char ch)
{
    if (ch == '\n') {
        monitor_putc('\n');
        line[line_len] = '\0';
        monitor_execute_line(line);
        line_len = 0u;
        prompt();
        return;
    }

    if (ch == '\b') {
        if (line_len > 0u) {
            --line_len;
            monitor_putc('\b');
        }
        return;
    }

    if (ch < ' ' || ch > '~') {
        return;
    }

    if (line_len + 1u >= MONITOR_LINE_MAX) {
        return;
    }

    line[line_len] = ch;
    ++line_len;
    monitor_putc(ch);
}

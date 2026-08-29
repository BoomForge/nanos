#include <kernel/platform.h>
#include <kernel/syscall/internal.h>
#include <kernel/types.h>

int syscall_unpack_position_size(uint32_t packed, int32_t *pos,
    uint32_t *size)
{
    if (pos == NULL || size == NULL) {
        return -1;
    }

    *pos = (int32_t)(packed >> 16);
    *size = packed & 0xffffu;
    return 0;
}

int syscall_copy_user_string(const char *user_text, char *out,
    uint32_t out_size)
{
    uint32_t i;

    if (user_text == NULL || out == NULL || out_size == 0u) {
        return -1;
    }

    for (i = 0u; i < out_size; ++i) {
        if (!platform_user_range_is_valid(user_text + i, 1u)) {
            return -1;
        }
        out[i] = user_text[i];
        if (out[i] == '\0') {
            return 0;
        }
    }

    out[out_size - 1u] = '\0';
    return -1;
}

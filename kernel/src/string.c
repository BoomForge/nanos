#include <kernel/string.h>

void *memset(void *dest, int value, size_t count)
{
    unsigned char *out;

    out = (unsigned char *)dest;
    while (count > 0u) {
        *out = (unsigned char)value;
        ++out;
        --count;
    }

    return dest;
}

void *memcpy(void *dest, const void *src, size_t count)
{
    unsigned char *out;
    const unsigned char *in;

    out = (unsigned char *)dest;
    in = (const unsigned char *)src;
    while (count > 0u) {
        *out = *in;
        ++out;
        ++in;
        --count;
    }

    return dest;
}

size_t strlen(const char *text)
{
    size_t len;

    len = 0u;
    while (text[len] != '\0') {
        ++len;
    }
    return len;
}

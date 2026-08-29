#ifndef KERNEL_STRING_H
#define KERNEL_STRING_H

#include <kernel/types.h>

void *memset(void *dest, int value, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
size_t strlen(const char *text);

#endif

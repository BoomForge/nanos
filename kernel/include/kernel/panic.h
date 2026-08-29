#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#include <kernel/compiler.h>

void panic(const char *message) NORETURN;

#endif

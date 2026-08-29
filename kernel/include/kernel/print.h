#ifndef KERNEL_PRINT_H
#define KERNEL_PRINT_H

#include <kernel/types.h>

void print_init(void);
void print_putc(char ch);
void print_write(const char *text);
void print_writeln(const char *text);
void print_hex32(uint32_t value);
void print_uint(uint32_t value);

#endif

#ifndef KERNEL_FB_CONSOLE_H
#define KERNEL_FB_CONSOLE_H

#include <kernel/types.h>

void fb_console_init(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void fb_console_clear(void);
void fb_console_putc(char ch);
void fb_console_write(const char *text);
void fb_console_writeln(const char *text);

#endif

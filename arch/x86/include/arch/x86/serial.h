#ifndef ARCH_X86_SERIAL_H
#define ARCH_X86_SERIAL_H

int serial_init(void);
void serial_putc(char ch);
void serial_write(const char *text);

#endif

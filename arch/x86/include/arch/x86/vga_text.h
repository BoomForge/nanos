#ifndef ARCH_X86_VGA_TEXT_H
#define ARCH_X86_VGA_TEXT_H

void vga_text_clear(void);
void vga_text_putc(char ch);
void vga_text_write(const char *text);

#endif

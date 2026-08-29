#include <arch/x86/vga_text.h>
#include <kernel/types.h>

#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u
#define VGA_MEMORY ((uint16_t *)0xb8000u)
#define VGA_COLOR 0x0fu

static uint32_t row;
static uint32_t col;

static uint16_t vga_entry(char ch)
{
    return (uint16_t)((uint16_t)VGA_COLOR << 8) | (uint8_t)ch;
}

void vga_text_clear(void)
{
    uint32_t i;

    row = 0;
    col = 0;
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        VGA_MEMORY[i] = vga_entry(' ');
    }
}

static void vga_text_newline(void)
{
    col = 0;
    if (row + 1u < VGA_HEIGHT) {
        ++row;
    }
}

void vga_text_putc(char ch)
{
    if (ch == '\n') {
        vga_text_newline();
        return;
    }

    VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry(ch);
    ++col;
    if (col >= VGA_WIDTH) {
        vga_text_newline();
    }
}

void vga_text_write(const char *text)
{
    while (*text != '\0') {
        vga_text_putc(*text);
        ++text;
    }
}

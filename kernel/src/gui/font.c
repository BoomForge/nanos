#include <kernel/framebuffer.h>
#include <kernel/gui/font.h>
#include <kernel/types.h>

static const uint8_t glyph_unknown[GUI_FONT_HEIGHT] = { 31, 17, 1, 2, 4, 0, 4 };
static const uint8_t glyph_space[GUI_FONT_HEIGHT] = { 0, 0, 0, 0, 0, 0, 0 };
static const uint8_t glyph_a[GUI_FONT_HEIGHT] = { 14, 17, 17, 31, 17, 17, 17 };
static const uint8_t glyph_b[GUI_FONT_HEIGHT] = { 30, 17, 17, 30, 17, 17, 30 };
static const uint8_t glyph_c[GUI_FONT_HEIGHT] = { 14, 17, 16, 16, 16, 17, 14 };
static const uint8_t glyph_d[GUI_FONT_HEIGHT] = { 30, 17, 17, 17, 17, 17, 30 };
static const uint8_t glyph_e[GUI_FONT_HEIGHT] = { 31, 16, 16, 30, 16, 16, 31 };
static const uint8_t glyph_f[GUI_FONT_HEIGHT] = { 31, 16, 16, 30, 16, 16, 16 };
static const uint8_t glyph_g[GUI_FONT_HEIGHT] = { 14, 17, 16, 23, 17, 17, 14 };
static const uint8_t glyph_h[GUI_FONT_HEIGHT] = { 17, 17, 17, 31, 17, 17, 17 };
static const uint8_t glyph_i[GUI_FONT_HEIGHT] = { 14, 4, 4, 4, 4, 4, 14 };
static const uint8_t glyph_j[GUI_FONT_HEIGHT] = { 7, 2, 2, 2, 18, 18, 12 };
static const uint8_t glyph_k[GUI_FONT_HEIGHT] = { 17, 18, 20, 24, 20, 18, 17 };
static const uint8_t glyph_l[GUI_FONT_HEIGHT] = { 16, 16, 16, 16, 16, 16, 31 };
static const uint8_t glyph_m[GUI_FONT_HEIGHT] = { 17, 27, 21, 21, 17, 17, 17 };
static const uint8_t glyph_n[GUI_FONT_HEIGHT] = { 17, 25, 21, 19, 17, 17, 17 };
static const uint8_t glyph_o[GUI_FONT_HEIGHT] = { 14, 17, 17, 17, 17, 17, 14 };
static const uint8_t glyph_p[GUI_FONT_HEIGHT] = { 30, 17, 17, 30, 16, 16, 16 };
static const uint8_t glyph_q[GUI_FONT_HEIGHT] = { 14, 17, 17, 17, 21, 18, 13 };
static const uint8_t glyph_r[GUI_FONT_HEIGHT] = { 30, 17, 17, 30, 20, 18, 17 };
static const uint8_t glyph_s[GUI_FONT_HEIGHT] = { 15, 16, 16, 14, 1, 1, 30 };
static const uint8_t glyph_t[GUI_FONT_HEIGHT] = { 31, 4, 4, 4, 4, 4, 4 };
static const uint8_t glyph_u[GUI_FONT_HEIGHT] = { 17, 17, 17, 17, 17, 17, 14 };
static const uint8_t glyph_v[GUI_FONT_HEIGHT] = { 17, 17, 17, 17, 17, 10, 4 };
static const uint8_t glyph_w[GUI_FONT_HEIGHT] = { 17, 17, 17, 21, 21, 21, 10 };
static const uint8_t glyph_x[GUI_FONT_HEIGHT] = { 17, 17, 10, 4, 10, 17, 17 };
static const uint8_t glyph_y[GUI_FONT_HEIGHT] = { 17, 17, 10, 4, 4, 4, 4 };
static const uint8_t glyph_z[GUI_FONT_HEIGHT] = { 31, 1, 2, 4, 8, 16, 31 };
static const uint8_t glyph_0[GUI_FONT_HEIGHT] = { 14, 17, 19, 21, 25, 17, 14 };
static const uint8_t glyph_1[GUI_FONT_HEIGHT] = { 4, 12, 4, 4, 4, 4, 14 };
static const uint8_t glyph_2[GUI_FONT_HEIGHT] = { 14, 17, 1, 2, 4, 8, 31 };
static const uint8_t glyph_3[GUI_FONT_HEIGHT] = { 30, 1, 1, 14, 1, 1, 30 };
static const uint8_t glyph_4[GUI_FONT_HEIGHT] = { 2, 6, 10, 18, 31, 2, 2 };
static const uint8_t glyph_5[GUI_FONT_HEIGHT] = { 31, 16, 30, 1, 1, 17, 14 };
static const uint8_t glyph_6[GUI_FONT_HEIGHT] = { 6, 8, 16, 30, 17, 17, 14 };
static const uint8_t glyph_7[GUI_FONT_HEIGHT] = { 31, 1, 2, 4, 8, 8, 8 };
static const uint8_t glyph_8[GUI_FONT_HEIGHT] = { 14, 17, 17, 14, 17, 17, 14 };
static const uint8_t glyph_9[GUI_FONT_HEIGHT] = { 14, 17, 17, 15, 1, 2, 12 };
static const uint8_t glyph_colon[GUI_FONT_HEIGHT] = { 0, 4, 4, 0, 4, 4, 0 };
static const uint8_t glyph_dot[GUI_FONT_HEIGHT] = { 0, 0, 0, 0, 0, 12, 12 };
static const uint8_t glyph_comma[GUI_FONT_HEIGHT] = { 0, 0, 0, 0, 0, 12, 4 };
static const uint8_t glyph_quote[GUI_FONT_HEIGHT] = { 4, 4, 4, 0, 0, 0, 0 };
static const uint8_t glyph_amp[GUI_FONT_HEIGHT] = { 12, 18, 20, 8, 21, 18, 13 };
static const uint8_t glyph_dash[GUI_FONT_HEIGHT] = { 0, 0, 0, 31, 0, 0, 0 };
static const uint8_t glyph_gt[GUI_FONT_HEIGHT] = { 16, 8, 4, 2, 4, 8, 16 };
static const uint8_t glyph_slash[GUI_FONT_HEIGHT] = { 1, 1, 2, 4, 8, 16, 16 };

static const uint8_t *font_rows(char ch)
{
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - ('a' - 'A'));
    }

    switch (ch) {
    case ' ': return glyph_space;
    case 'A': return glyph_a;
    case 'B': return glyph_b;
    case 'C': return glyph_c;
    case 'D': return glyph_d;
    case 'E': return glyph_e;
    case 'F': return glyph_f;
    case 'G': return glyph_g;
    case 'H': return glyph_h;
    case 'I': return glyph_i;
    case 'J': return glyph_j;
    case 'K': return glyph_k;
    case 'L': return glyph_l;
    case 'M': return glyph_m;
    case 'N': return glyph_n;
    case 'O': return glyph_o;
    case 'P': return glyph_p;
    case 'Q': return glyph_q;
    case 'R': return glyph_r;
    case 'S': return glyph_s;
    case 'T': return glyph_t;
    case 'U': return glyph_u;
    case 'V': return glyph_v;
    case 'W': return glyph_w;
    case 'X': return glyph_x;
    case 'Y': return glyph_y;
    case 'Z': return glyph_z;
    case '0': return glyph_0;
    case '1': return glyph_1;
    case '2': return glyph_2;
    case '3': return glyph_3;
    case '4': return glyph_4;
    case '5': return glyph_5;
    case '6': return glyph_6;
    case '7': return glyph_7;
    case '8': return glyph_8;
    case '9': return glyph_9;
    case ':': return glyph_colon;
    case '.': return glyph_dot;
    case ',': return glyph_comma;
    case '\'': return glyph_quote;
    case '&': return glyph_amp;
    case '-': return glyph_dash;
    case '>': return glyph_gt;
    case '/': return glyph_slash;
    default: return glyph_unknown;
    }
}

void gui_draw_char(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, char ch)
{
    const uint8_t *glyph;
    uint32_t gx;
    uint32_t gy;

    framebuffer_fill_rect(x, y, GUI_FONT_CELL_WIDTH, GUI_FONT_CELL_HEIGHT, bg);

    glyph = font_rows(ch);
    for (gy = 0u; gy < GUI_FONT_HEIGHT; ++gy) {
        for (gx = 0u; gx < GUI_FONT_WIDTH; ++gx) {
            if ((glyph[gy] & (uint8_t)(1u << (GUI_FONT_WIDTH - gx - 1u))) != 0u) {
                framebuffer_fill_rect(x + gx * GUI_FONT_SCALE, y + gy * GUI_FONT_SCALE,
                    GUI_FONT_SCALE, GUI_FONT_SCALE, fg);
            }
        }
    }
}

void gui_draw_text(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, const char *text)
{
    while (text != NULL && *text != '\0') {
        gui_draw_char(x, y, fg, bg, *text);
        x += GUI_FONT_CELL_WIDTH;
        ++text;
    }
}

#ifndef NANOS_GUI_WIDGETS_INC
#define NANOS_GUI_WIDGETS_INC

/*
 * Small app-side GUI helpers for assembly .nx programs.
 * These helpers only use public drawing syscalls; the kernel still owns
 * windows, clipping, redraw events, and input routing.
 */

.macro nanos_rect x, y, w, h, color
    movl $(((\x) << 16) | (\w)), %ebx
    movl $(((\y) << 16) | (\h)), %ecx
    movl $\color, %edx
    movl $NANOS_SYSCALL_DRAW_RECT, %eax
    int $0x80
.endm

.macro nanos_line x1, y1, x2, y2, color
    movl $(((\x1) << 16) | (\y1)), %ebx
    movl $(((\x2) << 16) | (\y2)), %ecx
    movl $\color, %edx
    movl $NANOS_SYSCALL_DRAW_LINE, %eax
    int $0x80
.endm

.macro nanos_text_at x, y, color
    movl $(((\x) << 16) | (\y)), %ebx
    movl $\color, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80
.endm

.macro nanos_border x, y, w, h, color
    nanos_line \x, \y, ((\x) + (\w) - 1), \y, \color
    nanos_line \x, ((\y) + (\h) - 1), ((\x) + (\w) - 1), ((\y) + (\h) - 1), \color
    nanos_line \x, \y, \x, ((\y) + (\h) - 1), \color
    nanos_line ((\x) + (\w) - 1), \y, ((\x) + (\w) - 1), ((\y) + (\h) - 1), \color
.endm

.macro nanos_widget_text_field x, y, w, h, fill, border
    nanos_rect \x, \y, \w, \h, \fill
    nanos_border \x, \y, \w, \h, \border
.endm

.macro nanos_widget_list x, y, w, h, fill, selected_fill, border
    nanos_rect \x, \y, \w, \h, \fill
    nanos_rect ((\x) + 4), ((\y) + 6), ((\w) - 8), 20, \selected_fill
    nanos_border \x, \y, \w, \h, \border
    nanos_line ((\x) + 4), ((\y) + 30), ((\x) + (\w) - 5), ((\y) + 30), \border
.endm

.macro nanos_widget_scrollbar x, y, h, track, thumb, border
    nanos_rect \x, \y, 12, \h, \track
    nanos_border \x, \y, 12, \h, \border
    nanos_rect ((\x) + 3), ((\y) + 18), 6, 24, \thumb
.endm

.macro nanos_widget_menu x, y, w, h, fill, border
    nanos_rect \x, \y, \w, \h, \fill
    nanos_border \x, \y, \w, \h, \border
    nanos_line ((\x) + 62), \y, ((\x) + 62), ((\y) + (\h) - 1), \border
.endm

.macro nanos_widget_button x, y, w, h, fill, border
    nanos_rect \x, \y, \w, \h, \fill
    nanos_border \x, \y, \w, \h, \border
.endm

.macro nanos_widget_dialog x, y, w, h, fill, title_fill, border
    nanos_rect \x, \y, \w, \h, \fill
    nanos_rect \x, \y, \w, 20, \title_fill
    nanos_border \x, \y, \w, \h, \border
    nanos_line \x, ((\y) + 20), ((\x) + (\w) - 1), ((\y) + 20), \border
.endm

#endif

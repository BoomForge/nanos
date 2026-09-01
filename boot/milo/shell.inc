/* M.I.L.O graphical shell and mouse-driven FAT12 root browser. */

.equ GUI_COLOR_BACKGROUND, 0x0007040b
.equ GUI_COLOR_HEADER,     0x00140c19
.equ GUI_COLOR_PANEL,      0x00110b16
.equ GUI_COLOR_PANEL_ALT,  0x001c1223
.equ GUI_COLOR_ACTIVE,     0x0043214d
.equ GUI_COLOR_PINK,       0x00ef70e8
.equ GUI_COLOR_LILAC,      0x00d6a6f2
.equ GUI_COLOR_TEXT,       0x00f2e8f4
.equ GUI_COLOR_DIM,        0x00836b88
.equ GUI_COLOR_GREEN,      0x006fe4aa
.equ GUI_COLOR_RED,        0x00ef6684
.equ GUI_COLOR_CLASSIC,    0x0055475d
.equ GUI_COLOR_LIGHT,      0x00e8d9ed
.equ GUI_COLOR_DARK,       0x00201525

.equ WM_HOME, 0
.equ WM_FILES, 1
.equ WM_TRAITS, 2
.equ WM_TERMINAL, 3
.equ WM_EDITOR, 4
.equ WM_COUNT, 5
.equ WM_NONE, 255
.equ WM_FLAG_VISIBLE, 1
.equ WM_FLAG_MINIMIZED, 2
.equ WM_FLAG_MAXIMIZED, 4
.equ WM_REFERENCE_X, 168
.equ WM_REFERENCE_Y, 48
.equ WM_ROOT_MENU_WIDTH, 232
.equ WM_ROOT_MENU_HEIGHT, 176
.equ WM_ROOT_MENU_ROW, 28
.equ WM_TASKBAR_Y, 704
.equ WM_TASKBAR_HEIGHT, 64
.equ WM_TASK_BUTTON_X, 16
.equ WM_TASK_BUTTON_Y, 724
.equ WM_TASK_BUTTON_WIDTH, 144
.equ WM_TASK_BUTTON_STRIDE, 152
.equ WM_ACTIVITY_SAMPLES, 20
.equ WM_ACTIVITY_GRAPH_MAX, 120
.equ WM_RESIZE_GRIP, 20

render_shell:
    call gui_save_terminal_state
    movl $0, (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start)
    movl $0, %eax
    movl $0, %ebx
    movl $1024, %ecx
    movl $768, %edx
    movl $GUI_COLOR_BACKGROUND, %esi
    call gui_fill_rectangle
    call wm_render_system_overlay
    call wm_render_all_windows
    cmpb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    je wm_render_no_menu
    call wm_render_root_menu
wm_render_no_menu:
    call wm_render_taskbar
    call gui_restore_terminal_state
    cmpb $WM_TERMINAL, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    jne wm_render_shell_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags + WM_TERMINAL - _start), %eax
    testb $WM_FLAG_VISIBLE, %al
    jz wm_render_shell_complete
    testb $WM_FLAG_MINIMIZED, %al
    jnz wm_render_shell_complete
    call draw_cursor
wm_render_shell_complete:
    ret

wm_initialize:
    movb $WM_HOME, (KERNEL_LOAD_ADDRESS + wm_z_order + 0 - _start)
    movb $WM_FILES, (KERNEL_LOAD_ADDRESS + wm_z_order + 1 - _start)
    movb $WM_TRAITS, (KERNEL_LOAD_ADDRESS + wm_z_order + 2 - _start)
    movb $WM_TERMINAL, (KERNEL_LOAD_ADDRESS + wm_z_order + 3 - _start)
    movb $WM_EDITOR, (KERNEL_LOAD_ADDRESS + wm_z_order + 4 - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_drag_window - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_resize_window - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    movb $0xff, (KERNEL_LOAD_ADDRESS + rtc_last_second - _start)
    movl (KERNEL_LOAD_ADDRESS + system_event_count - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + activity_previous_events - _start)
    call rtc_initialize
    call wm_sync_terminal_bounds
    ret

wm_sync_terminal_bounds:
    movl (KERNEL_LOAD_ADDRESS + wm_w + WM_TERMINAL * 4 - _start), %eax
    subl $32, %eax
    shrl $3, %eax
    cmpl $TERMINAL_COLUMNS, %eax
    jbe wm_terminal_columns_ready
    movl $TERMINAL_COLUMNS, %eax
wm_terminal_columns_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_visible_columns - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_h + WM_TERMINAL * 4 - _start), %eax
    subl $56, %eax
    shrl $4, %eax
    cmpl $TERMINAL_ROWS, %eax
    jbe wm_terminal_rows_ready
    movl $TERMINAL_ROWS, %eax
wm_terminal_rows_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_visible_rows - _start)

    movl (KERNEL_LOAD_ADDRESS + wm_x + WM_TERMINAL * 4 - _start), %eax
    addl $16, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_left - _start)
    addl $48, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_input_left - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_y + WM_TERMINAL * 4 - _start), %eax
    addl $40, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_top - _start)
    movl (KERNEL_LOAD_ADDRESS + terminal_visible_rows - _start), %ecx
    decl %ecx
    shll $4, %ecx
    addl %ecx, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_last_line - _start)
    addl $16, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_bottom - _start)
    movl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + terminal_visible_columns - _start), %ecx
    decl %ecx
    shll $3, %ecx
    addl %ecx, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_right - _start)
    ret

/* Read the battery-backed CMOS clock without inventing a software timebase.
 * QEMU should be launched with -rtc base=localtime so the RTC matches the
 * host's Australian local time. Physical PCs normally expose their configured
 * firmware time directly through the same registers. */
rtc_initialize:
    pushal
    call rtc_read_snapshot
    movb (KERNEL_LOAD_ADDRESS + rtc_second - _start), %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_last_second - _start)
    call rtc_format_display
    popal
    ret

/* Called from the existing polling loop. CMOS access is deliberately divided
 * down; a complete redraw occurs only when the hardware second changes. */
rtc_poll_update:
    pushal
    incl (KERNEL_LOAD_ADDRESS + rtc_poll_divider - _start)
    testl $0x00003fff, (KERNEL_LOAD_ADDRESS + rtc_poll_divider - _start)
    jnz rtc_poll_complete
    call rtc_read_snapshot
    movb (KERNEL_LOAD_ADDRESS + rtc_second - _start), %al
    cmpb (KERNEL_LOAD_ADDRESS + rtc_last_second - _start), %al
    je rtc_poll_complete
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_last_second - _start)
    call rtc_format_display
    call wm_sample_activity
    /* The drag preview is an XOR outline over the last complete frame. A live
     * repaint while it is present would consume that outline as background and
     * leave artifacts when the pointer moves again. Keep sampling the real
     * state, then repaint normally on the next second or drag release. */
    cmpb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_drag_window - _start)
    jne rtc_poll_complete
    cmpb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_resize_window - _start)
    jne rtc_poll_complete
    call hide_mouse_cursor
    call wm_render_live_update
    call draw_mouse_cursor
rtc_poll_complete:
    popal
    ret

rtc_read_snapshot:
    pushal
rtc_snapshot_retry:
    /* Register A bit 7 is set while the RTC is updating its fields. */
    movb $0x8a, %al
    outb %al, $0x70
    inb $0x71, %al
    testb $0x80, %al
    jnz rtc_snapshot_retry

    movb $0x80, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_raw_second - _start)
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_second - _start)
    movb $0x82, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_minute - _start)
    movb $0x84, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_hour - _start)
    movb $0x87, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_day - _start)
    movb $0x88, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_month - _start)
    movb $0x89, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_year - _start)
    movb $0x8b, %al
    outb %al, $0x70
    inb $0x71, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_register_b - _start)

    /* Reject a snapshot that crossed a second boundary mid-read. */
    movb $0x80, %al
    outb %al, $0x70
    inb $0x71, %al
    cmpb (KERNEL_LOAD_ADDRESS + rtc_raw_second - _start), %al
    jne rtc_snapshot_retry

    /* Re-enable NMI after the protected CMOS transaction. */
    movb $0x0d, %al
    outb %al, $0x70

    movb (KERNEL_LOAD_ADDRESS + rtc_hour - _start), %al
    andb $0x80, %al
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_pm_flag - _start)
    andb $0x7f, (KERNEL_LOAD_ADDRESS + rtc_hour - _start)

    /* Register B bit 2 selects binary fields; otherwise convert BCD. */
    testb $0x04, (KERNEL_LOAD_ADDRESS + rtc_register_b - _start)
    jnz rtc_fields_binary
    movb (KERNEL_LOAD_ADDRESS + rtc_second - _start), %al
    call rtc_bcd_to_binary
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_second - _start)
    movb (KERNEL_LOAD_ADDRESS + rtc_minute - _start), %al
    call rtc_bcd_to_binary
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_minute - _start)
    movb (KERNEL_LOAD_ADDRESS + rtc_hour - _start), %al
    call rtc_bcd_to_binary
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_hour - _start)
    movb (KERNEL_LOAD_ADDRESS + rtc_day - _start), %al
    call rtc_bcd_to_binary
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_day - _start)
    movb (KERNEL_LOAD_ADDRESS + rtc_month - _start), %al
    call rtc_bcd_to_binary
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_month - _start)
    movb (KERNEL_LOAD_ADDRESS + rtc_year - _start), %al
    call rtc_bcd_to_binary
    movb %al, (KERNEL_LOAD_ADDRESS + rtc_year - _start)
rtc_fields_binary:
    /* Register B bit 1 selects 24-hour mode. Normalize 12-hour RTCs. */
    testb $0x02, (KERNEL_LOAD_ADDRESS + rtc_register_b - _start)
    jnz rtc_snapshot_complete
    cmpb $12, (KERNEL_LOAD_ADDRESS + rtc_hour - _start)
    jne rtc_hour_not_twelve
    cmpb $0, (KERNEL_LOAD_ADDRESS + rtc_pm_flag - _start)
    jne rtc_snapshot_complete
    movb $0, (KERNEL_LOAD_ADDRESS + rtc_hour - _start)
    jmp rtc_snapshot_complete
rtc_hour_not_twelve:
    cmpb $0, (KERNEL_LOAD_ADDRESS + rtc_pm_flag - _start)
    je rtc_snapshot_complete
    addb $12, (KERNEL_LOAD_ADDRESS + rtc_hour - _start)
rtc_snapshot_complete:
    popal
    ret

/* AL=packed BCD, returns AL=binary. */
rtc_bcd_to_binary:
    pushl %ebx
    movb %al, %bl
    andb $0x0f, %bl
    andb $0xf0, %al
    shrb $4, %al
    movb $10, %ah
    mulb %ah
    addb %bl, %al
    popl %ebx
    ret

rtc_format_display:
    pushal
    movl $(KERNEL_LOAD_ADDRESS + rtc_display_text - _start), %ebx
    movb (KERNEL_LOAD_ADDRESS + rtc_day - _start), %al
    call rtc_store_two_digits
    movb $'/', 2(%ebx)
    addl $3, %ebx
    movb (KERNEL_LOAD_ADDRESS + rtc_month - _start), %al
    call rtc_store_two_digits
    movb $'/', 2(%ebx)
    movb $'2', 3(%ebx)
    movb $'0', 4(%ebx)
    addl $5, %ebx
    movb (KERNEL_LOAD_ADDRESS + rtc_year - _start), %al
    call rtc_store_two_digits
    movb $' ', 2(%ebx)
    movb $' ', 3(%ebx)
    addl $4, %ebx
    movb (KERNEL_LOAD_ADDRESS + rtc_hour - _start), %al
    call rtc_store_two_digits
    movb $':', 2(%ebx)
    addl $3, %ebx
    movb (KERNEL_LOAD_ADDRESS + rtc_minute - _start), %al
    call rtc_store_two_digits
    movb $0, 2(%ebx)
    popal
    ret

/* AL=value 0..99, EBX=two-character destination. */
rtc_store_two_digits:
    pushl %eax
    pushl %ecx
    pushl %edx
    movzbl %al, %eax
    xorl %edx, %edx
    movl $10, %ecx
    divl %ecx
    addb $'0', %al
    movb %al, 0(%ebx)
    addb $'0', %dl
    movb %dl, 1(%ebx)
    popl %edx
    popl %ecx
    popl %eax
    ret

wm_sample_activity:
    pushal
    movl (KERNEL_LOAD_ADDRESS + system_event_count - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + activity_current_events - _start)
    subl (KERNEL_LOAD_ADDRESS + activity_previous_events - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + activity_rate - _start)
    movl (KERNEL_LOAD_ADDRESS + activity_current_events - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + activity_previous_events - _start)
    xorl %ecx, %ecx
activity_shift_next:
    cmpl $(WM_ACTIVITY_SAMPLES - 1), %ecx
    jae activity_store_latest
    movb (KERNEL_LOAD_ADDRESS + activity_history + 1 - _start)(,%ecx), %al
    movb %al, (KERNEL_LOAD_ADDRESS + activity_history - _start)(,%ecx)
    incl %ecx
    jmp activity_shift_next
activity_store_latest:
    movl (KERNEL_LOAD_ADDRESS + activity_rate - _start), %eax
    cmpl $WM_ACTIVITY_GRAPH_MAX, %eax
    jbe activity_sample_clamped
    movl $WM_ACTIVITY_GRAPH_MAX, %eax
activity_sample_clamped:
    /* Map 0..120 measured events per second onto the 45-pixel graph. This
     * preserves useful gradation for normal PS/2 mouse packet rates instead of
     * saturating almost every active sample at the old 15-event ceiling. */
    imull $3, %eax
    shrl $3, %eax
    testl %eax, %eax
    jnz activity_sample_scaled
    cmpl $0, (KERNEL_LOAD_ADDRESS + activity_rate - _start)
    je activity_sample_scaled
    movl $1, %eax
activity_sample_scaled:
    movb %al, (KERNEL_LOAD_ADDRESS + activity_history + WM_ACTIVITY_SAMPLES - 1 - _start)
    popal
    ret

/* Redraw live root surfaces once per RTC second while preserving stacking. */
wm_render_live_update:
    call gui_save_terminal_state
    movl $0, (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start)
    call wm_render_system_overlay
    call wm_render_all_windows
    cmpb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    je wm_live_no_menu
    call wm_render_root_menu
wm_live_no_menu:
    call wm_render_taskbar
    call gui_restore_terminal_state
    cmpb $WM_TERMINAL, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    jne wm_live_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags + WM_TERMINAL - _start), %eax
    testb $WM_FLAG_VISIBLE, %al
    jz wm_live_complete
    testb $WM_FLAG_MINIMIZED, %al
    jnz wm_live_complete
    call draw_cursor
wm_live_complete:
    ret

/* A root-level system monitor inspired by Conky. It reports only native,
 * measurable state and remains behind ordinary application windows. */
wm_render_system_overlay:
    call gui_overlay_collect
    movl $808, %eax
    movl $32, %ebx
    movl $200, %ecx
    movl $608, %edx
    movl $GUI_COLOR_HEADER, %esi
    call gui_fill_rectangle
    movl $808, %eax
    movl $32, %ebx
    movl $3, %ecx
    movl $608, %edx
    movl $GUI_COLOR_PINK, %esi
    call gui_fill_rectangle

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_title - _start), %esi
    movl $824, %eax
    movl $48, %edx
    movl $GUI_COLOR_LILAC, %ecx
    call gui_print_at
    movl $824, %eax
    movl $72, %ebx
    movl $168, %ecx
    movl $2, %edx
    movl $GUI_COLOR_ACTIVE, %esi
    call gui_fill_rectangle

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_kernel - _start), %esi
    movl $824, %eax
    movl $88, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_version - _start), %esi
    movl $896, %eax
    movl $88, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_cpu - _start), %esi
    movl $824, %eax
    movl $116, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_cpu_mode - _start), %esi
    movl $872, %eax
    movl $116, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_activity - _start), %esi
    movl $824, %eax
    movl $144, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + activity_rate - _start), %eax
    movl $912, %ebx
    movl $144, %edx
    movl $GUI_COLOR_GREEN, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_events_per_second - _start), %esi
    movl $944, %eax
    movl $144, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    call wm_render_activity_graph
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_graph_scale - _start), %esi
    movl $824, %eax
    movl $220, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_apps - _start), %esi
    movl $824, %eax
    movl $236, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    call wm_count_application_states
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_open_apps - _start), %eax
    movl $872, %ebx
    movl $236, %edx
    movl $GUI_COLOR_GREEN, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_open_short - _start), %esi
    movl $888, %eax
    movl $236, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_minimized_apps - _start), %eax
    movl $936, %ebx
    movl $236, %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_min_short - _start), %esi
    movl $952, %eax
    movl $236, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    movl $WM_HOME, %edi
    movl $260, %edx
    call wm_render_application_state
    movl $WM_FILES, %edi
    movl $284, %edx
    call wm_render_application_state
    movl $WM_TRAITS, %edi
    movl $308, %edx
    call wm_render_application_state
    movl $WM_TERMINAL, %edi
    movl $332, %edx
    call wm_render_application_state
    movl $WM_EDITOR, %edi
    movl $356, %edx
    call wm_render_application_state

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_ram - _start), %esi
    movl $824, %eax
    movl $392, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl BOOT_MEMORY_MIB(%ebp), %eax
    movl $888, %ebx
    movl $392, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_mib_short - _start), %esi
    movl $944, %eax
    movl $392, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_disk - _start), %esi
    movl $824, %eax
    movl $420, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_free_kib - _start), %eax
    movl $872, %ebx
    movl $420, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_slash - _start), %esi
    movl $912, %eax
    movl $420, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_total_kib - _start), %eax
    movl $928, %ebx
    movl $420, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_kib - _start), %esi
    movl $968, %eax
    movl $420, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_files - _start), %esi
    movl $824, %eax
    movl $448, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_file_count - _start), %eax
    movl $888, %ebx
    movl $448, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_video - _start), %esi
    movl $824, %eax
    movl $476, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_video_mode - _start), %esi
    movl $880, %eax
    movl $476, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_temp - _start), %esi
    movl $824, %eax
    movl $504, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_unavailable - _start), %esi
    movl $888, %eax
    movl $504, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_network - _start), %esi
    movl $824, %eax
    movl $532, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_offline - _start), %esi
    movl $888, %eax
    movl $532, %edx
    movl $GUI_COLOR_GREEN, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_events - _start), %esi
    movl $824, %eax
    movl $560, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + system_event_count - _start), %eax
    movl $896, %ebx
    movl $560, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at

    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_hint - _start), %esi
    movl $824, %eax
    movl $592, %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_print_at
    ret

wm_render_activity_graph:
    pushal
    movl $824, %eax
    movl $172, %ebx
    movl $160, %ecx
    movl $48, %edx
    movl $GUI_COLOR_DARK, %esi
    call gui_fill_rectangle
    movl $824, %eax
    movl $218, %ebx
    movl $160, %ecx
    movl $2, %edx
    movl $GUI_COLOR_ACTIVE, %esi
    call gui_fill_rectangle
    movl $0, (KERNEL_LOAD_ADDRESS + gui_overlay_graph_index - _start)
gui_overlay_graph_next:
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_graph_index - _start), %ecx
    cmpl $WM_ACTIVITY_SAMPLES, %ecx
    jae gui_overlay_graph_complete
    movzbl (KERNEL_LOAD_ADDRESS + activity_history - _start)(,%ecx), %edx
    testl %edx, %edx
    jz gui_overlay_graph_advance
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_overlay_graph_height - _start)
    movl %ecx, %eax
    shll $3, %eax
    addl $824, %eax
    movl $218, %ebx
    subl %edx, %ebx
    movl $5, %ecx
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_graph_index - _start), %edi
    cmpl $(WM_ACTIVITY_SAMPLES - 1), %edi
    jne gui_overlay_graph_old
    movl $GUI_COLOR_PINK, %esi
    jmp gui_overlay_graph_color_ready
gui_overlay_graph_old:
    movl $GUI_COLOR_GREEN, %esi
gui_overlay_graph_color_ready:
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_graph_height - _start), %edx
    call gui_fill_rectangle
gui_overlay_graph_advance:
    incl (KERNEL_LOAD_ADDRESS + gui_overlay_graph_index - _start)
    jmp gui_overlay_graph_next
gui_overlay_graph_complete:
    popal
    ret

wm_count_application_states:
    pushal
    movl $0, (KERNEL_LOAD_ADDRESS + gui_overlay_open_apps - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_overlay_minimized_apps - _start)
    xorl %ecx, %ecx
wm_count_apps_next:
    cmpl $WM_COUNT, %ecx
    jae wm_count_apps_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%ecx), %edx
    testb $WM_FLAG_VISIBLE, %dl
    jz wm_count_apps_advance
    testb $WM_FLAG_MINIMIZED, %dl
    jz wm_count_app_open
    incl (KERNEL_LOAD_ADDRESS + gui_overlay_minimized_apps - _start)
    jmp wm_count_apps_advance
wm_count_app_open:
    incl (KERNEL_LOAD_ADDRESS + gui_overlay_open_apps - _start)
wm_count_apps_advance:
    incl %ecx
    jmp wm_count_apps_next
wm_count_apps_complete:
    popal
    ret

/* EDI=window identifier, EDX=screen row. */
wm_render_application_state:
    pushal
    movl %edi, (KERNEL_LOAD_ADDRESS + gui_overlay_app_id - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_overlay_app_y - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_task_title_table - _start)(,%edi,4), %esi
    movl $824, %eax
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl (KERNEL_LOAD_ADDRESS + gui_overlay_app_id - _start), %edi
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%edi), %eax
    testb $WM_FLAG_VISIBLE, %al
    jz wm_overlay_app_closed
    testb $WM_FLAG_MINIMIZED, %al
    jnz wm_overlay_app_minimized
    movzbl (KERNEL_LOAD_ADDRESS + wm_active - _start), %eax
    cmpl %edi, %eax
    jne wm_overlay_app_open
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_app_active - _start), %esi
    movl $GUI_COLOR_PINK, %ecx
    jmp wm_overlay_app_status_ready
wm_overlay_app_open:
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_app_open - _start), %esi
    movl $GUI_COLOR_GREEN, %ecx
    jmp wm_overlay_app_status_ready
wm_overlay_app_minimized:
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_app_minimized - _start), %esi
    movl $GUI_COLOR_PINK, %ecx
    jmp wm_overlay_app_status_ready
wm_overlay_app_closed:
    movl $(KERNEL_LOAD_ADDRESS + gui_overlay_app_closed - _start), %esi
    movl $GUI_COLOR_DIM, %ecx
wm_overlay_app_status_ready:
    movl $912, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_overlay_app_y - _start), %edx
    call gui_print_at
    popal
    ret

gui_overlay_collect:
    xorl %edi, %edi
    movl $2, %ecx
gui_overlay_cluster_next:
    movl %ecx, %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    testl %eax, %eax
    jnz gui_overlay_cluster_used
    incl %edi
gui_overlay_cluster_used:
    incl %ecx
    movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax
    addl $2, %eax
    cmpl %eax, %ecx
    jb gui_overlay_cluster_next
    movl %edi, %eax
    shrl $1, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_overlay_free_kib - _start)
    movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax
    shrl $1, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_overlay_total_kib - _start)

    xorl %ebx, %ebx
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
gui_overlay_file_next:
    cmpb $0, (%edi)
    je gui_overlay_files_complete
    cmpb $0xe5, (%edi)
    je gui_overlay_file_skip
    cmpb $0x0f, 11(%edi)
    je gui_overlay_file_skip
    testb $0x18, 11(%edi)
    jnz gui_overlay_file_skip
    incl %ebx
gui_overlay_file_skip:
    addl $32, %edi
    decl %ecx
    jnz gui_overlay_file_next
gui_overlay_files_complete:
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_overlay_file_count - _start)
    ret

wm_render_all_windows:
    movl $0, (KERNEL_LOAD_ADDRESS + wm_render_index - _start)
wm_render_window_next:
    movl (KERNEL_LOAD_ADDRESS + wm_render_index - _start), %eax
    cmpl $WM_COUNT, %eax
    jae wm_render_windows_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_z_order - _start)(,%eax), %eax
    call wm_render_window
    incl (KERNEL_LOAD_ADDRESS + wm_render_index - _start)
    jmp wm_render_window_next
wm_render_windows_complete:
    ret

/* EAX=window identifier. */
wm_render_window:
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_current - _start)
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    testb $WM_FLAG_VISIBLE, %dl
    jz wm_render_window_complete
    testb $WM_FLAG_MINIMIZED, %dl
    jnz wm_render_window_complete
    movl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + wm_current_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + wm_current_y - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + wm_current_w - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + wm_current_h - _start)

    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl $6, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $6, %ebx
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %edx
    movl $0x00020103, %esi
    call gui_fill_rectangle

    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_bevel_box

    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $32, %ebx
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %ecx
    subl $8, %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %edx
    subl $36, %edx
    movl (KERNEL_LOAD_ADDRESS + wm_current - _start), %edi
    cmpl $WM_TERMINAL, %edi
    je wm_window_terminal_client
    movl $GUI_COLOR_PANEL, %esi
    jmp wm_window_client_ready
wm_window_terminal_client:
    xorl %esi, %esi
wm_window_client_ready:
    call gui_fill_rectangle

    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $4, %ebx
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %ecx
    subl $8, %ecx
    movl $24, %edx
    movzbl (KERNEL_LOAD_ADDRESS + wm_active - _start), %edi
    cmpl (KERNEL_LOAD_ADDRESS + wm_current - _start), %edi
    jne wm_window_inactive_title
    movl $GUI_COLOR_ACTIVE, %esi
    jmp wm_window_title_color_ready
wm_window_inactive_title:
    movl $GUI_COLOR_DARK, %esi
wm_window_title_color_ready:
    call gui_fill_rectangle

    /* A narrow focus line gives the active frame a clear Fluxbox-like edge
     * without adding a compositor, gradients, or bitmap furniture. */
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $28, %ebx
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %ecx
    subl $8, %ecx
    movl $2, %edx
    movzbl (KERNEL_LOAD_ADDRESS + wm_active - _start), %edi
    cmpl (KERNEL_LOAD_ADDRESS + wm_current - _start), %edi
    jne wm_window_inactive_accent
    movl $GUI_COLOR_PINK, %esi
    jmp wm_window_accent_ready
wm_window_inactive_accent:
    movl $GUI_COLOR_DARK, %esi
wm_window_accent_ready:
    call gui_fill_rectangle

    movl (KERNEL_LOAD_ADDRESS + wm_current - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_title_table - _start)(,%eax,4), %esi
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl $12, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %edx
    addl $8, %edx
    movzbl (KERNEL_LOAD_ADDRESS + wm_active - _start), %edi
    cmpl (KERNEL_LOAD_ADDRESS + wm_current - _start), %edi
    jne wm_window_inactive_text
    movl $GUI_COLOR_TEXT, %ecx
    jmp wm_window_text_ready
wm_window_inactive_text:
    movl $GUI_COLOR_DIM, %ecx
wm_window_text_ready:
    call gui_print_at
    call wm_render_title_buttons
    call wm_enable_client_clip

    movl (KERNEL_LOAD_ADDRESS + wm_current - _start), %eax
    cmpl $WM_TERMINAL, %eax
    je wm_render_terminal_content
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %ebx
    subl $WM_REFERENCE_X, %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    subl $WM_REFERENCE_Y, %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start)
    cmpl $WM_EDITOR, %eax
    je wm_render_editor_content
    cmpl $WM_FILES, %eax
    je wm_render_files_content
    cmpl $WM_TRAITS, %eax
    je wm_render_traits_content
    call gui_render_home
    jmp wm_render_content_complete
wm_render_files_content:
    call gui_render_file_browser
    jmp wm_render_content_complete
wm_render_traits_content:
    call gui_render_traits
    jmp wm_render_content_complete
wm_render_editor_content:
    call gui_render_editor
    jmp wm_render_content_complete
wm_render_terminal_content:
    call wm_sync_terminal_bounds
    call render_terminal_buffer
wm_render_content_complete:
    movl $0, (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + gui_clip_enabled - _start)
    call wm_render_resize_grip
wm_render_window_complete:
    ret

wm_enable_client_clip:
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl $8, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_clip_left - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %eax
    addl $36, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_clip_top - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $8, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_clip_right - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %eax
    subl $8, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_clip_bottom - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + gui_clip_enabled - _start)
    ret

/* Three compact diagonal steps communicate the classic bottom-right resize
 * affordance without importing a cursor theme or bitmap widget set. */
wm_render_resize_grip:
    movl (KERNEL_LOAD_ADDRESS + wm_current - _start), %eax
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    testb $WM_FLAG_MAXIMIZED, %dl
    jnz wm_resize_grip_complete
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $18, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %ebx
    subl $6, %ebx
    movl $12, %ecx
    movl $2, %edx
    movl $GUI_COLOR_LIGHT, %esi
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $14, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %ebx
    subl $10, %ebx
    movl $8, %ecx
    movl $2, %edx
    movl $GUI_COLOR_LIGHT, %esi
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $10, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %ebx
    subl $14, %ebx
    movl $4, %ecx
    movl $2, %edx
    movl $GUI_COLOR_LIGHT, %esi
    call gui_fill_rectangle
wm_resize_grip_complete:
    ret

wm_render_title_buttons:
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $76, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $6, %ebx
    movl $20, %ecx
    movl $(KERNEL_LOAD_ADDRESS + wm_minimize_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_small_button
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $52, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $6, %ebx
    movl $20, %ecx
    movl $(KERNEL_LOAD_ADDRESS + wm_maximize_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_small_button
    movl (KERNEL_LOAD_ADDRESS + wm_current_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $28, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_y - _start), %ebx
    addl $6, %ebx
    movl $20, %ecx
    movl $(KERNEL_LOAD_ADDRESS + wm_close_label - _start), %edx
    movl $GUI_COLOR_RED, %esi
    call gui_draw_small_button
    ret

wm_render_taskbar:
    movl $0, %eax
    movl $WM_TASKBAR_Y, %ebx
    movl $1024, %ecx
    movl $WM_TASKBAR_HEIGHT, %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_bevel_box

    movl $(KERNEL_LOAD_ADDRESS + gui_taskbar_label - _start), %esi
    movl $16, %eax
    movl $708, %edx
    movl $GUI_COLOR_LILAC, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_taskbar_version - _start), %esi
    movl $888, %eax
    movl $708, %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + rtc_display_text - _start), %esi
    movl $872, %eax
    movl $740, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl $0, (KERNEL_LOAD_ADDRESS + wm_taskbar_count - _start)
    movl $0xffffffff, (KERNEL_LOAD_ADDRESS + wm_taskbar_slots - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + wm_taskbar_scan - _start)
wm_taskbar_scan_next:
    movl (KERNEL_LOAD_ADDRESS + wm_taskbar_scan - _start), %edi
    cmpl $WM_COUNT, %edi
    jae wm_taskbar_scan_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%edi), %eax
    testb $WM_FLAG_VISIBLE, %al
    jz wm_taskbar_scan_advance
    testb $WM_FLAG_MINIMIZED, %al
    jz wm_taskbar_scan_advance

    movl (KERNEL_LOAD_ADDRESS + wm_taskbar_count - _start), %ecx
    movl %edi, %eax
    movb %al, (KERNEL_LOAD_ADDRESS + wm_taskbar_slots - _start)(,%ecx)
    movl %ecx, %eax
    imull $WM_TASK_BUTTON_STRIDE, %eax
    addl $WM_TASK_BUTTON_X, %eax
    movl $WM_TASK_BUTTON_Y, %ebx
    movl $WM_TASK_BUTTON_WIDTH, %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_task_title_table - _start)(,%edi,4), %edx
    movl $GUI_COLOR_ACTIVE, %esi
    call gui_draw_classic_button
    incl (KERNEL_LOAD_ADDRESS + wm_taskbar_count - _start)
wm_taskbar_scan_advance:
    incl (KERNEL_LOAD_ADDRESS + wm_taskbar_scan - _start)
    jmp wm_taskbar_scan_next
wm_taskbar_scan_complete:
    cmpl $0, (KERNEL_LOAD_ADDRESS + wm_taskbar_count - _start)
    jne wm_taskbar_complete
    movl $(KERNEL_LOAD_ADDRESS + gui_taskbar_empty - _start), %esi
    movl $16, %eax
    movl $740, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
wm_taskbar_complete:
    ret

wm_render_root_menu:
    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $6, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $6, %ebx
    movl $WM_ROOT_MENU_WIDTH, %ecx
    movl $WM_ROOT_MENU_HEIGHT, %edx
    movl $0x00020103, %esi
    call gui_fill_rectangle

    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    movl $WM_ROOT_MENU_WIDTH, %ecx
    movl $WM_ROOT_MENU_HEIGHT, %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_bevel_box

    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $4, %ebx
    movl $224, %ecx
    movl $24, %edx
    movl $GUI_COLOR_ACTIVE, %esi
    call gui_fill_rectangle
    movl $(KERNEL_LOAD_ADDRESS + wm_root_menu_title - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $12, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %edx
    addl $8, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $32, %ebx
    movl $224, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_home_label - _start), %edx
    movl $WM_HOME, %edi
    call gui_draw_menu_item
    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $60, %ebx
    movl $224, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_files_label - _start), %edx
    movl $WM_FILES, %edi
    call gui_draw_menu_item
    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $88, %ebx
    movl $224, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_traits_label - _start), %edx
    movl $WM_TRAITS, %edi
    call gui_draw_menu_item
    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $116, %ebx
    movl $224, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_terminal_label - _start), %edx
    movl $WM_TERMINAL, %edi
    call gui_draw_menu_item
    movl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    addl $4, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %ebx
    addl $144, %ebx
    movl $224, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_label - _start), %edx
    movl $WM_EDITOR, %edi
    call gui_draw_menu_item
    ret

gui_render_home:
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    shrl $1, %eax
    addl $WM_REFERENCE_X, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_home_ring_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    addl $WM_REFERENCE_X, %eax
    subl $208, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_home_system_x - _start)
    movl %eax, %ecx
    addl $80, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_home_memory_x - _start)
    addl $48, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_home_mib_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %eax
    addl $WM_REFERENCE_Y, %eax
    subl $32, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_home_hint_y - _start)
    movl $(KERNEL_LOAD_ADDRESS + gui_home_heading - _start), %esi
    movl $280, %eax
    movl $104, %edx
    movl $GUI_COLOR_LILAC, %ecx
    call gui_print_at

    movl (KERNEL_LOAD_ADDRESS + gui_home_ring_x - _start), %eax
    movl $292, %ebx
    movl $112, %ecx
    movl $GUI_COLOR_PINK, %edx
    call gui_draw_circle
    movl (KERNEL_LOAD_ADDRESS + gui_home_ring_x - _start), %eax
    movl $292, %ebx
    movl $106, %ecx
    movl $GUI_COLOR_ACTIVE, %edx
    call gui_draw_circle
    movl (KERNEL_LOAD_ADDRESS + gui_home_ring_x - _start), %eax
    movl $292, %ebx
    movl $98, %ecx
    movl $GUI_COLOR_LILAC, %edx
    call gui_draw_circle

    movl $(KERNEL_LOAD_ADDRESS + gui_ring_name - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_ring_x - _start), %eax
    subl $28, %eax
    movl $278, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_ring_state - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_ring_x - _start), %eax
    subl $64, %eax
    movl $310, %edx
    movl $GUI_COLOR_GREEN, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_system_heading - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_system_x - _start), %eax
    movl $152, %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_version_status - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_system_x - _start), %eax
    movl $192, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_storage_status - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_system_x - _start), %eax
    movl $224, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_memory_status - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_system_x - _start), %eax
    movl $256, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl BOOT_MEMORY_MIB(%ebp), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_home_memory_x - _start), %ebx
    movl $256, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_mib_short - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_mib_x - _start), %eax
    movl $256, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_network_status - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_home_system_x - _start), %eax
    movl $288, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl $(KERNEL_LOAD_ADDRESS + gui_home_hint - _start), %esi
    movl $416, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_home_hint_y - _start), %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    ret

gui_render_traits:
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $432, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_trait_bar_width - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %eax
    addl $WM_REFERENCE_Y, %eax
    subl $80, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_trait_note_y - _start)
    movl $(KERNEL_LOAD_ADDRESS + gui_traits_heading - _start), %esi
    movl $280, %eax
    movl $104, %edx
    movl $GUI_COLOR_LILAC, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_interactions_label - _start), %esi
    movl $296, %eax
    movl $144, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 5 - _start), %eax
    movl $440, %ebx
    movl $144, %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_number_at

    movl $(KERNEL_LOAD_ADDRESS + gui_warmth_label - _start), %esi
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 6 - _start), %eax
    movl $200, %ebx
    call gui_draw_trait_row
    movl $(KERNEL_LOAD_ADDRESS + gui_cheek_label - _start), %esi
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 7 - _start), %eax
    movl $264, %ebx
    call gui_draw_trait_row
    movl $(KERNEL_LOAD_ADDRESS + gui_curiosity_label - _start), %esi
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 8 - _start), %eax
    movl $328, %ebx
    call gui_draw_trait_row

    movl $(KERNEL_LOAD_ADDRESS + gui_traits_note - _start), %esi
    movl $296, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_trait_note_y - _start), %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    ret

gui_draw_trait_row:
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_trait_value - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_trait_y - _start)
    movl $296, %eax
    movl %ebx, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_trait_value - _start), %eax
    movl $448, %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_trait_y - _start), %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_percent_label - _start), %esi
    movl $480, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_trait_y - _start), %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    movl $520, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_trait_y - _start), %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_trait_bar_width - _start), %ecx
    movl $16, %edx
    movl $GUI_COLOR_PANEL_ALT, %esi
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_trait_value - _start), %eax
    testl %eax, %eax
    jz gui_trait_complete
    imull (KERNEL_LOAD_ADDRESS + gui_trait_bar_width - _start), %eax
    xorl %edx, %edx
    movl $100, %ecx
    divl %ecx
    movl %eax, %ecx
    movl $520, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_trait_y - _start), %ebx
    movl $16, %edx
    movl $GUI_COLOR_PINK, %esi
    call gui_fill_rectangle
gui_trait_complete:
    ret

/* Native resize-aware document workspace.  The viewport is derived from the
 * current client rectangle on every repaint, so neither stale pixels nor
 * terminal geometry can leak into a resized Writer window. */
gui_render_editor:
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $64, %eax
    shrl $3, %eax
    cmpl $100, %eax
    jbe gui_editor_columns_ready
    movl $100, %eax
gui_editor_columns_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %eax
    subl $176, %eax
    shrl $4, %eax
    cmpl $24, %eax
    jbe gui_editor_rows_ready
    movl $24, %eax
gui_editor_rows_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start)

    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    call calculate_text_position
    movl (KERNEL_LOAD_ADDRESS + text_position_column - _start), %eax
    decl %eax
    xorl %edx, %edx
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start), %eax
    jb gui_editor_horizontal_ready
    movl (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start), %edx
    decl %edx
    subl %edx, %eax
    movl %eax, %edx
gui_editor_horizontal_ready:
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_editor_horizontal_offset - _start)
    movl (KERNEL_LOAD_ADDRESS + text_position_line - _start), %eax
    movl $1, %edx
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start), %eax
    jbe gui_editor_first_line_ready
    subl (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start), %eax
    incl %eax
    movl %eax, %edx
gui_editor_first_line_ready:
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_editor_first_line - _start)

    movl $(KERNEL_LOAD_ADDRESS + gui_editor_file_label - _start), %esi
    movl $192, %eax
    movl $88, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + editor_filename - _start), %esi
    movl $240, %eax
    movl $88, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_bytes_label - _start), %esi
    movl $448, %eax
    movl $88, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    movl $504, %ebx
    movl $88, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at

    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_name_mode - _start)
    je gui_editor_help_line
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_save_as_prompt - _start), %esi
    movl $192, %eax
    movl $112, %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + editor_save_as_name - _start), %esi
    movl $344, %eax
    movl $112, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    jmp gui_editor_header_complete
gui_editor_help_line:
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_help - _start), %esi
    movl $192, %eax
    movl $112, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
gui_editor_header_complete:

    movl $184, %eax
    movl $136, %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start), %ecx
    shll $3, %ecx
    addl $16, %ecx
    movl (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start), %edx
    shll $4, %edx
    addl $16, %edx
    movl $GUI_COLOR_PANEL_ALT, %esi
    call gui_fill_rectangle

    /* Locate the first visible logical line. */
    xorl %eax, %eax
    movl $1, %edx
gui_editor_seek_line:
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    jae gui_editor_seek_complete
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_first_line - _start), %edx
    jae gui_editor_seek_complete
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    jne gui_editor_seek_advance
    incl %edx
gui_editor_seek_advance:
    incl %eax
    jmp gui_editor_seek_line
gui_editor_seek_complete:
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_editor_render_index - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_editor_render_row - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_editor_render_column - _start)
gui_editor_text_next:
    movl (KERNEL_LOAD_ADDRESS + gui_editor_render_index - _start), %eax
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    jae gui_editor_text_complete
    movl (KERNEL_LOAD_ADDRESS + gui_editor_render_row - _start), %edx
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start), %edx
    jae gui_editor_text_complete
    movb EDITOR_BUFFER_ADDRESS(,%eax), %bl
    cmpb $'\n', %bl
    je gui_editor_text_newline
    movl (KERNEL_LOAD_ADDRESS + gui_editor_render_column - _start), %ecx
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_horizontal_offset - _start), %ecx
    jb gui_editor_text_advance_column
    subl (KERNEL_LOAD_ADDRESS + gui_editor_horizontal_offset - _start), %ecx
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start), %ecx
    jae gui_editor_text_advance_column
    movb %bl, (KERNEL_LOAD_ADDRESS + gui_editor_character - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + gui_editor_character + 1 - _start)
    shll $3, %ecx
    addl $192, %ecx
    movl %ecx, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_editor_render_row - _start), %edx
    shll $4, %edx
    addl $144, %edx
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_character - _start), %esi
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
gui_editor_text_advance_column:
    incl (KERNEL_LOAD_ADDRESS + gui_editor_render_column - _start)
    jmp gui_editor_text_advance
gui_editor_text_newline:
    incl (KERNEL_LOAD_ADDRESS + gui_editor_render_row - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_editor_render_column - _start)
gui_editor_text_advance:
    incl (KERNEL_LOAD_ADDRESS + gui_editor_render_index - _start)
    jmp gui_editor_text_next
gui_editor_text_complete:

    /* Caret position follows logical text rather than the last glyph drawn. */
    movl (KERNEL_LOAD_ADDRESS + text_position_line - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + gui_editor_first_line - _start), %edx
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start), %edx
    jae gui_editor_caret_complete
    movl (KERNEL_LOAD_ADDRESS + text_position_column - _start), %eax
    decl %eax
    subl (KERNEL_LOAD_ADDRESS + gui_editor_horizontal_offset - _start), %eax
    js gui_editor_caret_complete
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start), %eax
    jae gui_editor_caret_complete
    shll $3, %eax
    addl $192, %eax
    shll $4, %edx
    addl $144, %edx
    movl %edx, %ebx
    movl $2, %ecx
    movl $16, %edx
    movl $GUI_COLOR_PINK, %esi
    call gui_fill_rectangle
gui_editor_caret_complete:

    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %ebx
    addl $(WM_REFERENCE_Y - 48), %ebx
    movl $184, %eax
    movl $104, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_save_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %ebx
    addl $(WM_REFERENCE_Y - 48), %ebx
    movl $304, %eax
    movl $128, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_save_as_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    addl $(WM_REFERENCE_X - 128), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %ebx
    addl $(WM_REFERENCE_Y - 48), %ebx
    movl $112, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_close_label - _start), %edx
    movl $GUI_COLOR_RED, %esi
    call gui_draw_classic_button

    movzbl (KERNEL_LOAD_ADDRESS + editor_save_status - _start), %eax
    testl %eax, %eax
    jnz gui_editor_choose_status
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
    je gui_editor_render_complete
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_modified - _start), %esi
    jmp gui_editor_draw_status
gui_editor_choose_status:
    decl %eax
    cmpl $5, %eax
    ja gui_editor_render_complete
    movl (KERNEL_LOAD_ADDRESS + gui_editor_status_table - _start)(,%eax,4), %esi
gui_editor_draw_status:
    movl $448, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %edx
    addl $(WM_REFERENCE_Y - 36), %edx
    movl $GUI_COLOR_PINK, %ecx
    call gui_print_at
gui_editor_render_complete:
    ret

gui_render_file_browser:
    call gui_prepare_file_layout
    call gui_build_file_page
    movl $(KERNEL_LOAD_ADDRESS + gui_files_heading - _start), %esi
    movl $184, %eax
    movl $92, %edx
    movl $GUI_COLOR_LILAC, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_files_edition - _start), %esi
    movl $288, %eax
    movl $92, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_file_count_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_file_count_label_x - _start), %eax
    movl $92, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_total_files - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_count_x - _start), %ebx
    movl $92, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at

    movl $184, %eax
    movl $116, %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_file_table_width - _start), %ecx
    movl $40, %edx
    movl $GUI_COLOR_PANEL_ALT, %esi
    call gui_draw_bevel_box
    movl $(KERNEL_LOAD_ADDRESS + gui_location_label - _start), %esi
    movl $196, %eax
    movl $128, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_location_root - _start), %esi
    movl $280, %eax
    movl $128, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    movl (KERNEL_LOAD_ADDRESS + gui_file_previous_x - _start), %eax
    movl $116, %ebx
    movl $40, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_previous_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button
    movl (KERNEL_LOAD_ADDRESS + gui_file_next_x - _start), %eax
    movl $116, %ebx
    movl $40, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_next_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button

    movl $184, %eax
    movl $164, %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_file_table_width - _start), %ecx
    movl $24, %edx
    movl $GUI_COLOR_ACTIVE, %esi
    call gui_fill_rectangle
    movl $(KERNEL_LOAD_ADDRESS + gui_name_label - _start), %esi
    movl $216, %eax
    movl $168, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_type_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_file_type_x - _start), %eax
    movl $168, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl $(KERNEL_LOAD_ADDRESS + gui_size_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_file_size_x - _start), %eax
    movl $168, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at

    xorl %ebx, %ebx
gui_file_row_next:
    movzbl (KERNEL_LOAD_ADDRESS + gui_visible_files - _start), %eax
    cmpl %eax, %ebx
    jae gui_file_rows_complete
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_render_index - _start)
    movl %ebx, %eax
    imull $24, %eax
    addl $192, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_row_y - _start)
    movl $GUI_COLOR_PANEL, %esi
    testl $1, %ebx
    jz gui_file_row_face_ready
    movl $GUI_COLOR_PANEL_ALT, %esi
gui_file_row_face_ready:
    movzbl (KERNEL_LOAD_ADDRESS + gui_selected_index - _start), %eax
    cmpl %eax, %ebx
    jne gui_file_row_fill
    movl $GUI_COLOR_ACTIVE, %esi
gui_file_row_fill:
    movl $184, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_row_y - _start), %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_file_table_width - _start), %ecx
    movl $20, %edx
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_render_index - _start), %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_file_entries - _start)(,%ebx,4), %edi
    call gui_file_type_for_entry
    movl %esi, (KERNEL_LOAD_ADDRESS + gui_file_type_label - _start)
    movl (KERNEL_LOAD_ADDRESS + gui_file_type_color - _start), %esi
    movl $196, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_row_y - _start), %ebx
    addl $2, %ebx
    movl $8, %ecx
    movl $16, %edx
    call gui_fill_rectangle
    call gui_copy_render_name
    movl $(KERNEL_LOAD_ADDRESS + gui_render_name - _start), %esi
    movl $216, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_row_y - _start), %edx
    addl $2, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_file_type_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_file_type_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_row_y - _start), %edx
    addl $2, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl 28(%edi), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_size_x - _start), %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_row_y - _start), %edx
    addl $2, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_bytes_short - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_file_bytes_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_row_y - _start), %edx
    addl $2, %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_render_index - _start), %ebx
    incl %ebx
    jmp gui_file_row_next
gui_file_rows_complete:
    movl $(KERNEL_LOAD_ADDRESS + gui_file_status - _start), %esi
    movl $184, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_status_y - _start), %edx
    movl $GUI_COLOR_GREEN, %ecx
    call gui_print_at
    movl (KERNEL_LOAD_ADDRESS + gui_total_files - _start), %eax
    movl $320, %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_file_status_y - _start), %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_number_at
    movl $(KERNEL_LOAD_ADDRESS + gui_items_label - _start), %esi
    movl $352, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_status_y - _start), %edx
    movl $GUI_COLOR_DIM, %ecx
    call gui_print_at

    movl $184, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %ebx
    movl $104, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_open_label - _start), %edx
    movl $GUI_COLOR_ACTIVE, %esi
    call gui_draw_classic_button
    movl $304, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %ebx
    movl $96, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_edit_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button
    movl $416, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %ebx
    movl $96, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_copy_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button
    movl $528, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %ebx
    movl $112, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_rename_label - _start), %edx
    movl $GUI_COLOR_CLASSIC, %esi
    call gui_draw_classic_button
    movl $656, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %ebx
    movl $112, %ecx
    movl $(KERNEL_LOAD_ADDRESS + gui_delete_label - _start), %edx
    movl $GUI_COLOR_RED, %esi
    call gui_draw_classic_button
    ret

/* FileHound keeps its table, paging controls, status, and actions inside a
 * resized client. Coordinates remain in the established reference space so
 * existing moved-window input translation stays valid. */
gui_prepare_file_layout:
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    addl $WM_REFERENCE_X, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_file_layout_right - _start)
    movl %eax, %ecx
    subl $176, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_count_label_x - _start)
    movl %eax, %ecx
    subl $120, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_count_x - _start)
    movl %eax, %ecx
    subl $112, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_previous_x - _start)
    movl %eax, %ecx
    subl $56, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_next_x - _start)
    movl %eax, %ecx
    subl $376, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_type_x - _start)
    movl %eax, %ecx
    subl $208, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_size_x - _start)
    movl %eax, %ecx
    subl $144, %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_file_bytes_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_current_w - _start), %eax
    subl $32, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_file_table_width - _start)

    movl (KERNEL_LOAD_ADDRESS + wm_current_h - _start), %eax
    addl $WM_REFERENCE_Y, %eax
    subl $56, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start)
    subl $16, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_file_status_y - _start)
    subl $192, %eax
    xorl %edx, %edx
    movl $24, %ecx
    divl %ecx
    testl %eax, %eax
    jnz gui_file_capacity_nonzero
    movl $1, %eax
gui_file_capacity_nonzero:
    cmpl $GUI_FILE_ROWS, %eax
    jbe gui_file_capacity_ready
    movl $GUI_FILE_ROWS, %eax
gui_file_capacity_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_file_row_capacity - _start)
    ret

/* EDI=root entry. Return a compact FileHound type label in ESI and retain a
 * category accent for the native file marker. */
gui_file_type_for_entry:
    movl $(KERNEL_LOAD_ADDRESS + gui_type_file - _start), %esi
    movl $GUI_COLOR_DIM, (KERNEL_LOAD_ADDRESS + gui_file_type_color - _start)
    cmpb $'T', 8(%edi)
    jne gui_file_type_test_memory
    cmpb $'X', 9(%edi)
    jne gui_file_type_test_memory
    cmpb $'T', 10(%edi)
    jne gui_file_type_test_memory
    movl $(KERNEL_LOAD_ADDRESS + gui_type_text - _start), %esi
    movl $GUI_COLOR_PINK, (KERNEL_LOAD_ADDRESS + gui_file_type_color - _start)
    ret
gui_file_type_test_memory:
    cmpb $'M', 8(%edi)
    jne gui_file_type_complete
    cmpb $'E', 9(%edi)
    jne gui_file_type_complete
    cmpb $'M', 10(%edi)
    jne gui_file_type_complete
    movl $(KERNEL_LOAD_ADDRESS + gui_type_data - _start), %esi
    movl $GUI_COLOR_GREEN, (KERNEL_LOAD_ADDRESS + gui_file_type_color - _start)
gui_file_type_complete:
    ret

gui_build_file_page:
    movl $0, (KERNEL_LOAD_ADDRESS + gui_total_files - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + gui_visible_files - _start)
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
gui_build_entry_next:
    cmpb $0, (%edi)
    je gui_build_complete
    cmpb $0xe5, (%edi)
    je gui_build_entry_skip
    cmpb $0x0f, 11(%edi)
    je gui_build_entry_skip
    testb $0x18, 11(%edi)
    jnz gui_build_entry_skip
    movl (KERNEL_LOAD_ADDRESS + gui_total_files - _start), %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_page_offset - _start), %eax
    jb gui_build_count_entry
    movzbl (KERNEL_LOAD_ADDRESS + gui_visible_files - _start), %ebx
    cmpl (KERNEL_LOAD_ADDRESS + gui_file_row_capacity - _start), %ebx
    jae gui_build_count_entry
    movl %edi, (KERNEL_LOAD_ADDRESS + gui_file_entries - _start)(,%ebx,4)
    incl %ebx
    movb %bl, (KERNEL_LOAD_ADDRESS + gui_visible_files - _start)
gui_build_count_entry:
    incl (KERNEL_LOAD_ADDRESS + gui_total_files - _start)
gui_build_entry_skip:
    addl $32, %edi
    decl %ecx
    jnz gui_build_entry_next
gui_build_complete:
    movzbl (KERNEL_LOAD_ADDRESS + gui_visible_files - _start), %eax
    testl %eax, %eax
    jnz gui_build_has_files
    movb $0xff, (KERNEL_LOAD_ADDRESS + gui_selected_index - _start)
    ret
gui_build_has_files:
    movzbl (KERNEL_LOAD_ADDRESS + gui_selected_index - _start), %edx
    cmpl %eax, %edx
    jb gui_build_selection_valid
    movb $0, (KERNEL_LOAD_ADDRESS + gui_selected_index - _start)
gui_build_selection_valid:
    ret

/* Right-click root binding: place the application menu at the pointer. */
gui_handle_root_click:
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    cmpl $WM_TASKBAR_Y, %eax
    jae wm_root_click_complete
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %eax
    cmpl $(1024 - WM_ROOT_MENU_WIDTH), %eax
    jbe wm_root_menu_x_ready
    movl $(1024 - WM_ROOT_MENU_WIDTH), %eax
wm_root_menu_x_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_menu_x - _start)
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    cmpl $(WM_TASKBAR_Y - WM_ROOT_MENU_HEIGHT), %eax
    jbe wm_root_menu_y_ready
    movl $(WM_TASKBAR_Y - WM_ROOT_MENU_HEIGHT), %eax
wm_root_menu_y_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_menu_y - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    call render_shell
wm_root_click_complete:
    ret

/* Redraw only when the pointer crosses a root-menu row boundary. */
gui_handle_pointer_motion:
    pushal
    cmpb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    je wm_pointer_motion_complete
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    cmpl $4, %eax
    jb wm_pointer_not_on_menu
    cmpl $228, %eax
    jae wm_pointer_not_on_menu
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %eax
    cmpl $32, %eax
    jb wm_pointer_not_on_menu
    cmpl $172, %eax
    jae wm_pointer_not_on_menu
    subl $32, %eax
    xorl %edx, %edx
    movl $WM_ROOT_MENU_ROW, %ecx
    divl %ecx
    jmp wm_pointer_hover_ready
wm_pointer_not_on_menu:
    movl $WM_NONE, %eax
wm_pointer_hover_ready:
    movzbl (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start), %edx
    cmpl %edx, %eax
    je wm_pointer_motion_complete
    movb %al, (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start)
    call render_shell
wm_pointer_motion_complete:
    popal
    ret

gui_handle_click:
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start)
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_pointer_y - _start)
    cmpl $WM_TASKBAR_Y, %eax
    jae wm_click_taskbar
    cmpb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    jne wm_click_root_menu
    jmp wm_find_window

wm_click_taskbar:
    cmpl $WM_TASK_BUTTON_Y, %eax
    jb gui_click_complete
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start), %eax
    cmpl $WM_TASK_BUTTON_X, %eax
    jb gui_click_complete
    subl $WM_TASK_BUTTON_X, %eax
    xorl %edx, %edx
    movl $WM_TASK_BUTTON_STRIDE, %ecx
    divl %ecx
    cmpl $WM_TASK_BUTTON_WIDTH, %edx
    jae gui_click_complete
    cmpl (KERNEL_LOAD_ADDRESS + wm_taskbar_count - _start), %eax
    jae gui_click_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_taskbar_slots - _start)(,%eax), %eax
    cmpl $WM_COUNT, %eax
    jae gui_click_complete
    jmp wm_open_selected

wm_click_root_menu:
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + wm_menu_x - _start), %eax
    cmpl $4, %eax
    jb wm_close_root_menu
    cmpl $228, %eax
    jae wm_close_root_menu
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + wm_menu_y - _start), %edx
    cmpl $32, %edx
    jb wm_close_root_menu
    cmpl $60, %edx
    jb wm_open_home
    cmpl $88, %edx
    jb wm_open_files
    cmpl $116, %edx
    jb wm_open_traits
    cmpl $144, %edx
    jb wm_open_terminal
    cmpl $172, %edx
    jb wm_open_editor
wm_close_root_menu:
    movb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start)
    call render_shell
    ret

wm_find_window:
    movl $(WM_COUNT - 1), (KERNEL_LOAD_ADDRESS + wm_hit_index - _start)
wm_find_window_next:
    movl (KERNEL_LOAD_ADDRESS + wm_hit_index - _start), %eax
    testl %eax, %eax
    js wm_focus_root
    movzbl (KERNEL_LOAD_ADDRESS + wm_z_order - _start)(,%eax), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_hit_window - _start)
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    testb $WM_FLAG_VISIBLE, %dl
    jz wm_find_window_skip
    testb $WM_FLAG_MINIMIZED, %dl
    jnz wm_find_window_skip
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start), %edx
    cmpl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %edx
    jb wm_find_window_skip
    movl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ecx
    addl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ecx
    cmpl %ecx, %edx
    jae wm_find_window_skip
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_y - _start), %edx
    cmpl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %edx
    jb wm_find_window_skip
    movl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ecx
    addl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %ecx
    cmpl %ecx, %edx
    jb wm_window_found
wm_find_window_skip:
    decl (KERNEL_LOAD_ADDRESS + wm_hit_index - _start)
    jmp wm_find_window_next

wm_focus_root:
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    call render_shell
    ret

wm_window_found:
    movl (KERNEL_LOAD_ADDRESS + wm_hit_window - _start), %eax
    call wm_bring_to_front
    movl (KERNEL_LOAD_ADDRESS + wm_hit_window - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start), %ebx
    subl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + wm_local_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_y - _start), %ebx
    subl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ebx
    movl %ebx, (KERNEL_LOAD_ADDRESS + wm_local_y - _start)
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    testb $WM_FLAG_MAXIMIZED, %dl
    jnz wm_window_not_resize_grip
    movl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ecx
    subl $WM_RESIZE_GRIP, %ecx
    cmpl %ecx, (KERNEL_LOAD_ADDRESS + wm_local_x - _start)
    jb wm_window_not_resize_grip
    movl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %ecx
    subl $WM_RESIZE_GRIP, %ecx
    cmpl %ecx, (KERNEL_LOAD_ADDRESS + wm_local_y - _start)
    jb wm_window_not_resize_grip
    movb %al, (KERNEL_LOAD_ADDRESS + wm_resize_window - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_drag_window - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_y - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_w - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_h - _start)
    call render_shell
    call wm_xor_drag_outline
    ret
wm_window_not_resize_grip:
    cmpl $32, %ebx
    jae wm_window_client_click
    movl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ecx
    subl $28, %ecx
    cmpl %ecx, (KERNEL_LOAD_ADDRESS + wm_local_x - _start)
    jae wm_close_window
    subl $24, %ecx
    cmpl %ecx, (KERNEL_LOAD_ADDRESS + wm_local_x - _start)
    jae wm_toggle_maximize
    subl $24, %ecx
    cmpl %ecx, (KERNEL_LOAD_ADDRESS + wm_local_x - _start)
    jae wm_minimize_window
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %ecx
    testb $WM_FLAG_MAXIMIZED, %cl
    jnz wm_window_focus_only
    movb %al, (KERNEL_LOAD_ADDRESS + wm_drag_window - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_local_x - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_drag_offset_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_local_y - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_drag_offset_y - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_y - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_w - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_h - _start)
    call render_shell
    call wm_xor_drag_outline
    ret
wm_window_focus_only:
    call render_shell
    ret

wm_window_client_click:
    call render_shell
    movl (KERNEL_LOAD_ADDRESS + wm_hit_window - _start), %eax
    cmpl $WM_EDITOR, %eax
    je gui_editor_click
    cmpl $WM_FILES, %eax
    jne gui_click_complete
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne gui_click_complete
    cmpb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    jne gui_click_complete
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + wm_x + WM_FILES * 4 - _start), %eax
    addl $WM_REFERENCE_X, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_local_x - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + wm_y + WM_FILES * 4 - _start), %edx
    addl $WM_REFERENCE_Y, %edx
    jmp gui_file_click

gui_editor_click:
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    je gui_click_complete
    movl (KERNEL_LOAD_ADDRESS + wm_local_y - _start), %edx
    movl (KERNEL_LOAD_ADDRESS + wm_h + WM_EDITOR * 4 - _start), %ecx
    subl $56, %ecx
    cmpl %ecx, %edx
    jb gui_editor_click_text
    movl (KERNEL_LOAD_ADDRESS + wm_local_x - _start), %eax
    cmpl $16, %eax
    jb gui_click_complete
    cmpl $120, %eax
    jb editor_save
    cmpl $136, %eax
    jb gui_editor_click_close
    cmpl $264, %eax
    jb gui_editor_click_save_as
gui_editor_click_close:
    movl (KERNEL_LOAD_ADDRESS + wm_w + WM_EDITOR * 4 - _start), %ecx
    subl $128, %ecx
    cmpl %ecx, %eax
    jb gui_click_complete
    call editor_exit
    ret
gui_editor_click_save_as:
    call editor_begin_save_as
    ret

gui_editor_click_text:
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_name_mode - _start)
    jne gui_click_complete
    movl (KERNEL_LOAD_ADDRESS + wm_local_x - _start), %eax
    cmpl $24, %eax
    jb gui_click_complete
    subl $24, %eax
    shrl $3, %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_columns - _start), %eax
    jae gui_click_complete
    addl (KERNEL_LOAD_ADDRESS + gui_editor_horizontal_offset - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_editor_click_column - _start)
    movl (KERNEL_LOAD_ADDRESS + wm_local_y - _start), %eax
    cmpl $96, %eax
    jb gui_click_complete
    subl $96, %eax
    shrl $4, %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_visible_rows - _start), %eax
    jae gui_click_complete
    addl (KERNEL_LOAD_ADDRESS + gui_editor_first_line - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_editor_click_line - _start)
    xorl %eax, %eax
    movl $1, %edx
gui_editor_click_seek_line:
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    jae gui_editor_click_set
    cmpl (KERNEL_LOAD_ADDRESS + gui_editor_click_line - _start), %edx
    jae gui_editor_click_seek_column
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    jne gui_editor_click_seek_advance
    incl %edx
gui_editor_click_seek_advance:
    incl %eax
    jmp gui_editor_click_seek_line
gui_editor_click_seek_column:
    movl (KERNEL_LOAD_ADDRESS + gui_editor_click_column - _start), %ecx
gui_editor_click_column_next:
    testl %ecx, %ecx
    jz gui_editor_click_set
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    jae gui_editor_click_set
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    je gui_editor_click_set
    incl %eax
    decl %ecx
    jmp gui_editor_click_column_next
gui_editor_click_set:
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + editor_close_armed - _start)
    call render_shell
    ret

gui_file_click:
    cmpl $116, %edx
    jb gui_click_test_rows
    cmpl $156, %edx
    jae gui_click_test_rows
    movl (KERNEL_LOAD_ADDRESS + gui_file_previous_x - _start), %ecx
    cmpl %ecx, %eax
    jb gui_click_test_rows
    addl $40, %ecx
    cmpl %ecx, %eax
    jb gui_click_previous
    movl (KERNEL_LOAD_ADDRESS + gui_file_next_x - _start), %ecx
    cmpl %ecx, %eax
    jb gui_click_test_rows
    addl $40, %ecx
    cmpl %ecx, %eax
    jb gui_click_next
gui_click_test_rows:
    cmpl $184, %eax
    jb gui_click_test_actions
    movl (KERNEL_LOAD_ADDRESS + gui_file_layout_right - _start), %ecx
    subl $16, %ecx
    cmpl %ecx, %eax
    jae gui_click_test_actions
    cmpl $192, %edx
    jb gui_click_test_actions
    cmpl (KERNEL_LOAD_ADDRESS + gui_file_status_y - _start), %edx
    jae gui_click_test_actions
    movl %edx, %eax
    subl $192, %eax
    xorl %edx, %edx
    movl $24, %ecx
    divl %ecx
    movzbl (KERNEL_LOAD_ADDRESS + gui_visible_files - _start), %edx
    cmpl %edx, %eax
    jae gui_click_complete
    movb %al, (KERNEL_LOAD_ADDRESS + gui_selected_index - _start)
    call render_shell
    ret
gui_click_test_actions:
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + wm_y + WM_FILES * 4 - _start), %edx
    addl $WM_REFERENCE_Y, %edx
    cmpl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %edx
    jb gui_click_complete
    movl (KERNEL_LOAD_ADDRESS + gui_file_action_y - _start), %ecx
    addl $40, %ecx
    cmpl %ecx, %edx
    jae gui_click_complete
    movl (KERNEL_LOAD_ADDRESS + wm_pointer_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + wm_x + WM_FILES * 4 - _start), %eax
    addl $WM_REFERENCE_X, %eax
    cmpl $184, %eax
    jb gui_click_complete
    cmpl $288, %eax
    jb gui_action_open
    cmpl $304, %eax
    jb gui_click_complete
    cmpl $400, %eax
    jb gui_action_edit
    cmpl $416, %eax
    jb gui_click_complete
    cmpl $512, %eax
    jb gui_action_copy
    cmpl $528, %eax
    jb gui_click_complete
    cmpl $640, %eax
    jb gui_action_rename
    cmpl $656, %eax
    jb gui_click_complete
    cmpl $768, %eax
    jb gui_action_delete
    ret

wm_open_home:
    movl $WM_HOME, %eax
    jmp wm_open_selected
wm_open_files:
    movl $0, (KERNEL_LOAD_ADDRESS + gui_page_offset - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + gui_selected_index - _start)
    movl $WM_FILES, %eax
    jmp wm_open_selected
wm_open_traits:
    movl $WM_TRAITS, %eax
    jmp wm_open_selected
wm_open_terminal:
    movl $WM_TERMINAL, %eax
    jmp wm_open_selected
wm_open_editor:
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne wm_restore_editor
    movl $(KERNEL_LOAD_ADDRESS + gui_editor_default_name - _start), %esi
    call start_editor
    ret
wm_restore_editor:
    movl $WM_EDITOR, %eax
wm_open_selected:
    movb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start)
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    orb $WM_FLAG_VISIBLE, %dl
    andb $~WM_FLAG_MINIMIZED, %dl
    movb %dl, (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax)
    call wm_bring_to_front
    cmpl $WM_TERMINAL, %eax
    jne wm_open_render
    call wm_sync_terminal_bounds
wm_open_render:
    call render_shell
    ret

wm_focus_terminal_for_input:
    cmpb $WM_TERMINAL, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    jne wm_focus_terminal_open
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags + WM_TERMINAL - _start), %eax
    testb $WM_FLAG_VISIBLE, %al
    jz wm_focus_terminal_open
    testb $WM_FLAG_MINIMIZED, %al
    jz wm_focus_terminal_complete
wm_focus_terminal_open:
    movl $WM_TERMINAL, %eax
    call wm_open_selected
wm_focus_terminal_complete:
    ret

wm_focus_editor_for_input:
    cmpb $WM_EDITOR, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    jne wm_focus_editor_open
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags + WM_EDITOR - _start), %eax
    testb $WM_FLAG_VISIBLE, %al
    jz wm_focus_editor_open
    testb $WM_FLAG_MINIMIZED, %al
    jz wm_focus_editor_complete
wm_focus_editor_open:
    movl $WM_EDITOR, %eax
    call wm_open_selected
wm_focus_editor_complete:
    ret

wm_minimize_window:
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    orb $WM_FLAG_MINIMIZED, %dl
    movb %dl, (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax)
    call wm_select_top_window
    call render_shell
    ret
wm_close_window:
    cmpl $WM_EDITOR, %eax
    jne wm_close_window_commit
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    je wm_close_window_commit
    call editor_exit
    ret
wm_close_window_commit:
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    andb $~(WM_FLAG_VISIBLE | WM_FLAG_MINIMIZED | WM_FLAG_MAXIMIZED), %dl
    movb %dl, (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax)
    call wm_select_top_window
    call render_shell
    ret
wm_toggle_maximize:
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    testb $WM_FLAG_MAXIMIZED, %dl
    jnz wm_restore_window
    movl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_restore_x - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_restore_y - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_restore_w - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_restore_h - _start)(,%eax,4)
    movl $0, (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4)
    movl $0, (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4)
    movl $1024, (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4)
    movl $WM_TASKBAR_Y, (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4)
    orb $WM_FLAG_MAXIMIZED, %dl
    jmp wm_maximize_store
wm_restore_window:
    movl (KERNEL_LOAD_ADDRESS + wm_restore_x - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_restore_y - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_restore_w - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_restore_h - _start)(,%eax,4), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4)
    andb $~WM_FLAG_MAXIMIZED, %dl
wm_maximize_store:
    movb %dl, (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax)
    cmpl $WM_TERMINAL, %eax
    jne wm_maximize_render
    call wm_reposition_terminal_cursor
    call wm_sync_terminal_bounds
wm_maximize_render:
    call render_shell
    ret

/* EAX=id. Promote the window to the end of the fixed z-order array. */
wm_bring_to_front:
    movl %eax, (KERNEL_LOAD_ADDRESS + wm_promote_window - _start)
    xorl %ecx, %ecx
wm_promote_find:
    cmpl $WM_COUNT, %ecx
    jae wm_promote_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_z_order - _start)(,%ecx), %edx
    cmpl %eax, %edx
    je wm_promote_shift
    incl %ecx
    jmp wm_promote_find
wm_promote_shift:
    cmpl $(WM_COUNT - 1), %ecx
    jae wm_promote_complete
    movzbl (KERNEL_LOAD_ADDRESS + wm_z_order + 1 - _start)(,%ecx), %edx
    movb %dl, (KERNEL_LOAD_ADDRESS + wm_z_order - _start)(,%ecx)
    incl %ecx
    jmp wm_promote_shift
wm_promote_complete:
    movl (KERNEL_LOAD_ADDRESS + wm_promote_window - _start), %eax
    movb %al, (KERNEL_LOAD_ADDRESS + wm_z_order + WM_COUNT - 1 - _start)
    movb %al, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    ret

wm_select_top_window:
    movl $(WM_COUNT - 1), %ecx
wm_select_top_next:
    testl %ecx, %ecx
    js wm_select_none
    movzbl (KERNEL_LOAD_ADDRESS + wm_z_order - _start)(,%ecx), %eax
    movzbl (KERNEL_LOAD_ADDRESS + wm_flags - _start)(,%eax), %edx
    testb $WM_FLAG_VISIBLE, %dl
    jz wm_select_top_skip
    testb $WM_FLAG_MINIMIZED, %dl
    jz wm_select_top_found
wm_select_top_skip:
    decl %ecx
    jmp wm_select_top_next
wm_select_top_found:
    movb %al, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    ret
wm_select_none:
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_active - _start)
    ret

wm_reposition_terminal_cursor:
    movl BOOT_CURSOR_X(%ebp), %ecx
    subl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %ecx
    movl BOOT_CURSOR_Y(%ebp), %edx
    subl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %edx
    call wm_sync_terminal_bounds
    movl (KERNEL_LOAD_ADDRESS + terminal_visible_columns - _start), %eax
    decl %eax
    shll $3, %eax
    cmpl %eax, %ecx
    jbe wm_terminal_cursor_x_ready
    movl %eax, %ecx
wm_terminal_cursor_x_ready:
    movl (KERNEL_LOAD_ADDRESS + terminal_visible_rows - _start), %eax
    decl %eax
    shll $4, %eax
    cmpl %eax, %edx
    jbe wm_terminal_cursor_y_ready
    movl %eax, %edx
wm_terminal_cursor_y_ready:
    addl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %ecx
    movl %ecx, BOOT_CURSOR_X(%ebp)
    addl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %edx
    movl %edx, BOOT_CURSOR_Y(%ebp)
    ret

gui_handle_drag:
    movzbl (KERNEL_LOAD_ADDRESS + wm_resize_window - _start), %eax
    cmpl $WM_NONE, %eax
    jne wm_resize_drag
    movzbl (KERNEL_LOAD_ADDRESS + wm_drag_window - _start), %eax
    cmpl $WM_NONE, %eax
    je wm_drag_complete
    call wm_xor_drag_outline
    movzbl (KERNEL_LOAD_ADDRESS + wm_drag_window - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %ecx
    subl (KERNEL_LOAD_ADDRESS + wm_drag_offset_x - _start), %ecx
    testl %ecx, %ecx
    jns wm_drag_x_nonnegative
    xorl %ecx, %ecx
wm_drag_x_nonnegative:
    movl $1024, %edx
    subl (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4), %edx
    cmpl %edx, %ecx
    jbe wm_drag_x_ready
    movl %edx, %ecx
wm_drag_x_ready:
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_x - _start)
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %ecx
    subl (KERNEL_LOAD_ADDRESS + wm_drag_offset_y - _start), %ecx
    testl %ecx, %ecx
    jns wm_drag_y_nonnegative
    xorl %ecx, %ecx
wm_drag_y_nonnegative:
    movl $WM_TASKBAR_Y, %edx
    subl (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4), %edx
    cmpl %edx, %ecx
    jbe wm_drag_y_ready
    movl %edx, %ecx
wm_drag_y_ready:
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_y - _start)
    call wm_xor_drag_outline
wm_drag_complete:
    ret

wm_resize_drag:
    call wm_xor_drag_outline
    movzbl (KERNEL_LOAD_ADDRESS + wm_resize_window - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %ecx
    subl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %ecx
    incl %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_min_w - _start)(,%eax,4), %edx
    cmpl %edx, %ecx
    jae wm_resize_width_min_ready
    movl %edx, %ecx
wm_resize_width_min_ready:
    movl $1024, %edx
    subl (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4), %edx
    cmpl %edx, %ecx
    jbe wm_resize_width_ready
    movl %edx, %ecx
wm_resize_width_ready:
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_w - _start)

    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %ecx
    subl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %ecx
    incl %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_min_h - _start)(,%eax,4), %edx
    cmpl %edx, %ecx
    jae wm_resize_height_min_ready
    movl %edx, %ecx
wm_resize_height_min_ready:
    movl $WM_TASKBAR_Y, %edx
    subl (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4), %edx
    cmpl %edx, %ecx
    jbe wm_resize_height_ready
    movl %edx, %ecx
wm_resize_height_ready:
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_preview_h - _start)
    call wm_xor_drag_outline
    ret

gui_handle_release:
    movzbl (KERNEL_LOAD_ADDRESS + wm_resize_window - _start), %eax
    cmpl $WM_NONE, %eax
    jne wm_release_resize
    movzbl (KERNEL_LOAD_ADDRESS + wm_drag_window - _start), %eax
    cmpl $WM_NONE, %eax
    je wm_release_complete
    call wm_xor_drag_outline
    movzbl (KERNEL_LOAD_ADDRESS + wm_drag_window - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_preview_x - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_x - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_preview_y - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4)
    cmpl $WM_TERMINAL, %eax
    jne wm_release_position_ready
    call wm_reposition_terminal_cursor
wm_release_position_ready:
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_drag_window - _start)
    call render_shell
wm_release_complete:
    ret

wm_release_resize:
    call wm_xor_drag_outline
    movzbl (KERNEL_LOAD_ADDRESS + wm_resize_window - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_preview_w - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4)
    movl (KERNEL_LOAD_ADDRESS + wm_preview_h - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4)
    cmpl $WM_TERMINAL, %eax
    jne wm_release_resize_ready
    call wm_reposition_terminal_cursor
wm_release_resize_ready:
    movb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_resize_window - _start)
    call render_shell
    ret

wm_xor_drag_outline:
    pushal
    movl (KERNEL_LOAD_ADDRESS + wm_preview_w - _start), %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_preview_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_preview_y - _start), %ebx
    call wm_xor_horizontal
    movl (KERNEL_LOAD_ADDRESS + wm_preview_w - _start), %ecx
    movl (KERNEL_LOAD_ADDRESS + wm_preview_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_preview_y - _start), %ebx
    addl (KERNEL_LOAD_ADDRESS + wm_preview_h - _start), %ebx
    decl %ebx
    call wm_xor_horizontal
    movl (KERNEL_LOAD_ADDRESS + wm_preview_h - _start), %edx
    movl (KERNEL_LOAD_ADDRESS + wm_preview_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + wm_preview_y - _start), %ebx
    call wm_xor_vertical
    movl (KERNEL_LOAD_ADDRESS + wm_preview_h - _start), %edx
    movl (KERNEL_LOAD_ADDRESS + wm_preview_x - _start), %ecx
    addl (KERNEL_LOAD_ADDRESS + wm_preview_w - _start), %ecx
    decl %ecx
    movl %ecx, %eax
    movl (KERNEL_LOAD_ADDRESS + wm_preview_y - _start), %ebx
    call wm_xor_vertical
    popal
    ret

/* EAX=x, EBX=y, ECX=length. */
wm_xor_horizontal:
    movl %ebx, %edi
    imull BOOT_PITCH(%ebp), %edi
    addl BOOT_FRAMEBUFFER(%ebp), %edi
    movl %eax, %edx
    shll $2, %edx
    addl %edx, %edi
wm_xor_horizontal_next:
    xorl $0x00ffffff, (%edi)
    addl $4, %edi
    decl %ecx
    jnz wm_xor_horizontal_next
    ret

/* EAX=x, EBX=y, EDX=length. */
wm_xor_vertical:
    movl %ebx, %edi
    imull BOOT_PITCH(%ebp), %edi
    addl BOOT_FRAMEBUFFER(%ebp), %edi
    movl %eax, %ecx
    shll $2, %ecx
    addl %ecx, %edi
wm_xor_vertical_next:
    xorl $0x00ffffff, (%edi)
    addl BOOT_PITCH(%ebp), %edi
    decl %edx
    jnz wm_xor_vertical_next
    ret

gui_click_complete:
    ret

gui_click_previous:
    movl (KERNEL_LOAD_ADDRESS + gui_page_offset - _start), %eax
    testl %eax, %eax
    jz gui_click_complete
    cmpl (KERNEL_LOAD_ADDRESS + gui_file_row_capacity - _start), %eax
    jb gui_page_first
    subl (KERNEL_LOAD_ADDRESS + gui_file_row_capacity - _start), %eax
    jmp gui_page_previous_store
gui_page_first:
    xorl %eax, %eax
gui_page_previous_store:
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_page_offset - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + gui_selected_index - _start)
    call render_shell
    ret
gui_click_next:
    movl (KERNEL_LOAD_ADDRESS + gui_page_offset - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_file_row_capacity - _start), %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_total_files - _start), %eax
    jae gui_click_complete
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_page_offset - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + gui_selected_index - _start)
    call render_shell
    ret

gui_get_selected_entry:
    movzbl (KERNEL_LOAD_ADDRESS + gui_selected_index - _start), %eax
    movzbl (KERNEL_LOAD_ADDRESS + gui_visible_files - _start), %edx
    cmpl %edx, %eax
    jae gui_no_selected_entry
    movl (KERNEL_LOAD_ADDRESS + gui_file_entries - _start)(,%eax,4), %edi
    movl $1, %eax
    ret
gui_no_selected_entry:
    xorl %eax, %eax
    ret

gui_action_open:
    call gui_get_selected_entry
    testl %eax, %eax
    jz gui_click_complete
    pushl %edi
    movl $WM_TERMINAL, %eax
    call wm_open_selected
    popl %edi
    call clear_terminal
    movl $(KERNEL_LOAD_ADDRESS + gui_opening_message - _start), %esi
    call print_string
    pushl %edi
    call print_directory_name
    movl $(KERNEL_LOAD_ADDRESS + newline - _start), %esi
    call print_string
    popl %edi
    call file_found
    movl $0, (KERNEL_LOAD_ADDRESS + input_length - _start)
    call print_prompt
    call draw_cursor
    ret

gui_action_edit:
    call gui_get_selected_entry
    testl %eax, %eax
    jz gui_click_complete
    call gui_copy_entry_name
    movl $(KERNEL_LOAD_ADDRESS + gui_action_source - _start), %esi
    call start_editor
    ret

gui_action_copy:
    movb $GUI_ACTION_COPY, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    jmp gui_prepare_pending_action
gui_action_rename:
    movb $GUI_ACTION_RENAME, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    jmp gui_prepare_pending_action
gui_action_delete:
    movb $GUI_ACTION_DELETE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
gui_prepare_pending_action:
    call gui_get_selected_entry
    testl %eax, %eax
    jz gui_pending_no_entry
    call gui_copy_entry_name
    movl $WM_TERMINAL, %eax
    call wm_open_selected
    call clear_terminal
    movl $0, (KERNEL_LOAD_ADDRESS + input_length - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + input_buffer - _start)
    movzbl (KERNEL_LOAD_ADDRESS + gui_pending_action - _start), %eax
    cmpl $GUI_ACTION_COPY, %eax
    je gui_prompt_copy
    cmpl $GUI_ACTION_RENAME, %eax
    je gui_prompt_rename
    movl $(KERNEL_LOAD_ADDRESS + gui_delete_prompt - _start), %esi
    jmp gui_prompt_action
gui_prompt_copy:
    movl $(KERNEL_LOAD_ADDRESS + gui_copy_prompt - _start), %esi
    jmp gui_prompt_action
gui_prompt_rename:
    movl $(KERNEL_LOAD_ADDRESS + gui_rename_prompt - _start), %esi
gui_prompt_action:
    call print_string
    movl $(KERNEL_LOAD_ADDRESS + gui_action_source - _start), %esi
    call print_string
    cmpb $GUI_ACTION_DELETE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    je gui_prompt_delete_suffix
    movl $(KERNEL_LOAD_ADDRESS + gui_destination_suffix - _start), %esi
    jmp gui_prompt_suffix_ready
gui_prompt_delete_suffix:
    movl $(KERNEL_LOAD_ADDRESS + gui_delete_suffix - _start), %esi
gui_prompt_suffix_ready:
    call print_string
    call print_prompt
    call draw_cursor
    ret
gui_pending_no_entry:
    movb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    ret

gui_complete_pending_action:
    movzbl (KERNEL_LOAD_ADDRESS + gui_pending_action - _start), %eax
    movb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    cmpl $GUI_ACTION_COPY, %eax
    je gui_complete_copy
    cmpl $GUI_ACTION_RENAME, %eax
    je gui_complete_rename
    cmpl $GUI_ACTION_DELETE, %eax
    je gui_complete_delete
    ret
gui_complete_copy:
    cmpb $0, (KERNEL_LOAD_ADDRESS + input_buffer - _start)
    je gui_action_cancelled
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + second_name - _start)
    movl $(KERNEL_LOAD_ADDRESS + gui_action_source - _start), %esi
    call copy_file
    ret
gui_complete_rename:
    cmpb $0, (KERNEL_LOAD_ADDRESS + input_buffer - _start)
    je gui_action_cancelled
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + second_name - _start)
    movl $(KERNEL_LOAD_ADDRESS + gui_action_source - _start), %esi
    call rename_file
    ret
gui_complete_delete:
    call gui_input_is_yes
    testl %eax, %eax
    jz gui_action_cancelled
    movl $(KERNEL_LOAD_ADDRESS + gui_action_source - _start), %esi
    call delete_file
    ret
gui_action_cancelled:
    movl $(KERNEL_LOAD_ADDRESS + gui_cancelled_message - _start), %esi
    call print_string
    ret

gui_cancel_pending_action:
    movb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    call clear_terminal
    movl $0, (KERNEL_LOAD_ADDRESS + input_length - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + input_buffer - _start)
    movl $(KERNEL_LOAD_ADDRESS + gui_cancelled_message - _start), %esi
    call print_string
    call print_prompt
    call draw_cursor
    ret

gui_input_is_yes:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movzbl 0(%esi), %eax
    call uppercase_al
    cmpb $'Y', %al
    jne gui_input_not_yes
    movzbl 1(%esi), %eax
    call uppercase_al
    cmpb $'E', %al
    jne gui_input_not_yes
    movzbl 2(%esi), %eax
    call uppercase_al
    cmpb $'S', %al
    jne gui_input_not_yes
    cmpb $0, 3(%esi)
    jne gui_input_not_yes
    movl $1, %eax
    ret
gui_input_not_yes:
    xorl %eax, %eax
    ret

/* EDI=root entry. Build a printable 8.3 name in gui_action_source. */
gui_copy_entry_name:
    pushal
    movl $(KERNEL_LOAD_ADDRESS + gui_action_source - _start), %ebx
    jmp gui_copy_name_base

/* Rendering has a separate buffer so repainting a FileHound window cannot
 * overwrite a pending copy, rename, or delete action's source name. */
gui_copy_render_name:
    pushal
    movl $(KERNEL_LOAD_ADDRESS + gui_render_name - _start), %ebx
gui_copy_name_base:
    xorl %ecx, %ecx
gui_copy_name_base_next:
    cmpl $8, %ecx
    jae gui_copy_name_extension
    movb (%edi,%ecx), %al
    cmpb $' ', %al
    je gui_copy_name_extension
    movb %al, (%ebx)
    incl %ebx
    incl %ecx
    jmp gui_copy_name_base_next
gui_copy_name_extension:
    cmpb $' ', 8(%edi)
    je gui_copy_name_complete
    movb $'.', (%ebx)
    incl %ebx
    movl $8, %ecx
gui_copy_name_extension_next:
    cmpl $11, %ecx
    jae gui_copy_name_complete
    movb (%edi,%ecx), %al
    cmpb $' ', %al
    je gui_copy_name_complete
    movb %al, (%ebx)
    incl %ebx
    incl %ecx
    jmp gui_copy_name_extension_next
gui_copy_name_complete:
    movb $0, (%ebx)
    popal
    ret

gui_fill_rectangle:
    pushal
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start), %ebx
    testl %ecx, %ecx
    jz gui_fill_restore
    testl %edx, %edx
    jz gui_fill_restore
    cmpb $0, (KERNEL_LOAD_ADDRESS + gui_clip_enabled - _start)
    je gui_fill_clipped

    movl (KERNEL_LOAD_ADDRESS + gui_clip_left - _start), %edi
    cmpl %edi, %eax
    jae gui_fill_left_ready
    subl %eax, %edi
    cmpl %ecx, %edi
    jae gui_fill_restore
    subl %edi, %ecx
    addl %edi, %eax
gui_fill_left_ready:
    movl (KERNEL_LOAD_ADDRESS + gui_clip_top - _start), %edi
    cmpl %edi, %ebx
    jae gui_fill_top_ready
    subl %ebx, %edi
    cmpl %edx, %edi
    jae gui_fill_restore
    subl %edi, %edx
    addl %edi, %ebx
gui_fill_top_ready:
    movl %eax, %edi
    addl %ecx, %edi
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_right - _start), %edi
    jbe gui_fill_right_ready
    movl (KERNEL_LOAD_ADDRESS + gui_clip_right - _start), %ecx
    subl %eax, %ecx
    jbe gui_fill_restore
gui_fill_right_ready:
    movl %ebx, %edi
    addl %edx, %edi
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_bottom - _start), %edi
    jbe gui_fill_clipped
    movl (KERNEL_LOAD_ADDRESS + gui_clip_bottom - _start), %edx
    subl %ebx, %edx
    jbe gui_fill_restore
gui_fill_clipped:
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_rect_height - _start)
    movl %ebx, %edi
    imull BOOT_PITCH(%ebp), %edi
    addl BOOT_FRAMEBUFFER(%ebp), %edi
    movl %eax, %edx
    shll $2, %edx
    addl %edx, %edi
gui_fill_row:
    pushl %ecx
    movl %esi, %eax
    rep stosl
    popl %ecx
    movl BOOT_PITCH(%ebp), %edx
    movl %ecx, %eax
    shll $2, %eax
    subl %eax, %edx
    addl %edx, %edi
    decl (KERNEL_LOAD_ADDRESS + gui_rect_height - _start)
    jnz gui_fill_row
gui_fill_restore:
    popal
    ret

/* EAX=x, EBX=y, ECX=width, EDX=height, ESI=face color. */
gui_draw_bevel_box:
    pushal
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_bevel_x - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_bevel_y - _start)
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_bevel_w - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_bevel_h - _start)
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_y - _start), %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_w - _start), %ecx
    movl $2, %edx
    movl $GUI_COLOR_LIGHT, %esi
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_y - _start), %ebx
    movl $2, %ecx
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_h - _start), %edx
    movl $GUI_COLOR_LIGHT, %esi
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_y - _start), %ebx
    addl (KERNEL_LOAD_ADDRESS + gui_bevel_h - _start), %ebx
    subl $2, %ebx
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_w - _start), %ecx
    movl $2, %edx
    movl $GUI_COLOR_DARK, %esi
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_bevel_w - _start), %eax
    subl $2, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_y - _start), %ebx
    movl $2, %ecx
    movl (KERNEL_LOAD_ADDRESS + gui_bevel_h - _start), %edx
    movl $GUI_COLOR_DARK, %esi
    call gui_fill_rectangle
    popal
    ret

gui_draw_classic_button:
    pushal
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_button_x - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_button_y - _start)
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_button_width - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_button_label - _start)
    movl $40, %edx
    call gui_draw_bevel_box
    movl (KERNEL_LOAD_ADDRESS + gui_button_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    addl $12, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_button_y - _start), %edx
    addl $12, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    popal
    ret

/* Compact root-menu entry: EAX=x, EBX=y, ECX=width, EDX=label. */
gui_draw_menu_item:
    pushal
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_button_x - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_button_y - _start)
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_button_width - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_button_label - _start)
    movl $WM_ROOT_MENU_ROW, %edx
    movzbl (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start), %eax
    cmpl %edi, %eax
    jne gui_menu_item_idle
    movl $GUI_COLOR_ACTIVE, %esi
    jmp gui_menu_item_face_ready
gui_menu_item_idle:
    movl $GUI_COLOR_PANEL_ALT, %esi
gui_menu_item_face_ready:
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_button_y - _start), %ebx
    movl $4, %ecx
    movl $WM_ROOT_MENU_ROW, %edx
    movzbl (KERNEL_LOAD_ADDRESS + wm_menu_hover - _start), %eax
    cmpl %edi, %eax
    jne gui_menu_item_idle_accent
    movl $GUI_COLOR_PINK, %esi
    jmp gui_menu_item_accent_ready
gui_menu_item_idle_accent:
    movl $GUI_COLOR_ACTIVE, %esi
gui_menu_item_accent_ready:
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_button_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    addl $12, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_button_y - _start), %edx
    addl $6, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    popal
    ret

gui_draw_small_button:
    pushal
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_button_x - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_button_y - _start)
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_button_width - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_button_label - _start)
    movl $20, %edx
    call gui_draw_bevel_box
    movl (KERNEL_LOAD_ADDRESS + gui_button_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    addl $6, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_button_y - _start), %edx
    addl $2, %edx
    movl $GUI_COLOR_DARK, %ecx
    call gui_print_at
    popal
    ret

gui_draw_button:
    pushal
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_button_x - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_button_y - _start)
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_button_width - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_button_label - _start)
    movl $40, %edx
    call gui_fill_rectangle
    movl (KERNEL_LOAD_ADDRESS + gui_button_label - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + gui_button_x - _start), %eax
    addl $12, %eax
    movl (KERNEL_LOAD_ADDRESS + gui_button_y - _start), %edx
    addl $12, %edx
    movl $GUI_COLOR_TEXT, %ecx
    call gui_print_at
    popal
    ret

gui_print_at:
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start), %edx
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl %edx, BOOT_CURSOR_Y(%ebp)
    movl %ecx, BOOT_COLOR(%ebp)
    call gui_print_raw
    ret

/* Draw a GUI string without terminal capture, terminal newlines, or terminal
 * wrapping. GUI coordinates are clipped to the physical 1024x768 surface. */
gui_print_raw:
    cmpl $752, BOOT_CURSOR_Y(%ebp)
    ja gui_print_raw_complete
    cmpb $0, (KERNEL_LOAD_ADDRESS + gui_clip_enabled - _start)
    je gui_print_raw_next
    movl BOOT_CURSOR_Y(%ebp), %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_top - _start), %eax
    jb gui_print_raw_complete
    addl $16, %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_bottom - _start), %eax
    ja gui_print_raw_complete
gui_print_raw_next:
    movzbl (%esi), %eax
    testb %al, %al
    jz gui_print_raw_complete
    cmpl $1016, BOOT_CURSOR_X(%ebp)
    ja gui_print_raw_complete
    cmpb $0, (KERNEL_LOAD_ADDRESS + gui_clip_enabled - _start)
    je gui_print_raw_draw
    movl BOOT_CURSOR_X(%ebp), %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_left - _start), %eax
    jb gui_print_raw_skip
    addl $8, %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_right - _start), %eax
    ja gui_print_raw_complete
gui_print_raw_draw:
    /* Clip checks use EAX for coordinates; reload the actual byte before the
     * glyph call so resized client text cannot turn into coordinate glyphs. */
    movzbl (%esi), %eax
    pushal
    call draw_character
    popal
gui_print_raw_skip:
    incl %esi
    addl $8, BOOT_CURSOR_X(%ebp)
    jmp gui_print_raw_next
gui_print_raw_complete:
    ret

/* EAX=value, EBX=x, EDX=y, ECX=color. */
gui_number_at:
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start), %ebx
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start), %edx
    movl %ebx, BOOT_CURSOR_X(%ebp)
    movl %edx, BOOT_CURSOR_Y(%ebp)
    movl %ecx, BOOT_COLOR(%ebp)
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %edi
    movl $(KERNEL_LOAD_ADDRESS + decimal_buffer_end - _start), %edi
    movb $0, (%edi)
    movl $10, %ecx
gui_decimal_next:
    xorl %edx, %edx
    divl %ecx
    addb $'0', %dl
    decl %edi
    movb %dl, (%edi)
    testl %eax, %eax
    jnz gui_decimal_next
    movl %edi, %esi
    call gui_print_raw
    popl %edi
    popl %edx
    popl %ecx
    popl %ebx
    ret

gui_save_terminal_state:
    movl BOOT_CURSOR_X(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_saved_cursor_x - _start)
    movl BOOT_CURSOR_Y(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_saved_cursor_y - _start)
    movl BOOT_COLOR(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_saved_color - _start)
    movb (KERNEL_LOAD_ADDRESS + terminal_capture_enabled - _start), %al
    movb %al, (KERNEL_LOAD_ADDRESS + gui_saved_capture - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + terminal_capture_enabled - _start)
    ret
gui_restore_terminal_state:
    movl (KERNEL_LOAD_ADDRESS + gui_saved_cursor_x - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + gui_saved_cursor_y - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
    movl (KERNEL_LOAD_ADDRESS + gui_saved_color - _start), %eax
    movl %eax, BOOT_COLOR(%ebp)
    movb (KERNEL_LOAD_ADDRESS + gui_saved_capture - _start), %al
    movb %al, (KERNEL_LOAD_ADDRESS + terminal_capture_enabled - _start)
    ret

gui_draw_circle:
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_draw_offset_y - _start), %ebx
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start)
    movl %ecx, (KERNEL_LOAD_ADDRESS + gui_circle_x - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + gui_circle_y - _start)
    movl $1, %eax
    subl %ecx, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + gui_circle_decision - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + gui_circle_color - _start)
gui_circle_next:
    call gui_plot_circle_octants
    incl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start)
    movl (KERNEL_LOAD_ADDRESS + gui_circle_decision - _start), %eax
    testl %eax, %eax
    js gui_circle_inside
    decl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start)
    movl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %eax
    shll $1, %eax
    incl %eax
    addl %eax, (KERNEL_LOAD_ADDRESS + gui_circle_decision - _start)
    jmp gui_circle_test
gui_circle_inside:
    movl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    shll $1, %eax
    incl %eax
    addl %eax, (KERNEL_LOAD_ADDRESS + gui_circle_decision - _start)
gui_circle_test:
    movl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    cmpl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %eax
    jbe gui_circle_next
    ret

gui_plot_circle_octants:
    movl (KERNEL_LOAD_ADDRESS + gui_circle_color - _start), %ecx
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    addl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    addl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    addl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    addl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    addl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %edx
    call gui_plot_pixel
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_x - _start), %eax
    subl (KERNEL_LOAD_ADDRESS + gui_circle_y - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + gui_circle_center_y - _start), %edx
    subl (KERNEL_LOAD_ADDRESS + gui_circle_x - _start), %edx
    call gui_plot_pixel
    ret

gui_plot_pixel:
    cmpb $0, (KERNEL_LOAD_ADDRESS + gui_clip_enabled - _start)
    je gui_plot_pixel_draw
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_left - _start), %eax
    jb gui_plot_pixel_complete
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_right - _start), %eax
    jae gui_plot_pixel_complete
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_top - _start), %edx
    jb gui_plot_pixel_complete
    cmpl (KERNEL_LOAD_ADDRESS + gui_clip_bottom - _start), %edx
    jae gui_plot_pixel_complete
gui_plot_pixel_draw:
    imull BOOT_PITCH(%ebp), %edx
    addl BOOT_FRAMEBUFFER(%ebp), %edx
    shll $2, %eax
    addl %eax, %edx
    movl %ecx, (%edx)
gui_plot_pixel_complete:
    ret

gui_terminal_window_title: .asciz "TERMINAL // MILO COMMAND INTERFACE"
gui_window_home_title: .asciz "M.I.L.O SYSTEM"
gui_window_files_caption: .asciz "FILEHOUND // M.I.L.O"
gui_window_traits_caption: .asciz "M.I.L.O TRAIT MONITOR"
gui_window_editor_caption: .asciz "M.I.L.O WRITER"
wm_root_menu_title: .asciz "M.I.L.O APPLICATIONS"
gui_overlay_title: .asciz "M.I.L.O // LOCAL"
gui_overlay_kernel: .asciz "KERNEL"
gui_overlay_version: .asciz "0.28.1"
gui_overlay_cpu: .asciz "CPU"
gui_overlay_cpu_mode: .asciz "I386 // POLL"
gui_overlay_activity: .asciz "EVENT RATE"
gui_overlay_events_per_second: .asciz "EV/S"
gui_overlay_graph_scale: .asciz "0..120+ EV/S"
gui_overlay_apps: .asciz "APPS"
gui_overlay_open_short: .asciz "OPEN"
gui_overlay_min_short: .asciz "MIN"
gui_overlay_app_active: .asciz "ACTIVE"
gui_overlay_app_open: .asciz "OPEN"
gui_overlay_app_minimized: .asciz "MINIMIZED"
gui_overlay_app_closed: .asciz "CLOSED"
gui_overlay_events: .asciz "EVENTS"
gui_overlay_ram: .asciz "RAM"
gui_overlay_disk: .asciz "DISK"
gui_overlay_files: .asciz "FILES"
gui_overlay_video: .asciz "VIDEO"
gui_overlay_video_mode: .asciz "1024X768X32"
gui_overlay_temp: .asciz "TEMP"
gui_overlay_unavailable: .asciz "N/A"
gui_overlay_network: .asciz "NET"
gui_overlay_offline: .asciz "OFFLINE"
gui_overlay_slash: .asciz "/"
gui_overlay_kib: .asciz "KIB"
gui_overlay_hint: .asciz "RIGHT CLICK // APPS"
gui_taskbar_label: .asciz "TASKS // MINIMIZED"
gui_taskbar_empty: .asciz "NO MINIMIZED APPLICATIONS"
gui_taskbar_version: .asciz "M.I.L.O V0.28.1"
gui_home_label: .asciz "SYSTEM"
gui_files_label: .asciz "FILEHOUND"
gui_traits_label: .asciz "TRAITS"
gui_terminal_label: .asciz "TERMINAL"
gui_editor_label: .asciz "WRITER"
gui_editor_default_name: .asciz "UNTITLED.TXT"
gui_editor_file_label: .asciz "FILE"
gui_editor_bytes_label: .asciz "BYTES"
gui_editor_help: .asciz "TYPE TO EDIT // CTRL+S SAVE // ARROWS MOVE"
gui_editor_save_as_prompt: .asciz "SAVE AS (8.3)"
gui_editor_save_label: .asciz "SAVE"
gui_editor_save_as_label: .asciz "SAVE AS"
gui_editor_close_label: .asciz "CLOSE"
gui_editor_modified: .asciz "MODIFIED"
gui_editor_saved: .asciz "SAVED"
gui_editor_save_failed: .asciz "SAVE FAILED"
gui_editor_invalid_name: .asciz "INVALID 8.3 NAME"
gui_editor_unsaved: .asciz "UNSAVED // CLOSE AGAIN"
gui_editor_too_large: .asciz "FILE TOO LARGE"
gui_editor_load_failed: .asciz "LOAD FAILED"
wm_minimize_label: .asciz "_"
wm_maximize_label: .asciz "O"
wm_close_label: .asciz "X"
gui_home_heading: .asciz "SYSTEM HOME"
gui_ring_name: .asciz "M.I.L.O"
gui_ring_state: .asciz "AWAKE // LOCAL"
gui_system_heading: .asciz "SYSTEM"
gui_version_status: .asciz "KERNEL   0.28.1"
gui_storage_status: .asciz "STORAGE  FAT12"
gui_memory_status: .asciz "MEMORY"
gui_mib_short: .asciz "MIB"
gui_network_status: .asciz "NETWORK  NONE"
gui_home_hint: .asciz "OPEN FILEHOUND, TRAITS, OR TERMINAL"
gui_traits_heading: .asciz "DETERMINISTIC TRAITS"
gui_interactions_label: .asciz "INTERACTIONS"
gui_warmth_label: .asciz "WARMTH"
gui_cheek_label: .asciz "CHEEK"
gui_curiosity_label: .asciz "CURIOSITY"
gui_percent_label: .asciz "%"
gui_traits_note: .asciz "STRUCTURAL PATTERNS ONLY // NO PHRASES STORED"
gui_files_heading: .asciz "FILEHOUND"
gui_files_edition: .asciz "M.I.L.O FAT12 EDITION"
gui_file_count_label: .asciz "ITEMS"
gui_location_label: .asciz "LOCATION"
gui_location_root: .asciz "A:\\"
gui_name_label: .asciz "NAME"
gui_type_label: .asciz "TYPE"
gui_size_label: .asciz "SIZE"
gui_file_status: .asciz "FAT12 // LOCAL //"
gui_items_label: .asciz "ITEMS"
gui_type_text: .asciz "TEXT"
gui_type_data: .asciz "DATA"
gui_type_file: .asciz "FILE"
gui_previous_label: .asciz "<"
gui_next_label: .asciz ">"
gui_open_label: .asciz "OPEN"
gui_edit_label: .asciz "EDIT"
gui_copy_label: .asciz "COPY"
gui_rename_label: .asciz "RENAME"
gui_delete_label: .asciz "DELETE"
gui_bytes_short: .asciz "B"
gui_opening_message: .asciz "GUI OPEN // "
gui_copy_prompt: .asciz "GUI COPY // "
gui_rename_prompt: .asciz "GUI RENAME // "
gui_delete_prompt: .asciz "GUI DELETE // "
gui_destination_suffix: .asciz " // TYPE DESTINATION 8.3 NAME. ESC CANCELS.\n"
gui_delete_suffix: .asciz " // TYPE YES TO DELETE. ESC CANCELS.\n"
gui_cancelled_message: .asciz "GUI ACTION CANCELLED. NO FILES CHANGED.\n"

.align 4
gui_editor_status_table:
    .long KERNEL_LOAD_ADDRESS + gui_editor_saved - _start
    .long KERNEL_LOAD_ADDRESS + gui_editor_save_failed - _start
    .long KERNEL_LOAD_ADDRESS + gui_editor_invalid_name - _start
    .long KERNEL_LOAD_ADDRESS + gui_editor_unsaved - _start
    .long KERNEL_LOAD_ADDRESS + gui_editor_too_large - _start
    .long KERNEL_LOAD_ADDRESS + gui_editor_load_failed - _start

.align 4
gui_saved_cursor_x: .long 0
gui_saved_cursor_y: .long 0
gui_saved_color: .long 0
gui_draw_offset_x: .long 0
gui_draw_offset_y: .long 0
gui_clip_left: .long 0
gui_clip_top: .long 0
gui_clip_right: .long 1024
gui_clip_bottom: .long 768
gui_bevel_x: .long 0
gui_bevel_y: .long 0
gui_bevel_w: .long 0
gui_bevel_h: .long 0
gui_button_x: .long 0
gui_button_y: .long 0
gui_button_width: .long 0
gui_button_label: .long 0
gui_trait_value: .long 0
gui_trait_y: .long 0
gui_home_ring_x: .long 584
gui_home_system_x: .long 792
gui_home_memory_x: .long 872
gui_home_mib_x: .long 920
gui_home_hint_y: .long 472
gui_trait_bar_width: .long 400
gui_trait_note_y: .long 424
gui_circle_center_x: .long 0
gui_circle_center_y: .long 0
gui_circle_x: .long 0
gui_circle_y: .long 0
gui_circle_decision: .long 0
gui_circle_color: .long 0
gui_page_offset: .long 0
gui_total_files: .long 0
gui_row_y: .long 0
gui_render_index: .long 0
gui_rect_height: .long 0
gui_overlay_free_kib: .long 0
gui_overlay_total_kib: .long 0
gui_overlay_file_count: .long 0
gui_overlay_graph_index: .long 0
gui_overlay_graph_height: .long 0
gui_overlay_open_apps: .long 0
gui_overlay_minimized_apps: .long 0
gui_overlay_app_id: .long 0
gui_overlay_app_y: .long 0
gui_file_type_label: .long 0
gui_file_type_color: .long GUI_COLOR_DIM
gui_file_layout_right: .long 1000
gui_file_count_label_x: .long 824
gui_file_count_x: .long 880
gui_file_previous_x: .long 888
gui_file_next_x: .long 944
gui_file_type_x: .long 624
gui_file_size_x: .long 792
gui_file_bytes_x: .long 856
gui_file_table_width: .long 800
gui_file_action_y: .long 448
gui_file_status_y: .long 432
gui_file_row_capacity: .long GUI_FILE_ROWS
gui_editor_visible_columns: .long 0
gui_editor_visible_rows: .long 0
gui_editor_horizontal_offset: .long 0
gui_editor_first_line: .long 1
gui_editor_render_index: .long 0
gui_editor_render_row: .long 0
gui_editor_render_column: .long 0
gui_editor_click_line: .long 1
gui_editor_click_column: .long 0
system_event_count: .long 0
activity_previous_events: .long 0
activity_current_events: .long 0
activity_rate: .long 0
rtc_poll_divider: .long 0
wm_render_index: .long 0
wm_current: .long 0
wm_current_x: .long 0
wm_current_y: .long 0
wm_current_w: .long 0
wm_current_h: .long 0
wm_pointer_x: .long 0
wm_pointer_y: .long 0
wm_local_x: .long 0
wm_local_y: .long 0
wm_hit_index: .long 0
wm_hit_window: .long 0
wm_promote_window: .long 0
wm_drag_offset_x: .long 0
wm_drag_offset_y: .long 0
wm_preview_x: .long 0
wm_preview_y: .long 0
wm_preview_w: .long 0
wm_preview_h: .long 0
wm_menu_x: .long 0
wm_menu_y: .long 0
wm_taskbar_count: .long 0
wm_taskbar_scan: .long 0
wm_x: .long 64, 104, 144, 96, 120
wm_y: .long 64, 92, 120, 240, 72
wm_w: .long 832, 832, 832, 832, 760
wm_h: .long 456, 456, 456, 456, 560
wm_min_w: .long 720, 640, 560, 560, 560
wm_min_h: .long 456, 360, 400, 320, 320
wm_restore_x: .long 64, 104, 144, 96, 120
wm_restore_y: .long 64, 92, 120, 240, 72
wm_restore_w: .long 832, 832, 832, 832, 760
wm_restore_h: .long 456, 456, 456, 456, 560
wm_title_table:
    .long KERNEL_LOAD_ADDRESS + gui_window_home_title - _start
    .long KERNEL_LOAD_ADDRESS + gui_window_files_caption - _start
    .long KERNEL_LOAD_ADDRESS + gui_window_traits_caption - _start
    .long KERNEL_LOAD_ADDRESS + gui_terminal_window_title - _start
    .long KERNEL_LOAD_ADDRESS + gui_window_editor_caption - _start
wm_task_title_table:
    .long KERNEL_LOAD_ADDRESS + gui_home_label - _start
    .long KERNEL_LOAD_ADDRESS + gui_files_label - _start
    .long KERNEL_LOAD_ADDRESS + gui_traits_label - _start
    .long KERNEL_LOAD_ADDRESS + gui_terminal_label - _start
    .long KERNEL_LOAD_ADDRESS + gui_editor_label - _start
gui_file_entries: .space GUI_FILE_ROWS * 4
wm_z_order: .byte WM_HOME, WM_FILES, WM_TRAITS, WM_TERMINAL, WM_EDITOR
wm_flags: .byte 0, 0, 0, 0, 0
wm_active: .byte WM_NONE
wm_drag_window: .byte WM_NONE
wm_resize_window: .byte WM_NONE
wm_menu_open: .byte 0
wm_menu_hover: .byte WM_NONE
wm_taskbar_slots: .space WM_COUNT
activity_history: .space WM_ACTIVITY_SAMPLES
rtc_display_text: .ascii "00/00/2000  00:00\0"
rtc_raw_second: .byte 0
rtc_second: .byte 0
rtc_minute: .byte 0
rtc_hour: .byte 0
rtc_day: .byte 0
rtc_month: .byte 0
rtc_year: .byte 0
rtc_register_b: .byte 0
rtc_pm_flag: .byte 0
rtc_last_second: .byte 0xff
gui_saved_capture: .byte 1
gui_clip_enabled: .byte 0
gui_visible_files: .byte 0
gui_selected_index: .byte 0
gui_pending_action: .byte GUI_ACTION_NONE
gui_action_source: .space 13
gui_render_name: .space 13
gui_editor_character: .space 2

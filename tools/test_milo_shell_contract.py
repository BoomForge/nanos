#!/usr/bin/env python3
"""Static and binary checks for the V0.30.1 desktop and Pixel Studio."""

from pathlib import Path
import re
import sys


KERNEL_LIMIT = 48 * 1024
CURSOR_WIDTH = 12
CURSOR_HEIGHT = 16
CURSOR_BACKING = 0x32400
CURSOR_BACKING_END = CURSOR_BACKING + CURSOR_WIDTH * CURSOR_HEIGHT * 4
TERMINAL_BUFFER = 0x32800
TERMINAL_BUFFER_END = TERMINAL_BUFFER + 100 * 25
CURSOR_NEXT_BACKING = 0x33400
CURSOR_NEXT_BACKING_END = (CURSOR_NEXT_BACKING +
                           CURSOR_WIDTH * CURSOR_HEIGHT * 4)
IMAGE_BUFFER = 0x34000
IMAGE_BUFFER_END = IMAGE_BUFFER + 8192
IMAGE_UNDO_BUFFER = 0x36000
IMAGE_UNDO_BUFFER_END = IMAGE_UNDO_BUFFER + 8192
IMAGE_FILL_QUEUE = 0x38000
IMAGE_FILL_QUEUE_END = IMAGE_FILL_QUEUE + 128 * 96 * 2
FRAMEBUFFER_BACKING = 0x100000
FRAMEBUFFER_BACKING_END = FRAMEBUFFER_BACKING + 1024 * 768 * 4


def require(source, fragment):
    assert fragment in source, "missing shell contract fragment: %s" % fragment


def main():
    if len(sys.argv) != 5:
        raise SystemExit(
            "usage: test_milo_shell_contract.py KERNEL.S SHELL.INC MOUSE.INC KERNEL"
        )

    kernel_source = Path(sys.argv[1]).read_text()
    shell_source = Path(sys.argv[2]).read_text()
    mouse_source = Path(sys.argv[3]).read_text()
    kernel = Path(sys.argv[4]).read_bytes()

    assert len(kernel) <= KERNEL_LIMIT, len(kernel)
    assert "#define TERMINAL_COLUMNS 100" in kernel_source
    assert "#define TERMINAL_ROWS 25" in kernel_source
    assert "#define TERMINAL_BUFFER_ADDRESS 0x32800" in kernel_source
    assert "#define MOUSE_NEXT_BACKING_ADDRESS 0x33400" in kernel_source
    assert "#define IMAGE_BUFFER_ADDRESS 0x34000" in kernel_source
    assert "#define IMAGE_LIMIT 8192" in kernel_source
    assert "#define IMAGE_UNDO_ADDRESS 0x36000" in kernel_source
    assert "#define IMAGE_FILL_QUEUE_ADDRESS 0x38000" in kernel_source
    assert "#define GUI_FILE_ROWS 10" in kernel_source
    assert "#define MOUSE_MAX_X 1012" in kernel_source
    assert "#define MOUSE_MAX_Y 752" in kernel_source
    assert CURSOR_BACKING_END == 0x32700
    assert CURSOR_BACKING_END < TERMINAL_BUFFER
    assert TERMINAL_BUFFER_END < CURSOR_NEXT_BACKING
    assert CURSOR_NEXT_BACKING_END == 0x33700
    assert CURSOR_NEXT_BACKING_END < IMAGE_BUFFER
    assert IMAGE_BUFFER_END == 0x36000
    assert IMAGE_BUFFER_END == IMAGE_UNDO_BUFFER
    assert IMAGE_UNDO_BUFFER_END == IMAGE_FILL_QUEUE
    assert IMAGE_FILL_QUEUE_END == 0x3e000
    require(
        kernel_source,
        "call terminal_buffer_store_character\n    call draw_character",
    )

    for marker in (
        b"M.I.L.O APPLICATIONS",
        b"SYSTEM",
        b"FILEHOUND",
        b"TRAITS",
        b"TERMINAL",
        b"TERMINAL // MILO COMMAND INTERFACE",
        b"M.I.L.O SYSTEM",
        b"FILEHOUND // M.I.L.O",
        b"M.I.L.O FAT12 EDITION",
        b"LOCATION",
        b"NAME",
        b"TYPE",
        b"SIZE",
        b"M.I.L.O TRAIT MONITOR",
        b"DETERMINISTIC TRAITS",
        b"TYPE YES TO DELETE",
        b"M.I.L.O // LOCAL",
        b"I386 // POLL",
        b"1024X768X32",
        b"F1 // APPS",
        b"TASKS // MINIMIZED",
        b"NO MINIMIZED APPLICATIONS",
        b"M.I.L.O V0.30.1",
        b"EVENT RATE",
        b"0..120+ EV/S",
        b"EV/S",
        b"ACTIVE",
        b"MINIMIZED",
        b"M.I.L.O VERSION 0.30.1",
        b"M.I.L.O WRITER",
        b"CTRL+S SAVE",
        b"SHIFT+ARROWS/MOUSE SELECT",
        b"NAME (AUTO .TXT)",
        b"UNSAVED // CLOSE AGAIN",
        b"M.I.L.O PIXEL STUDIO",
        b"PIXEL STUDIO // NATIVE INDEXED IMAGE",
        b"M16 V1 // 4-BIT INDEXED",
        b"ARROWS MOVE // ENTER DRAW // [ ] COLOUR",
        b"SAVE AS (AUTO .M16)",
        b"NEW CANVAS // SELECT SIZE",
        b"32 X 32",
        b"64 X 48",
        b"96 X 64",
        b"128 X 96",
        b"ARROWS SELECT // ENTER CREATE // ESC CANCEL",
        b"UNSAVED // CLOSE AGAIN",
        b"SUPPORTED: M16 V1 // MAX 128X96 // 16 COLOURS",
    ):
        assert marker in kernel, marker

    # Startup is a root desktop with a lightweight system overlay and a sparse
    # taskbar. No launcher, dashboard application, or ordinary window is open.
    for fragment in (
        ".equ WM_COUNT, 6",
        ".equ WM_ROOT_MENU_WIDTH, 232",
        ".equ WM_ROOT_MENU_HEIGHT, 204",
        ".equ WM_ROOT_MENU_ROW, 28",
        ".equ WM_TASKBAR_Y, 704",
        ".equ WM_TASKBAR_HEIGHT, 64",
        "movl $1024, %ecx\n    movl $768, %edx",
        "wm_flags: .byte 0, 0, 0, 0, 0, 0",
        "wm_active: .byte WM_NONE",
        "call wm_render_system_overlay\n    call wm_render_all_windows",
        "call wm_render_all_windows",
        "call wm_render_root_menu",
        "wm_root_menu_title: .asciz \"M.I.L.O APPLICATIONS\"",
        "gui_draw_menu_item:",
        "gui_handle_root_click:",
        "movb $1, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)",
        "cmpl $(1024 - WM_ROOT_MENU_WIDTH), %eax",
        "cmpl $(WM_TASKBAR_Y - WM_ROOT_MENU_HEIGHT), %eax",
        "gui_handle_pointer_motion:",
        "wm_menu_hover: .byte WM_NONE",
        "cmpl %edi, %eax\n    jne gui_menu_item_idle",
        "wm_keyboard_toggle_menu:",
        "wm_keyboard_menu_move:",
        "wm_keyboard_menu_activate:",
    ):
        require(shell_source, fragment)
    for obsolete in (
        "wm_render_desktop_icons",
        "wm_render_start_menu",
        "gui_taskbar_status",
        "gui_milo_button_label",
    ):
        assert obsolete not in shell_source, obsolete

    # Only minimized windows become task buttons. Their slot mapping is built
    # from real window flags and a click restores the selected window.
    for fragment in (
        "wm_render_taskbar:",
        "testb $WM_FLAG_VISIBLE, %al",
        "testb $WM_FLAG_MINIMIZED, %al",
        "wm_taskbar_slots: .space WM_COUNT",
        "cmpl $WM_TASK_BUTTON_WIDTH, %edx",
        "movzbl (KERNEL_LOAD_ADDRESS + wm_taskbar_slots - _start)(,%eax), %eax",
        "jmp wm_open_selected",
        "call wm_render_taskbar",
        "rtc_display_text: .ascii \"00/00/2000  00:00\\0\"",
        "movl $888, %eax\n    movl $708, %edx",
        "movl $872, %eax\n    movl $740, %edx",
    ):
        require(shell_source, fragment)
    assert 888 + len("M.I.L.O V0.30.1") * 8 == 1008
    assert 872 + len("00/00/2000  00:00") * 8 == 1008

    # Six independent native application windows retain focus, stacking,
    # movement, bounded resizing, controls, terminal state, and file operations.
    for fragment in (
        ".equ WM_FLAG_VISIBLE, 1",
        ".equ WM_FLAG_MINIMIZED, 2",
        ".equ WM_FLAG_MAXIMIZED, 4",
        "wm_x: .long 64, 104, 144, 96, 120, 88",
        "wm_y: .long 64, 92, 120, 240, 72, 80",
        "wm_w: .long 832, 832, 832, 832, 760, 720",
        "wm_h: .long 456, 456, 456, 456, 560, 536",
        "wm_z_order: .byte WM_HOME, WM_FILES, WM_TRAITS, WM_TERMINAL, WM_EDITOR, WM_IMAGE",
        "call wm_bring_to_front",
        "wm_minimize_window:",
        "wm_close_window:",
        "wm_toggle_maximize:",
        "movl $0, (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4)",
        "movl $1024, (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4)",
        "movl $WM_TASKBAR_Y, (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4)",
        "wm_xor_drag_outline:",
        ".equ WM_RESIZE_GRIP, 20",
        "wm_min_w: .long 720, 640, 560, 560, 560, 600",
        "wm_min_h: .long 456, 360, 400, 320, 320, 420",
        "wm_resize_window: .byte WM_NONE",
        "wm_render_resize_grip:",
        "wm_resize_drag:",
        "wm_release_resize:",
        "wm_preview_w: .long 0",
        "wm_preview_h: .long 0",
        "movl (KERNEL_LOAD_ADDRESS + wm_min_w - _start)(,%eax,4), %edx",
        "movl (KERNEL_LOAD_ADDRESS + wm_min_h - _start)(,%eax,4), %edx",
        "movl %ecx, (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4)",
        "movl %ecx, (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4)",
        "call wm_reposition_terminal_cursor",
        "call render_terminal_buffer",
        "subl (KERNEL_LOAD_ADDRESS + wm_x + WM_FILES * 4 - _start), %eax",
        "subl (KERNEL_LOAD_ADDRESS + wm_y + WM_FILES * 4 - _start), %edx",
        "movb $0, (KERNEL_LOAD_ADDRESS + terminal_capture_enabled - _start)",
        "call gui_input_is_yes\n    testl %eax, %eax\n    jz gui_action_cancelled",
        "GUI ACTION CANCELLED. NO FILES CHANGED.",
    ):
        require(shell_source, fragment)

    # Desktop strings and primitives use a client-aware clip rectangle. They
    # never inherit Terminal wrapping or leak through a resized window frame.
    for fragment in (
        "gui_print_raw:",
        "cmpl $752, BOOT_CURSOR_Y(%ebp)",
        "cmpl $1016, BOOT_CURSOR_X(%ebp)",
        "call draw_character",
        "gui_decimal_next:",
        "call gui_print_raw",
        "wm_enable_client_clip:",
        "gui_clip_enabled: .byte 0",
        "gui_clip_left: .long 0",
        "gui_clip_right: .long 1024",
        "cmpl (KERNEL_LOAD_ADDRESS + gui_clip_right - _start), %edi",
        "gui_plot_pixel_complete:",
        "gui_print_raw_draw:\n    /* Clip checks use EAX for coordinates; reload the actual byte",
        "movzbl (%esi), %eax\n    pushal\n    call draw_character",
    ):
        require(shell_source, fragment)
    gui_text = shell_source[
        shell_source.index("gui_print_at:"):
        shell_source.index("gui_save_terminal_state:")
    ]
    assert "call print_string" not in gui_text
    assert "call print_u32" not in gui_text

    # Writer is a real fifth application with a resize-derived viewport,
    # native FAT12 persistence, mouse caret placement, and guarded close.
    for fragment in (
        "gui_render_editor:",
        "gui_editor_visible_columns: .long 0",
        "gui_editor_visible_rows: .long 0",
        "call calculate_text_position",
        "gui_editor_click:",
        "call editor_begin_save_as",
        "jb editor_save",
        "call start_editor",
    ):
        require(shell_source, fragment)
    for fragment in (
        "cmpb $0, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)",
        "movb $4, (KERNEL_LOAD_ADDRESS + editor_save_status - _start)",
        "movl $1, %eax\n    ret\n\nwrite_no_space:",
    ):
        require(kernel_source, fragment)
    editor_repaint = kernel_source[
        kernel_source.index("render_editor:"):
        kernel_source.index("calculate_text_position:")
    ]
    assert "call wm_render_window" in editor_repaint
    assert "call gui_save_terminal_state" in editor_repaint
    assert "call gui_restore_terminal_state" in editor_repaint
    assert "call wm_begin_composition" in editor_repaint
    assert "call wm_present_editor_region" in editor_repaint
    assert "call render_shell" not in editor_repaint

    # Whole-desktop updates compose in RAM and are presented as one complete
    # frame. Focused Writer repaint uses the same backbuffer with a bounded
    # region copy. Writer keeps compact inline style and paragraph controls.
    assert FRAMEBUFFER_BACKING_END == 0x400000
    for fragment in (
        ".equ WM_BACKBUFFER_ADDRESS, 0x100000",
        ".equ WM_FRAME_DWORDS, 786432",
        "call wm_begin_composition",
        "call wm_present_composition",
        "rep movsl",
        "wm_physical_framebuffer: .long 0",
        "wm_present_editor_region:",
        "gui_editor_bold_label: .asciz \"B\"",
        "gui_editor_italic_label: .asciz \"I\"",
        "gui_editor_underline_label: .asciz \"U\"",
        "gui_editor_left_label: .asciz \"L\"",
        "gui_editor_center_label: .asciz \"C\"",
        "gui_editor_right_label: .asciz \"R\"",
        "gui_editor_justify_label: .asciz \"J\"",
        "gui_editor_prepare_line:",
        "gui_editor_selection_cell_ready:",
        "gui_editor_mouse_drag:",
        "call draw_character_styled",
        "xorb $EDITOR_STYLE_BOLD",
        "xorb $EDITOR_STYLE_ITALIC",
        "xorb $EDITOR_STYLE_UNDERLINE",
    ):
        require(shell_source, fragment)
    for fragment in (
        "#define EDITOR_MARK_BOLD 2",
        "#define EDITOR_MARK_ITALIC 3",
        "#define EDITOR_MARK_UNDERLINE 4",
        "#define EDITOR_MARK_ALIGN_LEFT 5",
        "#define EDITOR_MARK_ALIGN_JUSTIFY 8",
        "editor_toggle_bold:",
        "editor_toggle_italic:",
        "editor_toggle_underline:",
        "editor_select_all:",
        "editor_apply_format:",
        "editor_apply_alignment:",
        "editor_newline_insert:",
        "addb $EDITOR_MARK_ALIGN_LEFT, %al",
        "movb (KERNEL_LOAD_ADDRESS + editor_pending_marker - _start), %bl",
        "editor_selection_anchor: .long -1",
        "editor_name_append_text:",
        "movb $'T', (KERNEL_LOAD_ADDRESS + editor_save_as_name + 1 - _start)(,%ecx)",
        "draw_character_styled:",
        "movb $0, (KERNEL_LOAD_ADDRESS + editor_save_as_name - _start)",
        "cmpb $EDITOR_MARK_BOLD, %al\n    je emit_complete",
    ):
        require(kernel_source, fragment)

    # A mouse selection starts at the cell that was actually pressed, not at
    # the previous keyboard caret. Resolving the pointer must precede anchoring.
    editor_click = shell_source[
        shell_source.index("gui_editor_click_text:"):
        shell_source.index("gui_editor_pointer_to_cursor:")
    ]
    assert editor_click.index("call gui_editor_pointer_to_cursor") < editor_click.index(
        "movl %eax, (KERNEL_LOAD_ADDRESS + editor_selection_anchor - _start)"
    )

    # FileHound is a native FAT12-root adaptation: one compact pane, aligned
    # metadata columns, category markers, honest paging, and the real existing
    # file operations. Every row follows the translated window coordinates.
    for fragment in (
        "gui_window_files_caption: .asciz \"FILEHOUND // M.I.L.O\"",
        "gui_files_label: .asciz \"FILEHOUND\"",
        "gui_files_edition: .asciz \"M.I.L.O FAT12 EDITION\"",
        "gui_location_root: .asciz \"A:\\\\\"",
        "gui_name_label: .asciz \"NAME\"",
        "gui_type_label: .asciz \"TYPE\"",
        "gui_size_label: .asciz \"SIZE\"",
        "gui_file_type_for_entry:",
        "gui_type_text: .asciz \"TEXT\"",
        "gui_type_data: .asciz \"DATA\"",
        "gui_type_image: .asciz \"IMAGE\"",
        "gui_type_file: .asciz \"FILE\"",
        "addl $192, %eax",
        "subl $192, %eax",
        "gui_prepare_file_layout:",
        "gui_file_row_capacity: .long GUI_FILE_ROWS",
        "cmpl (KERNEL_LOAD_ADDRESS + gui_file_status_y - _start), %edx",
        "addl (KERNEL_LOAD_ADDRESS + gui_file_row_capacity - _start), %eax",
        "call gui_copy_render_name",
        "gui_render_name: .space 13",
        "call gui_draw_classic_button",
        "gui_keyboard_file_previous:",
        "gui_keyboard_file_next:",
    ):
        require(shell_source, fragment)
    browser = shell_source[
        shell_source.index("gui_render_file_browser:"):
        shell_source.index("gui_build_file_page:")
    ]
    assert "BOOT_CURSOR_X" not in browser
    assert "BOOT_CURSOR_Y" not in browser
    assert "call print_directory_name" not in browser

    # F1 exposes the full application menu without a pointer. Arrow keys,
    # Enter, and Escape operate that menu; FileHound also supports keyboard
    # selection, paging, and opening.
    for fragment in (
        "cmpb $0x3b, %al\n    je keyboard_menu_toggle",
        "cmpb $0, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)",
        "keyboard_menu_key:",
        "keyboard_menu_extended:",
        "call wm_keyboard_toggle_menu",
        "call wm_keyboard_close_menu",
        "call wm_keyboard_menu_move",
        "call wm_keyboard_menu_activate",
        "keyboard_filehound_key:",
        "keyboard_filehound_extended:",
        "call gui_keyboard_file_previous",
        "call gui_keyboard_file_next",
        "call gui_action_open",
        "cmpb $0xb8, %al\n    je keyboard_alt_released",
        "cmpb $0x38, %al\n    je keyboard_alt_pressed",
        "cmpb $0x3e, %al\n    je keyboard_window_close",
        "cmpb $0x43, %al\n    je keyboard_window_minimize",
        "call wm_keyboard_close_active",
        "call wm_keyboard_minimize_active",
        "alt_pressed: .byte 0",
    ):
        require(kernel_source, fragment)
    for fragment in (
        "wm_keyboard_minimize_active:",
        "wm_keyboard_close_active:",
        "cmpl $WM_COUNT, %eax\n    jae wm_keyboard_window_complete",
    ):
        require(shell_source, fragment)

    # Pixel Studio edits compact M16 V1 in a dedicated fixed workspace. Its
    # undo and fill buffers are bounded and cannot overwrite Writer.
    for fragment in (
        ".equ WM_IMAGE, 5",
        "gui_render_image_viewer:",
        "gui_entry_is_m16:",
        "gui_entry_is_external_image:",
        "image_open_default:",
        "image_open_entry:",
        "image_validate:",
        "cmpl $0x3631494d, IMAGE_BUFFER_ADDRESS",
        "cmpb $1, IMAGE_BUFFER_ADDRESS + 4",
        "cmpl $128, %eax",
        "cmpl $96, %eax",
        "image_palette_colors: .space 16 * 4",
        "gui_image_invalid: .asciz \"INVALID OR TRUNCATED M16 IMAGE\"",
        "gui_image_unsupported: .asciz \"UNSUPPORTED IMAGE FORMAT OR DIMENSIONS\"",
        "gui_image_load_failed: .asciz \"FAT12 IMAGE READ FAILED\"",
        "image_apply_tool:",
        "image_paint_current_pixel:",
        "image_flood_fill:",
        "image_draw_line:",
        "image_draw_rectangle:",
        "image_begin_undo:",
        "image_undo:",
        "image_new:",
        "image_new_commit:",
        "gui_image_render_new_picker:",
        "gui_image_new_picker_click:",
        "image_new_move_selection:",
        ".equ IMAGE_NEW_CHOICES, 4",
        "image_new_width_table: .long 32, 64, 96, 128",
        "image_new_height_table: .long 32, 48, 64, 96",
        "image_new_mode: .byte 0",
        "image_new_selection: .byte 1",
        "image_save:",
        "image_begin_save_as:",
        "image_name_append_extension:",
        "movb $'M', (KERNEL_LOAD_ADDRESS + image_save_as_name + 1 - _start)(,%ecx)",
        "image_exit:",
        "image_mouse_painting: .byte 0",
        "image_default_palette:",
    ):
        require(shell_source, fragment)
    status_dword_guard = "cmpl $0, (KERNEL_LOAD_ADDRESS + image_status - _start)"
    status_byte_guard = "cmpb $0, (KERNEL_LOAD_ADDRESS + image_status - _start)"
    assert status_dword_guard not in shell_source
    assert shell_source.count(status_byte_guard) >= 4
    for fragment in (
        "load_entry_to_image_buffer:",
        "movl $IMAGE_BUFFER_ADDRESS, %edi",
        "cmpl $IMAGE_LIMIT, %eax",
        "#define IMAGE_UNDO_ADDRESS 0x36000",
        "#define IMAGE_FILL_QUEUE_ADDRESS 0x38000",
        "keyboard_image_key:",
        "keyboard_image_extended:",
        "call image_handle_character",
    ):
        require(kernel_source, fragment)

    # The Conky-like root overlay reports real native state. Storage is counted
    # from FAT12, activity is sampled from real input events, applications use
    # their true window flags, and unsupported thermal data remains unavailable.
    for fragment in (
        "wm_render_system_overlay:",
        "gui_overlay_collect:",
        "movl BOOT_MEMORY_MIB(%ebp), %eax",
        "movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax",
        "call get_fat12_entry",
        "movl BOOT_ROOT_ADDRESS(%ebp), %edi",
        "gui_overlay_unavailable: .asciz \"N/A\"",
        "gui_overlay_cpu_mode: .asciz \"I386 // POLL\"",
        "system_event_count: .long 0",
        "wm_render_activity_graph:",
        ".equ WM_ACTIVITY_GRAPH_MAX, 120",
        "cmpl $WM_ACTIVITY_GRAPH_MAX, %eax",
        "imull $3, %eax\n    shrl $3, %eax",
        "gui_overlay_activity: .asciz \"EVENT RATE\"",
        "gui_overlay_graph_scale: .asciz \"0..120+ EV/S\"",
        "activity_history: .space WM_ACTIVITY_SAMPLES",
        "wm_sample_activity:",
        "wm_count_application_states:",
        "wm_render_application_state:",
        "gui_overlay_app_minimized: .asciz \"MINIMIZED\"",
    ):
        require(shell_source, fragment)

    # Time is read from the CMOS RTC, normalized from BCD/12-hour firmware when
    # required, and refreshed from the existing polling loop once per second.
    for fragment in (
        "call initialize_mouse\n    call render_shell",
        "keyboard_wait:\n    call rtc_poll_update",
        "rtc_initialize:",
        "rtc_poll_update:",
        "rtc_read_snapshot:",
        "rtc_bcd_to_binary:",
        "rtc_format_display:",
        "outb %al, $0x70",
        "inb $0x71, %al",
        "testb $0x80, %al",
        "testb $0x04, (KERNEL_LOAD_ADDRESS + rtc_register_b - _start)",
        "testb $0x02, (KERNEL_LOAD_ADDRESS + rtc_register_b - _start)",
        "cmpb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_drag_window - _start)",
        "cmpb $WM_NONE, (KERNEL_LOAD_ADDRESS + wm_resize_window - _start)",
        "call wm_render_live_update",
    ):
        require(kernel_source if fragment.startswith("call initialize_mouse") or
                fragment.startswith("keyboard_wait:") else shell_source, fragment)

    # Terminal rows and columns follow the resized client while retaining the
    # fixed 100x25 logical backing store and clipping hidden cells on repaint.
    for fragment in (
        "terminal_visible_columns: .long TERMINAL_COLUMNS",
        "terminal_visible_rows: .long TERMINAL_ROWS",
        "movl (KERNEL_LOAD_ADDRESS + terminal_visible_columns - _start), %ecx",
        "movl (KERNEL_LOAD_ADDRESS + terminal_visible_rows - _start), %edx",
        "cmpl (KERNEL_LOAD_ADDRESS + terminal_visible_rows - _start), %eax",
        "cmpl (KERNEL_LOAD_ADDRESS + terminal_visible_columns - _start), %edx",
    ):
        require(kernel_source, fragment)

    # Each compact menu row maps to one independent application.
    for boundary, target in (
        (60, "wm_open_home"),
        (88, "wm_open_files"),
        (116, "wm_open_traits"),
        (144, "wm_open_terminal"),
        (172, "wm_open_editor"),
        (200, "wm_open_image"),
    ):
        require(shell_source, "cmpl $%d, %%edx\n    jb %s" % (boundary, target))

    shape = re.search(
        r"mouse_shape:\s*(.*?)\n\n\.align 4", mouse_source, re.DOTALL
    )
    assert shape
    words = re.findall(r"0x[0-9a-fA-F]+", shape.group(1))
    assert len(words) == CURSOR_HEIGHT, len(words)
    assert all(int(word, 16) < (1 << CURSOR_WIDTH) for word in words)
    for fragment in (
        "movb $0xa8, %al",
        "movb $0xf6",
        "movb $0xf4",
        "testb $0x08, %al",
        "cmpl $3, %ecx",
        "testl $2, %eax",
        "testl $2, %edx",
        "call gui_handle_root_click",
        "call gui_handle_pointer_motion",
        "call gui_handle_click",
        "call gui_handle_drag",
        "call gui_handle_release",
        "testb $0x10, %bl",
        "testb $0x20, %bl",
        "movl $MOUSE_BACKING_ADDRESS, %edi",
        "movl $MOUSE_BACKING_ADDRESS, %esi",
        "movl $MOUSE_NEXT_BACKING_ADDRESS, %esi",
        "movl $MOUSE_NEXT_BACKING_ADDRESS, %edi",
        "draw_mouse_cursor_at_next:",
        "mouse_next_row_address: .long 0",
    ):
        require(mouse_source, fragment)
    assert "testb $0xc0, %bl" not in mouse_source

    # The old cursor remains visible through all packet decoding. A fast,
    # non-overlapping motion draws the staged cursor into a second saved-under
    # buffer before restoring the old one, so the screen never contains a
    # deliberate blank-pointer frame. Overlapping/button motion retains the
    # conservative staged commit, and every path redraws before returning.
    require(
        mouse_source,
        "cmpl $3, %ecx\n    jb mouse_packet_incomplete\n"
        "    /* Keep the old cursor visible while the complete packet is decoded.",
    )
    complete_packet = mouse_source[mouse_source.index("cmpl $3, %ecx"):
                                   mouse_source.index("mouse_packet_incomplete:")]
    assert "call hide_mouse_cursor" not in complete_packet
    require(mouse_source, "mouse_next_x: .long 512")
    require(mouse_source, "mouse_next_y: .long 384")
    require(
        mouse_source,
        "mouse_commit_no_blank:\n"
        "    call draw_mouse_cursor_at_next\n"
        "    call hide_mouse_cursor\n"
        "    movl $MOUSE_NEXT_BACKING_ADDRESS, %esi\n"
        "    movl $MOUSE_BACKING_ADDRESS, %edi\n"
        "    movl $192, %ecx\n"
        "    rep movsl",
    )
    require(
        mouse_source,
        "call hide_mouse_cursor\n"
        "    movl (KERNEL_LOAD_ADDRESS + mouse_next_x - _start), %eax",
    )
    require(
        mouse_source,
        "mouse_packet_complete:\n"
        "    /* Redraw before returning to the polling loop.",
    )
    packet_complete = mouse_source[mouse_source.index("mouse_packet_complete:"):
                                   mouse_source.index("draw_mouse_cursor:")]
    assert "call draw_mouse_cursor" in packet_complete
    keyboard_mouse = kernel_source[kernel_source.index("keyboard_mouse_byte:"):
                                   kernel_source.index("keyboard_extended_prefix:")]
    assert "hide_mouse_cursor" not in keyboard_mouse
    require(kernel_source, "incl (KERNEL_LOAD_ADDRESS + system_event_count - _start)")

    print(
        "M.I.L.O V0.30.1 desktop contract: %d-byte kernel, no-blank pointer, "
        "keyboard windows, Writer, FileHound, native M16 Pixel Studio OK"
        % len(kernel)
    )


if __name__ == "__main__":
    main()

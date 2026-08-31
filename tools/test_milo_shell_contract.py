#!/usr/bin/env python3
"""Static and binary checks for the V0.26.2 polished native desktop."""

from pathlib import Path
import re
import sys


KERNEL_LIMIT = 32 * 1024
CURSOR_WIDTH = 12
CURSOR_HEIGHT = 16
CURSOR_BACKING = 0x32400
CURSOR_BACKING_END = CURSOR_BACKING + CURSOR_WIDTH * CURSOR_HEIGHT * 4
TERMINAL_BUFFER = 0x32800
TERMINAL_BUFFER_END = TERMINAL_BUFFER + 100 * 25


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
    assert "#define GUI_FILE_ROWS 10" in kernel_source
    assert "#define MOUSE_MAX_X 1012" in kernel_source
    assert "#define MOUSE_MAX_Y 752" in kernel_source
    assert CURSOR_BACKING_END == 0x32700
    assert CURSOR_BACKING_END < TERMINAL_BUFFER
    assert TERMINAL_BUFFER_END < 0x34000
    require(
        kernel_source,
        "call terminal_buffer_store_character\n    call draw_character",
    )
    require(
        kernel_source,
        "movb %al, (KERNEL_LOAD_ADDRESS + editor_render_character - _start)",
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
        b"RIGHT CLICK // APPS",
        b"M.I.L.O VERSION 0.26.2",
    ):
        assert marker in kernel, marker

    # Startup is a root desktop with a lightweight system overlay. No permanent
    # launcher, taskbar, dashboard application, or ordinary window is visible.
    for fragment in (
        ".equ WM_COUNT, 4",
        ".equ WM_ROOT_MENU_WIDTH, 232",
        ".equ WM_ROOT_MENU_HEIGHT, 148",
        ".equ WM_ROOT_MENU_ROW, 28",
        "movl $1024, %ecx\n    movl $768, %edx",
        "wm_flags: .byte 0, 0, 0, 0",
        "wm_active: .byte WM_NONE",
        "call wm_render_system_overlay\n    call wm_render_all_windows",
        "call wm_render_all_windows",
        "call wm_render_root_menu",
        "wm_root_menu_title: .asciz \"M.I.L.O APPLICATIONS\"",
        "gui_draw_menu_item:",
        "gui_handle_root_click:",
        "movb $1, (KERNEL_LOAD_ADDRESS + wm_menu_open - _start)",
        "cmpl $(1024 - WM_ROOT_MENU_WIDTH), %eax",
        "cmpl $(768 - WM_ROOT_MENU_HEIGHT), %eax",
        "gui_handle_pointer_motion:",
        "wm_menu_hover: .byte WM_NONE",
        "cmpl %edi, %eax\n    jne gui_menu_item_idle",
    ):
        require(shell_source, fragment)
    for obsolete in (
        "WM_TASKBAR_Y",
        "wm_render_taskbar",
        "wm_render_desktop_icons",
        "wm_render_start_menu",
        "gui_taskbar_status",
        "gui_milo_button_label",
    ):
        assert obsolete not in shell_source, obsolete

    # Four independent native application windows retain focus, stacking,
    # movement, controls, persistent terminal state, and real file operations.
    for fragment in (
        ".equ WM_FLAG_VISIBLE, 1",
        ".equ WM_FLAG_MINIMIZED, 2",
        ".equ WM_FLAG_MAXIMIZED, 4",
        "wm_x: .long 64, 104, 144, 96",
        "wm_y: .long 64, 92, 120, 240",
        "wm_w: .long 832, 832, 832, 832",
        "wm_h: .long 456, 456, 456, 456",
        "wm_z_order: .byte WM_HOME, WM_FILES, WM_TRAITS, WM_TERMINAL",
        "call wm_bring_to_front",
        "wm_minimize_window:",
        "wm_close_window:",
        "wm_toggle_maximize:",
        "movl $0, (KERNEL_LOAD_ADDRESS + wm_y - _start)(,%eax,4)",
        "movl $1024, (KERNEL_LOAD_ADDRESS + wm_w - _start)(,%eax,4)",
        "movl $768, (KERNEL_LOAD_ADDRESS + wm_h - _start)(,%eax,4)",
        "wm_xor_drag_outline:",
        "call wm_reposition_terminal_cursor",
        "call render_terminal_buffer",
        "subl (KERNEL_LOAD_ADDRESS + wm_x + WM_FILES * 4 - _start), %eax",
        "subl (KERNEL_LOAD_ADDRESS + wm_y + WM_FILES * 4 - _start), %edx",
        "movb $0, (KERNEL_LOAD_ADDRESS + terminal_capture_enabled - _start)",
        "call gui_input_is_yes\n    testl %eax, %eax\n    jz gui_action_cancelled",
        "GUI ACTION CANCELLED. NO FILES CHANGED.",
    ):
        require(shell_source, fragment)

    # Desktop strings use a screen-clipped GUI emitter. They must never inherit
    # terminal bounds or terminal capture, which previously wrapped the right
    # overlay and window contents back to the left edge.
    for fragment in (
        "gui_print_raw:",
        "cmpl $752, BOOT_CURSOR_Y(%ebp)",
        "cmpl $1016, BOOT_CURSOR_X(%ebp)",
        "call draw_character",
        "gui_decimal_next:",
        "call gui_print_raw",
    ):
        require(shell_source, fragment)
    gui_text = shell_source[
        shell_source.index("gui_print_at:"):
        shell_source.index("gui_save_terminal_state:")
    ]
    assert "call print_string" not in gui_text
    assert "call print_u32" not in gui_text

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
        "gui_type_file: .asciz \"FILE\"",
        "addl $192, %eax",
        "subl $192, %eax",
        "cmpl $432, %edx",
        "call gui_copy_render_name",
        "gui_render_name: .space 13",
        "call gui_draw_classic_button",
    ):
        require(shell_source, fragment)
    browser = shell_source[
        shell_source.index("gui_render_file_browser:"):
        shell_source.index("gui_build_file_page:")
    ]
    assert "BOOT_CURSOR_X" not in browser
    assert "BOOT_CURSOR_Y" not in browser
    assert "call print_directory_name" not in browser

    # The Conky-like root overlay reports real native state. Storage is counted
    # from FAT12 and unsupported thermal data is stated as unavailable.
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
    ):
        require(shell_source, fragment)

    # Each compact menu row maps to one independent application.
    for boundary, target in (
        (60, "wm_open_home"),
        (88, "wm_open_files"),
        (116, "wm_open_traits"),
        (144, "wm_open_terminal"),
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
    ):
        require(mouse_source, fragment)
    assert "testb $0xc0, %bl" not in mouse_source

    # Cursor backing is erased only after byte three completes a packet. The
    # first two PS/2 bytes leave the visible cursor untouched, eliminating the
    # old three-redraw flicker for each physical movement packet.
    require(
        mouse_source,
        "cmpl $3, %ecx\n    jb mouse_packet_incomplete\n"
        "    /* Keep the cursor visible while bytes one and two arrive.",
    )
    complete_packet = mouse_source[mouse_source.index("cmpl $3, %ecx"):
                                   mouse_source.index("mouse_packet_incomplete:")]
    assert complete_packet.count("call hide_mouse_cursor") == 1
    keyboard_mouse = kernel_source[kernel_source.index("keyboard_mouse_byte:"):
                                   kernel_source.index("keyboard_extended_prefix:")]
    assert "hide_mouse_cursor" not in keyboard_mouse
    require(kernel_source, "incl (KERNEL_LOAD_ADDRESS + system_event_count - _start)")

    print(
        "M.I.L.O polished desktop contract: %d-byte kernel, isolated GUI text, "
        "native status overlay, FileHound, packet-stable cursor, four windows OK"
        % len(kernel)
    )


if __name__ == "__main__":
    main()

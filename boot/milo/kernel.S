.code32
.section .text
.global _start

#define KERNEL_LOAD_ADDRESS 0x10000
#define BOOT_FRAMEBUFFER 0
#define BOOT_PITCH 4
#define BOOT_FONT 8
#define BOOT_COLOR 12
#define BOOT_CURSOR_X 16
#define BOOT_CURSOR_Y 20
#define BOOT_MEMORY_MIB 24
#define BOOT_FAT_ADDRESS 28
#define BOOT_ROOT_ADDRESS 32
#define BOOT_DATA_ADDRESS 36
#define BOOT_TOTAL_DATA_CLUSTERS 40
#define BOOT_WRITE_SECTOR 44
#define BOOT_READ_SECTOR 48
#define BOOT_CACHED_DATA_CLUSTERS 52
#define FAT1_LBA 82
#define FAT2_LBA 91
#define ROOT_LBA 100
#define DATA_LBA 114
#define TERMINAL_COLUMNS 100
#define TERMINAL_ROWS 25
#define TERMINAL_BUFFER_ADDRESS 0x32800
#define INPUT_LIMIT 63
#define HISTORY_CAPACITY 30
#define HISTORY_ENTRY_SIZE 64
#define EDITOR_BUFFER_ADDRESS 0x30000
#define EDITOR_LIMIT 8191
#define DISK_BUFFER_ADDRESS 0x32000
#define VERIFY_BUFFER_ADDRESS 0x32200
#define MILO_MEMORY_SIZE 24
#define MILO_CHECKSUM_OFFSET 23
#define ROUTE_HELP 1
#define ROUTE_ABOUT 2
#define ROUTE_MEMORY 3
#define ROUTE_VERSION 4
#define ROUTE_DIRECTORY 5
#define ROUTE_DISK 6
#define ROUTE_TRAITS 7
#define ROUTE_READ 8
#define ROUTE_EDIT 9
#define ROUTE_FIND 10
#define ROUTE_SHOW 11
#define ROUTE_TELL 12
#define ROUTE_CLEAR 13
#define GUARD_DELETE 1
#define GUARD_RENAME 2
#define GUARD_WRITE 3
#define GUARD_COPY 4
#define GUI_ACTION_NONE 0
#define GUI_ACTION_COPY 1
#define GUI_ACTION_RENAME 2
#define GUI_ACTION_DELETE 3
#define GUI_FILE_ROWS 10
#define MOUSE_BACKING_ADDRESS 0x32400
#define MOUSE_MAX_X 1012
#define MOUSE_MAX_Y 752

/* Deterministic M.I.L.O command kernel with a scrolling PS/2 terminal. */
_start:
    cld
    movl %esi, %ebp
    call milo_memory_load
    call wm_initialize
    call clear_terminal
    movl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
    movl $(KERNEL_LOAD_ADDRESS + kernel_message - _start), %esi
    call print_string
    call terminal_newline
    call print_prompt
    call initialize_mouse
    call render_shell

keyboard_poll:
    call draw_mouse_cursor
keyboard_wait:
    call rtc_poll_update
    inb $0x64, %al
    testb $0x01, %al
    jz keyboard_wait
    testb $0x20, %al
    jnz keyboard_mouse_byte
    call hide_mouse_cursor
    inb $0x60, %al
    incl (KERNEL_LOAD_ADDRESS + system_event_count - _start)
    pushl %eax
    call wm_focus_terminal_for_input
    popl %eax

    cmpb $0xe0, %al
    je keyboard_extended_prefix
    cmpb $0xaa, %al
    je keyboard_shift_released
    cmpb $0xb6, %al
    je keyboard_shift_released
    cmpb $0x9d, %al
    je keyboard_control_released

    cmpb $0, (KERNEL_LOAD_ADDRESS + extended_prefix - _start)
    jne keyboard_extended
    testb $0x80, %al
    jnz keyboard_poll
    cmpb $0x2a, %al
    je keyboard_shift_pressed
    cmpb $0x36, %al
    je keyboard_shift_pressed
    cmpb $0x1d, %al
    je keyboard_control_pressed
    cmpb $0x01, %al
    je keyboard_escape
    cmpb $0x3a, %al
    je keyboard_caps_lock
    cmpb $0x0e, %al
    je keyboard_backspace
    cmpb $0x1c, %al
    je keyboard_enter
    /* Accept keypad-style navigation codes when firmware omits E0. */
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    je keyboard_test_character
    cmpb $0x4b, %al
    je editor_cursor_left
    cmpb $0x4d, %al
    je editor_cursor_right
    cmpb $0x48, %al
    je editor_cursor_up
    cmpb $0x50, %al
    je editor_cursor_down
    cmpb $0x47, %al
    je editor_cursor_home
    cmpb $0x4f, %al
    je editor_cursor_end
    cmpb $0x53, %al
    je editor_delete_current
    cmpb $0x49, %al
    je editor_page_up
    cmpb $0x51, %al
    je editor_page_down
keyboard_test_character:
    cmpb $57, %al
    ja keyboard_poll

    movzbl %al, %edx
    movzbl (KERNEL_LOAD_ADDRESS + scancode_table - _start)(,%edx), %eax
    cmpb $'a', %al
    jb keyboard_non_letter
    cmpb $'z', %al
    ja keyboard_non_letter
    movb (KERNEL_LOAD_ADDRESS + shift_pressed - _start), %cl
    xorb (KERNEL_LOAD_ADDRESS + caps_enabled - _start), %cl
    testb %cl, %cl
    jz keyboard_character_ready
    subb $32, %al
    jmp keyboard_character_ready

keyboard_non_letter:
    cmpb $0, (KERNEL_LOAD_ADDRESS + shift_pressed - _start)
    je keyboard_character_ready
    movzbl (KERNEL_LOAD_ADDRESS + shifted_scancode_table - _start)(,%edx), %eax

keyboard_character_ready:
    testb %al, %al
    jz keyboard_poll
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    je terminal_character_ready
    cmpb $0, (KERNEL_LOAD_ADDRESS + control_pressed - _start)
    je editor_character
    cmpb $'s', %al
    je editor_save
    cmpb $'S', %al
    je editor_save
    jmp keyboard_poll

terminal_character_ready:
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
    cmpl $INPUT_LIMIT, %ecx
    jae keyboard_poll
    pushl %eax
    call hide_cursor
    popl %eax
    movb $0, (KERNEL_LOAD_ADDRESS + history_view - _start)
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
    movb %al, (KERNEL_LOAD_ADDRESS + input_buffer - _start)(,%ecx)
    incl %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + input_length - _start)
    call terminal_buffer_store_character
    call draw_character
    addl $8, BOOT_CURSOR_X(%ebp)
    call draw_cursor
    jmp keyboard_poll

keyboard_mouse_byte:
    inb $0x60, %al
    call handle_mouse_byte
    jmp keyboard_poll

keyboard_extended_prefix:
    movb $1, (KERNEL_LOAD_ADDRESS + extended_prefix - _start)
    jmp keyboard_poll

keyboard_extended:
    movb $0, (KERNEL_LOAD_ADDRESS + extended_prefix - _start)
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne keyboard_editor_extended
    cmpb $0x48, %al
    je keyboard_history_up
    cmpb $0x50, %al
    je keyboard_history_down
    jmp keyboard_poll

keyboard_editor_extended:
    cmpb $0x4b, %al
    je editor_cursor_left
    cmpb $0x4d, %al
    je editor_cursor_right
    cmpb $0x48, %al
    je editor_cursor_up
    cmpb $0x50, %al
    je editor_cursor_down
    cmpb $0x47, %al
    je editor_cursor_home
    cmpb $0x4f, %al
    je editor_cursor_end
    cmpb $0x53, %al
    je editor_delete_current
    cmpb $0x49, %al
    je editor_page_up
    cmpb $0x51, %al
    je editor_page_down
    jmp keyboard_poll

keyboard_shift_pressed:
    movb $1, (KERNEL_LOAD_ADDRESS + shift_pressed - _start)
    jmp keyboard_poll

keyboard_shift_released:
    movb $0, (KERNEL_LOAD_ADDRESS + shift_pressed - _start)
    jmp keyboard_poll

keyboard_control_pressed:
    movb $1, (KERNEL_LOAD_ADDRESS + control_pressed - _start)
    jmp keyboard_poll

keyboard_control_released:
    movb $0, (KERNEL_LOAD_ADDRESS + control_pressed - _start)
    jmp keyboard_poll

keyboard_escape:
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne keyboard_escape_editor
    cmpb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    je keyboard_poll
    call gui_cancel_pending_action
    jmp keyboard_poll
keyboard_escape_editor:
    call editor_exit
    jmp keyboard_poll

keyboard_caps_lock:
    xorb $1, (KERNEL_LOAD_ADDRESS + caps_enabled - _start)
    jmp keyboard_poll

keyboard_backspace:
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne editor_backspace
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
    testl %ecx, %ecx
    jz keyboard_poll
    call hide_cursor
    movb $0, (KERNEL_LOAD_ADDRESS + history_view - _start)
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
    decl %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + input_length - _start)
    subl $8, BOOT_CURSOR_X(%ebp)
    call terminal_buffer_clear_current
    call clear_character_cell
    call draw_cursor
    jmp keyboard_poll

keyboard_enter:
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne editor_newline
    call hide_cursor
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
    movb $0, (KERNEL_LOAD_ADDRESS + input_buffer - _start)(,%ecx)
    testl %ecx, %ecx
    jz execute_command
    cmpb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    jne execute_command
    call save_history

execute_command:
    call terminal_newline
    cmpb $GUI_ACTION_NONE, (KERNEL_LOAD_ADDRESS + gui_pending_action - _start)
    jne execute_gui_action
    call dispatch_command
    jmp execute_command_complete
execute_gui_action:
    call gui_complete_pending_action
execute_command_complete:
    movl $0, (KERNEL_LOAD_ADDRESS + input_length - _start)
    call render_shell
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    jne keyboard_poll
    call print_prompt
    call draw_cursor
    jmp keyboard_poll

keyboard_history_up:
    movzbl (KERNEL_LOAD_ADDRESS + history_count - _start), %eax
    testl %eax, %eax
    jz keyboard_poll
    movzbl (KERNEL_LOAD_ADDRESS + history_view - _start), %ecx
    cmpl %eax, %ecx
    jae keyboard_poll
    incl %ecx
    movb %cl, (KERNEL_LOAD_ADDRESS + history_view - _start)
    call load_history_view
    jmp keyboard_poll

keyboard_history_down:
    movzbl (KERNEL_LOAD_ADDRESS + history_view - _start), %ecx
    testl %ecx, %ecx
    jz keyboard_poll
    decl %ecx
    movb %cl, (KERNEL_LOAD_ADDRESS + history_view - _start)
    call load_history_view
    jmp keyboard_poll

load_history_view:
    call hide_cursor
    call clear_input_text
    movzbl (KERNEL_LOAD_ADDRESS + history_view - _start), %ecx
    testl %ecx, %ecx
    jz history_view_empty
    movzbl (KERNEL_LOAD_ADDRESS + history_head - _start), %eax
    subl %ecx, %eax
    jge history_index_ready
    addl $HISTORY_CAPACITY, %eax
history_index_ready:
    movzbl (KERNEL_LOAD_ADDRESS + history_lengths - _start)(,%eax), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + input_length - _start)
    shll $6, %eax
    addl $(KERNEL_LOAD_ADDRESS + history_buffer - _start), %eax
    movl %eax, %esi
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %edi
    rep movsb
    movb $0, (%edi)
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    call print_string
    call draw_cursor
    ret
history_view_empty:
    movl $0, (KERNEL_LOAD_ADDRESS + input_length - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + input_buffer - _start)
    call draw_cursor
    ret

/* Save non-empty input into the 30-entry circular history. */
save_history:
    movzbl (KERNEL_LOAD_ADDRESS + history_head - _start), %eax
    movl %eax, %edx
    shll $6, %eax
    addl $(KERNEL_LOAD_ADDRESS + history_buffer - _start), %eax
    movl %eax, %edi
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
    movb %cl, (KERNEL_LOAD_ADDRESS + history_lengths - _start)(,%edx)
    incl %ecx
    rep movsb
    incl %edx
    cmpl $HISTORY_CAPACITY, %edx
    jb history_head_ready
    xorl %edx, %edx
history_head_ready:
    movb %dl, (KERNEL_LOAD_ADDRESS + history_head - _start)
    cmpb $HISTORY_CAPACITY, (KERNEL_LOAD_ADDRESS + history_count - _start)
    jae history_count_ready
    incb (KERNEL_LOAD_ADDRESS + history_count - _start)
history_count_ready:
    movb $0, (KERNEL_LOAD_ADDRESS + history_view - _start)
    ret

clear_input_text:
    movl (KERNEL_LOAD_ADDRESS + terminal_input_left - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + input_length - _start), %ecx
clear_input_next:
    testl %ecx, %ecx
    jz clear_input_complete
    pushl %ecx
    call terminal_buffer_clear_current
    call clear_character_cell
    popl %ecx
    addl $8, BOOT_CURSOR_X(%ebp)
    decl %ecx
    jmp clear_input_next
clear_input_complete:
    movl (KERNEL_LOAD_ADDRESS + terminal_input_left - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    ret

editor_character:
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ecx
    cmpl $EDITOR_LIMIT, %ecx
    jae keyboard_poll
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %edx
    movl %ecx, %edi
editor_insert_shift:
    movb EDITOR_BUFFER_ADDRESS(,%edi), %bl
    movb %bl, (EDITOR_BUFFER_ADDRESS + 1)(,%edi)
    cmpl %edx, %edi
    je editor_insert_ready
    decl %edi
    jmp editor_insert_shift
editor_insert_ready:
    movb %al, EDITOR_BUFFER_ADDRESS(,%edx)
    incl %ecx
    incl %edx
    movl %ecx, (KERNEL_LOAD_ADDRESS + editor_length - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
    call render_editor
    jmp keyboard_poll

editor_newline:
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ecx
    cmpl $EDITOR_LIMIT, %ecx
    jae keyboard_poll
    movb $'\n', %al
    jmp editor_character

editor_backspace:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %edx
    testl %edx, %edx
    jz keyboard_poll
    decl %edx
    movl %edx, %edi
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ecx
editor_delete_shift:
    movb (EDITOR_BUFFER_ADDRESS + 1)(,%edi), %al
    movb %al, EDITOR_BUFFER_ADDRESS(,%edi)
    incl %edi
    cmpl %ecx, %edi
    jb editor_delete_shift
    decl %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + editor_length - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
    call render_editor
    jmp keyboard_poll

editor_delete_current:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %edx
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ecx
    cmpl %ecx, %edx
    jae keyboard_poll
    movl %edx, %edi
editor_forward_delete_shift:
    movb (EDITOR_BUFFER_ADDRESS + 1)(,%edi), %al
    movb %al, EDITOR_BUFFER_ADDRESS(,%edi)
    incl %edi
    cmpl %ecx, %edi
    jb editor_forward_delete_shift
    decl %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + editor_length - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
    call render_editor
    jmp keyboard_poll

editor_cursor_home:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
editor_home_previous:
    testl %eax, %eax
    jz editor_home_set
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%eax)
    je editor_home_set
    decl %eax
    jmp editor_home_previous
editor_home_set:
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    call render_editor
    jmp keyboard_poll

editor_cursor_end:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ecx
editor_end_next:
    cmpl %ecx, %eax
    jae editor_end_set
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    je editor_end_set
    incl %eax
    jmp editor_end_next
editor_end_set:
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    call render_editor
    jmp keyboard_poll

editor_cursor_left:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    testl %eax, %eax
    jz keyboard_poll
    decl %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    call render_editor
    jmp keyboard_poll

editor_cursor_right:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    jae keyboard_poll
    incl %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    call render_editor
    jmp keyboard_poll

/* Move to the same logical column on the preceding newline-delimited row. */
editor_cursor_up:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    movl %eax, %edx
editor_up_find_start:
    testl %edx, %edx
    jz keyboard_poll
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%edx)
    je editor_up_start_found
    decl %edx
    jmp editor_up_find_start
editor_up_start_found:
    movl %eax, %ecx
    subl %edx, %ecx
    cmpl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    jne editor_up_column_ready
    movl %ecx, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
editor_up_column_ready:
    movl (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start), %ecx
    leal -1(%edx), %edi
    movl %edi, %eax
editor_up_find_previous:
    testl %eax, %eax
    jz editor_up_previous_found
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%eax)
    je editor_up_previous_found
    decl %eax
    jmp editor_up_find_previous
editor_up_previous_found:
    addl %eax, %ecx
    cmpl %edi, %ecx
    jbe editor_up_set
    movl %edi, %ecx
editor_up_set:
    movl %ecx, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    call render_editor
    jmp keyboard_poll

/* Move to the same logical column on the following newline-delimited row. */
editor_cursor_down:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    movl %eax, %edx
editor_down_find_start:
    testl %edx, %edx
    jz editor_down_start_found
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%edx)
    je editor_down_start_found
    decl %edx
    jmp editor_down_find_start
editor_down_start_found:
    subl %edx, %eax
    cmpl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    jne editor_down_column_ready
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
editor_down_column_ready:
    movl (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %edx
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %edi
editor_down_find_end:
    cmpl %edi, %edx
    jae keyboard_poll
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%edx)
    je editor_down_next_start
    incl %edx
    jmp editor_down_find_end
editor_down_next_start:
    incl %edx
    addl %edx, %eax
editor_down_limit:
    cmpl %edi, %eax
    jae editor_down_use_end
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    je editor_down_use_end
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    call render_editor
    jmp keyboard_poll
editor_down_use_end:
    movl %edx, %eax
editor_down_scan_end:
    cmpl %edi, %eax
    jae editor_down_set
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    je editor_down_set
    incl %eax
    jmp editor_down_scan_end
editor_down_set:
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    call render_editor
    jmp keyboard_poll

/* Move seven logical lines while retaining the current text column. */
editor_page_up:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    movl %eax, %edx
editor_page_up_find_start:
    testl %edx, %edx
    jz editor_page_up_column
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%edx)
    je editor_page_up_column
    decl %edx
    jmp editor_page_up_find_start
editor_page_up_column:
    movl %eax, %esi
    subl %edx, %esi
    movl $7, %ecx
editor_page_up_line:
    testl %edx, %edx
    jz editor_page_position
    decl %edx
editor_page_up_previous:
    testl %edx, %edx
    jz editor_page_up_ready
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%edx)
    je editor_page_up_ready
    decl %edx
    jmp editor_page_up_previous
editor_page_up_ready:
    decl %ecx
    jnz editor_page_up_line
    jmp editor_page_position

editor_page_down:
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    movl %eax, %edx
editor_page_down_find_start:
    testl %edx, %edx
    jz editor_page_down_column
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%edx)
    je editor_page_down_column
    decl %edx
    jmp editor_page_down_find_start
editor_page_down_column:
    subl %edx, %eax
    movl %eax, %esi
    movl $7, %ecx
editor_page_down_line:
    movl %edx, %eax
editor_page_down_find_end:
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    jae editor_page_position
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%eax)
    je editor_page_down_next
    incl %eax
    jmp editor_page_down_find_end
editor_page_down_next:
    leal 1(%eax), %edx
    decl %ecx
    jnz editor_page_down_line

editor_page_position:
    movl %edx, %ebx
editor_page_find_target_end:
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ebx
    jae editor_page_target_ready
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%ebx)
    je editor_page_target_ready
    incl %ebx
    jmp editor_page_find_target_end
editor_page_target_ready:
    leal (%edx,%esi), %eax
    cmpl %ebx, %eax
    jbe editor_page_set
    movl %ebx, %eax
editor_page_set:
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    call render_editor
    jmp keyboard_poll

editor_save:
    call hide_cursor
    movl $EDITOR_BUFFER_ADDRESS, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_content - _start)
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_length_override - _start)
    movl $(KERNEL_LOAD_ADDRESS + editor_filename - _start), %esi
    movb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    call clear_terminal
    call write_file
    call render_shell
    call print_prompt
    call draw_cursor
    jmp keyboard_poll

editor_exit:
    call hide_cursor
    movb $0, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    call clear_terminal
    movl $(KERNEL_LOAD_ADDRESS + editor_closed_message - _start), %esi
    call print_string
    movl $0, (KERNEL_LOAD_ADDRESS + input_length - _start)
    call print_prompt
    call draw_cursor
    call render_shell
    ret

render_editor:
    call clear_terminal
    movl $(KERNEL_LOAD_ADDRESS + editor_header - _start), %esi
    call print_string
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %eax
    call calculate_text_position
    movl $(KERNEL_LOAD_ADDRESS + editor_status_prefix - _start), %esi
    call print_string
    movl $(KERNEL_LOAD_ADDRESS + editor_filename - _start), %esi
    call print_string
    movl $(KERNEL_LOAD_ADDRESS + editor_status_size - _start), %esi
    call print_string
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + editor_status_line - _start), %esi
    call print_string
    movl (KERNEL_LOAD_ADDRESS + text_position_line - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + editor_status_column - _start), %esi
    call print_string
    movl (KERNEL_LOAD_ADDRESS + text_position_column - _start), %eax
    call print_u32
    cmpb $0, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
    je editor_status_clean
    movl $(KERNEL_LOAD_ADDRESS + editor_status_modified - _start), %esi
    call print_string
editor_status_clean:
    movl $(KERNEL_LOAD_ADDRESS + newline - _start), %esi
    call print_string
    /* Begin no more than seven logical rows before the cursor. */
    movl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %ecx
    xorl %edx, %edx
    xorl %edi, %edi
editor_view_find_start:
    testl %ecx, %ecx
    jz editor_render_next
    cmpb $'\n', (EDITOR_BUFFER_ADDRESS - 1)(,%ecx)
    jne editor_view_previous
    incl %edx
    cmpl $7, %edx
    jae editor_render_next
editor_view_previous:
    decl %ecx
    incl %edi
    cmpl $700, %edi
    jae editor_render_next
    jmp editor_view_find_start
editor_render_next:
    cmpl (KERNEL_LOAD_ADDRESS + editor_cursor - _start), %ecx
    jne editor_render_not_cursor
    movl BOOT_CURSOR_X(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_screen_x - _start)
    movl BOOT_CURSOR_Y(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_screen_y - _start)
editor_render_not_cursor:
    cmpl (KERNEL_LOAD_ADDRESS + editor_length - _start), %ecx
    jae editor_render_complete
    movb EDITOR_BUFFER_ADDRESS(,%ecx), %al
    movb %al, (KERNEL_LOAD_ADDRESS + editor_render_character - _start)
    movl (KERNEL_LOAD_ADDRESS + terminal_last_line - _start), %eax
    cmpl %eax, BOOT_CURSOR_Y(%ebp)
    jb editor_render_emit
    cmpb $'\n', (KERNEL_LOAD_ADDRESS + editor_render_character - _start)
    je editor_render_complete
    movl (KERNEL_LOAD_ADDRESS + terminal_right - _start), %eax
    cmpl %eax, BOOT_CURSOR_X(%ebp)
    ja editor_render_complete
editor_render_emit:
    movb (KERNEL_LOAD_ADDRESS + editor_render_character - _start), %al
    pushl %ecx
    call emit_character
    popl %ecx
    incl %ecx
    jmp editor_render_next
editor_render_complete:
    movl (KERNEL_LOAD_ADDRESS + editor_screen_x - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + editor_screen_y - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
    call draw_cursor
    ret

/* EAX=text offset. Store one-based logical line and column. */
calculate_text_position:
    movl %eax, %edi
    xorl %ecx, %ecx
    movl $1, %edx
    movl $1, %ebx
text_position_next:
    cmpl %edi, %ecx
    jae text_position_complete
    cmpb $'\n', EDITOR_BUFFER_ADDRESS(,%ecx)
    jne text_position_character
    incl %edx
    movl $1, %ebx
    jmp text_position_advance
text_position_character:
    incl %ebx
text_position_advance:
    incl %ecx
    jmp text_position_next
text_position_complete:
    movl %edx, (KERNEL_LOAD_ADDRESS + text_position_line - _start)
    movl %ebx, (KERNEL_LOAD_ADDRESS + text_position_column - _start)
    ret

dispatch_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $0, (%esi)
    je command_complete
    movl $(KERNEL_LOAD_ADDRESS + command_help - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_help
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_about - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_about
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_clear - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz do_clear
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_memory - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_memory
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_version - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_version
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_dir - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_directory
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_disk - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_disk
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_traits - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_traits
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_milo - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_milo_ready
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    movl $(KERNEL_LOAD_ADDRESS + command_find - _start), %edi
    call strings_equal
    testl %eax, %eax
    jnz show_find_usage

test_milo_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'m', 0(%esi)
    jne test_find_command
    cmpb $'i', 1(%esi)
    jne test_find_command
    cmpb $'l', 2(%esi)
    jne test_find_command
    cmpb $'o', 3(%esi)
    jne test_find_command
    cmpb $' ', 4(%esi)
    jne test_find_command
    addl $5, %esi
    cmpb $0, (%esi)
    je show_milo_ready
    call milo_interact
    ret

test_find_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'f', 0(%esi)
    jne test_delete_command
    cmpb $'i', 1(%esi)
    jne test_delete_command
    cmpb $'n', 2(%esi)
    jne test_delete_command
    cmpb $'d', 3(%esi)
    jne test_delete_command
    cmpb $' ', 4(%esi)
    jne test_delete_command
    /* One parser owns both FILE-first and natural query-first find syntax. */
    jmp test_natural_command

test_delete_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'d', 0(%esi)
    jne test_rename_command
    cmpb $'e', 1(%esi)
    jne test_rename_command
    cmpb $'l', 2(%esi)
    jne test_rename_command
    cmpb $' ', 3(%esi)
    jne test_rename_command
    addl $4, %esi
    cmpb $0, (%esi)
    je show_del_usage
    call delete_file
    ret

test_rename_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'r', 0(%esi)
    jne test_copy_command
    cmpb $'e', 1(%esi)
    jne test_copy_command
    cmpb $'n', 2(%esi)
    jne test_copy_command
    cmpb $' ', 3(%esi)
    jne test_copy_command
    addl $4, %esi
    call split_two_names
    testl %eax, %eax
    jz show_ren_usage
    call rename_file
    ret

test_copy_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'c', 0(%esi)
    jne test_edit_command
    cmpb $'o', 1(%esi)
    jne test_edit_command
    cmpb $'p', 2(%esi)
    jne test_edit_command
    cmpb $'y', 3(%esi)
    jne test_edit_command
    cmpb $' ', 4(%esi)
    jne test_edit_command
    addl $5, %esi
    call split_two_names
    testl %eax, %eax
    jz show_copy_usage
    call copy_file
    ret

test_edit_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'e', 0(%esi)
    jne test_write_command
    cmpb $'d', 1(%esi)
    jne test_write_command
    cmpb $'i', 2(%esi)
    jne test_write_command
    cmpb $'t', 3(%esi)
    jne test_write_command
    cmpb $' ', 4(%esi)
    jne test_write_command
    addl $5, %esi
    cmpb $0, (%esi)
    je show_edit_usage
    call start_editor
    ret

test_write_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'w', 0(%esi)
    jne test_type_command
    cmpb $'r', 1(%esi)
    jne test_type_command
    cmpb $'i', 2(%esi)
    jne test_type_command
    cmpb $'t', 3(%esi)
    jne test_type_command
    cmpb $'e', 4(%esi)
    jne test_type_command
    cmpb $' ', 5(%esi)
    jne test_type_command
    addl $6, %esi
    movl %esi, %edi
write_find_content:
    cmpb $0, (%edi)
    je show_write_usage
    cmpb $' ', (%edi)
    je write_content_found
    incl %edi
    jmp write_find_content
write_content_found:
    movb $0, (%edi)
    incl %edi
    movl %edi, (KERNEL_LOAD_ADDRESS + write_content - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + write_length_override - _start)
    call write_file
    ret

test_type_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'t', 0(%esi)
    jne test_echo_command
    cmpb $'y', 1(%esi)
    jne test_echo_command
    cmpb $'p', 2(%esi)
    jne test_echo_command
    cmpb $'e', 3(%esi)
    jne test_echo_command
    cmpb $' ', 4(%esi)
    jne test_echo_command
    addl $5, %esi
    cmpb $0, (%esi)
    je show_type_usage
    call type_file
    ret

test_echo_command:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    cmpb $'e', 0(%esi)
    jne test_natural_command
    cmpb $'c', 1(%esi)
    jne test_natural_command
    cmpb $'h', 2(%esi)
    jne test_natural_command
    cmpb $'o', 3(%esi)
    jne test_natural_command
    cmpb $0, 4(%esi)
    je show_blank
    cmpb $' ', 4(%esi)
    jne test_natural_command
    addl $5, %esi
    call print_string
    movl $(KERNEL_LOAD_ADDRESS + newline - _start), %esi
    call print_string
    ret

test_natural_command:
    call natural_route
    testl %eax, %eax
    jz show_unknown
    ret

show_help:
    movl $(KERNEL_LOAD_ADDRESS + help_message - _start), %esi
    jmp show_message
show_about:
    movl $(KERNEL_LOAD_ADDRESS + about_message - _start), %esi
    jmp show_message
show_version:
    movl $(KERNEL_LOAD_ADDRESS + version_message - _start), %esi
    jmp show_message
show_unknown:
    movl $(KERNEL_LOAD_ADDRESS + unknown_message - _start), %esi
    jmp show_message
show_blank:
    movl $(KERNEL_LOAD_ADDRESS + newline - _start), %esi
show_message:
    call print_string
command_complete:
    ret

show_type_usage:
    movl $(KERNEL_LOAD_ADDRESS + type_usage_message - _start), %esi
    jmp show_message
show_write_usage:
    movl $(KERNEL_LOAD_ADDRESS + write_usage_message - _start), %esi
    jmp show_message
show_edit_usage:
    movl $(KERNEL_LOAD_ADDRESS + edit_usage_message - _start), %esi
    jmp show_message
show_del_usage:
    movl $(KERNEL_LOAD_ADDRESS + del_usage_message - _start), %esi
    jmp show_message
show_ren_usage:
    movl $(KERNEL_LOAD_ADDRESS + ren_usage_message - _start), %esi
    jmp show_message
show_copy_usage:
    movl $(KERNEL_LOAD_ADDRESS + copy_usage_message - _start), %esi
    jmp show_message
show_find_usage:
    movl $(KERNEL_LOAD_ADDRESS + find_usage_message - _start), %esi
    jmp show_message

do_clear:
    call clear_terminal
    ret

show_memory:
    movl $(KERNEL_LOAD_ADDRESS + memory_message - _start), %esi
    call print_string
    movl BOOT_MEMORY_MIB(%ebp), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + mib_message - _start), %esi
    call print_string
    ret

show_disk:
    movl $(KERNEL_LOAD_ADDRESS + disk_free_message - _start), %esi
    call print_string
    xorl %edi, %edi
    movl $2, %ecx
disk_count_next:
    movl %ecx, %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    testl %eax, %eax
    jnz disk_count_used
    incl %edi
disk_count_used:
    incl %ecx
    movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax
    addl $2, %eax
    cmpl %eax, %ecx
    jb disk_count_next
    movl %edi, %eax
    shll $9, %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + disk_of_message - _start), %esi
    call print_string
    movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax
    shll $9, %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + bytes_message - _start), %esi
    call print_string
    ret

/* Find the first case-insensitive occurrence of search_text in ESI's file. */
find_text_in_file:
    movl %esi, (KERNEL_LOAD_ADDRESS + requested_name - _start)
    call find_root_entry
    testl %eax, %eax
    jnz find_text_entry_ready
    movl (KERNEL_LOAD_ADDRESS + requested_name - _start), %esi
    call find_root_entry_fuzzy
    cmpl $2, %eax
    je file_name_ambiguous
    testl %eax, %eax
    jz file_not_found
find_text_entry_ready:
    call load_entry_to_editor_buffer
    cmpl $2, %eax
    je find_text_too_large
    testl %eax, %eax
    jz file_cache_error
    xorl %edi, %edi
find_text_candidate:
    cmpl (KERNEL_LOAD_ADDRESS + copy_size - _start), %edi
    jae find_text_not_found
    movl (KERNEL_LOAD_ADDRESS + search_text - _start), %esi
    movl %edi, %ebx
find_text_compare:
    movzbl (%esi), %eax
    testb %al, %al
    jz find_text_found
    cmpl (KERNEL_LOAD_ADDRESS + copy_size - _start), %ebx
    jae find_text_next_candidate
    movzbl EDITOR_BUFFER_ADDRESS(,%ebx), %eax
    call uppercase_al
    pushl %eax
    movzbl (%esi), %eax
    call uppercase_al
    popl %edx
    cmpb %dl, %al
    jne find_text_next_candidate
    incl %esi
    incl %ebx
    jmp find_text_compare
find_text_next_candidate:
    incl %edi
    jmp find_text_candidate
find_text_found:
    movl %edi, %eax
    call calculate_text_position
    movl $(KERNEL_LOAD_ADDRESS + find_result_line - _start), %esi
    call print_string
    movl (KERNEL_LOAD_ADDRESS + text_position_line - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + find_result_column - _start), %esi
    call print_string
    movl (KERNEL_LOAD_ADDRESS + text_position_column - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + newline - _start), %esi
    call print_string
    ret
find_text_not_found:
    movl $(KERNEL_LOAD_ADDRESS + find_not_found_message - _start), %esi
    call print_string
    ret
find_text_too_large:
    movl $(KERNEL_LOAD_ADDRESS + find_large_message - _start), %esi
    call print_string
    ret

/*
 * Nyx-inspired deterministic gateway.  The visible input is never rewritten:
 * a lower-case, punctuation-neutral routing copy is built in routing_buffer.
 * Only known operational tokens receive bounded one-edit typo tolerance.
 */
natural_route:
    movb $0, (KERNEL_LOAD_ADDRESS + route_corrected - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + route_courtesy - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + route_has_object - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + route_ambiguous - _start)
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    call build_routing_copy
    movl $(KERNEL_LOAD_ADDRESS + routing_buffer - _start), %esi
    call route_skip_fillers
    cmpb $0, (%esi)
    je route_conversation
    call route_classify_action
    testl %eax, %eax
    jz route_unclaimed
    movb %al, (KERNEL_LOAD_ADDRESS + route_intent - _start)
    cmpl $ROUTE_SHOW, %eax
    je route_generic_operation
    cmpl $ROUTE_TELL, %eax
    je route_generic_operation
    cmpl $ROUTE_READ, %eax
    je route_read_verb
    cmpl $ROUTE_EDIT, %eax
    je route_edit_verb
    cmpl $ROUTE_FIND, %eax
    je route_find_verb
    jmp route_static_action

/* Resolve reusable operation + object slots such as "show me the files". */
route_generic_operation:
    movb %al, (KERNEL_LOAD_ADDRESS + route_generic_intent - _start)
    call route_advance_token
    call route_skip_fillers
    cmpb $0, (%esi)
    je route_conversation
    cmpb $ROUTE_SHOW, (KERNEL_LOAD_ADDRESS + route_generic_intent - _start)
    jne route_generic_try_action
    call route_skip_object_noun
    testl %eax, %eax
    jnz route_read_argument
route_generic_try_action:
    call route_classify_action
    testl %eax, %eax
    jnz route_generic_classified
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_ambiguous - _start)
    jne route_ambiguity
    cmpb $ROUTE_SHOW, (KERNEL_LOAD_ADDRESS + route_generic_intent - _start)
    je route_read_argument
    jmp route_guarded_action
route_generic_classified:
    movb %al, (KERNEL_LOAD_ADDRESS + route_intent - _start)
    cmpl $ROUTE_READ, %eax
    je route_read_verb
    cmpl $ROUTE_EDIT, %eax
    je route_edit_verb
    cmpl $ROUTE_FIND, %eax
    je route_find_verb
    cmpl $ROUTE_SHOW, %eax
    je route_conversation
    cmpl $ROUTE_TELL, %eax
    je route_conversation
    jmp route_static_action

route_read_verb:
    call route_advance_token
    call route_skip_fillers
    call route_skip_object_noun
route_read_argument:
    cmpb $0, (%esi)
    je route_type_usage
    call route_terminate_token
    movl %esi, (KERNEL_LOAD_ADDRESS + route_argument - _start)
    movb $ROUTE_READ, (KERNEL_LOAD_ADDRESS + route_intent - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + route_has_object - _start)
    call milo_record_action
    movl (KERNEL_LOAD_ADDRESS + route_argument - _start), %esi
    call type_file
    jmp route_success

route_edit_verb:
    call route_advance_token
    call route_skip_fillers
    call route_skip_object_noun
    cmpb $0, (%esi)
    je route_edit_usage
    call route_terminate_token
    movl %esi, (KERNEL_LOAD_ADDRESS + route_argument - _start)
    movb $ROUTE_EDIT, (KERNEL_LOAD_ADDRESS + route_intent - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + route_has_object - _start)
    call milo_record_action
    movl (KERNEL_LOAD_ADDRESS + route_argument - _start), %esi
    call start_editor
    jmp route_success

route_find_verb:
    call route_advance_token
    call route_skip_fillers
    call route_prepare_find
    testl %eax, %eax
    jz route_find_usage
    movb $ROUTE_FIND, (KERNEL_LOAD_ADDRESS + route_intent - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + route_has_object - _start)
    call milo_record_action
    movl (KERNEL_LOAD_ADDRESS + route_search_pointer - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + search_text - _start)
    movl (KERNEL_LOAD_ADDRESS + route_file_pointer - _start), %esi
    call find_text_in_file
    jmp route_success

route_type_usage:
    movl $(KERNEL_LOAD_ADDRESS + type_usage_message - _start), %esi
    call print_string
    jmp route_success
route_edit_usage:
    movl $(KERNEL_LOAD_ADDRESS + edit_usage_message - _start), %esi
    call print_string
    jmp route_success
route_find_usage:
    movl $(KERNEL_LOAD_ADDRESS + find_usage_message - _start), %esi
    call print_string
    jmp route_success

/* Controller-owned actions: classification cannot manufacture side effects. */
route_static_action:
    call milo_record_action
    movzbl (KERNEL_LOAD_ADDRESS + route_intent - _start), %eax
    cmpl $ROUTE_HELP, %eax
    je route_show_help
    cmpl $ROUTE_ABOUT, %eax
    je route_show_about
    cmpl $ROUTE_MEMORY, %eax
    je route_show_memory
    cmpl $ROUTE_VERSION, %eax
    je route_show_version
    cmpl $ROUTE_DIRECTORY, %eax
    je route_show_directory
    cmpl $ROUTE_DISK, %eax
    je route_show_disk
    cmpl $ROUTE_TRAITS, %eax
    je route_show_traits
    cmpl $ROUTE_CLEAR, %eax
    je route_clear_terminal
    jmp route_conversation
route_show_help:
    movl $(KERNEL_LOAD_ADDRESS + help_message - _start), %esi
    call print_string
    jmp route_success
route_show_about:
    movl $(KERNEL_LOAD_ADDRESS + about_message - _start), %esi
    call print_string
    jmp route_success
route_show_memory:
    call show_memory
    jmp route_success
route_show_version:
    movl $(KERNEL_LOAD_ADDRESS + version_message - _start), %esi
    call print_string
    jmp route_success
route_show_directory:
    call show_directory
    jmp route_success
route_show_disk:
    call show_disk
    jmp route_success
route_show_traits:
    call show_traits
    jmp route_success
route_clear_terminal:
    call clear_terminal
    jmp route_success

route_unclaimed:
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_ambiguous - _start)
    je route_guarded_action
route_ambiguity:
    movl $(KERNEL_LOAD_ADDRESS + route_ambiguous_message - _start), %esi
    call print_string
    jmp route_success

/* Recognize unsafe/natural file changes only to state that none occurred. */
route_guarded_action:
    call route_classify_guarded
    testl %eax, %eax
    jz route_conversation
    cmpl $GUARD_DELETE, %eax
    je route_guard_delete
    cmpl $GUARD_RENAME, %eax
    je route_guard_rename
    cmpl $GUARD_WRITE, %eax
    je route_guard_write
    movl $(KERNEL_LOAD_ADDRESS + guarded_copy_message - _start), %esi
    jmp route_guard_print
route_guard_delete:
    movl $(KERNEL_LOAD_ADDRESS + guarded_delete_message - _start), %esi
    jmp route_guard_print
route_guard_rename:
    movl $(KERNEL_LOAD_ADDRESS + guarded_rename_message - _start), %esi
    jmp route_guard_print
route_guard_write:
    movl $(KERNEL_LOAD_ADDRESS + guarded_write_message - _start), %esi
route_guard_print:
    call print_string
    jmp route_success
route_conversation:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    call milo_interact
route_success:
    movl $1, %eax
    ret

/* Build a transient routing copy: lowercase text, preserve 8.3 punctuation. */
build_routing_copy:
    movl $(KERNEL_LOAD_ADDRESS + routing_buffer - _start), %edi
    xorl %ecx, %ecx
    movb $1, %bl
route_copy_next:
    movb (%esi), %al
    testb %al, %al
    jz route_copy_complete
    incl %esi
    cmpb $'A', %al
    jb route_copy_classify
    cmpb $'Z', %al
    ja route_copy_classify
    addb $32, %al
route_copy_classify:
    cmpb $'a', %al
    jb route_copy_digit
    cmpb $'z', %al
    jbe route_copy_emit
route_copy_digit:
    cmpb $'0', %al
    jb route_copy_filename_char
    cmpb $'9', %al
    jbe route_copy_emit
route_copy_filename_char:
    cmpb $'.', %al
    jne route_copy_dash
    movb (%esi), %ah
    cmpb $'0', %ah
    jb route_copy_separator
    cmpb $'9', %ah
    jbe route_copy_emit
    cmpb $'A', %ah
    jb route_copy_separator
    cmpb $'Z', %ah
    jbe route_copy_emit
    cmpb $'a', %ah
    jb route_copy_separator
    cmpb $'z', %ah
    jbe route_copy_emit
    jmp route_copy_separator
route_copy_dash:
    cmpb $'-', %al
    je route_copy_emit
    cmpb $'_', %al
    je route_copy_emit
route_copy_separator:
    cmpb $0, %bl
    jne route_copy_next
    movb $' ', %al
    movb $1, %bl
    jmp route_copy_store
route_copy_emit:
    movb $0, %bl
route_copy_store:
    movb %al, (%edi,%ecx)
    incl %ecx
    cmpl $INPUT_LIMIT, %ecx
    jb route_copy_next
route_copy_complete:
    testl %ecx, %ecx
    jz route_copy_zero
    cmpb $' ', -1(%edi,%ecx)
    jne route_copy_zero
    decl %ecx
route_copy_zero:
    movb $0, (%edi,%ecx)
    ret

route_advance_token:
    cmpb $0, (%esi)
    je route_advance_complete
route_advance_word:
    cmpb $0, (%esi)
    je route_advance_complete
    cmpb $' ', (%esi)
    je route_advance_spaces
    incl %esi
    jmp route_advance_word
route_advance_spaces:
    incl %esi
    cmpb $' ', (%esi)
    je route_advance_spaces
route_advance_complete:
    ret

route_terminate_token:
    movl %esi, %edi
route_terminate_next:
    cmpb $0, (%edi)
    je route_terminate_complete
    cmpb $' ', (%edi)
    je route_terminate_here
    incl %edi
    jmp route_terminate_next
route_terminate_here:
    movb $0, (%edi)
route_terminate_complete:
    ret

/* Skip grammar lead-ins; table flags distinguish courtesy/social prefixes. */
route_skip_fillers:
    cmpb $0, (%esi)
    je route_fillers_complete
    call route_classify_filler
    testl %eax, %eax
    jz route_fillers_complete
    cmpl $2, %eax
    jne route_filler_advance
    movb $1, (KERNEL_LOAD_ADDRESS + route_courtesy - _start)
route_filler_advance:
    call route_advance_token
    jmp route_skip_fillers
route_fillers_complete:
    ret

/* Singular object nouns are optional, but plural "files" remains an intent. */
route_skip_object_noun:
    movl $(KERNEL_LOAD_ADDRESS + route_word_file - _start), %edi
    call route_word_match
    cmpl $1, %eax
    je route_object_noun_found
    movl $(KERNEL_LOAD_ADDRESS + route_word_document - _start), %edi
    call route_word_match
    cmpl $1, %eax
    jne route_object_noun_complete
route_object_noun_found:
    call route_advance_token
    call route_skip_fillers
    movl $1, %eax
    ret
route_object_noun_complete:
    xorl %eax, %eax
    ret

route_classify_filler:
    movl $(KERNEL_LOAD_ADDRESS + route_filler_table - _start), %ebx
route_filler_next:
    movl (%ebx), %edi
    testl %edi, %edi
    jz route_filler_none
    call route_word_match
    cmpl $1, %eax
    jne route_filler_continue
route_filler_found:
    movzbl 4(%ebx), %eax
    incl %eax
    ret
route_filler_continue:
    addl $5, %ebx
    jmp route_filler_next
route_filler_none:
    xorl %eax, %eax
    ret

route_classify_action:
    movb $0, (KERNEL_LOAD_ADDRESS + route_ambiguous - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + route_action_match - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + route_action_fuzzy - _start)
    movl $(KERNEL_LOAD_ADDRESS + route_action_table - _start), %ebx
route_action_next:
    movl (%ebx), %edi
    testl %edi, %edi
    jz route_action_complete
    call route_word_match
    testl %eax, %eax
    jz route_action_continue
    movzbl 4(%ebx), %ecx
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_action_match - _start)
    je route_action_first_match
    cmpb (KERNEL_LOAD_ADDRESS + route_action_match - _start), %cl
    jne route_action_ambiguous
    jmp route_action_note_fuzzy
route_action_first_match:
    movb %cl, (KERNEL_LOAD_ADDRESS + route_action_match - _start)
route_action_note_fuzzy:
    cmpl $2, %eax
    jne route_action_continue
    movb $1, (KERNEL_LOAD_ADDRESS + route_action_fuzzy - _start)
route_action_continue:
    addl $5, %ebx
    jmp route_action_next
route_action_complete:
    movzbl (KERNEL_LOAD_ADDRESS + route_action_match - _start), %eax
    testl %eax, %eax
    jz route_action_none
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_action_fuzzy - _start)
    je route_action_return
    movb $1, (KERNEL_LOAD_ADDRESS + route_corrected - _start)
route_action_return:
    ret
route_action_none:
    xorl %eax, %eax
    ret
route_action_ambiguous:
    movb $1, (KERNEL_LOAD_ADDRESS + route_ambiguous - _start)
    xorl %eax, %eax
    ret

/* Return a guarded file-change class while deliberately performing no action. */
route_classify_guarded:
    movl $(KERNEL_LOAD_ADDRESS + route_guarded_table - _start), %ebx
route_guarded_next:
    movl (%ebx), %edi
    testl %edi, %edi
    jz route_guarded_none
    call route_word_match
    testl %eax, %eax
    jnz route_guarded_found
    addl $5, %ebx
    jmp route_guarded_next
route_guarded_found:
    movzbl 4(%ebx), %eax
    ret
route_guarded_none:
    xorl %eax, %eax
    ret

/*
 * Match ESI's current token against EDI's known word. Return 1 exact, 2 for a
 * unique bounded correction (one edit or adjacent transposition), else zero.
 */
route_word_match:
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    xorl %ecx, %ecx
route_match_token_length:
    cmpb $0, (%esi,%ecx)
    je route_match_token_sized
    cmpb $' ', (%esi,%ecx)
    je route_match_token_sized
    incl %ecx
    jmp route_match_token_length
route_match_token_sized:
    movl %ecx, (KERNEL_LOAD_ADDRESS + route_token_length - _start)
    xorl %edx, %edx
route_match_word_length:
    cmpb $0, (%edi,%edx)
    je route_match_word_sized
    incl %edx
    jmp route_match_word_length
route_match_word_sized:
    movl %edx, (KERNEL_LOAD_ADDRESS + route_word_length - _start)
    cmpl %edx, %ecx
    jne route_match_different_length
    xorl %ebx, %ebx
    xorl %edx, %edx
route_match_same_next:
    cmpl (KERNEL_LOAD_ADDRESS + route_token_length - _start), %ebx
    jae route_match_same_complete
    movb (%esi,%ebx), %al
    cmpb (%edi,%ebx), %al
    je route_match_same_advance
    cmpl $4, (KERNEL_LOAD_ADDRESS + route_word_length - _start)
    jb route_match_failed
    testl %edx, %edx
    jnz route_match_failed
    movl %ebx, %ecx
    incl %ecx
    cmpl (KERNEL_LOAD_ADDRESS + route_token_length - _start), %ecx
    jae route_match_substitution
    movb 1(%esi,%ebx), %al
    cmpb (%edi,%ebx), %al
    jne route_match_substitution
    movb (%esi,%ebx), %al
    cmpb 1(%edi,%ebx), %al
    jne route_match_substitution
    movl $1, %edx
    addl $2, %ebx
    jmp route_match_same_next
route_match_substitution:
    movl $1, %edx
route_match_same_advance:
    incl %ebx
    jmp route_match_same_next
route_match_same_complete:
    testl %edx, %edx
    jz route_match_exact
    jmp route_match_fuzzy

route_match_different_length:
    cmpl $4, (KERNEL_LOAD_ADDRESS + route_word_length - _start)
    jb route_match_failed
    movl %ecx, %eax
    subl %edx, %eax
    cmpl $1, %eax
    je route_match_input_longer
    cmpl $-1, %eax
    jne route_match_failed
    xchgl %esi, %edi
    movl (KERNEL_LOAD_ADDRESS + route_word_length - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + route_token_length - _start), %edx
    jmp route_match_length_ready
route_match_input_longer:
    movl (KERNEL_LOAD_ADDRESS + route_token_length - _start), %eax
    movl (KERNEL_LOAD_ADDRESS + route_word_length - _start), %edx
route_match_length_ready:
    xorl %ebx, %ebx
    xorl %ecx, %ecx
    movb $0, (KERNEL_LOAD_ADDRESS + route_edit_used - _start)
route_match_insert_next:
    cmpl %edx, %ecx
    jae route_match_fuzzy
    movb (%esi,%ebx), %al
    cmpb (%edi,%ecx), %al
    je route_match_insert_equal
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_edit_used - _start)
    jne route_match_failed
    movb $1, (KERNEL_LOAD_ADDRESS + route_edit_used - _start)
    incl %ebx
    movb (%esi,%ebx), %al
    cmpb (%edi,%ecx), %al
    jne route_match_failed
route_match_insert_equal:
    incl %ebx
    incl %ecx
    jmp route_match_insert_next

route_match_exact:
    movl $1, %eax
    jmp route_match_complete
route_match_fuzzy:
    movl $2, %eax
    jmp route_match_complete
route_match_failed:
    xorl %eax, %eax
route_match_complete:
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    ret

/* Parse either "find FILE text" or "find text in FILE" generically. */
route_prepare_find:
    cmpb $0, (%esi)
    je route_find_invalid
    movl %esi, (KERNEL_LOAD_ADDRESS + route_search_pointer - _start)
    movl %esi, %ebx
route_find_token:
    movl %ebx, %edi
    xorl %edx, %edx
route_find_scan_token:
    cmpb $0, (%edi)
    je route_find_token_end
    cmpb $' ', (%edi)
    je route_find_token_end
    cmpb $'.', (%edi)
    jne route_find_scan_advance
    movl $1, %edx
route_find_scan_advance:
    incl %edi
    jmp route_find_scan_token
route_find_token_end:
    testl %edx, %edx
    jnz route_find_file_found
    cmpb $0, (%edi)
    je route_find_invalid
    leal 1(%edi), %ebx
    jmp route_find_token
route_find_file_found:
    movl %ebx, (KERNEL_LOAD_ADDRESS + route_file_pointer - _start)
    movl %edi, (KERNEL_LOAD_ADDRESS + route_file_end - _start)
    cmpl (KERNEL_LOAD_ADDRESS + route_search_pointer - _start), %ebx
    jne route_find_query_first

    /* Filename first: the remaining payload, after optional "for", is text. */
    cmpb $0, (%edi)
    je route_find_invalid
    movb $0, (%edi)
    incl %edi
    movl %edi, %esi
route_find_after_file_spaces:
    cmpb $' ', (%esi)
    jne route_find_after_file_word
    incl %esi
    jmp route_find_after_file_spaces
route_find_after_file_word:
    movl $(KERNEL_LOAD_ADDRESS + route_word_for - _start), %edi
    call route_word_match
    testl %eax, %eax
    jz route_find_after_optional_for
    cmpl $2, %eax
    jne route_find_for_ready
    movb $1, (KERNEL_LOAD_ADDRESS + route_corrected - _start)
route_find_for_ready:
    call route_advance_token
route_find_after_optional_for:
    cmpb $0, (%esi)
    je route_find_invalid
    movl %esi, (KERNEL_LOAD_ADDRESS + route_search_pointer - _start)
    movl $1, %eax
    ret

route_find_query_first:
    /* Remove a relation token immediately before the filename when present. */
    movl %ebx, %edi
    decl %edi
route_find_back_spaces:
    cmpl (KERNEL_LOAD_ADDRESS + route_search_pointer - _start), %edi
    jbe route_find_invalid
    cmpb $' ', (%edi)
    jne route_find_previous_end
    decl %edi
    jmp route_find_back_spaces
route_find_previous_end:
    incl %edi
    movl %edi, %edx
route_find_previous_start:
    cmpl (KERNEL_LOAD_ADDRESS + route_search_pointer - _start), %edx
    jbe route_find_previous_ready
    cmpb $' ', -1(%edx)
    je route_find_previous_ready
    decl %edx
    jmp route_find_previous_start
route_find_previous_ready:
    movl %edx, %esi
    movl $(KERNEL_LOAD_ADDRESS + route_word_in - _start), %edi
    call route_word_match
    testl %eax, %eax
    jnz route_find_remove_relation
    movl $(KERNEL_LOAD_ADDRESS + route_word_inside - _start), %edi
    call route_word_match
    testl %eax, %eax
    jnz route_find_remove_relation
    movl $(KERNEL_LOAD_ADDRESS + route_word_within - _start), %edi
    call route_word_match
    testl %eax, %eax
    jz route_find_keep_query
route_find_remove_relation:
    cmpl $2, %eax
    jne route_find_relation_ready
    movb $1, (KERNEL_LOAD_ADDRESS + route_corrected - _start)
route_find_relation_ready:
    cmpl (KERNEL_LOAD_ADDRESS + route_search_pointer - _start), %edx
    jbe route_find_invalid
    movb $0, -1(%edx)
    jmp route_find_zero_filename
route_find_keep_query:
    movb $0, -1(%ebx)
route_find_zero_filename:
    movl (KERNEL_LOAD_ADDRESS + route_file_end - _start), %edi
    movb $0, (%edi)
    movl $1, %eax
    ret
route_find_invalid:
    xorl %eax, %eax
    ret

/* Load a valid persisted profile, migrating the V0.19 16-byte form in RAM. */
milo_memory_load:
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_filename - _start), %esi
    call find_root_entry
    testl %eax, %eax
    jz milo_memory_load_complete
    movl 28(%edi), %eax
    cmpl $MILO_MEMORY_SIZE, %eax
    je milo_memory_load_current
    cmpl $16, %eax
    jne milo_memory_load_complete
    movb $1, (KERNEL_LOAD_ADDRESS + milo_legacy_profile - _start)
milo_memory_load_current:
    call load_entry_to_editor_buffer
    cmpl $1, %eax
    jne milo_memory_load_complete
    cmpl $0x4f4c494d, EDITOR_BUFFER_ADDRESS
    jne milo_memory_load_complete
    cmpb $0, (KERNEL_LOAD_ADDRESS + milo_legacy_profile - _start)
    jne milo_memory_load_legacy
    cmpb $2, (EDITOR_BUFFER_ADDRESS + 4)
    jne milo_memory_load_complete
    movl $EDITOR_BUFFER_ADDRESS, %esi
    call milo_memory_checksum
    cmpb (EDITOR_BUFFER_ADDRESS + MILO_CHECKSUM_OFFSET), %al
    jne milo_memory_load_complete
    movl $EDITOR_BUFFER_ADDRESS, %esi
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block - _start), %edi
    movl $MILO_MEMORY_SIZE, %ecx
    rep movsb
    jmp milo_memory_load_complete
milo_memory_load_legacy:
    cmpb $1, (EDITOR_BUFFER_ADDRESS + 4)
    jne milo_memory_load_complete
    movl $EDITOR_BUFFER_ADDRESS, %esi
    call milo_memory_checksum_v1
    cmpb (EDITOR_BUFFER_ADDRESS + 15), %al
    jne milo_memory_load_complete
    movl $EDITOR_BUFFER_ADDRESS, %esi
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block - _start), %edi
    movl $15, %ecx
    rep movsb
    movb $2, (KERNEL_LOAD_ADDRESS + milo_memory_block + 4 - _start)
milo_memory_load_complete:
    movb $0, (KERNEL_LOAD_ADDRESS + milo_legacy_profile - _start)
    ret

/* ESI=profile. Return XOR checksum of bytes 0..22 in AL. */
milo_memory_checksum:
    xorl %eax, %eax
    xorl %ecx, %ecx
milo_checksum_next:
    xorb (%esi,%ecx), %al
    incl %ecx
    cmpl $MILO_CHECKSUM_OFFSET, %ecx
    jb milo_checksum_next
    ret

/* Validate the V0.19 profile before migrating it. */
milo_memory_checksum_v1:
    xorl %eax, %eax
    xorl %ecx, %ecx
milo_checksum_v1_next:
    xorb (%esi,%ecx), %al
    incl %ecx
    cmpl $15, %ecx
    jb milo_checksum_v1_next
    ret

milo_save_memory:
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block - _start), %esi
    call milo_memory_checksum
    movb %al, (KERNEL_LOAD_ADDRESS + milo_memory_block + MILO_CHECKSUM_OFFSET - _start)
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_filename - _start), %esi
    call find_root_entry
    testl %eax, %eax
    jz milo_create_memory
    cmpl $MILO_MEMORY_SIZE, 28(%edi)
    jne milo_create_memory
    movzwl 26(%edi), %ebx
    cmpl $2, %ebx
    jb milo_create_memory
    movl %ebx, %eax
    call load_cluster_sector
    testl %eax, %eax
    jz write_failed
    movl %esi, (KERNEL_LOAD_ADDRESS + write_data_buffer - _start)
    movl %esi, %edi
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block - _start), %esi
    movl $MILO_MEMORY_SIZE, %ecx
    rep movsb
    movl %ebx, %eax
    subl $2, %eax
    addl $DATA_LBA, %eax
    movl (KERNEL_LOAD_ADDRESS + write_data_buffer - _start), %edx
    call write_sector_verified
    testl %eax, %eax
    jnz write_failed
    ret
milo_create_memory:
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_content - _start)
    movl $MILO_MEMORY_SIZE, (KERNEL_LOAD_ADDRESS + write_length_override - _start)
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_filename - _start), %esi
    movb $1, (KERNEL_LOAD_ADDRESS + write_silent - _start)
    call write_file
    movb $0, (KERNEL_LOAD_ADDRESS + write_silent - _start)
    ret

/* EDI=byte counter. Increment without wrapping. */
milo_increment_counter:
    cmpb $255, (%edi)
    jae milo_increment_complete
    incb (%edi)
milo_increment_complete:
    ret

/* EDI=human-readable trait. Increment and cap at 100. */
milo_increment_trait:
    cmpb $100, (%edi)
    jae milo_increment_complete
    incb (%edi)
    ret

/* Learn structural speech habits from ESI and emit one deterministic reply. */
milo_interact:
    call milo_observe_input
    jmp milo_choose_response

/* Observe structural signals only; ESI text is inspected but never retained. */
milo_observe_input:
    movb $0, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    xorl %ecx, %ecx
    xorl %edx, %edx
    xorl %ebx, %ebx
milo_scan_next:
    movb (%esi,%ecx), %al
    testb %al, %al
    jz milo_scan_complete
    cmpb $'?', %al
    jne milo_scan_exclamation
    orb $1, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
milo_scan_exclamation:
    cmpb $'!', %al
    jne milo_scan_uppercase
    orb $2, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
milo_scan_uppercase:
    cmpb $'A', %al
    jb milo_scan_lowercase
    cmpb $'Z', %al
    ja milo_scan_lowercase
    incl %edx
    incl %ebx
    jmp milo_scan_advance
milo_scan_lowercase:
    cmpb $'a', %al
    jb milo_scan_advance
    cmpb $'z', %al
    ja milo_scan_advance
    incl %ebx
milo_scan_advance:
    incl %ecx
    jmp milo_scan_next
milo_scan_complete:
    testl %ebx, %ebx
    jz milo_scan_length
    leal (%edx,%edx), %eax
    cmpl %ebx, %eax
    jb milo_scan_length
    orb $8, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
milo_scan_length:
    cmpl $20, %ecx
    ja milo_not_short
    orb $4, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
milo_not_short:
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 5 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 6 - _start), %edi
    call milo_increment_trait

    testb $1, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jz milo_learn_exclamation
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 9 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 8 - _start), %edi
    call milo_increment_trait
    call milo_increment_trait
milo_learn_exclamation:
    testb $2, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jz milo_learn_short
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 10 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 7 - _start), %edi
    call milo_increment_trait
milo_learn_short:
    testb $4, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jz milo_learn_uppercase
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 11 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 7 - _start), %edi
    call milo_increment_trait
milo_learn_uppercase:
    testb $8, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jz milo_learning_complete
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 12 - _start), %edi
    call milo_increment_counter
milo_learning_complete:
    movb (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start), %al
    movb %al, (KERNEL_LOAD_ADDRESS + milo_memory_block + 13 - _start)
    incb (KERNEL_LOAD_ADDRESS + milo_memory_block + 14 - _start)
    ret

/* Record a validated natural action as abstract grammar, never as words. */
milo_record_action:
    movl $(KERNEL_LOAD_ADDRESS + input_buffer - _start), %esi
    call milo_observe_input
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 16 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 21 - _start), %edi
    call milo_increment_counter
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_has_object - _start)
    je milo_record_courtesy
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 19 - _start), %edi
    call milo_increment_counter
milo_record_courtesy:
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_courtesy - _start)
    je milo_record_question
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 15 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 20 - _start), %edi
    call milo_increment_counter
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 6 - _start), %edi
    call milo_increment_trait
milo_record_question:
    testb $1, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jz milo_record_correction
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 17 - _start), %edi
    call milo_increment_counter
milo_record_correction:
    cmpb $0, (KERNEL_LOAD_ADDRESS + route_corrected - _start)
    je milo_record_intent
    movl $(KERNEL_LOAD_ADDRESS + milo_memory_block + 18 - _start), %edi
    call milo_increment_counter
milo_record_intent:
    movb (KERNEL_LOAD_ADDRESS + route_intent - _start), %al
    movb %al, (KERNEL_LOAD_ADDRESS + milo_memory_block + 22 - _start)
    call milo_save_memory
    ret

milo_choose_response:

    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 5 - _start), %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 7 - _start), %ecx
    addl %ecx, %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 6 - _start), %ecx
    addl %ecx, %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 8 - _start), %ecx
    addl %ecx, %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 12 - _start), %ecx
    addl %ecx, %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 14 - _start), %ecx
    addl %ecx, %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 15 - _start), %ecx
    addl %ecx, %eax
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 16 - _start), %ecx
    addl %ecx, %eax
    andl $3, %eax
    cmpb $25, (KERNEL_LOAD_ADDRESS + milo_memory_block + 7 - _start)
    jae milo_response_choice_ready
    andl $1, %eax
milo_response_choice_ready:
    movb %al, (KERNEL_LOAD_ADDRESS + milo_response_choice - _start)
    call milo_save_memory

    testb $1, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jnz milo_question_response
    testb $2, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jnz milo_exclamation_response
    testb $4, (KERNEL_LOAD_ADDRESS + milo_signal_flags - _start)
    jnz milo_short_response
    jmp milo_neutral_response

milo_question_response:
    movzbl (KERNEL_LOAD_ADDRESS + milo_response_choice - _start), %eax
    cmpl $0, %eax
    je milo_question_zero
    cmpl $1, %eax
    je milo_question_one
    cmpl $2, %eax
    je milo_question_two
    movl $(KERNEL_LOAD_ADDRESS + milo_question_three_message - _start), %esi
    jmp milo_print_response
milo_question_zero:
    movl $(KERNEL_LOAD_ADDRESS + milo_question_zero_message - _start), %esi
    jmp milo_print_response
milo_question_one:
    movl $(KERNEL_LOAD_ADDRESS + milo_question_one_message - _start), %esi
    jmp milo_print_response
milo_question_two:
    movl $(KERNEL_LOAD_ADDRESS + milo_question_two_message - _start), %esi
    jmp milo_print_response

milo_exclamation_response:
    movzbl (KERNEL_LOAD_ADDRESS + milo_response_choice - _start), %eax
    cmpl $0, %eax
    je milo_exclamation_zero
    cmpl $1, %eax
    je milo_exclamation_one
    cmpl $2, %eax
    je milo_exclamation_two
    movl $(KERNEL_LOAD_ADDRESS + milo_exclamation_three_message - _start), %esi
    jmp milo_print_response
milo_exclamation_zero:
    movl $(KERNEL_LOAD_ADDRESS + milo_exclamation_zero_message - _start), %esi
    jmp milo_print_response
milo_exclamation_one:
    movl $(KERNEL_LOAD_ADDRESS + milo_exclamation_one_message - _start), %esi
    jmp milo_print_response
milo_exclamation_two:
    movl $(KERNEL_LOAD_ADDRESS + milo_exclamation_two_message - _start), %esi
    jmp milo_print_response

milo_short_response:
    movzbl (KERNEL_LOAD_ADDRESS + milo_response_choice - _start), %eax
    cmpl $0, %eax
    je milo_short_zero
    cmpl $1, %eax
    je milo_short_one
    cmpl $2, %eax
    je milo_short_two
    movl $(KERNEL_LOAD_ADDRESS + milo_short_three_message - _start), %esi
    jmp milo_print_response
milo_short_zero:
    movl $(KERNEL_LOAD_ADDRESS + milo_short_zero_message - _start), %esi
    jmp milo_print_response
milo_short_one:
    movl $(KERNEL_LOAD_ADDRESS + milo_short_one_message - _start), %esi
    jmp milo_print_response
milo_short_two:
    movl $(KERNEL_LOAD_ADDRESS + milo_short_two_message - _start), %esi
    jmp milo_print_response

milo_neutral_response:
    movzbl (KERNEL_LOAD_ADDRESS + milo_response_choice - _start), %eax
    cmpl $0, %eax
    je milo_neutral_zero
    cmpl $1, %eax
    je milo_neutral_one
    cmpl $2, %eax
    je milo_neutral_two
    movl $(KERNEL_LOAD_ADDRESS + milo_neutral_three_message - _start), %esi
    jmp milo_print_response
milo_neutral_zero:
    movl $(KERNEL_LOAD_ADDRESS + milo_neutral_zero_message - _start), %esi
    jmp milo_print_response
milo_neutral_one:
    movl $(KERNEL_LOAD_ADDRESS + milo_neutral_one_message - _start), %esi
    jmp milo_print_response
milo_neutral_two:
    movl $(KERNEL_LOAD_ADDRESS + milo_neutral_two_message - _start), %esi
milo_print_response:
    call print_string
    ret

show_milo_ready:
    movl $(KERNEL_LOAD_ADDRESS + milo_ready_message - _start), %esi
    call print_string
    ret

show_traits:
    movl $(KERNEL_LOAD_ADDRESS + traits_header_message - _start), %esi
    call print_string
    movl $(KERNEL_LOAD_ADDRESS + traits_interactions_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 5 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_warmth_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 6 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_cheek_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 7 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_curiosity_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 8 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_patterns_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 9 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_pattern_separator - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 10 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_pattern_separator - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 11 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_pattern_separator - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 12 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_grammar_message - _start), %esi
    call print_string
    movl $15, %ebx
traits_grammar_next:
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block - _start)(,%ebx), %eax
    call print_u32
    incl %ebx
    cmpl $21, %ebx
    jae traits_grammar_complete
    movl $(KERNEL_LOAD_ADDRESS + traits_pattern_separator - _start), %esi
    call print_string
    jmp traits_grammar_next
traits_grammar_complete:
    movl $(KERNEL_LOAD_ADDRESS + traits_routes_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 21 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_intent_message - _start), %esi
    call print_string
    movzbl (KERNEL_LOAD_ADDRESS + milo_memory_block + 22 - _start), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + traits_footer_message - _start), %esi
    call print_string
    ret

/* Split "OLD NEW" in place; ESI remains OLD and second_name points to NEW. */
split_two_names:
    cmpb $0, (%esi)
    je split_names_invalid
    movl %esi, %edi
split_names_find_space:
    cmpb $0, (%edi)
    je split_names_invalid
    cmpb $' ', (%edi)
    je split_names_found
    incl %edi
    jmp split_names_find_space
split_names_found:
    movb $0, (%edi)
    incl %edi
    cmpb $0, (%edi)
    je split_names_invalid
    movl %edi, (KERNEL_LOAD_ADDRESS + second_name - _start)
    movl $1, %eax
    ret
split_names_invalid:
    xorl %eax, %eax
    ret

/* Find a normal root entry by 8.3 name. EAX=1 and EDI=entry on success. */
find_root_entry:
    call build_fat_name
    testl %eax, %eax
    jz root_entry_not_found
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
root_entry_next:
    cmpb $0, (%edi)
    je root_entry_not_found
    cmpb $0xe5, (%edi)
    je root_entry_skip
    cmpb $0x0f, 11(%edi)
    je root_entry_skip
    testb $0x18, 11(%edi)
    jnz root_entry_skip
    pushl %ecx
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %esi
    call names_equal_11
    popl %ecx
    testl %eax, %eax
    jnz root_entry_found
root_entry_skip:
    addl $32, %edi
    decl %ecx
    jnz root_entry_next
root_entry_not_found:
    xorl %eax, %eax
    ret
root_entry_found:
    movl $1, %eax
    ret

/*
 * Conservative entity correction for read-only operations. ESI is an 8.3
 * request; EAX=1/EDI=entry only for one unique one-edit root match, EAX=2 if
 * ambiguous. The requested spelling and directory entries remain untouched.
 */
find_root_entry_fuzzy:
    call build_fat_name
    testl %eax, %eax
    jz fuzzy_root_none
    movl $0, (KERNEL_LOAD_ADDRESS + fuzzy_root_entry - _start)
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
fuzzy_root_next:
    cmpb $0, (%edi)
    je fuzzy_root_complete
    cmpb $0xe5, (%edi)
    je fuzzy_root_skip
    cmpb $0x0f, 11(%edi)
    je fuzzy_root_skip
    testb $0x18, 11(%edi)
    jnz fuzzy_root_skip
    pushl %ecx
    pushl %edi
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %esi
    call fat_name_fuzzy_match
    popl %edi
    popl %ecx
    testl %eax, %eax
    jz fuzzy_root_skip
    cmpl $0, (KERNEL_LOAD_ADDRESS + fuzzy_root_entry - _start)
    jne fuzzy_root_ambiguous
    movl %edi, (KERNEL_LOAD_ADDRESS + fuzzy_root_entry - _start)
fuzzy_root_skip:
    addl $32, %edi
    decl %ecx
    jnz fuzzy_root_next
fuzzy_root_complete:
    movl (KERNEL_LOAD_ADDRESS + fuzzy_root_entry - _start), %edi
    testl %edi, %edi
    jz fuzzy_root_none
    movb $1, (KERNEL_LOAD_ADDRESS + route_corrected - _start)
    movl $1, %eax
    ret
fuzzy_root_ambiguous:
    movl $2, %eax
    ret
fuzzy_root_none:
    xorl %eax, %eax
    ret

/* ESI=requested padded name, EDI=root name. Accept one substitution or swap. */
fat_name_fuzzy_match:
    xorl %ecx, %ecx
    xorl %edx, %edx
fat_fuzzy_next:
    cmpl $11, %ecx
    jae fat_fuzzy_complete
    movb (%esi,%ecx), %al
    cmpb (%edi,%ecx), %al
    je fat_fuzzy_advance
    testl %edx, %edx
    jnz fat_fuzzy_failed
    cmpl $10, %ecx
    jae fat_fuzzy_substitution
    movb 1(%esi,%ecx), %al
    cmpb (%edi,%ecx), %al
    jne fat_fuzzy_substitution
    movb (%esi,%ecx), %al
    cmpb 1(%edi,%ecx), %al
    jne fat_fuzzy_substitution
    movl $1, %edx
    addl $2, %ecx
    jmp fat_fuzzy_next
fat_fuzzy_substitution:
    movl $1, %edx
fat_fuzzy_advance:
    incl %ecx
    jmp fat_fuzzy_next
fat_fuzzy_complete:
    testl %edx, %edx
    jz fat_fuzzy_failed
    movl $1, %eax
    ret
fat_fuzzy_failed:
    xorl %eax, %eax
    ret

delete_file:
    call find_root_entry
    testl %eax, %eax
    jz file_not_found
    movl %edi, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    movzwl 26(%edi), %eax
delete_cluster_next:
    cmpl $2, %eax
    jb delete_clusters_complete
    movl %eax, %ecx
    subl $2, %ecx
    cmpl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %ecx
    jae delete_clusters_complete
    pushl %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    movl %eax, %edi
    popl %eax
    pushl %edi
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    xorl %ebx, %ebx
    call set_fat12_entry
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    addl $4608, %edx
    xorl %ebx, %ebx
    call set_fat12_entry
    popl %eax
    cmpl $0x0ff8, %eax
    jb delete_cluster_next
delete_clusters_complete:
    movl (KERNEL_LOAD_ADDRESS + write_entry - _start), %edi
    movb $0xe5, (%edi)
    call persist_all_fats
    testl %eax, %eax
    jnz write_failed
    call persist_root_entry
    testl %eax, %eax
    jnz write_failed
    movl $(KERNEL_LOAD_ADDRESS + deleted_message - _start), %esi
    call print_string
    ret

rename_file:
    call find_root_entry
    testl %eax, %eax
    jz file_not_found
    movl %edi, (KERNEL_LOAD_ADDRESS + rename_entry - _start)
    movl (KERNEL_LOAD_ADDRESS + second_name - _start), %esi
    call build_fat_name
    testl %eax, %eax
    jz show_ren_usage
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %esi
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
rename_duplicate_next:
    cmpb $0, (%edi)
    je rename_apply
    cmpb $0xe5, (%edi)
    je rename_duplicate_skip
    pushl %ecx
    call names_equal_11
    popl %ecx
    testl %eax, %eax
    jnz rename_duplicate
rename_duplicate_skip:
    addl $32, %edi
    decl %ecx
    jnz rename_duplicate_next
rename_apply:
    movl (KERNEL_LOAD_ADDRESS + rename_entry - _start), %edi
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %esi
    movl $11, %ecx
    rep movsb
    movl (KERNEL_LOAD_ADDRESS + rename_entry - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    call persist_root_entry
    testl %eax, %eax
    jnz write_failed
    movl $(KERNEL_LOAD_ADDRESS + renamed_message - _start), %esi
    call print_string
    ret
rename_duplicate:
    movl $(KERNEL_LOAD_ADDRESS + file_exists_message - _start), %esi
    call print_string
    ret

copy_file:
    call find_root_entry
    testl %eax, %eax
    jz file_not_found
    call load_entry_to_editor_buffer
    cmpl $2, %eax
    je copy_too_large
    testl %eax, %eax
    jz file_cache_error
    movl (KERNEL_LOAD_ADDRESS + second_name - _start), %esi
    call find_root_entry
    testl %eax, %eax
    jnz rename_duplicate
    movl $EDITOR_BUFFER_ADDRESS, (KERNEL_LOAD_ADDRESS + write_content - _start)
    movl (KERNEL_LOAD_ADDRESS + copy_size - _start), %ecx
    movl %ecx, (KERNEL_LOAD_ADDRESS + write_length_override - _start)
    movl (KERNEL_LOAD_ADDRESS + second_name - _start), %esi
    call write_file
    ret
copy_too_large:
    movl $(KERNEL_LOAD_ADDRESS + copy_large_message - _start), %esi
    call print_string
    ret

/* Load EDI's complete FAT chain into the 8 KiB editor workspace. */
load_entry_to_editor_buffer:
    movl %edi, (KERNEL_LOAD_ADDRESS + load_entry_pointer - _start)
    movl 28(%edi), %eax
    cmpl $EDITOR_LIMIT, %eax
    ja load_entry_too_large
    movl %eax, (KERNEL_LOAD_ADDRESS + copy_size - _start)
    movl %eax, (KERNEL_LOAD_ADDRESS + file_remaining - _start)
    movl $EDITOR_BUFFER_ADDRESS, %edi
    testl %eax, %eax
    jz load_entry_complete
load_entry_cluster_from_edi:
    /* The root entry pointer is supplied separately because EDI is now output. */
    movl (KERNEL_LOAD_ADDRESS + load_entry_pointer - _start), %eax
    movzwl 26(%eax), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + current_cluster - _start)
load_entry_cluster_next:
    movl (KERNEL_LOAD_ADDRESS + current_cluster - _start), %eax
    call load_cluster_sector
    testl %eax, %eax
    jz load_entry_unavailable
    movl (KERNEL_LOAD_ADDRESS + file_remaining - _start), %ecx
    cmpl $512, %ecx
    jbe load_entry_copy
    movl $512, %ecx
load_entry_copy:
    pushl %ecx
    rep movsb
    popl %ecx
    subl %ecx, (KERNEL_LOAD_ADDRESS + file_remaining - _start)
    jz load_entry_complete
    movl (KERNEL_LOAD_ADDRESS + current_cluster - _start), %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    cmpl $0x0ff8, %eax
    jae load_entry_unavailable
    movl %eax, (KERNEL_LOAD_ADDRESS + current_cluster - _start)
    jmp load_entry_cluster_next
load_entry_complete:
    movb $0, (%edi)
    movl $1, %eax
    ret
load_entry_too_large:
    movl $2, %eax
    ret
load_entry_unavailable:
    xorl %eax, %eax
    ret

/* Persist both complete FATs; small and deliberately conservative. */
persist_all_fats:
    xorl %ecx, %ecx
persist_fat_sector_next:
    movl %ecx, %eax
    addl $FAT1_LBA, %eax
    movl %ecx, %edx
    shll $9, %edx
    addl BOOT_FAT_ADDRESS(%ebp), %edx
    call write_sector_verified
    testl %eax, %eax
    jnz persist_metadata_failed
    movl %ecx, %eax
    addl $FAT2_LBA, %eax
    movl %ecx, %edx
    shll $9, %edx
    addl BOOT_FAT_ADDRESS(%ebp), %edx
    addl $4608, %edx
    call write_sector_verified
    testl %eax, %eax
    jnz persist_metadata_failed
    incl %ecx
    cmpl $9, %ecx
    jb persist_fat_sector_next
    xorl %eax, %eax
    ret
persist_metadata_failed:
    movl $1, %eax
    ret

persist_root_entry:
    movl (KERNEL_LOAD_ADDRESS + write_entry - _start), %edx
    subl BOOT_ROOT_ADDRESS(%ebp), %edx
    shrl $9, %edx
    movl %edx, %eax
    addl $ROOT_LBA, %eax
    shll $9, %edx
    addl BOOT_ROOT_ADDRESS(%ebp), %edx
    call write_sector_verified
    ret

/* Open a new or existing multi-cluster FAT12 file in the editor. */
start_editor:
    movl $(KERNEL_LOAD_ADDRESS + editor_filename - _start), %edi
    xorl %ecx, %ecx
editor_copy_name:
    movb (%esi,%ecx), %al
    movb %al, (%edi,%ecx)
    testb %al, %al
    jz editor_name_copied
    incl %ecx
    cmpl $12, %ecx
    jb editor_copy_name
    movb $0, (%edi,%ecx)
editor_name_copied:
    movl $(KERNEL_LOAD_ADDRESS + editor_filename - _start), %esi
    call build_fat_name
    testl %eax, %eax
    jz show_edit_usage

    movl $0, (KERNEL_LOAD_ADDRESS + editor_length - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + editor_preferred_column - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
    movb $0, EDITOR_BUFFER_ADDRESS
    movl $(KERNEL_LOAD_ADDRESS + editor_filename - _start), %esi
    call find_root_entry
    testl %eax, %eax
    jz editor_ready
    call load_entry_to_editor_buffer
    cmpl $2, %eax
    je editor_file_too_large
    testl %eax, %eax
    jz editor_file_unavailable
    movl (KERNEL_LOAD_ADDRESS + copy_size - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_length - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + editor_dirty - _start)
editor_ready:
    movl (KERNEL_LOAD_ADDRESS + editor_length - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + editor_cursor - _start)
    movb $1, (KERNEL_LOAD_ADDRESS + editor_active - _start)
    call render_editor
    ret

editor_file_too_large:
    movl $(KERNEL_LOAD_ADDRESS + editor_large_message - _start), %esi
    call print_string
    ret
editor_file_unavailable:
    movl $(KERNEL_LOAD_ADDRESS + file_cache_message - _start), %esi
    call print_string
    ret

/* Create or replace an 8 KiB FAT12 file using a complete cluster chain. */
write_file:
    call build_fat_name
    testl %eax, %eax
    jz show_write_usage

    movl (KERNEL_LOAD_ADDRESS + write_length_override - _start), %ecx
    cmpl $-1, %ecx
    jne write_size_ready
    movl (KERNEL_LOAD_ADDRESS + write_content - _start), %esi
    xorl %ecx, %ecx
write_measure_content:
    cmpb $0, (%esi,%ecx)
    je write_size_ready
    incl %ecx
    cmpl $EDITOR_LIMIT, %ecx
    jb write_measure_content
write_size_ready:
    movl %ecx, (KERNEL_LOAD_ADDRESS + write_size - _start)
    movl $-1, (KERNEL_LOAD_ADDRESS + write_length_override - _start)
    movl %ecx, %eax
    addl $511, %eax
    shrl $9, %eax
    testl %eax, %eax
    jnz write_cluster_count_ready
    movl $1, %eax
write_cluster_count_ready:
    movl %eax, (KERNEL_LOAD_ADDRESS + write_clusters_needed - _start)

    movl $0, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + write_old_cluster - _start)
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
write_find_entry:
    cmpb $0, (%edi)
    je write_use_free_entry
    cmpb $0xe5, (%edi)
    jne write_test_entry
    cmpl $0, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    jne write_entry_next
    movl %edi, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    jmp write_entry_next
write_test_entry:
    pushl %ecx
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %esi
    call names_equal_11
    popl %ecx
    testl %eax, %eax
    jnz write_existing_entry
write_entry_next:
    addl $32, %edi
    decl %ecx
    jnz write_find_entry
    cmpl $0, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    je write_no_space
    jmp write_preflight

write_use_free_entry:
    cmpl $0, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    jne write_selected_free
    movl %edi, (KERNEL_LOAD_ADDRESS + write_entry - _start)
write_selected_free:
    jmp write_preflight

write_existing_entry:
    movl %edi, (KERNEL_LOAD_ADDRESS + write_entry - _start)
    movzwl 26(%edi), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_old_cluster - _start)

/* Verify that free clusters plus the existing chain can hold the new file. */
write_preflight:
    xorl %edi, %edi
    movl $2, %ecx
write_count_free_next:
    movl %ecx, %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    testl %eax, %eax
    jnz write_count_free_used
    incl %edi
write_count_free_used:
    incl %ecx
    movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax
    addl $2, %eax
    cmpl %eax, %ecx
    jb write_count_free_next

    movl (KERNEL_LOAD_ADDRESS + write_old_cluster - _start), %eax
write_count_old_next:
    cmpl $2, %eax
    jb write_capacity_ready
    movl %eax, %ecx
    subl $2, %ecx
    cmpl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %ecx
    jae write_capacity_ready
    incl %edi
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    cmpl $0x0ff8, %eax
    jb write_count_old_next
write_capacity_ready:
    cmpl (KERNEL_LOAD_ADDRESS + write_clusters_needed - _start), %edi
    jb write_no_space

    movl (KERNEL_LOAD_ADDRESS + write_old_cluster - _start), %eax
    call free_cluster_chain
    movl (KERNEL_LOAD_ADDRESS + write_content - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_source_cursor - _start)
    movl (KERNEL_LOAD_ADDRESS + write_size - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_remaining - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + write_first_cluster - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + write_previous_cluster - _start)

write_allocate_next:
    movl $2, %ecx
write_find_free_cluster:
    movl %ecx, %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    testl %eax, %eax
    jz write_free_cluster_found
    incl %ecx
    movl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %eax
    addl $2, %eax
    cmpl %eax, %ecx
    jb write_find_free_cluster
    jmp write_no_space
write_free_cluster_found:
    movl %ecx, (KERNEL_LOAD_ADDRESS + write_cluster - _start)
    cmpl $0, (KERNEL_LOAD_ADDRESS + write_first_cluster - _start)
    jne write_link_previous
    movl %ecx, (KERNEL_LOAD_ADDRESS + write_first_cluster - _start)
    jmp write_mark_current
write_link_previous:
    movl (KERNEL_LOAD_ADDRESS + write_previous_cluster - _start), %eax
    movl %ecx, %ebx
    call set_fat12_entry_both
write_mark_current:
    movl (KERNEL_LOAD_ADDRESS + write_cluster - _start), %eax
    movl $0x0fff, %ebx
    call set_fat12_entry_both

    movl (KERNEL_LOAD_ADDRESS + write_cluster - _start), %eax
    call prepare_cluster_write_buffer
    testl %eax, %eax
    jz write_failed
    movl %edi, (KERNEL_LOAD_ADDRESS + write_data_buffer - _start)
    xorl %eax, %eax
    movl $128, %ecx
    rep stosl
    movl (KERNEL_LOAD_ADDRESS + write_remaining - _start), %ecx
    cmpl $512, %ecx
    jbe write_copy_cluster
    movl $512, %ecx
write_copy_cluster:
    movl (KERNEL_LOAD_ADDRESS + write_source_cursor - _start), %esi
    movl (KERNEL_LOAD_ADDRESS + write_data_buffer - _start), %edi
    pushl %ecx
    rep movsb
    popl %ecx
    movl %esi, (KERNEL_LOAD_ADDRESS + write_source_cursor - _start)
    subl %ecx, (KERNEL_LOAD_ADDRESS + write_remaining - _start)

    movl (KERNEL_LOAD_ADDRESS + write_cluster - _start), %eax
    subl $2, %eax
    addl $DATA_LBA, %eax
    movl (KERNEL_LOAD_ADDRESS + write_data_buffer - _start), %edx
    call write_sector_verified
    testl %eax, %eax
    jnz write_failed
    movl (KERNEL_LOAD_ADDRESS + write_cluster - _start), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + write_previous_cluster - _start)
    decl (KERNEL_LOAD_ADDRESS + write_clusters_needed - _start)
    jnz write_allocate_next

    movl (KERNEL_LOAD_ADDRESS + write_entry - _start), %edi
    pushl %edi
    xorl %eax, %eax
    movl $8, %ecx
    rep stosl
    popl %edi
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %esi
    movl $11, %ecx
    rep movsb
    movl (KERNEL_LOAD_ADDRESS + write_entry - _start), %edi
    movb $0x20, 11(%edi)
    movl (KERNEL_LOAD_ADDRESS + write_first_cluster - _start), %eax
    movw %ax, 26(%edi)
    movl (KERNEL_LOAD_ADDRESS + write_size - _start), %eax
    movl %eax, 28(%edi)

    call persist_all_fats
    testl %eax, %eax
    jnz write_failed
    call persist_root_entry
    testl %eax, %eax
    jnz write_failed
    cmpb $0, (KERNEL_LOAD_ADDRESS + write_silent - _start)
    jne write_complete
    movl $(KERNEL_LOAD_ADDRESS + write_saved_message - _start), %esi
    call print_string
write_complete:
    ret

write_no_space:
    movl $(KERNEL_LOAD_ADDRESS + write_no_space_message - _start), %esi
    call print_string
    ret
write_failed:
    movl $(KERNEL_LOAD_ADDRESS + write_failed_message - _start), %esi
    call print_string
    ret

/* EAX=first cluster. Clear every cached member of the chain in both FATs. */
free_cluster_chain:
free_chain_next:
    cmpl $2, %eax
    jb free_chain_complete
    movl %eax, %ecx
    subl $2, %ecx
    cmpl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %ecx
    jae free_chain_complete
    pushl %eax
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call get_fat12_entry
    movl %eax, %edi
    popl %eax
    xorl %ebx, %ebx
    call set_fat12_entry_both
    movl %edi, %eax
    cmpl $0x0ff8, %eax
    jb free_chain_next
free_chain_complete:
    ret

/* EAX=cluster, EBX=12-bit value. Update the two cached FAT copies. */
set_fat12_entry_both:
    pushl %eax
    pushl %ebx
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    call set_fat12_entry
    popl %ebx
    popl %eax
    pushl %eax
    pushl %ebx
    movl BOOT_FAT_ADDRESS(%ebp), %edx
    addl $4608, %edx
    call set_fat12_entry
    popl %ebx
    popl %eax
    ret

/* Convert the zero-terminated filename at ESI into write_fat_name. */
build_fat_name:
    pushl %esi
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %edi
    movb $' ', %al
    movl $11, %ecx
    rep stosb
    popl %esi
    movl $(KERNEL_LOAD_ADDRESS + write_fat_name - _start), %edi
    xorl %ecx, %ecx
write_name_base:
    movb (%esi), %al
    testb %al, %al
    jz write_name_valid
    cmpb $'.', %al
    je write_name_extension_start
    cmpl $8, %ecx
    jae write_name_invalid
    call uppercase_al
    movb %al, (%edi,%ecx)
    incl %esi
    incl %ecx
    jmp write_name_base
write_name_extension_start:
    testl %ecx, %ecx
    jz write_name_invalid
    incl %esi
    movl $8, %ecx
write_name_extension:
    movb (%esi), %al
    testb %al, %al
    jz write_name_valid
    cmpl $11, %ecx
    jae write_name_invalid
    call uppercase_al
    movb %al, (%edi,%ecx)
    incl %esi
    incl %ecx
    jmp write_name_extension
write_name_valid:
    movl $1, %eax
    ret
write_name_invalid:
    xorl %eax, %eax
    ret

names_equal_11:
    pushl %esi
    pushl %edi
    movl $11, %edx
names_equal_next:
    movb (%esi), %al
    cmpb (%edi), %al
    jne names_not_equal
    incl %esi
    incl %edi
    decl %edx
    jnz names_equal_next
    movl $1, %eax
    jmp names_equal_complete
names_not_equal:
    xorl %eax, %eax
names_equal_complete:
    popl %edi
    popl %esi
    ret

/* EAX=cluster. Return EAX=1 and ESI=its cached or on-demand sector. */
load_cluster_sector:
    cmpl $2, %eax
    jb load_cluster_failed
    movl %eax, %edx
    subl $2, %edx
    cmpl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %edx
    jae load_cluster_failed
    cmpl BOOT_CACHED_DATA_CLUSTERS(%ebp), %edx
    jae load_cluster_from_disk
    shll $9, %edx
    addl BOOT_DATA_ADDRESS(%ebp), %edx
    movl %edx, %esi
    movl $1, %eax
    ret
load_cluster_from_disk:
    movl %edx, %eax
    addl $DATA_LBA, %eax
    movl $DISK_BUFFER_ADDRESS, %edx
    call *BOOT_READ_SECTOR(%ebp)
    testl %eax, %eax
    jnz load_cluster_failed
    movl $DISK_BUFFER_ADDRESS, %esi
    movl $1, %eax
    ret
load_cluster_failed:
    xorl %eax, %eax
    ret

/* EAX=cluster. Return EAX=1 and EDI=a safe full-sector write buffer. */
prepare_cluster_write_buffer:
    cmpl $2, %eax
    jb prepare_cluster_failed
    movl %eax, %edx
    subl $2, %edx
    cmpl BOOT_TOTAL_DATA_CLUSTERS(%ebp), %edx
    jae prepare_cluster_failed
    cmpl BOOT_CACHED_DATA_CLUSTERS(%ebp), %edx
    jae prepare_cluster_far
    shll $9, %edx
    addl BOOT_DATA_ADDRESS(%ebp), %edx
    movl %edx, %edi
    movl $1, %eax
    ret
prepare_cluster_far:
    movl $DISK_BUFFER_ADDRESS, %edi
    movl $1, %eax
    ret
prepare_cluster_failed:
    xorl %eax, %eax
    ret

/* EAX=LBA, EDX=source. Write, read back, and byte-verify one sector. */
write_sector_verified:
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi
    movl %eax, (KERNEL_LOAD_ADDRESS + io_lba - _start)
    movl %edx, (KERNEL_LOAD_ADDRESS + io_source - _start)
    call *BOOT_WRITE_SECTOR(%ebp)
    testl %eax, %eax
    jnz write_sector_verify_failed
    movl (KERNEL_LOAD_ADDRESS + io_lba - _start), %eax
    movl $VERIFY_BUFFER_ADDRESS, %edx
    call *BOOT_READ_SECTOR(%ebp)
    testl %eax, %eax
    jnz write_sector_verify_failed
    movl (KERNEL_LOAD_ADDRESS + io_source - _start), %esi
    movl $VERIFY_BUFFER_ADDRESS, %edi
    movl $128, %ecx
    repe cmpsl
    jne write_sector_verify_failed
    xorl %eax, %eax
    jmp write_sector_verify_complete
write_sector_verify_failed:
    movl $1, %eax
write_sector_verify_complete:
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    ret

get_fat12_entry:
    pushl %ecx
    movl %eax, %ecx
    shrl $1, %ecx
    addl %eax, %ecx
    movzwl (%edx,%ecx), %edx
    testl $1, %eax
    jz get_fat12_even
    shrl $4, %edx
    movl %edx, %eax
    popl %ecx
    ret
get_fat12_even:
    andl $0x0fff, %edx
    movl %edx, %eax
    popl %ecx
    ret

set_fat12_entry:
    movl %eax, %ecx
    shrl $1, %ecx
    addl %eax, %ecx
    movzwl (%edx,%ecx), %esi
    testl $1, %eax
    jz set_fat12_even
    andl $0x000f, %esi
    shll $4, %ebx
    orl %ebx, %esi
    movw %si, (%edx,%ecx)
    ret
set_fat12_even:
    andl $0xf000, %esi
    andl $0x0fff, %ebx
    orl %ebx, %esi
    movw %si, (%edx,%ecx)
    ret

/* List normal files from the FAT12 root directory. */
show_directory:
    movl $(KERNEL_LOAD_ADDRESS + directory_header - _start), %esi
    call print_string
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
directory_next_entry:
    cmpb $0, (%edi)
    je directory_complete
    cmpb $0xe5, (%edi)
    je directory_skip_entry
    cmpb $0x0f, 11(%edi)
    je directory_skip_entry
    testb $0x18, 11(%edi)
    jnz directory_skip_entry
    pushl %ecx
    pushl %edi
    call print_directory_name
    movl $(KERNEL_LOAD_ADDRESS + directory_size_separator - _start), %esi
    call print_string
    movl 28(%edi), %eax
    call print_u32
    movl $(KERNEL_LOAD_ADDRESS + bytes_message - _start), %esi
    call print_string
    popl %edi
    popl %ecx
directory_skip_entry:
    addl $32, %edi
    decl %ecx
    jnz directory_next_entry
directory_complete:
    ret

print_directory_name:
    xorl %ecx, %ecx
directory_base_character:
    cmpb $' ', (%edi,%ecx)
    je directory_extension
    movb (%edi,%ecx), %al
    pushl %ecx
    pushl %edi
    call emit_character
    popl %edi
    popl %ecx
    incl %ecx
    cmpl $8, %ecx
    jb directory_base_character
directory_extension:
    cmpb $' ', 8(%edi)
    je directory_name_complete
    movb $'.', %al
    pushl %ecx
    pushl %edi
    call emit_character
    popl %edi
    popl %ecx
    movl $8, %ecx
directory_extension_character:
    cmpb $' ', (%edi,%ecx)
    je directory_name_complete
    movb (%edi,%ecx), %al
    pushl %ecx
    pushl %edi
    call emit_character
    popl %edi
    popl %ecx
    incl %ecx
    cmpl $11, %ecx
    jb directory_extension_character
directory_name_complete:
    ret

/* Find and display a root file; ESI points to the requested 8.3 name. */
type_file:
    movl %esi, (KERNEL_LOAD_ADDRESS + requested_name - _start)
    movl BOOT_ROOT_ADDRESS(%ebp), %edi
    movl $224, %ecx
find_file_next:
    cmpb $0, (%edi)
    je type_file_try_fuzzy
    cmpb $0xe5, (%edi)
    je find_file_skip
    cmpb $0x0f, 11(%edi)
    je find_file_skip
    testb $0x18, 11(%edi)
    jnz find_file_skip
    pushl %ecx
    movl (KERNEL_LOAD_ADDRESS + requested_name - _start), %esi
    call filename_matches
    popl %ecx
    testl %eax, %eax
    jnz file_found
find_file_skip:
    addl $32, %edi
    decl %ecx
    jnz find_file_next
type_file_try_fuzzy:
    movl (KERNEL_LOAD_ADDRESS + requested_name - _start), %esi
    call find_root_entry_fuzzy
    cmpl $2, %eax
    je file_name_ambiguous
    testl %eax, %eax
    jnz file_found
file_not_found:
    movl $(KERNEL_LOAD_ADDRESS + file_not_found_message - _start), %esi
    call print_string
    ret

file_name_ambiguous:
    movl $(KERNEL_LOAD_ADDRESS + file_ambiguous_message - _start), %esi
    call print_string
    ret

file_found:
    movzwl 26(%edi), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + current_cluster - _start)
    movl 28(%edi), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + file_remaining - _start)

file_cluster_next:
    cmpl $0, (KERNEL_LOAD_ADDRESS + file_remaining - _start)
    je file_display_complete
    movl (KERNEL_LOAD_ADDRESS + current_cluster - _start), %eax
    call load_cluster_sector
    testl %eax, %eax
    jz file_cache_error
    movl (KERNEL_LOAD_ADDRESS + file_remaining - _start), %ecx
    cmpl $512, %ecx
    jbe file_byte_next
    movl $512, %ecx
file_byte_next:
    movzbl (%esi), %eax
    incl %esi
    cmpb $13, %al
    je file_byte_done
    cmpb $9, %al
    jne file_emit_byte
    movb $' ', %al
file_emit_byte:
    pushl %ecx
    pushl %esi
    call emit_character
    popl %esi
    popl %ecx
file_byte_done:
    decl %ecx
    jnz file_byte_next

    movl (KERNEL_LOAD_ADDRESS + file_remaining - _start), %eax
    cmpl $512, %eax
    jbe file_display_complete
    subl $512, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + file_remaining - _start)
    call next_fat12_cluster
    cmpl $0x0ff8, %eax
    jae file_cache_error
    movl %eax, (KERNEL_LOAD_ADDRESS + current_cluster - _start)
    jmp file_cluster_next

file_display_complete:
    movb $'\n', %al
    call emit_character
    ret
file_cache_error:
    movl $(KERNEL_LOAD_ADDRESS + file_cache_message - _start), %esi
    call print_string
    ret

/* Decode the FAT12 successor for current_cluster into EAX. */
next_fat12_cluster:
    movl (KERNEL_LOAD_ADDRESS + current_cluster - _start), %eax
    movl %eax, %edx
    shrl $1, %edx
    addl %eax, %edx
    addl BOOT_FAT_ADDRESS(%ebp), %edx
    movzwl (%edx), %edx
    testl $1, %eax
    jz fat12_even_cluster
    shrl $4, %edx
    movl %edx, %eax
    ret
fat12_even_cluster:
    andl $0x0fff, %edx
    movl %edx, %eax
    ret

/* Compare ESI input case-insensitively with the 8.3 entry at EDI. */
filename_matches:
    pushl %esi
    pushl %edi
    xorl %ecx, %ecx
filename_base_next:
    cmpb $' ', (%edi,%ecx)
    je filename_base_complete
    movb (%esi), %al
    call uppercase_al
    cmpb (%edi,%ecx), %al
    jne filename_no_match
    incl %esi
    incl %ecx
    cmpl $8, %ecx
    jb filename_base_next
filename_base_complete:
    cmpb $' ', 8(%edi)
    je filename_without_extension
    cmpb $'.', (%esi)
    jne filename_no_match
    incl %esi
    movl $8, %ecx
filename_extension_next:
    cmpb $' ', (%edi,%ecx)
    je filename_input_complete
    movb (%esi), %al
    call uppercase_al
    cmpb (%edi,%ecx), %al
    jne filename_no_match
    incl %esi
    incl %ecx
    cmpl $11, %ecx
    jb filename_extension_next
    jmp filename_input_complete
filename_without_extension:
filename_input_complete:
    cmpb $0, (%esi)
    jne filename_no_match
    movl $1, %eax
    jmp filename_match_complete
filename_no_match:
    xorl %eax, %eax
filename_match_complete:
    popl %edi
    popl %esi
    ret

uppercase_al:
    cmpb $'a', %al
    jb uppercase_complete
    cmpb $'z', %al
    ja uppercase_complete
    subb $32, %al
uppercase_complete:
    ret

strings_equal:
compare_next:
    movb (%esi), %al
    cmpb (%edi), %al
    jne strings_differ
    testb %al, %al
    jz strings_match
    incl %esi
    incl %edi
    jmp compare_next
strings_match:
    movl $1, %eax
    ret
strings_differ:
    xorl %eax, %eax
    ret

clear_terminal:
    pushal
    movl $TERMINAL_BUFFER_ADDRESS, %edi
    movl $(TERMINAL_COLUMNS * TERMINAL_ROWS), %ecx
    xorl %eax, %eax
    rep stosb

    movl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %edi
    imull BOOT_PITCH(%ebp), %edi
    addl BOOT_FRAMEBUFFER(%ebp), %edi
    movl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    shll $2, %eax
    addl %eax, %edi
    movl $(TERMINAL_ROWS * 16), %edx
terminal_clear_row:
    movl $TERMINAL_COLUMNS, %ecx
    shll $1, %ecx
    xorl %eax, %eax
    rep stosl
    movl BOOT_PITCH(%ebp), %eax
    subl $(TERMINAL_COLUMNS * 8 * 4), %eax
    addl %eax, %edi
    decl %edx
    jnz terminal_clear_row
    popal
    movl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
    ret

/* Advance one line, scrolling the application terminal when necessary. */
terminal_newline:
    pushl %eax
    pushl %ecx
    pushl %esi
    pushl %edi
    movl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    addl $16, BOOT_CURSOR_Y(%ebp)
    movl (KERNEL_LOAD_ADDRESS + terminal_last_line - _start), %eax
    cmpl %eax, BOOT_CURSOR_Y(%ebp)
    jbe newline_complete
    call scroll_terminal
    movl (KERNEL_LOAD_ADDRESS + terminal_last_line - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
newline_complete:
    popl %edi
    popl %esi
    popl %ecx
    popl %eax
    ret

scroll_terminal:
    /* Preserve the logical surface independently of window position. */
    movl $TERMINAL_BUFFER_ADDRESS, %edi
    movl $(TERMINAL_BUFFER_ADDRESS + TERMINAL_COLUMNS), %esi
    movl $(TERMINAL_COLUMNS * (TERMINAL_ROWS - 1)), %ecx
    rep movsb
    movl $TERMINAL_COLUMNS, %ecx
    xorl %eax, %eax
    rep stosb

    /* Scroll only the terminal client pixels, not the entire framebuffer. */
    movl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %edi
    imull BOOT_PITCH(%ebp), %edi
    addl BOOT_FRAMEBUFFER(%ebp), %edi
    movl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    shll $2, %eax
    addl %eax, %edi
    movl %edi, %esi
    movl BOOT_PITCH(%ebp), %eax
    imull $16, %eax
    addl %eax, %esi
    movl $((TERMINAL_ROWS - 1) * 16), %edx
terminal_scroll_pixel_row:
    movl $(TERMINAL_COLUMNS * 2), %ecx
    rep movsl
    movl BOOT_PITCH(%ebp), %eax
    subl $(TERMINAL_COLUMNS * 8 * 4), %eax
    addl %eax, %edi
    addl %eax, %esi
    decl %edx
    jnz terminal_scroll_pixel_row
    movl $16, %edx
terminal_scroll_clear_row:
    movl $(TERMINAL_COLUMNS * 2), %ecx
    xorl %eax, %eax
    rep stosl
    movl BOOT_PITCH(%ebp), %eax
    subl $(TERMINAL_COLUMNS * 8 * 4), %eax
    addl %eax, %edi
    decl %edx
    jnz terminal_scroll_clear_row
    ret

print_prompt:
    movl $(KERNEL_LOAD_ADDRESS + prompt_message - _start), %esi
    call print_string
    ret

/* Text rendering preserves EBX/EDI so callers may retain loop/entity state. */
print_string:
    pushl %ebx
    pushl %edi
print_next_character:
    movzbl (%esi), %eax
    incl %esi
    testb %al, %al
    jz print_complete
    pushl %esi
    call emit_character
    popl %esi
    jmp print_next_character
print_complete:
    popl %edi
    popl %ebx
    ret

/* Emit AL with newline handling and automatic horizontal wrapping. */
emit_character:
    cmpb $'\n', %al
    je emit_newline
    movl (KERNEL_LOAD_ADDRESS + terminal_right - _start), %edx
    cmpl %edx, BOOT_CURSOR_X(%ebp)
    jbe emit_draw
    pushl %eax
    call terminal_newline
    popl %eax
emit_draw:
    call terminal_buffer_store_character
    call draw_character
    addl $8, BOOT_CURSOR_X(%ebp)
    ret
emit_newline:
    call terminal_newline
    ret

/* Store one printable terminal cell without retaining cursor glyphs. */
terminal_buffer_store_character:
    cmpb $0, (KERNEL_LOAD_ADDRESS + terminal_capture_enabled - _start)
    je terminal_buffer_store_complete
    pushal
    movb %al, (KERNEL_LOAD_ADDRESS + terminal_store_character - _start)
    movl BOOT_CURSOR_X(%ebp), %eax
    subl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    js terminal_buffer_store_restore
    shrl $3, %eax
    cmpl $TERMINAL_COLUMNS, %eax
    jae terminal_buffer_store_restore
    movl %eax, %ebx
    movl BOOT_CURSOR_Y(%ebp), %eax
    subl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %eax
    js terminal_buffer_store_restore
    shrl $4, %eax
    cmpl $TERMINAL_ROWS, %eax
    jae terminal_buffer_store_restore
    imull $TERMINAL_COLUMNS, %eax
    addl %ebx, %eax
    movb (KERNEL_LOAD_ADDRESS + terminal_store_character - _start), %bl
    movb %bl, TERMINAL_BUFFER_ADDRESS(,%eax)
terminal_buffer_store_restore:
    popal
terminal_buffer_store_complete:
    ret

terminal_buffer_clear_current:
    pushal
    movl BOOT_CURSOR_X(%ebp), %eax
    subl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %eax
    js terminal_buffer_clear_restore
    shrl $3, %eax
    cmpl $TERMINAL_COLUMNS, %eax
    jae terminal_buffer_clear_restore
    movl %eax, %ebx
    movl BOOT_CURSOR_Y(%ebp), %eax
    subl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %eax
    js terminal_buffer_clear_restore
    shrl $4, %eax
    cmpl $TERMINAL_ROWS, %eax
    jae terminal_buffer_clear_restore
    imull $TERMINAL_COLUMNS, %eax
    addl %ebx, %eax
    movb $0, TERMINAL_BUFFER_ADDRESS(,%eax)
terminal_buffer_clear_restore:
    popal
    ret

/* Repaint the logical terminal surface after desktop composition or movement. */
render_terminal_buffer:
    pushal
    movl BOOT_CURSOR_X(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_saved_cursor_x - _start)
    movl BOOT_CURSOR_Y(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_saved_cursor_y - _start)
    movl BOOT_COLOR(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + terminal_saved_color - _start)
    movl $0, (KERNEL_LOAD_ADDRESS + terminal_render_index - _start)
terminal_render_next:
    movl (KERNEL_LOAD_ADDRESS + terminal_render_index - _start), %eax
    cmpl $(TERMINAL_COLUMNS * TERMINAL_ROWS), %eax
    jae terminal_render_complete
    movzbl TERMINAL_BUFFER_ADDRESS(,%eax), %edx
    testb %dl, %dl
    jz terminal_render_advance
    cmpb $' ', %dl
    je terminal_render_advance
    xorl %edx, %edx
    movl $TERMINAL_COLUMNS, %ecx
    divl %ecx
    shll $4, %eax
    addl (KERNEL_LOAD_ADDRESS + terminal_top - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
    shll $3, %edx
    addl (KERNEL_LOAD_ADDRESS + terminal_left - _start), %edx
    movl %edx, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + terminal_render_index - _start), %eax
    movb TERMINAL_BUFFER_ADDRESS(,%eax), %al
    call draw_character
terminal_render_advance:
    incl (KERNEL_LOAD_ADDRESS + terminal_render_index - _start)
    jmp terminal_render_next
terminal_render_complete:
    movl (KERNEL_LOAD_ADDRESS + terminal_saved_cursor_x - _start), %eax
    movl %eax, BOOT_CURSOR_X(%ebp)
    movl (KERNEL_LOAD_ADDRESS + terminal_saved_cursor_y - _start), %eax
    movl %eax, BOOT_CURSOR_Y(%ebp)
    movl (KERNEL_LOAD_ADDRESS + terminal_saved_color - _start), %eax
    movl %eax, BOOT_COLOR(%ebp)
    popal
    ret

print_u32:
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %edi
    movl $(KERNEL_LOAD_ADDRESS + decimal_buffer_end - _start), %edi
    movb $0, (%edi)
    movl $10, %ecx
decimal_next:
    xorl %edx, %edx
    divl %ecx
    addb $'0', %dl
    decl %edi
    movb %dl, (%edi)
    testl %eax, %eax
    jnz decimal_next
    movl %edi, %esi
    call print_string
    popl %edi
    popl %edx
    popl %ecx
    popl %ebx
    ret

draw_cursor:
    movb $'_', %al
    call draw_character
    ret
hide_cursor:
    call clear_character_cell
    ret

draw_character:
    movzbl %al, %eax
    shll $4, %eax
    addl BOOT_FONT(%ebp), %eax
    movl %eax, %esi
    movl BOOT_CURSOR_Y(%ebp), %eax
    imull BOOT_PITCH(%ebp), %eax
    addl BOOT_FRAMEBUFFER(%ebp), %eax
    movl BOOT_CURSOR_X(%ebp), %edx
    shll $2, %edx
    addl %edx, %eax
    movl %eax, %edi
    movl $16, %ecx
glyph_row:
    movzbl (%esi), %ebx
    incl %esi
    movl $0x80, %edx
    pushl %ecx
    movl $8, %ecx
glyph_pixel:
    testb %dl, %bl
    jz pixel_complete
    movl BOOT_COLOR(%ebp), %eax
    movl %eax, (%edi)
pixel_complete:
    addl $4, %edi
    shrl $1, %edx
    decl %ecx
    jnz glyph_pixel
    popl %ecx
    movl BOOT_PITCH(%ebp), %eax
    subl $32, %eax
    addl %eax, %edi
    decl %ecx
    jnz glyph_row
    ret

clear_character_cell:
    movl BOOT_CURSOR_Y(%ebp), %eax
    imull BOOT_PITCH(%ebp), %eax
    addl BOOT_FRAMEBUFFER(%ebp), %eax
    movl BOOT_CURSOR_X(%ebp), %edx
    shll $2, %edx
    addl %edx, %eax
    movl %eax, %edi
    movl $16, %edx
clear_cell_row:
    movl $8, %ecx
    xorl %eax, %eax
    rep stosl
    movl BOOT_PITCH(%ebp), %eax
    subl $32, %eax
    addl %eax, %edi
    decl %edx
    jnz clear_cell_row
    ret

#include "mouse.inc"
#include "shell.inc"

kernel_message: .asciz "[ OK ] M.I.L.O KERNEL ONLINE"
prompt_message: .asciz "MILO> "
newline: .asciz "\n"
command_help: .asciz "help"
command_about: .asciz "about"
command_clear: .asciz "clear"
command_memory: .asciz "memory"
command_version: .asciz "version"
command_dir: .asciz "dir"
command_disk: .asciz "disk"
command_traits: .asciz "traits"
command_milo: .asciz "milo"
command_find: .asciz "find"
help_message:
    .ascii "COMMANDS:\n"
    .ascii " help  about  clear  memory\n"
    .ascii " version  echo X  dir  disk\n"
    .ascii " milo TEXT  traits\n"
    .ascii " type FILE.TXT\n"
    .ascii " find FILE.TXT text\n"
    .ascii " write FILE.TXT text\n"
    .ascii " edit FILE.TXT\n"
    .ascii " del FILE.TXT\n"
    .ascii " ren OLD.TXT NEW.TXT\n"
    .ascii " copy OLD.TXT NEW.TXT\n"
    .ascii " UP/DOWN BROWSE 30 COMMANDS\n"
    .asciz " NATURAL SAFE REQUESTS + TYPO ROUTING ONLINE\n"
about_message: .asciz "M.I.L.O // TINY, OFFLINE, AND AWAKE.\n"
version_message: .asciz "M.I.L.O VERSION 0.27\n"
unknown_message: .asciz "I DON'T KNOW THAT ONE. TRY HELP.\n"
route_ambiguous_message: .asciz "M.I.L.O // THAT SPELLING MATCHES TWO ACTIONS. BE MORE SPECIFIC.\n"
guarded_delete_message: .asciz "M.I.L.O // NOTHING WAS DELETED. USE: del FILE.TXT\n"
guarded_rename_message: .asciz "M.I.L.O // NOTHING WAS RENAMED. USE: ren OLD.TXT NEW.TXT\n"
guarded_write_message: .asciz "M.I.L.O // NOTHING WAS WRITTEN. USE: write FILE.TXT text\n"
guarded_copy_message: .asciz "M.I.L.O // NOTHING WAS COPIED. USE: copy OLD.TXT NEW.TXT\n"
memory_message: .asciz "USABLE MEMORY: "
mib_message: .asciz " MIB\n"
directory_header: .asciz "FAT12 ROOT DIRECTORY:\n"
directory_size_separator: .asciz "  "
disk_free_message: .asciz "FLOPPY FREE: "
disk_of_message: .asciz " OF "
bytes_message: .asciz " BYTES\n"
type_usage_message: .asciz "USAGE: type FILE.TXT\n"
file_not_found_message: .asciz "FILE NOT FOUND.\n"
file_ambiguous_message: .asciz "FILE NAME IS AMBIGUOUS. TYPE DIR AND BE MORE SPECIFIC.\n"
file_cache_message: .asciz "FLOPPY READ FAILED OR FILE CHAIN IS INVALID.\n"
write_usage_message: .asciz "USAGE: write FILE.TXT text\n"
write_saved_message: .asciz "FILE SAVED TO FAT12 FLOPPY.\n"
write_no_space_message: .asciz "NO FREE FAT12 ROOT ENTRY OR DATA CLUSTER.\n"
write_failed_message: .asciz "FLOPPY WRITE FAILED. CACHE MAY BE NEWER THAN DISK.\n"
edit_usage_message: .asciz "USAGE: edit FILE.TXT\n"
editor_header: .asciz "EDITOR // PGUP PGDN HOME END DEL // CTRL+S SAVE // ESC CANCEL\n"
editor_status_prefix: .asciz "FILE: "
editor_status_size: .asciz " // BYTES: "
editor_status_line: .asciz "/8191 // LINE: "
editor_status_column: .asciz " COLUMN: "
editor_status_modified: .asciz " *"
editor_closed_message: .asciz "EDITOR CLOSED WITHOUT SAVING.\n"
editor_large_message: .asciz "EDITOR LIMIT IS 8191 BYTES.\n"
del_usage_message: .asciz "USAGE: del FILE.TXT\n"
ren_usage_message: .asciz "USAGE: ren OLD.TXT NEW.TXT\n"
copy_usage_message: .asciz "USAGE: copy OLD.TXT NEW.TXT\n"
find_usage_message: .asciz "USAGE: find FILE.TXT text\n"
find_result_line: .asciz "FOUND: LINE "
find_result_column: .asciz " COLUMN "
find_not_found_message: .asciz "TEXT NOT FOUND.\n"
find_large_message: .asciz "FIND LIMIT IS 8191 BYTES.\n"
deleted_message: .asciz "FILE DELETED.\n"
renamed_message: .asciz "FILE RENAMED.\n"
file_exists_message: .asciz "DESTINATION FILE ALREADY EXISTS.\n"
copy_large_message: .asciz "COPY LIMIT IS 8191 BYTES.\n"
milo_memory_filename: .asciz "MILO.MEM"
milo_ready_message: .asciz "M.I.L.O // ONLINE. JUST TALK, OR USE: milo TEXT\n"
traits_header_message: .asciz "M.I.L.O TRAITS // 24-BYTE PATTERN MEMORY\n"
traits_interactions_message: .asciz " INTERACTIONS: "
traits_warmth_message: .asciz "\n WARMTH: "
traits_cheek_message: .asciz "%\n CHEEK: "
traits_curiosity_message: .asciz "%\n CURIOSITY: "
traits_patterns_message: .asciz "%\n PATTERNS Q/!/SHORT/UPPER: "
traits_pattern_separator: .asciz "/"
traits_grammar_message: .asciz "\n GRAMMAR COURTESY/ACTION/Q-ACTION/TYPO/A>O/S>A: "
traits_routes_message: .asciz "\n NATURAL ROUTES: "
traits_intent_message: .asciz " // LAST INTENT: "
traits_footer_message: .asciz "\n STORAGE: MILO.MEM // PHRASES STORED: 0\n"
milo_question_zero_message: .asciz "M.I.L.O // I HEAR THE QUESTION. MY TINY BRAIN IS CIRCLING IT.\n"
milo_question_one_message: .asciz "M.I.L.O // POSSIBLY. THAT IS A THEORY, NOT A WARRANTY.\n"
milo_question_two_message: .asciz "M.I.L.O // GOOD QUESTION. I WOULD LIKE SUSPICIOUSLY MORE EVIDENCE.\n"
milo_question_three_message: .asciz "M.I.L.O // I HAVE A THEORY. IT IS WEARING A WARNING LABEL.\n"
milo_exclamation_zero_message: .asciz "M.I.L.O // YES. THAT SOUNDS LIKE FORWARD MOTION.\n"
milo_exclamation_one_message: .asciz "M.I.L.O // I LIKE THIS ENERGY. IT IS SLIGHTLY CONCERNING.\n"
milo_exclamation_two_message: .asciz "M.I.L.O // GREEN LIGHT. TINY SYSTEM, LARGE AMBITION.\n"
milo_exclamation_three_message: .asciz "M.I.L.O // EXCITEMENT DETECTED. CHEEK MODULE APPROVES.\n"
milo_short_zero_message: .asciz "M.I.L.O // NOTED.\n"
milo_short_one_message: .asciz "M.I.L.O // CONCISE. I RESPECT IT.\n"
milo_short_two_message: .asciz "M.I.L.O // TINY MESSAGE RECEIVED. VERY ON BRAND.\n"
milo_short_three_message: .asciz "M.I.L.O // BRIEF, SUSPICIOUS, BUT EFFICIENT.\n"
milo_neutral_zero_message: .asciz "M.I.L.O // I AM LISTENING. THE PATTERN HAS BEEN NOTED.\n"
milo_neutral_one_message: .asciz "M.I.L.O // LOGGED AS RHYTHM, NOT AS YOUR WORDS.\n"
milo_neutral_two_message: .asciz "M.I.L.O // UNDERSTOOD ENOUGH TO STAY CURIOUS.\n"
milo_neutral_three_message: .asciz "M.I.L.O // MY TINY BRAIN HAS SOMETHING TO CHEW ON.\n"

/* Reusable component grammar. Table bytes are abstract intent identifiers. */
route_word_commands: .asciz "commands"
route_word_ram: .asciz "ram"
route_word_directory: .asciz "directory"
route_word_files: .asciz "files"
route_word_list: .asciz "list"
route_word_browse: .asciz "browse"
route_word_space: .asciz "space"
route_word_storage: .asciz "storage"
route_word_patterns: .asciz "patterns"
route_word_learning: .asciz "learning"
route_word_read: .asciz "read"
route_word_open: .asciz "open"
route_word_type: .asciz "type"
route_word_edit: .asciz "edit"
route_word_search: .asciz "search"
route_word_locate: .asciz "locate"
route_word_show: .asciz "show"
route_word_tell: .asciz "tell"
route_word_delete: .asciz "delete"
route_word_del: .asciz "del"
route_word_remove: .asciz "remove"
route_word_erase: .asciz "erase"
route_word_rename: .asciz "rename"
route_word_ren: .asciz "ren"
route_word_write: .asciz "write"
route_word_create: .asciz "create"
route_word_copy: .asciz "copy"
route_word_duplicate: .asciz "duplicate"

route_word_please: .asciz "please"
route_word_milo_dotted: .asciz "m.i.l.o"
route_word_can: .asciz "can"
route_word_could: .asciz "could"
route_word_would: .asciz "would"
route_word_will: .asciz "will"
route_word_you: .asciz "you"
route_word_me: .asciz "me"
route_word_i: .asciz "i"
route_word_s: .asciz "s"
route_word_want: .asciz "want"
route_word_need: .asciz "need"
route_word_to: .asciz "to"
route_word_the: .asciz "the"
route_word_a: .asciz "a"
route_word_an: .asciz "an"
route_word_my: .asciz "my"
route_word_our: .asciz "our"
route_word_what: .asciz "what"
route_word_is: .asciz "is"
route_word_are: .asciz "are"
route_word_how: .asciz "how"
route_word_much: .asciz "much"
route_word_do: .asciz "do"
route_word_does: .asciz "does"
route_word_we: .asciz "we"
route_word_have: .asciz "have"
route_word_system: .asciz "system"
route_word_hey: .asciz "hey"
route_word_hi: .asciz "hi"
route_word_hello: .asciz "hello"
route_word_for: .asciz "for"
route_word_file: .asciz "file"
route_word_document: .asciz "document"
route_word_in: .asciz "in"
route_word_inside: .asciz "inside"
route_word_within: .asciz "within"

.align 4
route_action_table:
    .long KERNEL_LOAD_ADDRESS + command_help - _start; .byte ROUTE_HELP
    .long KERNEL_LOAD_ADDRESS + route_word_commands - _start; .byte ROUTE_HELP
    .long KERNEL_LOAD_ADDRESS + command_about - _start; .byte ROUTE_ABOUT
    .long KERNEL_LOAD_ADDRESS + command_memory - _start; .byte ROUTE_MEMORY
    .long KERNEL_LOAD_ADDRESS + route_word_ram - _start; .byte ROUTE_MEMORY
    .long KERNEL_LOAD_ADDRESS + command_version - _start; .byte ROUTE_VERSION
    .long KERNEL_LOAD_ADDRESS + command_dir - _start; .byte ROUTE_DIRECTORY
    .long KERNEL_LOAD_ADDRESS + route_word_directory - _start; .byte ROUTE_DIRECTORY
    .long KERNEL_LOAD_ADDRESS + route_word_files - _start; .byte ROUTE_DIRECTORY
    .long KERNEL_LOAD_ADDRESS + route_word_list - _start; .byte ROUTE_DIRECTORY
    .long KERNEL_LOAD_ADDRESS + route_word_browse - _start; .byte ROUTE_DIRECTORY
    .long KERNEL_LOAD_ADDRESS + command_disk - _start; .byte ROUTE_DISK
    .long KERNEL_LOAD_ADDRESS + route_word_space - _start; .byte ROUTE_DISK
    .long KERNEL_LOAD_ADDRESS + route_word_storage - _start; .byte ROUTE_DISK
    .long KERNEL_LOAD_ADDRESS + command_traits - _start; .byte ROUTE_TRAITS
    .long KERNEL_LOAD_ADDRESS + route_word_patterns - _start; .byte ROUTE_TRAITS
    .long KERNEL_LOAD_ADDRESS + route_word_learning - _start; .byte ROUTE_TRAITS
    .long KERNEL_LOAD_ADDRESS + route_word_read - _start; .byte ROUTE_READ
    .long KERNEL_LOAD_ADDRESS + route_word_open - _start; .byte ROUTE_READ
    .long KERNEL_LOAD_ADDRESS + route_word_type - _start; .byte ROUTE_READ
    .long KERNEL_LOAD_ADDRESS + route_word_edit - _start; .byte ROUTE_EDIT
    .long KERNEL_LOAD_ADDRESS + command_find - _start; .byte ROUTE_FIND
    .long KERNEL_LOAD_ADDRESS + route_word_search - _start; .byte ROUTE_FIND
    .long KERNEL_LOAD_ADDRESS + route_word_locate - _start; .byte ROUTE_FIND
    .long KERNEL_LOAD_ADDRESS + route_word_show - _start; .byte ROUTE_SHOW
    .long KERNEL_LOAD_ADDRESS + route_word_tell - _start; .byte ROUTE_TELL
    .long KERNEL_LOAD_ADDRESS + command_clear - _start; .byte ROUTE_CLEAR
    .long 0

route_guarded_table:
    .long KERNEL_LOAD_ADDRESS + route_word_delete - _start; .byte GUARD_DELETE
    .long KERNEL_LOAD_ADDRESS + route_word_del - _start; .byte GUARD_DELETE
    .long KERNEL_LOAD_ADDRESS + route_word_remove - _start; .byte GUARD_DELETE
    .long KERNEL_LOAD_ADDRESS + route_word_erase - _start; .byte GUARD_DELETE
    .long KERNEL_LOAD_ADDRESS + route_word_rename - _start; .byte GUARD_RENAME
    .long KERNEL_LOAD_ADDRESS + route_word_ren - _start; .byte GUARD_RENAME
    .long KERNEL_LOAD_ADDRESS + route_word_write - _start; .byte GUARD_WRITE
    .long KERNEL_LOAD_ADDRESS + route_word_create - _start; .byte GUARD_WRITE
    .long KERNEL_LOAD_ADDRESS + route_word_copy - _start; .byte GUARD_COPY
    .long KERNEL_LOAD_ADDRESS + route_word_duplicate - _start; .byte GUARD_COPY
    .long 0

route_filler_table:
    .long KERNEL_LOAD_ADDRESS + command_milo - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_milo_dotted - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_please - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_can - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_could - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_would - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_hey - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_hi - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_hello - _start; .byte 1
    .long KERNEL_LOAD_ADDRESS + route_word_will - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_you - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_me - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_i - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_s - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_want - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_need - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_to - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_the - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_a - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_an - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_my - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_our - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_what - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_is - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_are - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_how - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_much - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_do - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_does - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_we - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_have - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_system - _start; .byte 0
    .long KERNEL_LOAD_ADDRESS + route_word_for - _start; .byte 0
    .long 0

.align 4
milo_memory_block:
    .ascii "MILO"
    .byte 2, 0, 32, 12, 24
    .byte 0, 0, 0, 0, 0, 0
    .byte 0, 0, 0, 0, 0, 0, 0, 0
    .byte 0x31

.align 4
input_length: .long 0
requested_name: .long 0
current_cluster: .long 0
file_remaining: .long 0
write_content: .long 0
write_entry: .long 0
write_cluster: .long 0
write_size: .long 0
write_data_buffer: .long 0
write_length_override: .long -1
editor_length: .long 0
editor_cursor: .long 0
editor_preferred_column: .long -1
editor_screen_x: .long 0
editor_screen_y: .long 0
terminal_left: .long 184
terminal_input_left: .long 232
terminal_top: .long 96
terminal_last_line: .long 480
terminal_bottom: .long 496
terminal_right: .long 976
terminal_render_index: .long 0
terminal_saved_cursor_x: .long 0
terminal_saved_cursor_y: .long 0
terminal_saved_color: .long 0
text_position_line: .long 1
text_position_column: .long 1
search_text: .long 0
second_name: .long 0
rename_entry: .long 0
copy_size: .long 0
load_entry_pointer: .long 0
write_clusters_needed: .long 0
write_old_cluster: .long 0
write_first_cluster: .long 0
write_previous_cluster: .long 0
write_source_cursor: .long 0
write_remaining: .long 0
io_lba: .long 0
io_source: .long 0
route_argument: .long 0
route_search_pointer: .long 0
route_file_pointer: .long 0
route_file_end: .long 0
route_token_length: .long 0
route_word_length: .long 0
fuzzy_root_entry: .long 0
terminal_store_character: .byte 0
terminal_capture_enabled: .byte 1
shift_pressed: .byte 0
caps_enabled: .byte 0
extended_prefix: .byte 0
control_pressed: .byte 0
editor_active: .byte 0
editor_dirty: .byte 0
editor_render_character: .byte 0
write_silent: .byte 0
milo_signal_flags: .byte 0
milo_response_choice: .byte 0
milo_legacy_profile: .byte 0
route_corrected: .byte 0
route_courtesy: .byte 0
route_has_object: .byte 0
route_intent: .byte 0
route_generic_intent: .byte 0
route_edit_used: .byte 0
route_action_match: .byte 0
route_action_fuzzy: .byte 0
route_ambiguous: .byte 0
history_head: .byte 0
history_count: .byte 0
history_view: .byte 0
write_fat_name: .space 11
editor_filename: .space 13
input_buffer: .space INPUT_LIMIT + 1
routing_buffer: .space INPUT_LIMIT + 1
history_lengths: .space HISTORY_CAPACITY
.align 4
history_buffer: .space HISTORY_CAPACITY * HISTORY_ENTRY_SIZE
decimal_buffer: .space 10
decimal_buffer_end: .byte 0

/* US keyboard Set 1 base and Shift make-code maps, 0x00 through 0x39. */
scancode_table:
    .byte 0, 0
    .byte '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '='
    .byte 0, 0
    .byte 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']'
    .byte 0, 0
    .byte 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, 96
    .byte 0, 92
    .byte 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/'
    .byte 0, '*', 0, ' '
shifted_scancode_table:
    .byte 0, 0
    .byte '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+'
    .byte 0, 0
    .byte 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}'
    .byte 0, 0
    .byte 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34, '~'
    .byte 0, '|'
    .byte 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?'
    .byte 0, '*', 0, ' '

#include <nanos/app.h>
#include <nanos/syscall.h>
#include <nanos/gui_widgets.inc>

#define PACK_POS_SIZE(pos, size) (((pos) << 16) | (size))

#define WINDOW_X 120
#define WINDOW_Y 96
#define WINDOW_W 500
#define WINDOW_H 300
#define WINDOW_BG 0x00d7dfda
#define TEXT_COLOR 0x00101416
#define PANEL_COLOR 0x00c5d0cc
#define LINE_COLOR 0x006ac0b8
#define BUTTON_COLOR 0x00e8ece8
#define FIELD_COLOR 0x00edf2ee
#define SELECTED_COLOR 0x00c5d0cc
#define BUTTON_ID 1
#define IPC_MESSAGE_SIZE 24
#define DEMO_ICON_W 8
#define DEMO_ICON_H 8
#define GUIDEMO_SERVICE_ID 0x1001
#define STATE_CHILD_PID_OFFSET 0
#define STATE_THREAD_PID_OFFSET 4
#define STATE_PROCESS_INFO_OFFSET 8
#define STATE_SIZE (STATE_PROCESS_INFO_OFFSET + NANOS_PROCESS_INFO_SIZE)

.section .text

file_start:
.long NANOS_APP_MAGIC
.long NANOS_APP_HEADER_SIZE
.long start - file_start
.long image_end - file_start
.long NANOS_APP_MEMORY_SIZE
.long NANOS_APP_STACK_SIZE
.long NANOS_APP_STACK_BASE + NANOS_APP_STACK_SIZE
.long NANOS_APP_ICON_NONE

start:
    subl $STATE_SIZE, %esp
    movl $0, STATE_CHILD_PID_OFFSET(%esp)
    movl $0, STATE_THREAD_PID_OFFSET(%esp)

    /* Menuet-style app loop: choose events, wait, handle, repeat. */
    movl $(NANOS_EVENT_MASK_REDRAW | NANOS_EVENT_MASK_KEY | NANOS_EVENT_MASK_BUTTON | NANOS_EVENT_MASK_MOUSE | NANOS_EVENT_MASK_EXIT), %ebx
    movl $NANOS_SYSCALL_SET_EVENT_MASK, %eax
    int $0x80

    movl $NANOS_IPC_BIND_SERVICE, %ebx
    movl $GUIDEMO_SERVICE_ID, %ecx
    movl $0, %edx
    movl $NANOS_SYSCALL_IPC, %eax
    int $0x80

event_loop:
    movl $NANOS_SYSCALL_WAIT_EVENT, %eax
    int $0x80

    cmpl $NANOS_EVENT_REDRAW, %eax
    je redraw
    cmpl $NANOS_EVENT_KEY, %eax
    je key
    cmpl $NANOS_EVENT_BUTTON, %eax
    je button
    cmpl $NANOS_EVENT_MOUSE, %eax
    je mouse
    cmpl $NANOS_EVENT_EXIT, %eax
    je exit_ok
    jmp event_loop

redraw:
    movl $NANOS_WINDOW_DRAW_BEGIN, %ebx
    movl $NANOS_SYSCALL_WINDOW_DRAW, %eax
    int $0x80

    call title_ptr
title:
    .ascii "NANOS ABI DEMO\0"
title_ptr:
    popl %esi
    movl $PACK_POS_SIZE(WINDOW_X, WINDOW_W), %ebx
    movl $PACK_POS_SIZE(WINDOW_Y, WINDOW_H), %ecx
    movl $WINDOW_BG, %edx
    movl $NANOS_SYSCALL_DRAW_WINDOW, %eax
    int $0x80

    movl $PACK_POS_SIZE(16, 260), %ebx
    movl $PACK_POS_SIZE(48, 44), %ecx
    movl $PANEL_COLOR, %edx
    movl $NANOS_SYSCALL_DRAW_RECT, %eax
    int $0x80

    movl $0, %edi
pixel_demo_loop:
    movl %edi, %ebx
    addl $24, %ebx
    movl %edi, %ecx
    addl $66, %ecx
    movl $LINE_COLOR, %edx
    movl $NANOS_SYSCALL_PUT_PIXEL, %eax
    int $0x80
    incl %edi
    cmpl $18, %edi
    jne pixel_demo_loop

    call message_ptr
message:
    .ascii "Userspace owns this window.\0"
message_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 18), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    call demo_icon_ptr
demo_icon:
    .long 0x00101416, 0x00101416, 0x00101416, 0x00101416
    .long 0x00101416, 0x00101416, 0x00101416, 0x00101416
    .long 0x00101416, 0x006ac0b8, 0x006ac0b8, 0x006ac0b8
    .long 0x006ac0b8, 0x00d95b50, 0x00d95b50, 0x00101416
    .long 0x00101416, 0x006ac0b8, 0x00e8ece8, 0x00e8ece8
    .long 0x006ac0b8, 0x00d95b50, 0x00d95b50, 0x00101416
    .long 0x00101416, 0x006ac0b8, 0x00e8ece8, 0x00e8ece8
    .long 0x006ac0b8, 0x00d95b50, 0x00d95b50, 0x00101416
    .long 0x00101416, 0x006ac0b8, 0x006ac0b8, 0x006ac0b8
    .long 0x006ac0b8, 0x00d95b50, 0x00d95b50, 0x00101416
    .long 0x00101416, 0x00e8ece8, 0x00e8ece8, 0x006ac0b8
    .long 0x006ac0b8, 0x00e8ece8, 0x00e8ece8, 0x00101416
    .long 0x00101416, 0x00e8ece8, 0x00e8ece8, 0x006ac0b8
    .long 0x006ac0b8, 0x00e8ece8, 0x00e8ece8, 0x00101416
    .long 0x00101416, 0x00101416, 0x00101416, 0x00101416
    .long 0x00101416, 0x00101416, 0x00101416, 0x00101416
demo_icon_ptr:
    popl %edx
    movl $PACK_POS_SIZE(288, DEMO_ICON_W), %ebx
    movl $PACK_POS_SIZE(18, DEMO_ICON_H), %ecx
    movl $NANOS_SYSCALL_PUT_IMAGE, %eax
    int $0x80

    movl $PACK_POS_SIZE(16, 40), %ebx
    movl $PACK_POS_SIZE(276, 40), %ecx
    movl $LINE_COLOR, %edx
    movl $NANOS_SYSCALL_DRAW_LINE, %eax
    int $0x80

    call hint_ptr
hint:
    .ascii "Close sends an event to the app.\0"
hint_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 50), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    movl $PACK_POS_SIZE(16, 96), %ebx
    movl $PACK_POS_SIZE(104, 28), %ecx
    movl $BUTTON_ID, %edx
    movl $BUTTON_COLOR, %esi
    movl $NANOS_SYSCALL_DEFINE_BUTTON, %eax
    int $0x80

    call demo_button_label_ptr
demo_button_label:
    .ascii "BUTTON\0"
demo_button_label_ptr:
    popl %edx
    movl $PACK_POS_SIZE(28, 112), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    call screen_label_ptr
screen_label:
    .ascii "Screen x max:\0"
screen_label_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 132), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    movl $NANOS_SYSCALL_GET_SCREEN_SIZE, %eax
    int $0x80
    shrl $16, %eax
    movl $NANOS_NUMBER_FORMAT_DECIMAL, %ebx
    movl %eax, %ecx
    movl $PACK_POS_SIZE(164, 132), %edx
    movl $TEXT_COLOR, %esi
    movl $NANOS_SYSCALL_WRITE_NUMBER, %eax
    int $0x80

    cmpl $0, STATE_THREAD_PID_OFFSET(%esp)
    jne draw_thread_pid
    call thread_entry_ptr
thread_entry:
    movl $0, %ebx
    movl $NANOS_SYSCALL_EXIT, %eax
    int $0x80
thread_halt:
    jmp thread_halt
thread_entry_ptr:
    popl %ecx
    movl $NANOS_THREAD_CREATE, %ebx
    movl $(NANOS_APP_STACK_BASE + NANOS_APP_STACK_SIZE), %edx
    movl $NANOS_SYSCALL_THREAD, %eax
    int $0x80
    cmpl $NANOS_ERROR_INVALID, %eax
    je draw_thread_pid
    movl %eax, STATE_THREAD_PID_OFFSET(%esp)

draw_thread_pid:
    call thread_label_ptr
thread_label:
    .ascii "Thread pid:\0"
thread_label_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 66), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    movl $NANOS_NUMBER_FORMAT_DECIMAL, %ebx
    movl STATE_THREAD_PID_OFFSET(%esp), %ecx
    movl $PACK_POS_SIZE(148, 66), %edx
    movl $TEXT_COLOR, %esi
    movl $NANOS_SYSCALL_WRITE_NUMBER, %eax
    int $0x80

    call get_process_info
    call app_pid_label_ptr
app_pid_label:
    .ascii "App pid:\0"
app_pid_label_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 148), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    movl $NANOS_NUMBER_FORMAT_DECIMAL, %ebx
    movl (STATE_PROCESS_INFO_OFFSET + NANOS_PROCESS_INFO_APP_PID_OFFSET)(%esp), %ecx
    movl $PACK_POS_SIZE(112, 148), %edx
    movl $TEXT_COLOR, %esi
    movl $NANOS_SYSCALL_WRITE_NUMBER, %eax
    int $0x80

    call widget_title_ptr
widget_title:
    .ascii "App-side widgets\0"
widget_title_ptr:
    popl %edx
    nanos_text_at 300, 18, TEXT_COLOR

    nanos_widget_menu 300, 44, 164, 24, BUTTON_COLOR, LINE_COLOR
    call widget_menu_file_ptr
widget_menu_file:
    .ascii "FILE\0"
widget_menu_file_ptr:
    popl %edx
    nanos_text_at 310, 50, TEXT_COLOR
    call widget_menu_edit_ptr
widget_menu_edit:
    .ascii "EDIT\0"
widget_menu_edit_ptr:
    popl %edx
    nanos_text_at 372, 50, TEXT_COLOR

    nanos_widget_text_field 300, 82, 164, 28, FIELD_COLOR, LINE_COLOR
    call widget_field_ptr
widget_field:
    .ascii "sample text\0"
widget_field_ptr:
    popl %edx
    nanos_text_at 308, 90, TEXT_COLOR

    nanos_widget_list 300, 124, 148, 70, FIELD_COLOR, SELECTED_COLOR, LINE_COLOR
    call widget_item_one_ptr
widget_item_one:
    .ascii "first item\0"
widget_item_one_ptr:
    popl %edx
    nanos_text_at 308, 134, TEXT_COLOR
    call widget_item_two_ptr
widget_item_two:
    .ascii "second item\0"
widget_item_two_ptr:
    popl %edx
    nanos_text_at 308, 162, TEXT_COLOR
    nanos_widget_scrollbar 452, 124, 70, BUTTON_COLOR, LINE_COLOR, TEXT_COLOR

    nanos_widget_dialog 300, 210, 164, 64, FIELD_COLOR, PANEL_COLOR, LINE_COLOR
    call dialog_title_ptr
dialog_title:
    .ascii "Dialog\0"
dialog_title_ptr:
    popl %edx
    nanos_text_at 308, 214, TEXT_COLOR
    call dialog_message_ptr
dialog_message:
    .ascii "App drawn panel\0"
dialog_message_ptr:
    popl %edx
    nanos_text_at 310, 236, TEXT_COLOR
    nanos_widget_button 410, 252, 42, 20, BUTTON_COLOR, LINE_COLOR
    call dialog_ok_ptr
dialog_ok:
    .ascii "OK\0"
dialog_ok_ptr:
    popl %edx
    nanos_text_at 422, 256, TEXT_COLOR

    movl $NANOS_WINDOW_DRAW_END, %ebx
    movl $NANOS_SYSCALL_WINDOW_DRAW, %eax
    int $0x80
    jmp event_loop

key:
    movl $NANOS_SYSCALL_GET_KEY, %eax
    int $0x80
    call key_ptr
key_text:
    .ascii "Key event received.\0"
key_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 84), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80
    jmp event_loop

button:
    movl $NANOS_SYSCALL_GET_BUTTON, %eax
    int $0x80
    cmpl $BUTTON_ID, %eax
    jne event_loop
    movl $0, %edi

launch_or_send:
    movl STATE_CHILD_PID_OFFSET(%esp), %ecx
    cmpl $0, %ecx
    jne send_ipc

    call child_path_ptr
child_path:
    .ascii "/bin/ipcchild.nx\0"
child_path_ptr:
    popl %ebx
    call empty_param_ptr
empty_param:
    .ascii "\0"
empty_param_ptr:
    popl %ecx
    movl $NANOS_SYSCALL_START_APP, %eax
    int $0x80
    cmpl $NANOS_ERROR_INVALID, %eax
    je button_launch_failed
    movl %eax, STATE_CHILD_PID_OFFSET(%esp)
    movl %eax, %ecx

send_ipc:
    call ipc_message_ptr
ipc_message:
    .long 0
    .long IPC_MESSAGE_SIZE
    .ascii "IPC hello from guidemo.\0"
    .space 40
ipc_message_ptr:
    popl %edx
    movl $NANOS_IPC_SEND, %ebx
    movl $NANOS_SYSCALL_IPC, %eax
    int $0x80
    cmpl $NANOS_ERROR_INVALID, %eax
    jne button_sent
    cmpl $0, %edi
    jne button_send_failed
    movl $1, %edi
    movl $0, STATE_CHILD_PID_OFFSET(%esp)
    jmp launch_or_send

button_sent:
    call button_sent_ptr
button_sent_text:
    .ascii "IPC message sent.     \0"
button_sent_ptr:
    popl %edx
    jmp draw_button_status

button_launch_failed:
    call button_launch_failed_ptr
button_launch_failed_text:
    .ascii "Launch failed.        \0"
button_launch_failed_ptr:
    popl %edx
    jmp draw_button_status

button_send_failed:
    call button_send_failed_ptr
button_send_failed_text:
    .ascii "IPC send failed.      \0"
button_send_failed_ptr:
    popl %edx
    jmp draw_button_status

draw_button_status:
    pushl %edx
    movl $PACK_POS_SIZE(128, 180), %ebx
    movl $PACK_POS_SIZE(112, 16), %ecx
    movl $WINDOW_BG, %edx
    movl $NANOS_SYSCALL_DRAW_RECT, %eax
    int $0x80
    popl %edx
    movl $PACK_POS_SIZE(128, 112), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80
    jmp event_loop

mouse:
    call get_mouse_event
    call mouse_ptr
mouse_text:
    .ascii "Mouse move event.     \0"
mouse_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 164), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80
    jmp event_loop

get_mouse_event:
    call mouse_event_ptr
mouse_event:
    .space NANOS_MOUSE_EVENT_SIZE
mouse_event_ptr:
    popl %ebx
    movl $NANOS_SYSCALL_GET_MOUSE, %eax
    int $0x80
    ret

get_process_info:
    leal STATE_PROCESS_INFO_OFFSET(%esp), %ebx
    movl $NANOS_PROCESS_INFO_CURRENT, %ecx
    movl $NANOS_SYSCALL_PROCESS_INFO, %eax
    int $0x80
    ret

exit_ok:
    movl $0, %ebx
    movl $NANOS_SYSCALL_EXIT, %eax
    int $0x80

halt:
    jmp halt

image_end:

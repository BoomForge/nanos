#include <nanos/app.h>
#include <nanos/syscall.h>

#define PACK_POS_SIZE(pos, size) (((pos) << 16) | (size))

#define WINDOW_X 500
#define WINDOW_Y 128
#define WINDOW_W 300
#define WINDOW_H 150
#define WINDOW_BG 0x00d7dfda
#define TEXT_COLOR 0x00101416
#define PANEL_COLOR 0x00c5d0cc
#define LINE_COLOR 0x006ac0b8
#define STATE_IPC_MESSAGE_OFFSET 4
#define STATE_SIZE (STATE_IPC_MESSAGE_OFFSET + NANOS_IPC_MESSAGE_SIZE)

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
    movl $0, (%esp)

    movl $(NANOS_EVENT_MASK_REDRAW | NANOS_EVENT_MASK_IPC | NANOS_EVENT_MASK_EXIT), %ebx
    movl $NANOS_SYSCALL_SET_EVENT_MASK, %eax
    int $0x80

event_loop:
    movl $NANOS_SYSCALL_WAIT_EVENT, %eax
    int $0x80

    cmpl $NANOS_EVENT_REDRAW, %eax
    je redraw
    cmpl $NANOS_EVENT_IPC, %eax
    je ipc
    cmpl $NANOS_EVENT_EXIT, %eax
    je exit_ok
    jmp event_loop

redraw:
    movl $NANOS_WINDOW_DRAW_BEGIN, %ebx
    movl $NANOS_SYSCALL_WINDOW_DRAW, %eax
    int $0x80

    call title_ptr
title:
    .ascii "IPC CHILD\0"
title_ptr:
    popl %esi
    movl $PACK_POS_SIZE(WINDOW_X, WINDOW_W), %ebx
    movl $PACK_POS_SIZE(WINDOW_Y, WINDOW_H), %ecx
    movl $WINDOW_BG, %edx
    movl $NANOS_SYSCALL_DRAW_WINDOW, %eax
    int $0x80

    movl $PACK_POS_SIZE(16, 260), %ebx
    movl $PACK_POS_SIZE(42, 50), %ecx
    movl $PANEL_COLOR, %edx
    movl $NANOS_SYSCALL_DRAW_RECT, %eax
    int $0x80

    call label_ptr
label:
    .ascii "IPC receiver window\0"
label_ptr:
    popl %edx
    movl $PACK_POS_SIZE(16, 18), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    movl $PACK_POS_SIZE(16, 40), %ebx
    movl $PACK_POS_SIZE(276, 40), %ecx
    movl $LINE_COLOR, %edx
    movl $NANOS_SYSCALL_DRAW_LINE, %eax
    int $0x80

    call get_received_flag
    cmpl $0, (%edi)
    jne redraw_last_message

    call waiting_ptr
waiting_text:
    .ascii "Waiting for IPC.\0"
waiting_ptr:
    popl %edx
    jmp redraw_status_text

redraw_last_message:
    call get_ipc_message
    addl $NANOS_IPC_MESSAGE_DATA_OFFSET, %edx

redraw_status_text:
    movl $PACK_POS_SIZE(24, 66), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80

    movl $NANOS_WINDOW_DRAW_END, %ebx
    movl $NANOS_SYSCALL_WINDOW_DRAW, %eax
    int $0x80
    jmp event_loop

ipc:
    movl $NANOS_IPC_PENDING, %ebx
    movl $0, %ecx
    movl $0, %edx
    movl $NANOS_SYSCALL_IPC, %eax
    int $0x80
    cmpl $0, %eax
    je event_loop

    call get_ipc_message
    movl $NANOS_IPC_RECV, %ebx
    movl $0, %ecx
    movl $NANOS_SYSCALL_IPC, %eax
    int $0x80
    cmpl $0, %eax
    je event_loop

    call get_received_flag
    movl $1, (%edi)

    call get_ipc_message
    addl $NANOS_IPC_MESSAGE_DATA_OFFSET, %edx
    pushl %edx
    movl $PACK_POS_SIZE(24, 240), %ebx
    movl $PACK_POS_SIZE(66, 16), %ecx
    movl $PANEL_COLOR, %edx
    movl $NANOS_SYSCALL_DRAW_RECT, %eax
    int $0x80
    popl %edx
    movl $PACK_POS_SIZE(24, 66), %ebx
    movl $TEXT_COLOR, %ecx
    movl $NANOS_SYSCALL_WRITE_TEXT, %eax
    int $0x80
    jmp event_loop

get_received_flag:
    leal 4(%esp), %edi
    ret

get_ipc_message:
    leal (4 + STATE_IPC_MESSAGE_OFFSET)(%esp), %edx
    ret

exit_ok:
    movl $0, %ebx
    movl $NANOS_SYSCALL_EXIT, %eax
    int $0x80

halt:
    jmp halt

image_end:

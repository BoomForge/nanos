/* Polled PS/2 mouse, standard three-byte packets, and a saved-under cursor. */

initialize_mouse:
    movb $0, (KERNEL_LOAD_ADDRESS + mouse_enabled - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + mouse_packet_index - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + mouse_buttons - _start)
    movb $0, (KERNEL_LOAD_ADDRESS + mouse_cursor_visible - _start)
    movl $512, (KERNEL_LOAD_ADDRESS + mouse_x - _start)
    movl $384, (KERNEL_LOAD_ADDRESS + mouse_y - _start)

    call ps2_wait_input_clear
    testl %eax, %eax
    jz initialize_mouse_complete
    movb $0xa8, %al
    outb %al, $0x64

    movb $0xf6, (KERNEL_LOAD_ADDRESS + mouse_command - _start)
    call mouse_send_device_command
    testl %eax, %eax
    jz initialize_mouse_complete
    movb $0xf4, (KERNEL_LOAD_ADDRESS + mouse_command - _start)
    call mouse_send_device_command
    testl %eax, %eax
    jz initialize_mouse_complete
    movb $1, (KERNEL_LOAD_ADDRESS + mouse_enabled - _start)
initialize_mouse_complete:
    ret

mouse_send_device_command:
    call ps2_wait_input_clear
    testl %eax, %eax
    jz mouse_command_failed
    movb $0xd4, %al
    outb %al, $0x64
    call ps2_wait_input_clear
    testl %eax, %eax
    jz mouse_command_failed
    movb (KERNEL_LOAD_ADDRESS + mouse_command - _start), %al
    outb %al, $0x60
    call ps2_wait_output_ready
    testl %eax, %eax
    jz mouse_command_failed
    inb $0x60, %al
    cmpb $0xfa, %al
    jne mouse_command_failed
    movl $1, %eax
    ret
mouse_command_failed:
    xorl %eax, %eax
    ret

ps2_wait_input_clear:
    movl $0x00020000, %ecx
ps2_wait_input_next:
    inb $0x64, %al
    testb $0x02, %al
    jz ps2_wait_input_ready
    decl %ecx
    jnz ps2_wait_input_next
    xorl %eax, %eax
    ret
ps2_wait_input_ready:
    movl $1, %eax
    ret

ps2_wait_output_ready:
    movl $0x00020000, %ecx
ps2_wait_output_next:
    inb $0x64, %al
    testb $0x01, %al
    jnz ps2_wait_output_complete
    decl %ecx
    jnz ps2_wait_output_next
    xorl %eax, %eax
    ret
ps2_wait_output_complete:
    movl $1, %eax
    ret

/* AL=one auxiliary byte. Resynchronize on the mandatory first-byte bit. */
handle_mouse_byte:
    cmpb $0, (KERNEL_LOAD_ADDRESS + mouse_enabled - _start)
    je handle_mouse_complete
    movzbl (KERNEL_LOAD_ADDRESS + mouse_packet_index - _start), %ecx
    testl %ecx, %ecx
    jnz mouse_store_byte
    testb $0x08, %al
    jz handle_mouse_complete
mouse_store_byte:
    movb %al, (KERNEL_LOAD_ADDRESS + mouse_packet - _start)(,%ecx)
    incl %ecx
    cmpl $3, %ecx
    jb mouse_packet_incomplete
    /* Keep the cursor visible while bytes one and two arrive. Erase it only
     * once the complete packet can immediately move and redraw it. */
    call hide_mouse_cursor
    movb $0, (KERNEL_LOAD_ADDRESS + mouse_packet_index - _start)
    incl (KERNEL_LOAD_ADDRESS + system_event_count - _start)
    call apply_mouse_packet
    ret
mouse_packet_incomplete:
    movb %cl, (KERNEL_LOAD_ADDRESS + mouse_packet_index - _start)
handle_mouse_complete:
    ret

apply_mouse_packet:
    pushal
    movzbl (KERNEL_LOAD_ADDRESS + mouse_packet - _start), %ebx

    /* Decode from the packet sign bits even when an overflow bit is present.
     * QEMU can set overflow during a quick move toward a screen edge; V0.23
     * discarded those packets completely and made the pointer feel blocked. */
    movzbl (KERNEL_LOAD_ADDRESS + mouse_packet + 1 - _start), %eax
    testb $0x10, %bl
    jz mouse_x_delta_ready
    subl $256, %eax
mouse_x_delta_ready:
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %edx
    addl %eax, %edx
    testl %edx, %edx
    js mouse_x_zero
    cmpl $MOUSE_MAX_X, %edx
    jbe mouse_x_store
    movl $MOUSE_MAX_X, %edx
    jmp mouse_x_store
mouse_x_zero:
    xorl %edx, %edx
mouse_x_store:
    movl %edx, (KERNEL_LOAD_ADDRESS + mouse_x - _start)

    movzbl (KERNEL_LOAD_ADDRESS + mouse_packet + 2 - _start), %eax
    testb $0x20, %bl
    jz mouse_y_delta_ready
    subl $256, %eax
mouse_y_delta_ready:
    negl %eax
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %edx
    addl %eax, %edx
    testl %edx, %edx
    js mouse_y_zero
    cmpl $MOUSE_MAX_Y, %edx
    jbe mouse_y_store
    movl $MOUSE_MAX_Y, %edx
    jmp mouse_y_store
mouse_y_zero:
    xorl %edx, %edx
mouse_y_store:
    movl %edx, (KERNEL_LOAD_ADDRESS + mouse_y - _start)
    call gui_handle_pointer_motion

mouse_buttons_ready:
    movzbl (KERNEL_LOAD_ADDRESS + mouse_buttons - _start), %edx
    movl %ebx, %eax
    andl $7, %eax
    movb %al, (KERNEL_LOAD_ADDRESS + mouse_buttons - _start)

    /* Fluxbox-style root binding: a new right press opens the root menu. */
    testl $2, %eax
    jz mouse_test_left_button
    testl $2, %edx
    jnz mouse_packet_complete
    call gui_handle_root_click
    jmp mouse_packet_complete

mouse_test_left_button:
    testl $1, %eax
    jz mouse_left_released
    testl $1, %edx
    jnz mouse_left_dragged
    call gui_handle_click
    jmp mouse_packet_complete
mouse_left_dragged:
    call gui_handle_drag
    jmp mouse_packet_complete
mouse_left_released:
    testl $1, %edx
    jz mouse_packet_complete
    call gui_handle_release
mouse_packet_complete:
    popal
    ret

draw_mouse_cursor:
    cmpb $0, (KERNEL_LOAD_ADDRESS + mouse_enabled - _start)
    je draw_mouse_complete
    cmpb $0, (KERNEL_LOAD_ADDRESS + mouse_cursor_visible - _start)
    jne draw_mouse_complete
    pushal

    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    imull BOOT_PITCH(%ebp), %eax
    addl BOOT_FRAMEBUFFER(%ebp), %eax
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %edx
    shll $2, %edx
    addl %edx, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + mouse_row_address - _start)
    movl $MOUSE_BACKING_ADDRESS, %edi
    movl $16, %ebx
mouse_save_row:
    movl (KERNEL_LOAD_ADDRESS + mouse_row_address - _start), %esi
    movl $12, %ecx
    rep movsl
    movl (KERNEL_LOAD_ADDRESS + mouse_row_address - _start), %eax
    addl BOOT_PITCH(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + mouse_row_address - _start)
    decl %ebx
    jnz mouse_save_row

    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    imull BOOT_PITCH(%ebp), %eax
    addl BOOT_FRAMEBUFFER(%ebp), %eax
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %edx
    shll $2, %edx
    addl %edx, %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + mouse_row_address - _start)
    movl $(KERNEL_LOAD_ADDRESS + mouse_shape - _start), %esi
    movl $16, %ebx
mouse_draw_row:
    movzwl (%esi), %edx
    addl $2, %esi
    movl (KERNEL_LOAD_ADDRESS + mouse_row_address - _start), %edi
    movl $12, %ecx
mouse_draw_pixel:
    testl $0x0800, %edx
    jz mouse_draw_pixel_done
    movl $0x00f4a4ee, (%edi)
mouse_draw_pixel_done:
    addl $4, %edi
    shll $1, %edx
    decl %ecx
    jnz mouse_draw_pixel
    movl (KERNEL_LOAD_ADDRESS + mouse_row_address - _start), %eax
    addl BOOT_PITCH(%ebp), %eax
    movl %eax, (KERNEL_LOAD_ADDRESS + mouse_row_address - _start)
    decl %ebx
    jnz mouse_draw_row
    movb $1, (KERNEL_LOAD_ADDRESS + mouse_cursor_visible - _start)
    popal
draw_mouse_complete:
    ret

hide_mouse_cursor:
    cmpb $0, (KERNEL_LOAD_ADDRESS + mouse_cursor_visible - _start)
    je hide_mouse_complete
    pushal
    movl (KERNEL_LOAD_ADDRESS + mouse_y - _start), %eax
    imull BOOT_PITCH(%ebp), %eax
    addl BOOT_FRAMEBUFFER(%ebp), %eax
    movl (KERNEL_LOAD_ADDRESS + mouse_x - _start), %edx
    shll $2, %edx
    addl %edx, %eax
    movl %eax, %edi
    movl $MOUSE_BACKING_ADDRESS, %esi
    movl $16, %ebx
mouse_restore_row:
    movl $12, %ecx
    rep movsl
    movl BOOT_PITCH(%ebp), %eax
    subl $48, %eax
    addl %eax, %edi
    decl %ebx
    jnz mouse_restore_row
    movb $0, (KERNEL_LOAD_ADDRESS + mouse_cursor_visible - _start)
    popal
hide_mouse_complete:
    ret

.align 2
mouse_shape:
    .word 0x0800, 0x0c00, 0x0e00, 0x0f00
    .word 0x0f80, 0x0fc0, 0x0fe0, 0x0ff0
    .word 0x0f00, 0x0d80, 0x0980, 0x00c0
    .word 0x00c0, 0x0060, 0x0060, 0x0000

.align 4
mouse_x: .long 512
mouse_y: .long 384
mouse_row_address: .long 0
mouse_packet: .space 3
mouse_packet_index: .byte 0
mouse_buttons: .byte 0
mouse_enabled: .byte 0
mouse_cursor_visible: .byte 0
mouse_command: .byte 0

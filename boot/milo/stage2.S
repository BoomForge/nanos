.code16
.section .text
.global _start

#define STAGE2_LOAD_ADDRESS 0x8000
#define KERNEL_LOAD_SEGMENT 0x1000
#define KERNEL_LOAD_ADDRESS 0x10000
#define KERNEL_DISK_SECTOR 1
#define KERNEL_DISK_HEAD 1
#define KERNEL_SECTOR_COUNT 64
#define FILESYSTEM_CACHE_SEGMENT 0x2000
#define FILESYSTEM_CACHE_ADDRESS 0x20000
#define FILESYSTEM_CACHE_SECTORS 128
#define FILESYSTEM_ROOT_ADDRESS 0x22400
#define FILESYSTEM_DATA_ADDRESS 0x24000
#define FILESYSTEM_CACHED_DATA_CLUSTERS 96
#define FILESYSTEM_TOTAL_DATA_CLUSTERS 2766
#define FILESYSTEM_FIRST_CYLINDER 2
#define FILESYSTEM_FIRST_HEAD 0
#define FILESYSTEM_FIRST_SECTOR 11
#define E820_FUNCTION 0xe820
#define E820_MAGIC 0x534d4150
#define E820_ENTRY_SIZE 24
#define E820_MAX_ENTRIES 32
#define E820_TYPE_USABLE 1
#define GDT_CODE_SELECTOR 0x08
#define GDT_DATA_SELECTOR 0x10
#define GDT_CODE16_SELECTOR 0x18
#define GDT_DATA16_SELECTOR 0x20
#define PROTECTED_MODE_STACK 0x00090000
#define VGA_TEXT_MEMORY 0x000b8000
#define VGA_TEXT_ROW 12
#define VGA_TEXT_ATTRIBUTE 0x0d
#define VBE_GET_CONTROLLER_INFO 0x4f00
#define VBE_GET_MODE_INFO 0x4f01
#define VBE_SET_MODE 0x4f02
#define VBE_SUCCESS 0x004f
#define VBE_LINEAR_MODE_FLAG 0x4000
#define VBE_MODE_SUPPORTED 0x0001
#define VBE_LINEAR_FRAMEBUFFER 0x0080
#define SPLASH_WIDTH 100
#define SPLASH_ORIGIN_X 112
#define SPLASH_ORIGIN_Y 96
#define SPLASH_HEADER_BYTES 76
#define SPLASH_STATUS_X 32
#define SPLASH_STATUS_Y 672
#define SPLASH_STATUS_COLOR 0x00ef70e8

/*
 * M.I.L.O Stage 2 loader.
 *
 * Stage 1 loads this code from floppy sectors 2-17 at physical address 0x8000
 * and preserves the BIOS boot-drive number in DL. It discovers memory,
 * initializes video, loads the M.I.L.O kernel and transfers control to it.
 */
_start:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7c00, %sp
    sti
    cld

    movb %dl, (STAGE2_LOAD_ADDRESS + boot_drive - _start)
    movw $(STAGE2_LOAD_ADDRESS + stage2_message - _start), %si
    call print_string

    call detect_memory
    jc memory_error

    movw $(STAGE2_LOAD_ADDRESS + memory_message - _start), %si
    call print_string

    movl (STAGE2_LOAD_ADDRESS + usable_bytes - _start), %eax
    shrl $20, %eax
    call print_u32
    movw $(STAGE2_LOAD_ADDRESS + mib_message - _start), %si
    call print_string

    call load_kernel
    jc kernel_load_error
    call load_filesystem
    jc filesystem_load_error
    call prepare_graphics
    jc graphics_error
    call enter_protected_mode
    jmp stage2_complete

memory_error:
    movw $(STAGE2_LOAD_ADDRESS + memory_error_message - _start), %si
    call print_string
    jmp stage2_complete

graphics_error:
    movw $(STAGE2_LOAD_ADDRESS + graphics_error_message - _start), %si
    call print_string
    jmp stage2_complete

kernel_load_error:
    movw $(STAGE2_LOAD_ADDRESS + kernel_error_message - _start), %si
    call print_string
    jmp stage2_complete

filesystem_load_error:
    movw $(STAGE2_LOAD_ADDRESS + filesystem_error_message - _start), %si
    call print_string
    jmp stage2_complete

/* Load the reserved 32 KiB M.I.L.O kernel region at physical 0x10000. */
load_kernel:
    movb $3, (STAGE2_LOAD_ADDRESS + disk_retries - _start)

kernel_read_attempt:
    xorw %ax, %ax
    movb (STAGE2_LOAD_ADDRESS + boot_drive - _start), %dl
    int $0x13

    movb $KERNEL_SECTOR_COUNT, (STAGE2_LOAD_ADDRESS + kernel_remaining - _start)
    movb $0, (STAGE2_LOAD_ADDRESS + kernel_cylinder - _start)
    movb $KERNEL_DISK_HEAD, (STAGE2_LOAD_ADDRESS + kernel_head - _start)
    movb $KERNEL_DISK_SECTOR, (STAGE2_LOAD_ADDRESS + kernel_sector - _start)
    movw $0, (STAGE2_LOAD_ADDRESS + kernel_offset - _start)

kernel_read_next:
    movw $KERNEL_LOAD_SEGMENT, %ax
    movw %ax, %es
    movw (STAGE2_LOAD_ADDRESS + kernel_offset - _start), %bx
    movb $0x02, %ah
    movb $1, %al
    movb (STAGE2_LOAD_ADDRESS + kernel_cylinder - _start), %ch
    movb (STAGE2_LOAD_ADDRESS + kernel_sector - _start), %cl
    movb (STAGE2_LOAD_ADDRESS + kernel_head - _start), %dh
    movb (STAGE2_LOAD_ADDRESS + boot_drive - _start), %dl
    int $0x13
    jc kernel_read_failed

    addw $512, (STAGE2_LOAD_ADDRESS + kernel_offset - _start)
    incb (STAGE2_LOAD_ADDRESS + kernel_sector - _start)
    cmpb $19, (STAGE2_LOAD_ADDRESS + kernel_sector - _start)
    jb kernel_sector_ready
    movb $1, (STAGE2_LOAD_ADDRESS + kernel_sector - _start)
    xorb $1, (STAGE2_LOAD_ADDRESS + kernel_head - _start)
    cmpb $0, (STAGE2_LOAD_ADDRESS + kernel_head - _start)
    jne kernel_sector_ready
    incb (STAGE2_LOAD_ADDRESS + kernel_cylinder - _start)
kernel_sector_ready:
    decb (STAGE2_LOAD_ADDRESS + kernel_remaining - _start)
    jnz kernel_read_next
    jmp kernel_loaded

kernel_read_failed:
    decb (STAGE2_LOAD_ADDRESS + disk_retries - _start)
    jnz kernel_read_attempt

    xorw %ax, %ax
    movw %ax, %es
    stc
    ret

kernel_loaded:
    xorw %ax, %ax
    movw %ax, %es
    clc
    ret

/* Cache FAT1, FAT2, the root directory and 96 data clusters at 0x20000. */
load_filesystem:
    movb $3, (STAGE2_LOAD_ADDRESS + disk_retries - _start)

filesystem_read_attempt:
    xorw %ax, %ax
    movb (STAGE2_LOAD_ADDRESS + boot_drive - _start), %dl
    int $0x13
    movb $FILESYSTEM_CACHE_SECTORS, (STAGE2_LOAD_ADDRESS + fs_remaining - _start)
    movb $FILESYSTEM_FIRST_CYLINDER, (STAGE2_LOAD_ADDRESS + fs_cylinder - _start)
    movb $FILESYSTEM_FIRST_HEAD, (STAGE2_LOAD_ADDRESS + fs_head - _start)
    movb $FILESYSTEM_FIRST_SECTOR, (STAGE2_LOAD_ADDRESS + fs_sector - _start)
    movw $0, (STAGE2_LOAD_ADDRESS + fs_offset - _start)

filesystem_read_next:
    movw $FILESYSTEM_CACHE_SEGMENT, %ax
    movw %ax, %es
    movw (STAGE2_LOAD_ADDRESS + fs_offset - _start), %bx
    movb $0x02, %ah
    movb $1, %al
    movb (STAGE2_LOAD_ADDRESS + fs_cylinder - _start), %ch
    movb (STAGE2_LOAD_ADDRESS + fs_sector - _start), %cl
    movb (STAGE2_LOAD_ADDRESS + fs_head - _start), %dh
    movb (STAGE2_LOAD_ADDRESS + boot_drive - _start), %dl
    int $0x13
    jc filesystem_read_failed

    addw $512, (STAGE2_LOAD_ADDRESS + fs_offset - _start)
    incb (STAGE2_LOAD_ADDRESS + fs_sector - _start)
    cmpb $19, (STAGE2_LOAD_ADDRESS + fs_sector - _start)
    jb filesystem_sector_ready
    movb $1, (STAGE2_LOAD_ADDRESS + fs_sector - _start)
    xorb $1, (STAGE2_LOAD_ADDRESS + fs_head - _start)
    cmpb $0, (STAGE2_LOAD_ADDRESS + fs_head - _start)
    jne filesystem_sector_ready
    incb (STAGE2_LOAD_ADDRESS + fs_cylinder - _start)

filesystem_sector_ready:
    decb (STAGE2_LOAD_ADDRESS + fs_remaining - _start)
    jnz filesystem_read_next
    xorw %ax, %ax
    movw %ax, %es
    clc
    ret

filesystem_read_failed:
    decb (STAGE2_LOAD_ADDRESS + disk_retries - _start)
    jnz filesystem_read_attempt
    xorw %ax, %ax
    movw %ax, %es
    stc
    ret

/*
 * Use the BIOS E820 interface to retain the physical memory map and total all
 * usable regions that fit below 4 GiB. Carry is set if no valid entry exists.
 */
detect_memory:
    pushw %di
    xorl %ebx, %ebx
    movl $0, (STAGE2_LOAD_ADDRESS + usable_bytes - _start)
    movw $0, (STAGE2_LOAD_ADDRESS + memory_map_count - _start)
    movw $(STAGE2_LOAD_ADDRESS + memory_map - _start), %di

memory_next:
    movl $1, 20(%di)
    movl $E820_FUNCTION, %eax
    movl $E820_MAGIC, %edx
    movl $E820_ENTRY_SIZE, %ecx
    int $0x15
    jc memory_finished
    cmpl $E820_MAGIC, %eax
    jne memory_failed
    cmpl $20, %ecx
    jb memory_failed

    incw (STAGE2_LOAD_ADDRESS + memory_map_count - _start)
    cmpl $E820_TYPE_USABLE, 16(%di)
    jne memory_entry_done
    cmpl $0, 4(%di)
    jne memory_entry_done
    cmpl $0, 12(%di)
    jne memory_entry_done

    movl 8(%di), %eax
    addl %eax, (STAGE2_LOAD_ADDRESS + usable_bytes - _start)

memory_entry_done:
    addw $E820_ENTRY_SIZE, %di
    cmpw $E820_MAX_ENTRIES, (STAGE2_LOAD_ADDRESS + memory_map_count - _start)
    jae memory_finished
    testl %ebx, %ebx
    jnz memory_next

memory_finished:
    cmpw $0, (STAGE2_LOAD_ADDRESS + memory_map_count - _start)
    je memory_failed
    clc
    popw %di
    ret

memory_failed:
    stc
    popw %di
    ret

print_string:
print_character:
    lodsb
    testb %al, %al
    jz print_complete
    movb $0x0e, %ah
    movw $0x0007, %bx
    int $0x10
    jmp print_character

print_complete:
    ret

/* Print the unsigned 32-bit value in EAX as decimal text. */
print_u32:
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushw %di

    movw $(STAGE2_LOAD_ADDRESS + decimal_buffer_end - _start), %di
    movb $0, (%di)
    movl $10, %ecx

decimal_next:
    xorl %edx, %edx
    divl %ecx
    addb $'0', %dl
    decw %di
    movb %dl, (%di)
    testl %eax, %eax
    jnz decimal_next

    movw %di, %si
    call print_string

    popw %di
    popl %edx
    popl %ecx
    popl %ebx
    ret

/*
 * Capture the BIOS 8x16 font, validate the 1024x768x32 linear framebuffer
 * mode, retain its geometry and then activate it before protected mode.
 */
prepare_graphics:
    pushw %es
    pushw %di
    pushw %bp

    movw $0x1130, %ax
    movb $0x06, %bh
    int $0x10

    xorl %eax, %eax
    movw %es, %ax
    shll $4, %eax
    movzwl %bp, %edx
    addl %edx, %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + font_address - _start)

    xorw %ax, %ax
    movw %ax, %es
    movw $(STAGE2_LOAD_ADDRESS + vbe_controller_info - _start), %di
    movl $0x32454256, 0(%di)
    movw $VBE_GET_CONTROLLER_INFO, %ax
    int $0x10
    cmpw $VBE_SUCCESS, %ax
    jne graphics_failed

    movw 14(%di), %ax
    movw %ax, (STAGE2_LOAD_ADDRESS + mode_list_offset - _start)
    movw 16(%di), %ax
    movw %ax, (STAGE2_LOAD_ADDRESS + mode_list_segment - _start)

graphics_next_mode:
    movw (STAGE2_LOAD_ADDRESS + mode_list_segment - _start), %ax
    movw %ax, %es
    movw (STAGE2_LOAD_ADDRESS + mode_list_offset - _start), %si
    movw %es:(%si), %cx
    addw $2, %si
    movw %si, (STAGE2_LOAD_ADDRESS + mode_list_offset - _start)
    cmpw $0xffff, %cx
    je graphics_failed
    movw %cx, (STAGE2_LOAD_ADDRESS + selected_vbe_mode - _start)

    xorw %ax, %ax
    movw %ax, %es
    movw $(STAGE2_LOAD_ADDRESS + vbe_mode_info - _start), %di
    movw $VBE_GET_MODE_INFO, %ax
    int $0x10
    cmpw $VBE_SUCCESS, %ax
    jne graphics_next_mode

    movw 0(%di), %ax
    andw $(VBE_MODE_SUPPORTED | VBE_LINEAR_FRAMEBUFFER), %ax
    cmpw $(VBE_MODE_SUPPORTED | VBE_LINEAR_FRAMEBUFFER), %ax
    jne graphics_next_mode
    cmpw $1024, 18(%di)
    jne graphics_next_mode
    cmpw $768, 20(%di)
    jne graphics_next_mode
    cmpb $32, 25(%di)
    jne graphics_next_mode
    cmpl $0, 40(%di)
    je graphics_next_mode

    movzwl 16(%di), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + framebuffer_pitch - _start)
    movzwl 18(%di), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + framebuffer_width - _start)
    movzwl 20(%di), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + framebuffer_height - _start)
    movl 40(%di), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + framebuffer_address - _start)

    movw $VBE_SET_MODE, %ax
    movw (STAGE2_LOAD_ADDRESS + selected_vbe_mode - _start), %bx
    orw $VBE_LINEAR_MODE_FLAG, %bx
    int $0x10
    cmpw $VBE_SUCCESS, %ax
    jne graphics_failed

    clc
    popw %bp
    popw %di
    popw %es
    ret

graphics_failed:
    stc
    popw %bp
    popw %di
    popw %es
    ret

/*
 * Enable A20, install a flat 4 GiB GDT and leave the BIOS real-mode
 * environment. The far jump flushes the instruction pipeline and loads the
 * 32-bit code selector.
 */
enter_protected_mode:
    cli

    inb $0x92, %al
    orb $0x02, %al
    andb $0xfe, %al
    outb %al, $0x92

    lgdt (STAGE2_LOAD_ADDRESS + gdt_descriptor - _start)
    movl %cr0, %eax
    orl $0x00000001, %eax
    movl %eax, %cr0

    .byte 0x66, 0xea
    .long STAGE2_LOAD_ADDRESS + protected_mode_entry - _start
    .word GDT_CODE_SELECTOR

stage2_complete:
    cli
halt:
    hlt
    jmp halt

.code32
protected_mode_entry:
    movw $GDT_DATA_SELECTOR, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $PROTECTED_MODE_STACK, %esp
    cld

    call clear_framebuffer
    call render_splash
    call render_status

    movl (STAGE2_LOAD_ADDRESS + framebuffer_address - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 0 - _start)
    movl (STAGE2_LOAD_ADDRESS + framebuffer_pitch - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 4 - _start)
    movl (STAGE2_LOAD_ADDRESS + font_address - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 8 - _start)
    movl $SPLASH_STATUS_COLOR, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 12 - _start)
    movl $32, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 16 - _start)
    movl $736, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 20 - _start)
    movl (STAGE2_LOAD_ADDRESS + usable_bytes - _start), %eax
    shrl $20, %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 24 - _start)
    movl $FILESYSTEM_CACHE_ADDRESS, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 28 - _start)
    movl $FILESYSTEM_ROOT_ADDRESS, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 32 - _start)
    movl $FILESYSTEM_DATA_ADDRESS, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 36 - _start)
    movl $FILESYSTEM_TOTAL_DATA_CLUSTERS, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 40 - _start)
    movl $(STAGE2_LOAD_ADDRESS + bios_write_sector - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 44 - _start)
    movl $(STAGE2_LOAD_ADDRESS + bios_read_sector - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 48 - _start)
    movl $FILESYSTEM_CACHED_DATA_CLUSTERS, (STAGE2_LOAD_ADDRESS + kernel_boot_info + 52 - _start)
    movl $(STAGE2_LOAD_ADDRESS + kernel_boot_info - _start), %esi
    movl $KERNEL_LOAD_ADDRESS, %eax
    call *%eax
    jmp protected_mode_complete

clear_framebuffer:
    movl (STAGE2_LOAD_ADDRESS + framebuffer_address - _start), %edi
    movl (STAGE2_LOAD_ADDRESS + framebuffer_pitch - _start), %ecx
    imull (STAGE2_LOAD_ADDRESS + framebuffer_height - _start), %ecx
    shrl $2, %ecx
    xorl %eax, %eax
    rep stosl
    ret

render_splash:
    movl $(STAGE2_LOAD_ADDRESS + splash_data + SPLASH_HEADER_BYTES - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + splash_cursor - _start)
    movzwl (STAGE2_LOAD_ADDRESS + splash_data + 8 - _start), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + splash_remaining - _start)

splash_next_cell:
    cmpl $0, (STAGE2_LOAD_ADDRESS + splash_remaining - _start)
    je splash_complete

    movl (STAGE2_LOAD_ADDRESS + splash_cursor - _start), %esi
    movzwl 0(%esi), %eax
    xorl %edx, %edx
    movl $SPLASH_WIDTH, %ecx
    divl %ecx

    shll $4, %eax
    addl $SPLASH_ORIGIN_Y, %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + glyph_y - _start)

    shll $3, %edx
    addl $SPLASH_ORIGIN_X, %edx
    movl %edx, (STAGE2_LOAD_ADDRESS + glyph_x - _start)

    movzbl 2(%esi), %eax
    movb %al, (STAGE2_LOAD_ADDRESS + glyph_character - _start)
    movzbl 3(%esi), %ebx
    movl $(STAGE2_LOAD_ADDRESS + splash_data + 12 - _start), %edi
    movl (%edi,%ebx,4), %eax
    movl %eax, (STAGE2_LOAD_ADDRESS + glyph_color - _start)

    addl $4, (STAGE2_LOAD_ADDRESS + splash_cursor - _start)
    decl (STAGE2_LOAD_ADDRESS + splash_remaining - _start)
    call draw_glyph
    jmp splash_next_cell

splash_complete:
    ret

render_status:
    movl $(STAGE2_LOAD_ADDRESS + splash_status - _start), %esi
    movl $SPLASH_STATUS_X, (STAGE2_LOAD_ADDRESS + glyph_x - _start)
    movl $SPLASH_STATUS_Y, (STAGE2_LOAD_ADDRESS + glyph_y - _start)
    movl $SPLASH_STATUS_COLOR, (STAGE2_LOAD_ADDRESS + glyph_color - _start)

status_next_character:
    movzbl (%esi), %eax
    incl %esi
    testb %al, %al
    jz status_complete
    cmpb $'\n', %al
    je status_new_line
    movb %al, (STAGE2_LOAD_ADDRESS + glyph_character - _start)
    pushl %esi
    call draw_glyph
    popl %esi
    addl $8, (STAGE2_LOAD_ADDRESS + glyph_x - _start)
    jmp status_next_character

status_new_line:
    movl $SPLASH_STATUS_X, (STAGE2_LOAD_ADDRESS + glyph_x - _start)
    addl $16, (STAGE2_LOAD_ADDRESS + glyph_y - _start)
    jmp status_next_character

status_complete:
    ret

/* Draw one 8x16 ROM-font glyph into the 32-bit linear framebuffer. */
draw_glyph:
    movzbl (STAGE2_LOAD_ADDRESS + glyph_character - _start), %eax
    shll $4, %eax
    addl (STAGE2_LOAD_ADDRESS + font_address - _start), %eax
    movl %eax, %esi

    movl (STAGE2_LOAD_ADDRESS + glyph_y - _start), %eax
    imull (STAGE2_LOAD_ADDRESS + framebuffer_pitch - _start), %eax
    addl (STAGE2_LOAD_ADDRESS + framebuffer_address - _start), %eax
    movl (STAGE2_LOAD_ADDRESS + glyph_x - _start), %edx
    shll $2, %edx
    addl %edx, %eax
    movl %eax, %edi
    movl $16, %ecx

glyph_next_row:
    movzbl (%esi), %ebx
    incl %esi
    movl $0x80, %edx
    movl $8, %ebp

glyph_next_pixel:
    testb %dl, %bl
    jz glyph_pixel_complete
    movl (STAGE2_LOAD_ADDRESS + glyph_color - _start), %eax
    movl %eax, (%edi)

glyph_pixel_complete:
    addl $4, %edi
    shrl $1, %edx
    decl %ebp
    jnz glyph_next_pixel

    movl (STAGE2_LOAD_ADDRESS + framebuffer_pitch - _start), %eax
    subl $32, %eax
    addl %eax, %edi
    decl %ecx
    jnz glyph_next_row
    ret

/* EAX=LBA and EDX=physical buffer; returns EAX=0 on success. */
bios_read_sector:
    movb $0x02, (STAGE2_LOAD_ADDRESS + bios_sector_operation - _start)
    jmp bios_transfer_sector

bios_write_sector:
    movb $0x03, (STAGE2_LOAD_ADDRESS + bios_sector_operation - _start)
bios_transfer_sector:
    cli
    pushal
    movl %esp, (STAGE2_LOAD_ADDRESS + bios_saved_esp - _start)
    movl %edx, %ebx
    xorl %edx, %edx
    movl $36, %ecx
    divl %ecx
    movb %al, (STAGE2_LOAD_ADDRESS + bios_write_cylinder - _start)
    movl %edx, %eax
    xorl %edx, %edx
    movl $18, %ecx
    divl %ecx
    movb %al, (STAGE2_LOAD_ADDRESS + bios_write_head - _start)
    incb %dl
    movb %dl, (STAGE2_LOAD_ADDRESS + bios_write_sector_number - _start)
    movl %ebx, %eax
    andl $0x0f, %eax
    movw %ax, (STAGE2_LOAD_ADDRESS + bios_write_offset - _start)
    shrl $4, %ebx
    movw %bx, (STAGE2_LOAD_ADDRESS + bios_write_segment - _start)
    movb $1, (STAGE2_LOAD_ADDRESS + bios_write_result - _start)
    ljmp $GDT_CODE16_SELECTOR, $(STAGE2_LOAD_ADDRESS + bios_write_pm16 - _start)

.code16
bios_write_pm16:
    movw $GDT_DATA16_SELECTOR, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movl %cr0, %eax
    andl $0xfffffffe, %eax
    movl %eax, %cr0
    ljmp $0x0000, $(STAGE2_LOAD_ADDRESS + bios_write_real - _start)

bios_write_real:
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %ss
    movw $0x7000, %sp
    movb $3, (STAGE2_LOAD_ADDRESS + bios_write_retries - _start)
bios_write_attempt:
    movw (STAGE2_LOAD_ADDRESS + bios_write_segment - _start), %ax
    movw %ax, %es
    movw (STAGE2_LOAD_ADDRESS + bios_write_offset - _start), %bx
    movb (STAGE2_LOAD_ADDRESS + bios_sector_operation - _start), %ah
    movb $1, %al
    movb (STAGE2_LOAD_ADDRESS + bios_write_cylinder - _start), %ch
    movb (STAGE2_LOAD_ADDRESS + bios_write_sector_number - _start), %cl
    movb (STAGE2_LOAD_ADDRESS + bios_write_head - _start), %dh
    movb (STAGE2_LOAD_ADDRESS + boot_drive - _start), %dl
    int $0x13
    jnc bios_write_succeeded
    xorw %ax, %ax
    movb (STAGE2_LOAD_ADDRESS + boot_drive - _start), %dl
    int $0x13
    decb (STAGE2_LOAD_ADDRESS + bios_write_retries - _start)
    jnz bios_write_attempt
    jmp bios_write_reenter_pm
bios_write_succeeded:
    movb $0, (STAGE2_LOAD_ADDRESS + bios_write_result - _start)
bios_write_reenter_pm:
    cli
    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0
    .byte 0x66, 0xea
    .long STAGE2_LOAD_ADDRESS + bios_write_resume - _start
    .word GDT_CODE_SELECTOR

.code32
bios_write_resume:
    movw $GDT_DATA_SELECTOR, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl (STAGE2_LOAD_ADDRESS + bios_saved_esp - _start), %esp
    popal
    movzbl (STAGE2_LOAD_ADDRESS + bios_write_result - _start), %eax
    ret

protected_mode_complete:
    cli
protected_halt:
    hlt
    jmp protected_halt

.code16

boot_drive:
    .byte 0

disk_retries:
    .byte 0
kernel_remaining:
    .byte 0
kernel_cylinder:
    .byte 0
kernel_head:
    .byte 0
kernel_sector:
    .byte 0
kernel_offset:
    .word 0
fs_remaining:
    .byte 0
fs_cylinder:
    .byte 0
fs_head:
    .byte 0
fs_sector:
    .byte 0
fs_offset:
    .word 0
bios_write_retries:
    .byte 0
bios_write_cylinder:
    .byte 0
bios_write_head:
    .byte 0
bios_write_sector_number:
    .byte 0
bios_write_result:
    .byte 1
bios_sector_operation:
    .byte 0
.align 2
bios_write_segment:
    .word 0
bios_write_offset:
    .word 0
.align 4
bios_saved_esp:
    .long 0

stage2_message:
    .asciz "M.I.L.O stage 2 online\r\n"

memory_message:
    .asciz "Usable memory: "

mib_message:
    .asciz " MiB\r\n"

memory_error_message:
    .asciz "M.I.L.O memory detection failed\r\n"

graphics_error_message:
    .asciz "M.I.L.O graphics initialization failed\r\n"

kernel_error_message:
    .asciz "M.I.L.O kernel load failed\r\n"

filesystem_error_message:
    .asciz "M.I.L.O FAT12 cache load failed\r\n"

splash_status:
    .ascii "[ OK ] MEMORY MAP READY\n"
    .ascii "[ OK ] VIDEO ONLINE\n"
    .ascii "[ OK ] 32-BIT MODE ONLINE\n"
    .asciz "M.I.L.O SYSTEM INITIALISING..."

.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00cf9a000000ffff
    .quad 0x00cf92000000ffff
    .quad 0x00009a000000ffff
    .quad 0x000092000000ffff
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt - 1
    .long STAGE2_LOAD_ADDRESS + gdt - _start

.align 4
usable_bytes:
    .long 0

memory_map_count:
    .word 0

.align 4
font_address:
    .long 0
mode_list_offset:
    .word 0
mode_list_segment:
    .word 0
selected_vbe_mode:
    .word 0
framebuffer_address:
    .long 0
framebuffer_pitch:
    .long 0
framebuffer_width:
    .long 0
framebuffer_height:
    .long 0
splash_cursor:
    .long 0
splash_remaining:
    .long 0
glyph_x:
    .long 0
glyph_y:
    .long 0
glyph_color:
    .long 0
glyph_character:
    .byte 0

.align 4
kernel_boot_info:
    .space 56

decimal_buffer:
    .space 10
decimal_buffer_end:
    .byte 0

.align 4
memory_map:
    .space E820_ENTRY_SIZE * E820_MAX_ENTRIES

.align 16
vbe_controller_info:
    .space 512

.align 16
vbe_mode_info:
    .space 256

.align 4
splash_data:
    .incbin "build/M.I.L.O/splash.bin"
splash_data_end:

.code16
.section .text
.global _start

#define STAGE2_LOAD_ADDRESS 0x8000
#define STAGE2_FIRST_SECTOR 2
#define STAGE2_SECTOR_COUNT 16

/*
 * M.I.L.O BIOS boot sector.
 *
 * The BIOS loads this sector at physical address 0x7c00 and enters with the
 * boot drive in DL. Stage 1 loads the larger Stage 2 loader from reserved
 * FAT12 sectors while preserving the BIOS drive number for later disk reads.
 */
_start:
    jmp boot_code
    nop

    /* FAT12 BIOS parameter block; loader sectors are reserved from the FAT. */
    .ascii "MILO30.1"
    .word 512
    .byte 1
    .word 114
    .byte 2
    .word 224
    .word 2880
    .byte 0xf0
    .word 9
    .word 18
    .word 2
    .long 0
    .long 0
    .byte 0
    .byte 0
    .byte 0x29
    .long 0x4d494c4f
    .ascii "M.I.L.O    "
    .ascii "FAT12   "

boot_code:
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7c00, %sp
    sti
    cld

    movb %dl, (0x7c00 + boot_drive - _start)

    /* Clear firmware text immediately and stay visually silent until Stage 2
     * can present the native M.I.L.O splash. Firmware errors still print. */
    movw $0x0003, %ax
    int $0x10
    movw $0x0100, %ax
    movw $0x2000, %cx
    int $0x10

    movb $3, (0x7c00 + retries_remaining - _start)

read_stage2:
    xorw %ax, %ax
    movb (0x7c00 + boot_drive - _start), %dl
    int $0x13

    xorw %ax, %ax
    movw %ax, %es
    movw $STAGE2_LOAD_ADDRESS, %bx
    movb $0x02, %ah
    movb $STAGE2_SECTOR_COUNT, %al
    movb $0x00, %ch
    movb $STAGE2_FIRST_SECTOR, %cl
    movb $0x00, %dh
    movb (0x7c00 + boot_drive - _start), %dl
    int $0x13
    jnc stage2_loaded

    decb (0x7c00 + retries_remaining - _start)
    jnz read_stage2
    jmp disk_error

stage2_loaded:
    movb (0x7c00 + boot_drive - _start), %dl
    ljmp $0x0000, $STAGE2_LOAD_ADDRESS

disk_error:
    movw $(0x7c00 + error_message - _start), %si
    call print_string
    jmp boot_complete

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

boot_complete:
    cli
halt:
    hlt
    jmp halt

boot_drive:
    .byte 0

retries_remaining:
    .byte 0

error_message:
    .asciz "M.I.L.O disk read error\r\n"

.org 510
.word 0xaa55

#define MULTIBOOT2_MAGIC 0x36d76289
#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6
#define MULTIBOOT2_ARCH_I386 0
#define MULTIBOOT2_HEADER_LENGTH (multiboot_header_end - multiboot_header)
#define MULTIBOOT2_CHECKSUM -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCH_I386 + MULTIBOOT2_HEADER_LENGTH)

.section .multiboot2,"a"
.align 8
.globl multiboot_header
multiboot_header:
    .long MULTIBOOT2_HEADER_MAGIC
    .long MULTIBOOT2_ARCH_I386
    .long MULTIBOOT2_HEADER_LENGTH
    .long MULTIBOOT2_CHECKSUM

    .align 8
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32

    .align 8
    .short 0
    .short 0
    .long 8
multiboot_header_end:

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

.section .text
.globl _start
.type _start, @function
_start:
    cli
    movl $stack_top, %esp
    xorl %ebp, %ebp

    pushl %ebx
    pushl %eax
    call kmain

.hang:
    cli
    hlt
    jmp .hang

.size _start, . - _start

.section .text

.macro ISR_NOERR number
.globl isr\number
.type isr\number, @function
isr\number:
    pushl $0
    pushl $\number
    jmp interrupt_common
.endm

.macro ISR_ERR number
.globl isr\number
.type isr\number, @function
isr\number:
    pushl $\number
    jmp interrupt_common
.endm

ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR 30
ISR_NOERR 31

ISR_NOERR 32
ISR_NOERR 33
ISR_NOERR 34
ISR_NOERR 35
ISR_NOERR 36
ISR_NOERR 37
ISR_NOERR 38
ISR_NOERR 39
ISR_NOERR 40
ISR_NOERR 41
ISR_NOERR 42
ISR_NOERR 43
ISR_NOERR 44
ISR_NOERR 45
ISR_NOERR 46
ISR_NOERR 47
ISR_NOERR 128

.globl interrupt_common
.type interrupt_common, @function
interrupt_common:
    pusha

    movw %ds, %ax
    pushl %eax
    movw %es, %ax
    pushl %eax
    movw %fs, %ax
    pushl %eax
    movw %gs, %ax
    pushl %eax

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl %esp
    call x86_interrupt_dispatch
    addl $4, %esp

    popl %eax
    movw %ax, %gs
    popl %eax
    movw %ax, %fs
    popl %eax
    movw %ax, %es
    popl %eax
    movw %ax, %ds

    popa
    addl $8, %esp
    iret

.size interrupt_common, . - interrupt_common

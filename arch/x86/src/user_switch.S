.section .text

.globl x86_user_save_context
.type x86_user_save_context, @function
x86_user_save_context:
    movl 4(%esp), %edx
    movl %esp, (%edx)
    movl 8(%esp), %edx
    movl %ebp, (%edx)
    movl 12(%esp), %edx
    movl $x86_user_save_context_resume, (%edx)
    xorl %eax, %eax

x86_user_save_context_resume:
    ret

.size x86_user_save_context, . - x86_user_save_context

.globl x86_user_restore_context
.type x86_user_restore_context, @function
x86_user_restore_context:
    movl 4(%esp), %edx
    movl 8(%esp), %ecx
    movl 12(%esp), %ebx
    movl %edx, %esp
    movl %ecx, %ebp
    movl $1, %eax
    jmp *%ebx

.size x86_user_restore_context, . - x86_user_restore_context

.globl x86_user_enter_context
.type x86_user_enter_context, @function
x86_user_enter_context:
    movl 4(%esp), %edx

    movw $0x23, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    pushl $0x23
    pushl 36(%edx)
    pushl 32(%edx)
    pushl $0x1b
    pushl 28(%edx)

    movl 20(%edx), %edi
    movl 16(%edx), %esi
    movl 24(%edx), %ebp
    movl 4(%edx), %ebx
    movl 8(%edx), %ecx
    movl 0(%edx), %eax
    movl 12(%edx), %edx
    iret

.size x86_user_enter_context, . - x86_user_enter_context

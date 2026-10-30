.section __TEXT,__text
.globl _main
.p2align 2
_main:
    movq $77, %rdi
    pushq $60
    popq %rax
    syscall

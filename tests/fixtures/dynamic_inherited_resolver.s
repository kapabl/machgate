.section __TEXT,__text
.globl _main
.p2align 2
_main:
    sub sp, sp, #16
    mov x0, #8
    mov x1, sp
    bl _getrlimit
    add sp, sp, #16
    mov x16, #1
    svc #0x80

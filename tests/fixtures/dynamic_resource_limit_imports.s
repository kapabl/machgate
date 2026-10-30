.section __TEXT,__text
.globl _main
.p2align 2
_main:
    sub sp, sp, #16
    mov x0, #8
    mov x1, sp
    bl _getrlimit
    cbnz w0, failed
    ldp x8, x9, [sp]
    cbz x8, failed
    cmp x9, x8
    b.lo failed
    mov x0, #8
    mov x1, sp
    bl _setrlimit
    cbnz w0, failed
    mov x0, #0
    b exit

failed:
    mov x0, #91

exit:
    add sp, sp, #16
    mov x16, #1
    svc #0x80

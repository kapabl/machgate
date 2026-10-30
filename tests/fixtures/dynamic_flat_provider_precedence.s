.section __TEXT,__text
.globl _main
.p2align 2
_main:
    sub sp, sp, #16
    mov x0, sp
    mov x1, xzr
    bl _pthread_key_create
    cbnz w0, 1f
    ldr w9, [sp]
    cmp w9, #128
    b.ne 1f
    mov x0, #69
    b 2f
1:
    mov x0, #1
2:
    add sp, sp, #16
    mov x16, #1
    svc #0x80

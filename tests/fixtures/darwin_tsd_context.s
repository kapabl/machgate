.section __TEXT,__text,regular,pure_instructions
.p2align 2
.globl _main
_main:
    stp x20, x19, [sp, #-16]!
    sub sp, sp, #16
    mov x0, sp
    mov x1, xzr
    bl _pthread_key_create
    cbnz w0, 1f
    ldr w19, [sp]
    cmp w19, #511
    b.hi 1f
    mov x20, #0x4d47
    mov w0, w19
    mov x1, x20
    bl _pthread_setspecific
    cbnz w0, 1f
    mrs x8, TPIDRRO_EL0
    ldr x9, [x8, x19, lsl #3]
    cmp x9, x20
    cset w0, ne
    b 2f
1:
    mov w0, #1
2:
    add sp, sp, #16
    ldp x20, x19, [sp], #16
    mov x16, #1
    svc #0x80

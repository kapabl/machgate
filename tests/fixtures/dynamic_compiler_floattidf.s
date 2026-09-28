.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #42
    mov x1, #0
    bl ___floattidf
    umov x9, v0.d[0]
    cbnz x9, failure
    umov x9, v0.d[1]
    movz x10, #0x5000, lsl #32
    movk x10, #0x4004, lsl #48
    cmp x9, x10
    b.ne failure
    mov x0, #68
    mov x16, #1
    svc #0x80

failure:
    mov x0, #94
    mov x16, #1
    svc #0x80

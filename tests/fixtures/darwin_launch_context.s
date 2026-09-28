.section __TEXT,__text,regular,pure_instructions
.p2align 2
.globl _main
_main:
    cmp x0, #3
    b.ne fail

    ldr x4, [sp]
    cmp x4, x0
    b.ne fail
    ldr x4, [sp, #8]
    ldr x5, [x1]
    cmp x4, x5
    b.ne fail

    add x4, x1, x0, lsl #3
    ldr x5, [x4], #8
    cbnz x5, fail
    cmp x2, x4
    b.ne fail

scan_environment:
    ldr x5, [x4], #8
    cbnz x5, scan_environment
    cmp x3, x4
    b.ne fail

    ldr x5, [x3]
    cbz x5, fail
    ldr x6, [x3, #8]
    cbnz x6, fail

    ldr x6, [x5]
    mov x7, #0x7865
    movk x7, #0x6365, lsl #16
    movk x7, #0x7475, lsl #32
    movk x7, #0x6261, lsl #48
    cmp x6, x7
    b.ne fail

    mov x0, #0
    b exit

fail:
    mov x0, #64

exit:
    mov x16, #1
    svc #0x80

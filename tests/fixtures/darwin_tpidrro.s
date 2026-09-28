.section __TEXT,__text,regular,pure_instructions
.p2align 2
.globl _main
_main:
    mrs x5, TPIDRRO_EL0
    mrs x6, TPIDR_EL0
    cmp x5, x6
    b.eq 1f
    ldr x7, [x5]
    cmp x7, x5
    b.ne 1f
    mov x7, #0x4d47
    str x7, [x5, #0x300]
    ldr x8, [x5, #0x300]
    cmp x8, x7
    cset w0, ne
    b 2f
1:
    mov w0, #1
2:
    mov x16, #1
    svc #0x80

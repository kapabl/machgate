.section __TEXT,__text
.globl _main
.p2align 2
_main:
    sub sp, sp, #16
    mov w8, #7
    str w8, [sp]
    adrp x8, _mach_task_self_@GOTPAGE
    ldr x8, [x8, _mach_task_self_@GOTPAGEOFF]
    ldr w0, [x8]
    mov w1, #1
    mov x2, sp
    mov w3, #1
    bl _task_policy_set
    cmp w0, #46
    cset w0, ne
    mov x16, #1
    svc #0x80

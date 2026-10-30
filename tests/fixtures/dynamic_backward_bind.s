.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x9, _a_low@PAGE
    ldr x9, [x9, _a_low@PAGEOFF]
    blr x9
    cmp w0, #73
    b.ne failure

    adrp x9, _b_middle@PAGE
    ldr x9, [x9, _b_middle@PAGEOFF]
    blr x9
    cmp w0, #99
    b.ne failure

    adrp x9, _a_high@PAGE
    ldr x9, [x9, _a_high@PAGEOFF]
    blr x9
    cmp w0, #73
    b.ne failure

    mov x0, #0
    mov x16, #1
    svc #0x80

failure:
    mov x0, #91
    mov x16, #1
    svc #0x80

.section __DATA,__data
.p2align 3
_a_low:
    .quad _machgate_mapped_probe
    .space BACKWARD_TARGET - 8
_b_middle:
    .quad _machgate_override_probe
    .space FINAL_A_OFFSET - BACKWARD_TARGET - 8
_a_high:
    .quad _machgate_mapped_probe

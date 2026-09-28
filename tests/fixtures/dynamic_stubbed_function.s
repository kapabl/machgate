.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x9, _machgate_absent_probe@GOTPAGE
    ldr x9, [x9, _machgate_absent_probe@GOTPAGEOFF]
    blr x9
    mov x0, #93
    mov x16, #1
    svc #0x80

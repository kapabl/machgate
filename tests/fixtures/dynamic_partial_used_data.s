.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x9, _machgate_absent_data@GOTPAGE
    ldr x9, [x9, _machgate_absent_data@GOTPAGEOFF]
    ldr x0, [x9]
    mov x16, #1
    svc #0x80

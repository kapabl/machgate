.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x9, _machgate_optional_data@GOTPAGE
    ldr x9, [x9, _machgate_optional_data@GOTPAGEOFF]
    cbnz x9, failure
    mov x0, #0
    mov x16, #1
    svc #0x80

failure:
    mov x0, #92
    mov x16, #1
    svc #0x80

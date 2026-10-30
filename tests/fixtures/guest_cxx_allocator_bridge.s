.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #0
    mov x16, #1
    svc #0x80

.globl __Znwm
.p2align 2
__Znwm:
    mov x0, #0
    ret

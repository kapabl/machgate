.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #0
    mov x1, #0
    bl ___clear_cache
    mov x0, #66
    mov x16, #1
    svc #0x80

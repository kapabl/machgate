.section __TEXT,__text
.globl _main
.p2align 2
_main:
    bl _machgate_absent_probe
    mov x16, #1
    svc #0x80

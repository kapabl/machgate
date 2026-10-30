.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #41
    mov x16, #1
    svc #0x80

.globl _unused_import_call
.p2align 2
_unused_import_call:
    bl _machgate_absent_probe
    ret

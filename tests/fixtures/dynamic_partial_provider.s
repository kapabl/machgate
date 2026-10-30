.section __TEXT,__text
.globl _main
.p2align 2
_main:
    bl _machgate_mapped_probe
    mov x16, #1
    svc #0x80

.section __DATA,__data
.p2align 3
_unused_missing_pointer:
    .quad _machgate_absent_probe

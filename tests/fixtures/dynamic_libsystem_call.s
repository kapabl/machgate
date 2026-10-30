.section __TEXT,__text
.globl _main
.p2align 2
_main:
    bl _mach_absolute_time
    cbz x0, failed
    mov x0, #1
    adrp x1, message@PAGE
    add x1, x1, message@PAGEOFF
    mov x2, #11
    mov x16, #4
    svc #0x80
    mov x0, #0
    mov x16, #1
    svc #0x80
failed:
    mov x0, #45
    mov x16, #1
    svc #0x80

.section __TEXT,__cstring,cstring_literals
message:
    .asciz "dynamic-ok\n"

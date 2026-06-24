; darwin_execve_exit42.s - exec another Mach-O through Darwin SYS_execve

.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x0, target_path@PAGE
    add x0, x0, target_path@PAGEOFF
    adrp x1, target_argv@PAGE
    add x1, x1, target_argv@PAGEOFF
    mov x2, #0
    mov x16, #59
    svc #0x80

    mov x0, #1
    mov x16, #1
    svc #0x80

.section __DATA,__data
.p2align 3
target_argv:
    .quad target_path
    .quad 0

.section __TEXT,__cstring
target_path:
    .asciz "darwin_exit42"

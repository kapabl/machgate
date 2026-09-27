; darwin_fstat.s - verify Darwin-layout fstat st_size
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x0, path@PAGE
    add x0, x0, path@PAGEOFF
    mov x1, #0
    mov x2, #0
    mov x16, #5        ; Darwin SYS_open
    svc #0x80
    b.cs fail
    mov x19, x0

    mov x0, x19
    adrp x1, statbuf@PAGE
    add x1, x1, statbuf@PAGEOFF
    mov x16, #189      ; Darwin SYS_fstat
    svc #0x80
    b.cs fail
    cbnz x0, fail

    adrp x20, statbuf@PAGE
    add x20, x20, statbuf@PAGEOFF
    ldr x0, [x20, #96] ; Darwin struct stat st_size
    cmp x0, #14
    b.ne fail

    mov x0, x19
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail

    mov x0, #0
    mov x16, #1
    svc #0x80

fail:
    mov x0, #1
    mov x16, #1
    svc #0x80

.section __TEXT,__cstring
path:
    .asciz "syscall_data.txt"

.section __DATA,__data
.p2align 3
statbuf:
    .space 144

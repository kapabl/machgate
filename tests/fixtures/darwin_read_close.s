; darwin_read_close.s - read stdin and close fd using Darwin syscalls
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #0
    adrp x1, buf@PAGE
    add x1, x1, buf@PAGEOFF
    mov x2, #8
    mov x16, #3        ; Darwin SYS_read
    svc #0x80
    b.cs fail
    cmp x0, #8
    b.ne fail

    mov x0, #1
    adrp x1, buf@PAGE
    add x1, x1, buf@PAGEOFF
    mov x2, #8
    mov x16, #4        ; Darwin SYS_write
    svc #0x80
    b.cs fail
    cmp x0, #8
    b.ne fail

    mov x0, #0
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail
    cbnz x0, fail

    mov x0, #0
    mov x16, #1        ; Darwin SYS_exit
    svc #0x80

fail:
    mov x0, #1
    mov x16, #1
    svc #0x80

.section __DATA,__data
.p2align 3
buf:
    .space 8

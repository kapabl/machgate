; darwin_open_lseek.s - open a file, seek, read, write using Darwin syscalls
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x0, path@PAGE
    add x0, x0, path@PAGEOFF
    mov x1, #0         ; Darwin O_RDONLY
    mov x2, #0
    mov x16, #5        ; Darwin SYS_open
    svc #0x80
    b.cs fail
    mov x19, x0

    mov x0, x19
    mov x1, #6
    mov x2, #0         ; SEEK_SET
    mov x16, #199      ; Darwin SYS_lseek
    svc #0x80
    b.cs fail
    cmp x0, #6
    b.ne fail

    mov x0, x19
    adrp x1, buf@PAGE
    add x1, x1, buf@PAGEOFF
    mov x2, #7
    mov x16, #3        ; Darwin SYS_read
    svc #0x80
    b.cs fail
    cmp x0, #7
    b.ne fail

    mov x0, #1
    adrp x1, buf@PAGE
    add x1, x1, buf@PAGEOFF
    mov x2, #7
    mov x16, #4        ; Darwin SYS_write
    svc #0x80
    b.cs fail

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
buf:
    .space 8

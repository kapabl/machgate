; darwin_read_again.s - read an empty non-blocking pipe, expect Darwin EAGAIN
; Apple references:
; - apple-oss-distributions/xnu/bsd/kern/syscalls.master: SYS_read is 3
; - apple-oss-distributions/xnu/bsd/sys/errno.h: Darwin EAGAIN is 35
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #0          ; stdin: non-blocking pipe read end with no data
    adrp x1, buf@PAGE
    add x1, x1, buf@PAGEOFF
    mov x2, #1
    mov x16, #3         ; Darwin SYS_read
    svc #0x80
    b.cc fail           ; read must fail
    cmp x0, #35         ; Darwin EAGAIN, not the raw Linux EAGAIN (11)
    b.ne fail

    mov x0, #42
    mov x16, #1         ; Darwin SYS_exit
    svc #0x80

fail:
    mov x0, #1
    mov x16, #1
    svc #0x80

.section __DATA,__data
.p2align 3
buf:
    .space 16

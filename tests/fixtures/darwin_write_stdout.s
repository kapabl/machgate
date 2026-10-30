; darwin_write_stdout.s - write stdout using Darwin syscall convention
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #1
    adrp x1, msg@PAGE
    add x1, x1, msg@PAGEOFF
    mov x2, #13
    mov x16, #4        ; Darwin SYS_write
    svc #0x80

    mov x0, #0
    mov x16, #1        ; Darwin SYS_exit
    svc #0x80

.section __TEXT,__cstring
msg:
    .asciz "darwin write\n"

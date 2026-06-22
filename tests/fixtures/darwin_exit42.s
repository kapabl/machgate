; darwin_exit42.s - exit(42) via Darwin ARM64 syscall convention
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #42
    mov x16, #1        ; Darwin SYS_exit
    svc #0x80

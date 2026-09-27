; darwin_mmap_protect.s - mmap, mprotect, munmap using Darwin syscalls
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #0
    mov x1, #4096
    mov x2, #3         ; PROT_READ | PROT_WRITE
    mov x3, #0x1002    ; MAP_ANON | MAP_PRIVATE
    mov x4, #-1
    mov x5, #0
    mov x16, #197      ; Darwin SYS_mmap
    svc #0x80
    b.cs fail
    mov x19, x0

    movz w1, #0x4d4d   ; "MM"
    movk w1, #0x5041, lsl #16 ; "AP"
    str w1, [x19]
    mov w1, #10
    strb w1, [x19, #4]

    mov x0, x19
    mov x1, #4096
    mov x2, #1         ; PROT_READ
    mov x16, #74       ; Darwin SYS_mprotect
    svc #0x80
    b.cs fail
    cbnz x0, fail

    mov x0, #1
    mov x1, x19
    mov x2, #5
    mov x16, #4        ; Darwin SYS_write
    svc #0x80
    b.cs fail
    cmp x0, #5
    b.ne fail

    mov x0, x19
    mov x1, #4096
    mov x16, #73       ; Darwin SYS_munmap
    svc #0x80
    b.cs fail
    cbnz x0, fail

    mov x0, #0
    mov x16, #1
    svc #0x80

fail:
    mov x0, #1
    mov x16, #1
    svc #0x80

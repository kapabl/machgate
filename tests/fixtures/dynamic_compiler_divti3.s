.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x8, positive_case@PAGE
    add x8, x8, positive_case@PAGEOFF
    ldp x0, x1, [x8]
    ldp x2, x3, [x8, #16]
    bl ___divti3
    adrp x8, positive_case@PAGE
    add x8, x8, positive_case@PAGEOFF
    ldp x9, x10, [x8, #32]
    cmp x0, x9
    b.ne failure
    cmp x1, x10
    b.ne failure

    adrp x8, negative_dividend_case@PAGE
    add x8, x8, negative_dividend_case@PAGEOFF
    ldp x0, x1, [x8]
    ldp x2, x3, [x8, #16]
    bl ___divti3
    adrp x8, negative_dividend_case@PAGE
    add x8, x8, negative_dividend_case@PAGEOFF
    ldp x9, x10, [x8, #32]
    cmp x0, x9
    b.ne failure
    cmp x1, x10
    b.ne failure

    adrp x8, negative_divisor_case@PAGE
    add x8, x8, negative_divisor_case@PAGEOFF
    ldp x0, x1, [x8]
    ldp x2, x3, [x8, #16]
    bl ___divti3
    adrp x8, negative_divisor_case@PAGE
    add x8, x8, negative_divisor_case@PAGEOFF
    ldp x9, x10, [x8, #32]
    cmp x0, x9
    b.ne failure
    cmp x1, x10
    b.ne failure

    mov x0, #70
    mov x16, #1
    svc #0x80

failure:
    mov x0, #95
    mov x16, #1
    svc #0x80

.section __DATA,__const
.p2align 4
positive_case:
    .quad 0x0123456789abcdef
    .quad 0x123456789abcdef0
    .quad 0x0123456789abcdef
    .quad 0x0000000000000000
    .quad 0x0000000000000001
    .quad 0x0000000000000010

negative_dividend_case:
    .quad 0xfedcba9876543211
    .quad 0xedcba9876543210f
    .quad 0x0123456789abcdef
    .quad 0x0000000000000000
    .quad 0xffffffffffffffff
    .quad 0xffffffffffffffef

negative_divisor_case:
    .quad 0x0fedcba987654321
    .quad 0x7fedcba987654321
    .quad 0xedcba98765432110
    .quad 0xffffffffffffffff
    .quad 0xf8fffffffffff962
    .quad 0xfffffffffffffff8

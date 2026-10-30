.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x9, _missing_function_pointer@PAGE
    add x9, x9, _missing_function_pointer@PAGEOFF
    ldr x9, [x9]
    blr x9
    mov x16, #1
    svc #0x80

.section __DATA,__la_symbol_ptr,lazy_symbol_pointers
.p2align 3
_missing_function_pointer:
    .quad _machgate_absent_probe

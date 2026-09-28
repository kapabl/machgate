.section __TEXT,__text
.globl _machgate_mapped_probe
.p2align 2
_machgate_mapped_probe:
    mov x0, #1
    ret

.globl _machgate_override_probe
.p2align 2
_machgate_override_probe:
    mov x0, #2
    ret

.globl _machgate_absent_probe
.p2align 2
_machgate_absent_probe:
    mov x0, #3
    ret

.globl ___clear_cache
.p2align 2
___clear_cache:
    ret

.globl ___floattidf
.p2align 2
___floattidf:
    ret

.globl dyld_stub_binder
.p2align 2
dyld_stub_binder:
    ret

.section __DATA,__data
.globl _machgate_optional_data
.p2align 3
_machgate_optional_data:
    .quad 1

.globl _machgate_absent_data
.p2align 3
_machgate_absent_data:
    .quad 3

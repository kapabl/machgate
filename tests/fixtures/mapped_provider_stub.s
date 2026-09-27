.section __TEXT,__text
.globl _machgate_mapped_probe
.p2align 2
_machgate_mapped_probe:
    mov x0, #1
    ret

.globl dyld_stub_binder
.p2align 2
dyld_stub_binder:
    ret

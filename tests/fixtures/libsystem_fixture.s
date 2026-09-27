.section __TEXT,__text
.globl _mach_absolute_time
.p2align 2
_mach_absolute_time:
    mov x0, #1
    ret

.globl dyld_stub_binder
.p2align 2
dyld_stub_binder:
    ret

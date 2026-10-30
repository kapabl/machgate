.section __TEXT,__text

.globl _machgate_override_probe
.p2align 2
_machgate_override_probe:
    mov x0, #41
    ret

.globl _main
.p2align 2
_main:
    bl _machgate_override_probe
    mov x16, #1
    svc #0x80

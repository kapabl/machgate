.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x9, _machgate_guest_probe@GOTPAGE
    ldr x9, [x9, _machgate_guest_probe@GOTPAGEOFF]
    blr x9
    mov x16, #1
    svc #0x80

.globl _machgate_guest_probe
.weak_definition _machgate_guest_probe
.p2align 2
_machgate_guest_probe:
    mov x0, #67
    ret

.text
.p2align 2

.macro validate_arguments marker
    cmp x1, #0x11
    b.ne 9f
    cmp x2, #0x22
    b.ne 9f
    cmp x3, #0x33
    b.ne 9f
    cmp x4, #0x44
    b.ne 9f
    cmp x5, #0x55
    b.ne 9f
    cmp x6, #0x66
    b.ne 9f
    cmp x7, #0x77
    b.ne 9f
    mov x9, #\marker
    str x9, [x0]
    mov x0, xzr
    ret
9:
    mov x9, #0xee
    str x9, [x0]
    mov x0, xzr
    ret
.endm

.globl _ZN21MachGateStructorProbeC1Ev
.type _ZN21MachGateStructorProbeC1Ev, %function
_ZN21MachGateStructorProbeC1Ev:
    .cfi_startproc
    validate_arguments 0xc1
    .cfi_endproc
.size _ZN21MachGateStructorProbeC1Ev, .-_ZN21MachGateStructorProbeC1Ev

.globl _ZN21MachGateStructorProbeC2Ev
.type _ZN21MachGateStructorProbeC2Ev, %function
.set _ZN21MachGateStructorProbeC2Ev, _ZN21MachGateStructorProbeC1Ev

.globl _ZN21MachGateStructorProbeD1Ev
.type _ZN21MachGateStructorProbeD1Ev, %function
_ZN21MachGateStructorProbeD1Ev:
    .cfi_startproc
    validate_arguments 0xd1
    .cfi_endproc
.size _ZN21MachGateStructorProbeD1Ev, .-_ZN21MachGateStructorProbeD1Ev

.globl _ZN21MachGateStructorProbeD2Ev
.type _ZN21MachGateStructorProbeD2Ev, %function
_ZN21MachGateStructorProbeD2Ev:
    nop
    .cfi_startproc
    validate_arguments 0xd2
    .cfi_endproc
.size _ZN21MachGateStructorProbeD2Ev, .-_ZN21MachGateStructorProbeD2Ev

.globl _ZN21MachGateStructorProbeD0Ev
.type _ZN21MachGateStructorProbeD0Ev, %function
_ZN21MachGateStructorProbeD0Ev:
    mov x0, xzr
    ret
.size _ZN21MachGateStructorProbeD0Ev, .-_ZN21MachGateStructorProbeD0Ev

.section .note.GNU-stack,"",%progbits

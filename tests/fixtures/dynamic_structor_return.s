.section __TEXT,__text
.globl _main
.p2align 2
_main:
    adrp x19, _structor_object@PAGE
    add x19, x19, _structor_object@PAGEOFF

    mov x0, x19
    mov x1, #0x11
    mov x2, #0x22
    mov x3, #0x33
    mov x4, #0x44
    mov x5, #0x55
    mov x6, #0x66
    mov x7, #0x77
    bl __ZN21MachGateStructorProbeC1Ev
    cmp x0, x19
    b.ne Lfail_c1_return
    ldr x9, [x19]
    cmp x9, #0xc1
    b.ne Lfail_arguments

    mov x0, x19
    mov x1, #0x11
    mov x2, #0x22
    mov x3, #0x33
    mov x4, #0x44
    mov x5, #0x55
    mov x6, #0x66
    mov x7, #0x77
    bl __ZN21MachGateStructorProbeC2Ev
    cmp x0, x19
    b.ne Lfail_c2_return

    mov x0, x19
    mov x1, #0x11
    mov x2, #0x22
    mov x3, #0x33
    mov x4, #0x44
    mov x5, #0x55
    mov x6, #0x66
    mov x7, #0x77
    bl __ZN21MachGateStructorProbeD1Ev
    cmp x0, x19
    b.ne Lfail_d1_return
    ldr x9, [x19]
    cmp x9, #0xd1
    b.ne Lfail_arguments

    adrp x9, _d2_with_addend@PAGE
    ldr x9, [x9, _d2_with_addend@PAGEOFF]
    mov x0, x19
    mov x1, #0x11
    mov x2, #0x22
    mov x3, #0x33
    mov x4, #0x44
    mov x5, #0x55
    mov x6, #0x66
    mov x7, #0x77
    blr x9
    cmp x0, x19
    b.ne Lfail_d2_return
    ldr x10, [x19]
    cmp x10, #0xd2
    b.ne Lfail_arguments

    mov x0, x19
    bl __ZN21MachGateStructorProbeD0Ev
    cbnz x0, Lfail_d0_adapted

    adrp x20, __ZN21MachGateStructorProbeC1Ev@GOTPAGE
    ldr x20, [x20, __ZN21MachGateStructorProbeC1Ev@GOTPAGEOFF]
    adrp x21, __ZN21MachGateStructorProbeC2Ev@GOTPAGE
    ldr x21, [x21, __ZN21MachGateStructorProbeC2Ev@GOTPAGEOFF]
    cmp x20, x21
    b.ne Lfail_alias_not_cached

    mov x0, x20
    bl _machgate_structor_mapping_is_rx
    cmp x0, #1
    b.ne Lfail_mapping_permissions

    adrp x1, __ZN21MachGateStructorProbeD1Ev@GOTPAGE
    ldr x1, [x1, __ZN21MachGateStructorProbeD1Ev@GOTPAGEOFF]
    mov x0, x20
    bl _machgate_structor_same_page
    cmp x0, #1
    b.ne Lfail_page_packing

    mov x0, #0
    b Lexit
Lfail_c1_return:
    mov x0, #81
    b Lexit
Lfail_c2_return:
    mov x0, #82
    b Lexit
Lfail_d1_return:
    mov x0, #83
    b Lexit
Lfail_d2_return:
    mov x0, #84
    b Lexit
Lfail_d0_adapted:
    mov x0, #85
    b Lexit
Lfail_arguments:
    mov x0, #86
    b Lexit
Lfail_alias_not_cached:
    mov x0, #87
    b Lexit
Lfail_mapping_permissions:
    mov x0, #88
    b Lexit
Lfail_page_packing:
    mov x0, #89
Lexit:
    mov x16, #1
    svc #0x80

.section __DATA,__data
.p2align 3
_structor_object:
    .quad 0
_d2_with_addend:
    .quad __ZN21MachGateStructorProbeD2Ev + 4

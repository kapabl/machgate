	.text
	.align 2
	.global machgate_probe_tlv_registers
	.type machgate_probe_tlv_registers, %function
machgate_probe_tlv_registers:
	sub sp, sp, #48
	stp x29, x30, [sp]
	stp x19, x20, [sp, #16]
	str x18, [sp, #32]
	mov x29, sp
	mov x19, x0
	mov x0, x1
	mov x1, #1
	mov x2, #2
	mov x3, #3
	mov x4, #4
	mov x5, #5
	mov x6, #6
	mov x7, #7
	mov x8, #8
	mov x9, #9
	mov x10, #10
	mov x11, #11
	mov x12, #12
	mov x13, #13
	mov x14, #14
	mov x15, #15
	mov x16, #16
	mov x17, #17
	blr x19
	cbz x0, failed
	cmp x1, #1
	b.ne failed
	cmp x2, #2
	b.ne failed
	cmp x3, #3
	b.ne failed
	cmp x4, #4
	b.ne failed
	cmp x5, #5
	b.ne failed
	cmp x6, #6
	b.ne failed
	cmp x7, #7
	b.ne failed
	cmp x8, #8
	b.ne failed
	cmp x9, #9
	b.ne failed
	cmp x10, #10
	b.ne failed
	cmp x11, #11
	b.ne failed
	cmp x12, #12
	b.ne failed
	cmp x13, #13
	b.ne failed
	cmp x14, #14
	b.ne failed
	cmp x15, #15
	b.ne failed
	cmp x16, #16
	b.ne failed
	cmp x17, #17
	b.ne failed
	ldr x1, [sp, #32]
	cmp x18, x1
	b.ne failed
	mov w0, #0
	b complete

failed:
	mov w0, #1

complete:
	ldr x18, [sp, #32]
	ldp x19, x20, [sp, #16]
	ldp x29, x30, [sp]
	add sp, sp, #48
	ret
	.size machgate_probe_tlv_registers, .-machgate_probe_tlv_registers

	.section .note.GNU-stack,"",%progbits

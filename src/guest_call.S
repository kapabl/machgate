	.text
	.align 2
	.global machgate_call_guest_initializer
	.type machgate_call_guest_initializer, %function
machgate_call_guest_initializer:
	stp x29, x30, [sp, #-16]!
	stp x19, x20, [sp, #-16]!
	mov x19, sp
	mov x20, x0
	mov w0, w1
	mov x1, x2
	mov x2, x3
	mov x3, x4
	mov sp, x5
	mov x29, xzr
	blr x20
	mov sp, x19
	ldp x19, x20, [sp], #16
	ldp x29, x30, [sp], #16
	ret
	.size machgate_call_guest_initializer, .-machgate_call_guest_initializer

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

	.align 2
	.global machgate_enter_guest
	.type machgate_enter_guest, %function
machgate_enter_guest:
	mov x9, x0
	mov sp, x1
	mov x0, x2
	mov x1, x3
	mov x2, x4
	mov x3, x5
	adrp x30, machgate_guest_return
	add x30, x30, :lo12:machgate_guest_return
	br x9
	.size machgate_enter_guest, .-machgate_enter_guest

	.align 2
	.type machgate_guest_return, %function
machgate_guest_return:
	bl machgate_terminate_after_guest_return
	brk #0
	.size machgate_guest_return, .-machgate_guest_return

	.section .note.GNU-stack,"",%progbits

	.text
	.align 2
	.global _tlv_bootstrap
	.type _tlv_bootstrap, %function
_tlv_bootstrap:
	sub sp, sp, #160
	stp x1, x2, [sp]
	stp x3, x4, [sp, #16]
	stp x5, x6, [sp, #32]
	stp x7, x8, [sp, #48]
	stp x9, x10, [sp, #64]
	stp x11, x12, [sp, #80]
	stp x13, x14, [sp, #96]
	stp x15, x16, [sp, #112]
	stp x17, x18, [sp, #128]
	str x30, [sp, #144]
	bl _tlv_bootstrap_impl
	ldr x30, [sp, #144]
	ldp x17, x18, [sp, #128]
	ldp x15, x16, [sp, #112]
	ldp x13, x14, [sp, #96]
	ldp x11, x12, [sp, #80]
	ldp x9, x10, [sp, #64]
	ldp x7, x8, [sp, #48]
	ldp x5, x6, [sp, #32]
	ldp x3, x4, [sp, #16]
	ldp x1, x2, [sp]
	add sp, sp, #160
	ret
	.size _tlv_bootstrap, .-_tlv_bootstrap

	.section .note.GNU-stack,"",%progbits

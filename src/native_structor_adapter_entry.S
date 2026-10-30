.text
.p2align 2
.globl machgate_native_structor_adapter_entry
.type machgate_native_structor_adapter_entry, %function
machgate_native_structor_adapter_entry:
	.cfi_startproc
	stp x29, x30, [sp, #-32]!
	.cfi_def_cfa_offset 32
	.cfi_offset x29, -32
	.cfi_offset x30, -24
	mov x29, sp
	.cfi_def_cfa_register x29
	str x0, [sp, #16]
	blr x16
	ldr x0, [sp, #16]
	ldp x29, x30, [sp], #32
	.cfi_restore x29
	.cfi_restore x30
	.cfi_def_cfa sp, 0
	ret
	.cfi_endproc
.size machgate_native_structor_adapter_entry, .-machgate_native_structor_adapter_entry

.section .note.GNU-stack,"",%progbits

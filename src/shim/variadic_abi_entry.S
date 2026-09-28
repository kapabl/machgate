.text
.p2align 2
.global printf
.type printf, %function
printf:
	mov x9, sp
	stp x18, x30, [sp, #-16]!
	mov x1, x9
	bl machgate_darwin_printf_from_stack
	ldp x18, x30, [sp], #16
	ret
.size printf, .-printf

.p2align 2
.global vfprintf
.type vfprintf, %function
vfprintf:
	stp x18, x30, [sp, #-16]!
	bl machgate_darwin_vfprintf_from_stack
	ldp x18, x30, [sp], #16
	ret
.size vfprintf, .-vfprintf

.p2align 2
.global vsnprintf
.type vsnprintf, %function
vsnprintf:
	stp x18, x30, [sp, #-16]!
	bl machgate_darwin_vsnprintf_from_stack
	ldp x18, x30, [sp], #16
	ret
.size vsnprintf, .-vsnprintf

.p2align 2
.global snprintf
.type snprintf, %function
snprintf:
	mov x9, sp
	stp x18, x30, [sp, #-16]!
	mov x3, x9
	bl machgate_darwin_snprintf_from_stack
	ldp x18, x30, [sp], #16
	ret
.size snprintf, .-snprintf

.section .note.GNU-stack,"",%progbits

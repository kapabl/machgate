.text
.p2align 2
.global fcntl
.type fcntl, %function
fcntl:
	mov x9, sp
	stp x18, x30, [sp, #-16]!
	ldr x2, [x9]
	bl machgate_darwin_fcntl_fixed
	ldp x18, x30, [sp], #16
	ret
.size fcntl, .-fcntl

.p2align 2
.global ioctl
.type ioctl, %function
ioctl:
	mov x9, sp
	stp x18, x30, [sp, #-16]!
	ldr x2, [x9]
	bl machgate_darwin_ioctl_fixed
	ldp x18, x30, [sp], #16
	ret
.size ioctl, .-ioctl

.section .note.GNU-stack,"",%progbits

; darwin_panicwrap_self_reexec.s - focused panicwrap-shaped fork/reexec probe

.equ SYS_EXIT, 1
.equ SYS_FORK, 2
.equ SYS_READ, 3
.equ SYS_WRITE, 4
.equ SYS_CLOSE, 6
.equ SYS_WAIT4, 7
.equ SYS_PIPE, 42
.equ SYS_EXECVE, 59
.equ SYS_DUP2, 90
.equ WNOHANG, 1

.section __TEXT,__text
.globl _main
.p2align 2
_main:
	ldr x19, [sp, #8]
	add x20, sp, #16
	bl is_child_reexec
	cbnz x0, child_main
	b parent_main

parent_main:
	adrp x0, stdout_pipe@PAGE
	add x0, x0, stdout_pipe@PAGEOFF
	mov x16, #SYS_PIPE
	svc #0x80
	b.cs exit_syscall_errno
	adrp x9, stdout_pipe@PAGE
	add x9, x9, stdout_pipe@PAGEOFF
	stp w0, w1, [x9]
	mov x19, x0
	mov x20, x1

	mov x16, #SYS_FORK
	svc #0x80
	b.cs exit_syscall_errno
	cbz x0, fork_child
	mov x21, x0

	mov x0, x20
	bl close_fd

	mov x22, #60000
wait_loop:
	mov x0, x21
	adrp x1, wait_status@PAGE
	add x1, x1, wait_status@PAGEOFF
	mov x2, #WNOHANG
	mov x3, #0
	mov x16, #SYS_WAIT4
	svc #0x80
	b.cs exit_syscall_errno
	cmp x0, x21
	b.eq wait_done
	cbnz x0, exit_wait_pid
	subs x22, x22, #1
	b.ne wait_loop
	b exit_wait_timeout

wait_done:
	adrp x9, wait_status@PAGE
	add x9, x9, wait_status@PAGEOFF
	ldr w10, [x9]
	cbnz w10, exit_wait_status

	mov x0, x19
	adrp x1, stdout_buffer@PAGE
	add x1, x1, stdout_buffer@PAGEOFF
	mov x2, #16
	mov x16, #SYS_READ
	svc #0x80
	b.cs exit_syscall_errno
	cmp x0, #16
	b.ne exit_short_read

	adrp x0, stdout_buffer@PAGE
	add x0, x0, stdout_buffer@PAGEOFF
	adrp x1, child_stdout@PAGE
	add x1, x1, child_stdout@PAGEOFF
	mov x2, #16
	bl memory_equals
	cbz x0, exit_output_mismatch

	mov x0, x19
	bl close_fd
	mov x0, #0
	b exit_code

fork_child:
	mov x0, x20
	mov x1, #1
	bl dup2_fd

	mov x0, x19
	bl close_fd
	mov x0, x20
	bl close_fd

	adrp x0, self_path@PAGE
	add x0, x0, self_path@PAGEOFF
	adrp x1, child_argv@PAGE
	add x1, x1, child_argv@PAGEOFF
	adrp x2, child_envp@PAGE
	add x2, x2, child_envp@PAGEOFF
	mov x16, #SYS_EXECVE
	svc #0x80
	b.cs exit_syscall_errno
	b exit_exec_returned

child_main:
	mov x0, #1
	adrp x1, child_stdout@PAGE
	add x1, x1, child_stdout@PAGEOFF
	mov x2, #16
	mov x16, #SYS_WRITE
	svc #0x80
	b.cs exit_syscall_errno
	cmp x0, #16
	b.ne exit_short_write

	mov x0, #0
	b exit_code

is_child_reexec:
	cmp x19, #2
	b.lt not_child
	mov x0, #1
	ret
not_child:
	mov x0, #0
	ret

close_fd:
	mov x16, #SYS_CLOSE
	svc #0x80
	ret

dup2_fd:
	mov x16, #SYS_DUP2
	svc #0x80
	b.cs exit_syscall_errno
	ret

memory_equals:
	cbz x2, memory_equal
memory_loop:
	ldrb w9, [x0], #1
	ldrb w10, [x1], #1
	cmp w9, w10
	b.ne memory_not_equal
	subs x2, x2, #1
	b.ne memory_loop
memory_equal:
	mov x0, #1
	ret
memory_not_equal:
	mov x0, #0
	ret

exit_syscall_errno:
	b exit_code
exit_wait_timeout:
	mov x0, #82
	b exit_code
exit_wait_pid:
	mov x0, #83
	b exit_code
exit_wait_status:
	mov x0, #84
	b exit_code
exit_short_read:
	mov x0, #85
	b exit_code
exit_output_mismatch:
	mov x0, #86
	b exit_code
exit_exec_returned:
	mov x0, #87
	b exit_code
exit_short_write:
	mov x0, #88
	b exit_code
exit_code:
	mov x16, #SYS_EXIT
	svc #0x80
	b exit_code

.section __DATA,__data
.p2align 3
stdout_pipe:
	.space 8
wait_status:
	.word 0
stdout_buffer:
	.space 16
child_argv:
	.quad self_path
	.quad child_arg
	.quad 0
child_envp:
	.quad child_cookie
	.quad 0

.section __TEXT,__cstring
self_path:
	.asciz "/proc/self/exe"
child_arg:
	.asciz "--panicwrap-child"
child_cookie:
	.asciz "PACKER_WRAP_COOKIE=loop-p4"
child_stdout:
	.asciz "panicwrap child\n"

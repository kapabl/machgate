; darwin_fork_exec_self.s - execve("/proc/self/exe") to test guest-path reexec (fork-safe tracing target).
; Directly execs self-via-/proc to reach 42 after reexec. (fork path limited by stub)

.section __TEXT,__text
.globl _main
.p2align 2
_main:
	; execve("/proc/self/exe", target_argv, NULL) -> will resolve to self, reexec loader, exit42
	adrp x0, target_path@PAGE
	add x0, x0, target_path@PAGEOFF
	adrp x1, target_argv@PAGE
	add x1, x1, target_argv@PAGEOFF
	mov x2, #0
	mov x16, #59         ; Darwin SYS_execve
	svc #0x80
	; exec failed: exit 99
	mov x0, #99
	mov x16, #1
	svc #0x80

.section __DATA,__data
.p2align 3
target_argv:
	.quad target_path
	.quad 0

.section __TEXT,__cstring
target_path:
	.asciz "/proc/self/exe"

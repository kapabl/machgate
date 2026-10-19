.section __TEXT,__text
.globl _mach_absolute_time
.p2align 2
_mach_absolute_time:
    mov x0, #1
    ret

.globl _printf
.p2align 2
_printf:
    ret

.globl _snprintf
.p2align 2
_snprintf:
    ret

.globl _fcntl
.p2align 2
_fcntl:
    ret

.globl _ioctl
.p2align 2
_ioctl:
    ret

.globl _pipe
.p2align 2
_pipe:
    ret

.globl _close
.p2align 2
_close:
    ret

.globl _vfprintf
.p2align 2
_vfprintf:
    ret

.globl _vsnprintf
.p2align 2
_vsnprintf:
    ret

.globl _fflush
.p2align 2
_fflush:
    ret

.globl _getrlimit
.p2align 2
_getrlimit:
    ret

.globl _setrlimit
.p2align 2
_setrlimit:
    ret

.globl _task_policy_set
.p2align 2
_task_policy_set:
    ret

.globl __Unwind_Backtrace
.p2align 2
__Unwind_Backtrace:
    ret

.globl __Unwind_GetIP
.p2align 2
__Unwind_GetIP:
    ret

.globl _pthread_key_create
.p2align 2
_pthread_key_create:
    ret

.globl dyld_stub_binder
.p2align 2
dyld_stub_binder:
    ret

.section __DATA,__data
.globl _stdout
.p2align 3
_stdout:
    .quad 0

.globl _mach_task_self_
.p2align 2
_mach_task_self_:
    .long 0

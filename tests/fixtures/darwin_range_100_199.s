; darwin_range_100_199.s - representative Darwin syscalls 100-199
.section __TEXT,__text
.globl _main
.p2align 2
_main:
    mov x0, #0
    mov x1, #0
    mov x16, #100      ; Darwin SYS_getpriority
    svc #0x80
    b.cs fail
    cmp x0, #20
    b.gt fail

    adrp x0, tvbuf@PAGE
    add x0, x0, tvbuf@PAGEOFF
    mov x1, #0
    adrp x2, abstime@PAGE
    add x2, x2, abstime@PAGEOFF
    mov x16, #116      ; Darwin SYS_gettimeofday
    svc #0x80
    b.cs fail
    adrp x9, tvbuf@PAGE
    add x9, x9, tvbuf@PAGEOFF
    ldr x10, [x9]
    cbz x10, fail
    adrp x9, abstime@PAGE
    add x9, x9, abstime@PAGEOFF
    ldr x10, [x9]
    cbz x10, fail

    mov x0, #0
    mov x1, #0
    mov x16, #122      ; Darwin SYS_settimeofday no-op subset
    svc #0x80
    b.cs fail

    adrp x0, invalid_timeval@PAGE
    add x0, x0, invalid_timeval@PAGEOFF
    mov x1, #0
    mov x16, #122      ; Darwin SYS_settimeofday mutation denied
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x0, #0
    adrp x1, olddelta@PAGE
    add x1, x1, olddelta@PAGEOFF
    mov x16, #140      ; Darwin SYS_adjtime query subset
    svc #0x80
    b.cs fail

    adrp x0, invalid_timeval@PAGE
    add x0, x0, invalid_timeval@PAGEOFF
    mov x1, #0
    mov x16, #140      ; Darwin SYS_adjtime mutation denied
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x0, #1
    adrp x1, iovecs@PAGE
    add x1, x1, iovecs@PAGEOFF
    mov x2, #2
    mov x16, #121      ; Darwin SYS_writev
    svc #0x80
    b.cs fail
    cmp x0, #13
    b.ne fail

    mov x0, #1         ; Darwin AF_UNIX
    mov x1, #1         ; Darwin SOCK_STREAM
    mov x2, #0
    adrp x3, sockfds@PAGE
    add x3, x3, sockfds@PAGEOFF
    mov x16, #135      ; Darwin SYS_socketpair
    svc #0x80
    b.cs fail
    adrp x9, sockfds@PAGE
    add x9, x9, sockfds@PAGEOFF
    ldr w22, [x9]
    ldr w23, [x9, #4]

    mov x0, x22
    mov x1, #0xffff    ; Darwin SOL_SOCKET
    mov x2, #4         ; Darwin SO_REUSEADDR
    adrp x3, oneval@PAGE
    add x3, x3, oneval@PAGEOFF
    mov x4, #4
    mov x16, #105      ; Darwin SYS_setsockopt
    svc #0x80
    b.cs fail

    adrp x9, optlen@PAGE
    add x9, x9, optlen@PAGEOFF
    mov w10, #4
    str w10, [x9]
    mov x0, x22
    mov x1, #0xffff    ; Darwin SOL_SOCKET
    mov x2, #4         ; Darwin SO_REUSEADDR
    adrp x3, optval@PAGE
    add x3, x3, optval@PAGEOFF
    mov x4, x9
    mov x16, #118      ; Darwin SYS_getsockopt
    svc #0x80
    b.cs fail
    adrp x9, optval@PAGE
    add x9, x9, optval@PAGEOFF
    ldr w10, [x9]
    cbz w10, fail

    mov x0, x22
    adrp x1, sockmsg@PAGE
    add x1, x1, sockmsg@PAGEOFF
    mov x2, #4
    mov x3, #0
    mov x4, #0
    mov x5, #0
    mov x16, #133      ; Darwin SYS_sendto
    svc #0x80
    b.cs fail
    cmp x0, #4
    b.ne fail

    mov x0, x23
    adrp x1, sockbuf@PAGE
    add x1, x1, sockbuf@PAGEOFF
    mov x2, #4
    mov x16, #3        ; Darwin SYS_read
    svc #0x80
    b.cs fail
    cmp x0, #4
    b.ne fail
    adrp x9, sockbuf@PAGE
    add x9, x9, sockbuf@PAGEOFF
    ldrb w10, [x9]
    cmp w10, #'s'
    b.ne fail
    ldrb w10, [x9, #3]
    cmp w10, #'k'
    b.ne fail

    mov x0, x22
    adrp x1, classic_sendmsg@PAGE
    add x1, x1, classic_sendmsg@PAGEOFF
    mov x2, #0
    mov x16, #28       ; Darwin SYS_sendmsg
    svc #0x80
    b.cs fail
    cmp x0, #4
    b.ne fail

    mov x0, x23
    adrp x1, classic_recvfrombuf@PAGE
    add x1, x1, classic_recvfrombuf@PAGEOFF
    mov x2, #4
    mov x3, #0
    mov x4, #0
    mov x5, #0
    mov x16, #29       ; Darwin SYS_recvfrom
    svc #0x80
    b.cs fail
    cmp x0, #4
    b.ne fail
    adrp x9, classic_recvfrombuf@PAGE
    add x9, x9, classic_recvfrombuf@PAGEOFF
    ldrb w10, [x9]
    cmp w10, #'s'
    b.ne fail

    mov x0, x23
    adrp x1, sockreply@PAGE
    add x1, x1, sockreply@PAGEOFF
    mov x2, #4
    mov x16, #4        ; Darwin SYS_write
    svc #0x80
    b.cs fail
    cmp x0, #4
    b.ne fail

    mov x0, x22
    adrp x1, classic_recvmsg@PAGE
    add x1, x1, classic_recvmsg@PAGEOFF
    mov x2, #0
    mov x16, #27       ; Darwin SYS_recvmsg
    svc #0x80
    b.cs fail
    cmp x0, #4
    b.ne fail
    adrp x9, classic_recvmsgbuf@PAGE
    add x9, x9, classic_recvmsgbuf@PAGEOFF
    ldrb w10, [x9]
    cmp w10, #'p'
    b.ne fail

    mov x0, x22
    mov x1, #1         ; Darwin SHUT_WR
    mov x16, #134      ; Darwin SYS_shutdown
    svc #0x80
    b.cs fail

    mov x0, x22
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail
    mov x0, x23
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail

    adrp x0, sockpath@PAGE
    add x0, x0, sockpath@PAGEOFF
    mov x16, #10       ; Darwin SYS_unlink
    svc #0x80

    mov x0, #1         ; Darwin AF_UNIX
    mov x1, #1         ; Darwin SOCK_STREAM
    mov x2, #0
    mov x16, #97       ; Darwin SYS_socket
    svc #0x80
    b.cs fail
    mov x24, x0

    mov x0, x24
    adrp x1, sock_addr@PAGE
    add x1, x1, sock_addr@PAGEOFF
    mov x2, #21
    mov x16, #104      ; Darwin SYS_bind
    svc #0x80
    b.cs fail

    mov x0, x24
    mov x1, #4
    mov x16, #106      ; Darwin SYS_listen
    svc #0x80
    b.cs fail

    mov x0, #1         ; Darwin AF_UNIX
    mov x1, #1         ; Darwin SOCK_STREAM
    mov x2, #0
    mov x16, #97       ; Darwin SYS_socket
    svc #0x80
    b.cs fail
    mov x25, x0

    mov x0, x25
    adrp x1, sock_addr@PAGE
    add x1, x1, sock_addr@PAGEOFF
    mov x2, #21
    mov x16, #98       ; Darwin SYS_connect
    svc #0x80
    b.cs fail

    mov x0, x24
    adrp x1, accepted_addr@PAGE
    add x1, x1, accepted_addr@PAGEOFF
    adrp x2, accepted_len@PAGE
    add x2, x2, accepted_len@PAGEOFF
    mov x16, #30       ; Darwin SYS_accept
    svc #0x80
    b.cs fail
    mov x26, x0

    mov x0, x25
    adrp x1, peer_addr@PAGE
    add x1, x1, peer_addr@PAGEOFF
    adrp x2, peer_len@PAGE
    add x2, x2, peer_len@PAGEOFF
    mov x16, #31       ; Darwin SYS_getpeername
    svc #0x80
    b.cs fail
    adrp x9, peer_addr@PAGE
    add x9, x9, peer_addr@PAGEOFF
    ldrb w10, [x9, #1]
    cmp w10, #1
    b.ne fail

    mov x0, x24
    adrp x1, name_addr@PAGE
    add x1, x1, name_addr@PAGEOFF
    adrp x2, name_len@PAGE
    add x2, x2, name_len@PAGEOFF
    mov x16, #32       ; Darwin SYS_getsockname
    svc #0x80
    b.cs fail
    adrp x9, name_addr@PAGE
    add x9, x9, name_addr@PAGEOFF
    ldrb w10, [x9, #1]
    cmp w10, #1
    b.ne fail

    mov x0, x26
    mov x16, #6
    svc #0x80
    b.cs fail
    mov x0, x25
    mov x16, #6
    svc #0x80
    b.cs fail
    mov x0, x24
    mov x16, #6
    svc #0x80
    b.cs fail
    adrp x0, sockpath@PAGE
    add x0, x0, sockpath@PAGEOFF
    mov x16, #10
    svc #0x80
    b.cs fail

    adrp x0, tmpdir@PAGE
    add x0, x0, tmpdir@PAGEOFF
    mov x1, #0700
    mov x16, #136      ; Darwin SYS_mkdir
    svc #0x80
    b.cs fail

    adrp x0, tmpdir@PAGE
    add x0, x0, tmpdir@PAGEOFF
    mov x16, #137      ; Darwin SYS_rmdir
    svc #0x80
    b.cs fail

    adrp x0, tmpfile@PAGE
    add x0, x0, tmpfile@PAGEOFF
    mov x1, #0x602     ; Darwin O_RDWR | O_CREAT | O_TRUNC
    mov x2, #0600
    mov x16, #5        ; Darwin SYS_open
    svc #0x80
    b.cs fail
    mov x20, x0

    mov x0, x20
    mov x1, #0
    mov x16, #139      ; Darwin SYS_futimes
    svc #0x80
    b.cs fail

    adrp x0, hostuuid@PAGE
    add x0, x0, hostuuid@PAGEOFF
    mov x1, #0
    mov x16, #142      ; Darwin SYS_gethostuuid
    svc #0x80
    b.cs fail
    adrp x9, hostuuid@PAGE
    add x9, x9, hostuuid@PAGEOFF
    ldr x10, [x9]
    cbz x10, fail

    adrp x0, tmpfile@PAGE
    add x0, x0, tmpfile@PAGEOFF
    mov x1, #0
    mov x16, #138      ; Darwin SYS_utimes
    svc #0x80
    b.cs fail

    adrp x0, tmpfile@PAGE
    add x0, x0, tmpfile@PAGEOFF
    adrp x1, statfsbuf@PAGE
    add x1, x1, statfsbuf@PAGEOFF
    mov x16, #157      ; Darwin SYS_statfs
    svc #0x80
    b.cs fail
    adrp x9, statfsbuf@PAGE
    add x9, x9, statfsbuf@PAGEOFF
    ldr w10, [x9]
    cbz w10, fail

    mov x0, x20
    adrp x1, fstatfsbuf@PAGE
    add x1, x1, fstatfsbuf@PAGEOFF
    mov x16, #158      ; Darwin SYS_fstatfs
    svc #0x80
    b.cs fail
    adrp x9, fstatfsbuf@PAGE
    add x9, x9, fstatfsbuf@PAGEOFF
    ldr w10, [x9]
    cbz w10, fail

    adrp x0, tmpfile@PAGE
    add x0, x0, tmpfile@PAGEOFF
    mov x1, #4         ; Darwin _PC_NAME_MAX
    mov x16, #191      ; Darwin SYS_pathconf
    svc #0x80
    b.cs fail
    cbz x0, fail

    mov x0, x20
    mov x1, #5         ; Darwin _PC_PATH_MAX
    mov x16, #192      ; Darwin SYS_fpathconf
    svc #0x80
    b.cs fail
    cbz x0, fail

    mov x0, #3
    adrp x1, selfcounts@PAGE
    add x1, x1, selfcounts@PAGEOFF
    mov x2, #32
    mov x16, #186      ; Darwin SYS_thread_selfcounts
    svc #0x80
    b.cs fail

    mov x0, #99
    adrp x1, selfcounts@PAGE
    add x1, x1, selfcounts@PAGEOFF
    mov x2, #32
    mov x16, #186      ; Darwin SYS_thread_selfcounts
    svc #0x80
    b.cc fail
    cmp x0, #45
    b.ne fail

    mov x0, x20
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail

    adrp x0, dotpath@PAGE
    add x0, x0, dotpath@PAGEOFF
    mov x1, #0x100000  ; Darwin O_DIRECTORY
    mov x2, #0
    mov x16, #5        ; Darwin SYS_open
    svc #0x80
    b.cs fail
    mov x21, x0

    mov x0, x21
    adrp x1, dirbuf@PAGE
    add x1, x1, dirbuf@PAGEOFF
    mov x2, #4096
    adrp x3, dirbase@PAGE
    add x3, x3, dirbase@PAGEOFF
    mov x16, #196      ; Darwin SYS_getdirentries
    svc #0x80
    b.cs fail
    cbz x0, fail

    mov x0, x21
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail

    adrp x0, path@PAGE
    add x0, x0, path@PAGEOFF
    mov x1, #0
    mov x2, #0
    mov x16, #5        ; Darwin SYS_open
    svc #0x80
    b.cs fail
    mov x19, x0

    mov x0, x19
    adrp x1, readbuf@PAGE
    add x1, x1, readbuf@PAGEOFF
    mov x2, #7
    mov x3, #6
    mov x16, #153      ; Darwin SYS_pread
    svc #0x80
    b.cs fail
    cmp x0, #7
    b.ne fail
    adrp x9, readbuf@PAGE
    add x9, x9, readbuf@PAGEOFF
    ldrb w10, [x9]
    cmp w10, #'s'
    b.ne fail
    ldrb w10, [x9, #6]
    cmp w10, #'k'
    b.ne fail

    adrp x0, path@PAGE
    add x0, x0, path@PAGEOFF
    adrp x1, statbuf@PAGE
    add x1, x1, statbuf@PAGEOFF
    mov x16, #188      ; Darwin SYS_stat
    svc #0x80
    b.cs fail
    adrp x9, statbuf@PAGE
    add x9, x9, statbuf@PAGEOFF
    ldr x10, [x9, #96]
    cmp x10, #14
    b.ne fail

    mov x0, #8
    adrp x1, rlimitbuf@PAGE
    add x1, x1, rlimitbuf@PAGEOFF
    mov x16, #194      ; Darwin SYS_getrlimit RLIMIT_NOFILE
    svc #0x80
    b.cs fail
    adrp x9, rlimitbuf@PAGE
    add x9, x9, rlimitbuf@PAGEOFF
    ldr x10, [x9]
    cbz x10, fail

    mov x0, #1
    mov x16, #152      ; Darwin SYS_setprivexec, no child exec in MachGate
    svc #0x80
    b.cs fail

    mov x0, #0
    mov x1, #0
    mov x2, #0
    mov x3, #0
    mov x4, #0
    mov x16, #180      ; Darwin SYS_kdebug_trace, no-op
    svc #0x80
    b.cs fail

    mov x0, #0         ; Darwin P_ALL
    mov x1, #0
    adrp x2, waitid_info@PAGE
    add x2, x2, waitid_info@PAGEOFF
    mov x3, #1         ; Darwin WNOHANG
    mov x16, #173      ; Darwin SYS_waitid
    svc #0x80
    b.cc fail
    cmp x0, #10        ; Darwin ECHILD
    b.ne fail

    mov x0, #0         ; Darwin P_ALL
    mov x1, #0
    adrp x2, waitid_info@PAGE
    add x2, x2, waitid_info@PAGEOFF
    mov x3, #0
    mov x16, #173      ; Darwin SYS_waitid, single-process blocking subset
    svc #0x80
    b.cc fail
    cmp x0, #10        ; Darwin ECHILD
    b.ne fail

    adrp x0, sigusr1_set@PAGE
    add x0, x0, sigusr1_set@PAGEOFF
    mov x16, #111      ; Darwin SYS_sigsuspend
    svc #0x80
    b.cc fail
    cmp x0, #4
    b.ne fail

    mov x16, #184      ; Darwin SYS_sigreturn
    svc #0x80
    b.cc fail
    cmp x0, #22
    b.ne fail

    mov x0, #0
    mov x1, #0         ; Darwin CS_OPS_STATUS
    adrp x2, csops_status@PAGE
    add x2, x2, csops_status@PAGEOFF
    mov x3, #4
    mov x16, #169      ; Darwin SYS_csops
    svc #0x80
    b.cs fail
    adrp x9, csops_status@PAGE
    add x9, x9, csops_status@PAGEOFF
    ldr w10, [x9]
    cbnz w10, fail

    mov x0, #0
    mov x1, #0         ; Darwin CS_OPS_STATUS
    adrp x2, csops_audit_status@PAGE
    add x2, x2, csops_audit_status@PAGEOFF
    mov x3, #4
    adrp x4, audit_token@PAGE
    add x4, x4, audit_token@PAGEOFF
    mov x16, #170      ; Darwin SYS_csops_audittoken
    svc #0x80
    b.cs fail
    adrp x9, csops_audit_status@PAGE
    add x9, x9, csops_audit_status@PAGEOFF
    ldr w10, [x9]
    cbnz w10, fail

    mov x0, #0
    mov x1, #99
    adrp x2, csops_status@PAGE
    add x2, x2, csops_status@PAGEOFF
    mov x3, #4
    mov x16, #169      ; Darwin SYS_csops unsupported op
    svc #0x80
    b.cc fail
    cmp x0, #45
    b.ne fail

    adrp x0, panic_uuid@PAGE
    add x0, x0, panic_uuid@PAGEOFF
    mov x1, #0
    mov x2, #0
    mov x3, #0
    adrp x4, panic_msg@PAGE
    add x4, x4, panic_msg@PAGEOFF
    mov x16, #185      ; Darwin SYS_sys_panic_with_data
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x0, #0
    mov x1, #0
    mov x16, #155      ; Darwin SYS_nfssvc
    svc #0x80
    b.cc fail
    cmp x0, #45
    b.ne fail

    adrp x0, dotpath@PAGE
    add x0, x0, dotpath@PAGEOFF
    mov x1, #0
    mov x16, #159      ; Darwin SYS_unmount
	svc #0x80
	b.cc fail
	cmp x0, #1
	b.ne fail

    adrp x0, missing_path@PAGE
    add x0, x0, missing_path@PAGEOFF
    mov x1, #0
    mov x16, #159      ; Darwin SYS_unmount missing path
	svc #0x80
	b.cc fail
	cmp x0, #2
	b.ne fail

	adrp x0, tmpfile@PAGE
	add x0, x0, tmpfile@PAGEOFF
	adrp x1, filehandle_buf@PAGE
	add x1, x1, filehandle_buf@PAGEOFF
	mov x16, #161      ; Darwin SYS_getfh
    svc #0x80
    b.cc fail
    cmp x0, #45
    b.ne fail

    mov x0, #-1
    mov x1, #0
    mov x16, #164      ; Darwin SYS_funmount invalid fd
	svc #0x80
	b.cc fail
	cmp x0, #9
	b.ne fail

    mov x0, x19
    mov x1, #0
    mov x16, #164      ; Darwin SYS_funmount
	svc #0x80
	b.cc fail
	cmp x0, #1
	b.ne fail

    adrp x0, dotpath@PAGE
    add x0, x0, dotpath@PAGEOFF
    mov x1, #0
    mov x2, #0
    mov x3, #0
	mov x16, #165      ; Darwin SYS_quotactl
	svc #0x80
	b.cc fail
	cmp x0, #45
	b.ne fail

    mov x0, #0
    adrp x1, dotpath@PAGE
    add x1, x1, dotpath@PAGEOFF
    mov x2, #0
    mov x3, #0
    mov x16, #167      ; Darwin SYS_mount null type
	svc #0x80
	b.cc fail
	cmp x0, #14
	b.ne fail

    adrp x0, mount_type@PAGE
    add x0, x0, mount_type@PAGEOFF
    adrp x1, dotpath@PAGE
    add x1, x1, dotpath@PAGEOFF
    mov x2, #0
    mov x3, #0
    mov x16, #167      ; Darwin SYS_mount
	svc #0x80
	b.cc fail
	cmp x0, #1
	b.ne fail

    mov x16, #198      ; reserved nosys
    svc #0x80
    b.cc fail
    cmp x0, #78
    b.ne fail

    mov x0, x19
    mov x16, #6        ; Darwin SYS_close
    svc #0x80
    b.cs fail

    mov x0, #0
    mov x16, #1
    svc #0x80

fail:
    mov x0, #1
    mov x16, #1
	svc #0x80

.section __TEXT,__cstring
msg_a:
    .asciz "range"
msg_b:
    .asciz "100_199\n"
sockmsg:
    .asciz "sock"
sockreply:
    .asciz "pong"
path:
    .asciz "syscall_data.txt"
tmpdir:
    .asciz "range_100_199.tmp"
tmpfile:
	.asciz "range_100_199.file"
missing_path:
	.asciz "range_100_199.missing"
mount_type:
	.asciz "apfs"
dotpath:
	.asciz "."
sockpath:
    .asciz "range_100_199.sock"
panic_msg:
    .asciz "machgate panic data unsupported"
sock_addr:
    .byte 21
    .byte 1
    .asciz "range_100_199.sock"

.section __DATA,__data
.p2align 3
iovecs:
    .quad msg_a
    .quad 5
    .quad msg_b
    .quad 8
tvbuf:
    .space 16
abstime:
    .quad 0
olddelta:
    .space 16
invalid_timeval:
    .quad 0
    .long 1000000
    .long 0
readbuf:
    .space 8
statbuf:
    .space 144
rlimitbuf:
    .space 16
sigusr1_set:
    .long 0x20000000
statfsbuf:
    .space 2168
fstatfsbuf:
    .space 2168
selfcounts:
    .space 40
hostuuid:
    .space 16
panic_uuid:
    .space 16
csops_status:
    .long 1
csops_audit_status:
    .long 1
audit_token:
    .space 32
filehandle_buf:
    .space 64
dirbuf:
    .space 4096
dirbase:
    .quad 0
waitid_info:
    .space 128
sockfds:
    .space 8
oneval:
    .long 1
optval:
    .long 0
optlen:
    .long 4
sockbuf:
    .space 8
classic_sendmsg:
    .quad 0
    .long 0
    .long 0
    .quad classic_send_iov
    .long 1
    .long 0
    .quad 0
    .long 0
    .long 0
classic_recvmsg:
    .quad 0
    .long 0
    .long 0
    .quad classic_recv_iov
    .long 1
    .long 0
    .quad 0
    .long 0
    .long 0
classic_send_iov:
    .quad sockmsg
    .quad 4
classic_recv_iov:
    .quad classic_recvmsgbuf
    .quad 4
classic_recvfrombuf:
    .space 8
classic_recvmsgbuf:
    .space 8
accepted_addr:
    .space 128
peer_addr:
    .space 128
name_addr:
    .space 128
accepted_len:
    .long 128
peer_len:
    .long 128
name_len:
    .long 128

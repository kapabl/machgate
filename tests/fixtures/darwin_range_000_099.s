; darwin_range_000_099.s - representative Darwin syscalls from range 0-99

.section __TEXT,__text
.globl _main
_main:
    mov x21, #70

    mov x21, #20
    mov x16, #20
    svc #0x80
    b.cs fail
    cbz x0, fail
    mov x22, x0

    mov x21, #37
    mov x0, x22
    mov x1, #0
    mov x16, #37
    svc #0x80
    b.cs fail

    mov x21, #46
    mov x0, #30
    adrp x1, sigignore_action@PAGE
    add x1, x1, sigignore_action@PAGEOFF
    adrp x2, sigold_action@PAGE
    add x2, x2, sigold_action@PAGEOFF
    mov x16, #46
    svc #0x80
    b.cs fail

    mov x21, #46
    mov x0, #30
    mov x1, #0
    adrp x2, sigquery_action@PAGE
    add x2, x2, sigquery_action@PAGEOFF
    mov x16, #46
    svc #0x80
    b.cs fail
    adrp x9, sigquery_action@PAGE
    add x9, x9, sigquery_action@PAGEOFF
    ldr x10, [x9]
    cmp x10, #1
    b.ne fail

    mov x21, #46
    mov x0, #30
    adrp x1, sigdefault_action@PAGE
    add x1, x1, sigdefault_action@PAGEOFF
    adrp x2, sigold_default_action@PAGE
    add x2, x2, sigold_default_action@PAGEOFF
    mov x16, #46
    svc #0x80
    b.cs fail
    adrp x9, sigold_default_action@PAGE
    add x9, x9, sigold_default_action@PAGEOFF
    ldr x10, [x9]
    cmp x10, #1
    b.ne fail

    mov x21, #46
    mov x0, #31
    adrp x1, sigignore_flags_action@PAGE
    add x1, x1, sigignore_flags_action@PAGEOFF
    adrp x2, sigold_flags_action@PAGE
    add x2, x2, sigold_flags_action@PAGEOFF
    mov x16, #46
    svc #0x80
    b.cs fail

    mov x21, #46
    mov x0, #31
    mov x1, #0
    adrp x2, sigquery_flags_action@PAGE
    add x2, x2, sigquery_flags_action@PAGEOFF
    mov x16, #46
    svc #0x80
    b.cs fail
    adrp x9, sigquery_flags_action@PAGE
    add x9, x9, sigquery_flags_action@PAGEOFF
    ldr x10, [x9]
    cmp x10, #1
    b.ne fail
    ldr w10, [x9, #8]
    mov w11, #1
    lsl w11, w11, #29
    cmp w10, w11
    b.ne fail
    ldr w10, [x9, #12]
    cmp w10, #0x7f
    b.ne fail

    mov x21, #46
    mov x0, #31
    adrp x1, sigdefault_action@PAGE
    add x1, x1, sigdefault_action@PAGEOFF
    mov x2, #0
    mov x16, #46
    svc #0x80
    b.cs fail

    mov x21, #46
    mov x0, #31
    adrp x1, sigguest_handler_action@PAGE
    add x1, x1, sigguest_handler_action@PAGEOFF
    mov x2, #0
    mov x16, #46
    svc #0x80
    b.cc fail
    cmp x0, #78
    b.ne fail

    mov x21, #46
    mov x0, #31
    mov x1, #0
    adrp x2, sigquery_after_guest_action@PAGE
    add x2, x2, sigquery_after_guest_action@PAGEOFF
    mov x16, #46
    svc #0x80
    b.cs fail
    adrp x9, sigquery_after_guest_action@PAGE
    add x9, x9, sigquery_after_guest_action@PAGEOFF
    ldr x10, [x9]
    cbnz x10, fail

    mov x21, #48
    mov x0, #1
    adrp x1, sigusr1_set@PAGE
    add x1, x1, sigusr1_set@PAGEOFF
    adrp x2, sigold_set@PAGE
    add x2, x2, sigold_set@PAGEOFF
    mov x16, #48
    svc #0x80
    b.cs fail

    mov x21, #37
    mov x0, x22
    mov x1, #30
    mov x16, #37
    svc #0x80
    b.cs fail

    mov x21, #52
    adrp x0, sigpending_set@PAGE
    add x0, x0, sigpending_set@PAGEOFF
    mov x16, #52
    svc #0x80
    b.cs fail
    adrp x9, sigpending_set@PAGE
    add x9, x9, sigpending_set@PAGEOFF
    ldr w10, [x9]
    tbz w10, #29, fail

    mov x21, #46
    mov x0, #30
    adrp x1, sigignore_action@PAGE
    add x1, x1, sigignore_action@PAGEOFF
    mov x2, #0
    mov x16, #46
    svc #0x80
    b.cs fail

    mov x21, #48
    mov x0, #2
    adrp x1, sigusr1_set@PAGE
    add x1, x1, sigusr1_set@PAGEOFF
    mov x2, #0
    mov x16, #48
    svc #0x80
    b.cs fail

    mov x21, #53
    mov x0, #0
    adrp x1, sigalt_old@PAGE
    add x1, x1, sigalt_old@PAGEOFF
    mov x16, #53
    svc #0x80
    b.cs fail

    mov x21, #24
    mov x16, #24
    svc #0x80
    b.cs fail

    mov x21, #25
    mov x16, #25
    svc #0x80
    b.cs fail

    mov x21, #26
    mov x0, #0
    mov x1, #0
    mov x2, #0
    mov x3, #0
    mov x16, #26
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x21, #38
    mov x0, #0
    mov x16, #38
    svc #0x80
    b.cc fail
    cmp x0, #22
    b.ne fail

    mov x21, #47
    mov x16, #47
    svc #0x80
    b.cs fail

    mov x21, #43
    mov x16, #43
    svc #0x80
    b.cs fail

    mov x21, #42
    mov x16, #42
    svc #0x80
    b.cs fail
    mov x23, x0
    mov x24, x1

    mov x21, #4
    mov x0, x24
    adrp x1, pipe_byte@PAGE
    add x1, x1, pipe_byte@PAGEOFF
    mov x2, #1
    mov x16, #4
    svc #0x80
    b.cs fail
    cmp x0, #1
    b.ne fail

    mov x21, #54
    mov x0, x23
    movz x1, #0x667f
    movk x1, #0x4004, lsl #16
    adrp x2, ioctl_count@PAGE
    add x2, x2, ioctl_count@PAGEOFF
    mov x16, #54
    svc #0x80
    b.cs fail
    adrp x9, ioctl_count@PAGE
    add x9, x9, ioctl_count@PAGEOFF
    ldr w10, [x9]
    cmp w10, #1
    b.ne fail

    mov x21, #54
    mov x0, x23
    movz x1, #0x6601
    movk x1, #0x2000, lsl #16
    mov x2, #0
    mov x16, #54
    svc #0x80
    b.cs fail

    mov x21, #54
    mov x0, x23
    movz x1, #0x6602
    movk x1, #0x2000, lsl #16
    mov x2, #0
    mov x16, #54
    svc #0x80
    b.cs fail

    mov w10, #1
    lsl w10, w10, w23
    adrp x9, select_readfds@PAGE
    add x9, x9, select_readfds@PAGEOFF
    str w10, [x9]
    mov x21, #93
    add x0, x23, #1
    mov x1, x9
    mov x2, #0
    mov x3, #0
    adrp x4, select_timeout@PAGE
    add x4, x4, select_timeout@PAGEOFF
    mov x16, #93
    svc #0x80
    b.cs fail
    cmp x0, #1
    b.ne fail

    mov x21, #3
    mov x0, x23
    adrp x1, pipe_readbuf@PAGE
    add x1, x1, pipe_readbuf@PAGEOFF
    mov x2, #1
    mov x16, #3
    svc #0x80
    b.cs fail
    cmp x0, #1
    b.ne fail

    mov x21, #6
    mov x0, x23
    mov x16, #6
    svc #0x80
    b.cs fail

    mov x21, #6
    mov x0, x24
    mov x16, #6
    svc #0x80
    b.cs fail

    mov x21, #39
    mov x16, #39
    svc #0x80
    b.cs fail
    cbz x0, fail

    mov x21, #7
    mov x0, #-1
    adrp x1, wait_status@PAGE
    add x1, x1, wait_status@PAGEOFF
    mov x2, #1
    mov x3, #0
    mov x16, #7
    svc #0x80
    b.cc fail
    cmp x0, #10
    b.ne fail

    mov x21, #7
    mov x0, #-1
    adrp x1, wait_status@PAGE
    add x1, x1, wait_status@PAGEOFF
    mov x2, #0
    mov x3, #0
    mov x16, #7
    svc #0x80
    b.cc fail
    cmp x0, #10
    b.ne fail

    mov x21, #50
    adrp x0, login_name@PAGE
    add x0, x0, login_name@PAGEOFF
    mov x16, #50
    svc #0x80
    b.cs fail

    mov x21, #49
    adrp x0, login_buffer@PAGE
    add x0, x0, login_buffer@PAGEOFF
    mov x1, #32
    mov x16, #49
    svc #0x80
    b.cs fail
    adrp x9, login_buffer@PAGE
    add x9, x9, login_buffer@PAGEOFF
    ldrb w10, [x9]
    cmp w10, #'m'
    b.ne fail

    mov x21, #51
    mov x0, #0
    mov x16, #51
    svc #0x80
    b.cs fail

    mov x21, #2
    mov x16, #2
    svc #0x80
    b.cc fail
    cmp x0, #35
    b.ne fail

    mov x21, #59
    adrp x0, access_path@PAGE
    add x0, x0, access_path@PAGEOFF
    mov x1, #0
    mov x2, #0
    mov x16, #59
    svc #0x80
    b.cc fail
    cmp x0, #45
    b.ne fail

    mov x21, #61
    adrp x0, access_path@PAGE
    add x0, x0, access_path@PAGEOFF
    mov x16, #61
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x21, #66
    mov x16, #66
    svc #0x80
    b.cc fail
    cmp x0, #35
    b.ne fail

    mov x21, #67
    mov x16, #67
    svc #0x80
    b.cs fail

    mov x21, #68
    mov x16, #68
    svc #0x80
    b.cs fail

    mov x21, #55
    mov x0, #0
    mov x1, #0
    mov x16, #55
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x21, #81
    mov x16, #81
    svc #0x80
    b.cs fail

    mov x21, #89
    mov x16, #89
    svc #0x80
    b.cs fail
    cbz x0, fail

    mov x21, #79
    mov x0, #0
    mov x1, #0
    mov x16, #79
    svc #0x80
    b.cs fail

    mov x21, #18
    adrp x0, statfsbuf@PAGE
    add x0, x0, statfsbuf@PAGEOFF
    mov x1, #2168
    mov x16, #18
    svc #0x80
    b.cs fail
    cbz x0, fail

    mov x21, #33
    adrp x0, access_path@PAGE
    add x0, x0, access_path@PAGEOFF
    mov x1, #4
    mov x16, #33
    svc #0x80
    b.cs fail

    mov x21, #34
    adrp x0, access_path@PAGE
    add x0, x0, access_path@PAGEOFF
    mov x1, #0
    mov x16, #34
    svc #0x80
    b.cs fail

    mov x21, #57
    adrp x0, symlink_target@PAGE
    add x0, x0, symlink_target@PAGEOFF
    adrp x1, symlink_path@PAGE
    add x1, x1, symlink_path@PAGEOFF
    mov x16, #57
    svc #0x80
    b.cs fail

    mov x21, #58
    adrp x0, symlink_path@PAGE
    add x0, x0, symlink_path@PAGEOFF
    adrp x1, linkbuf@PAGE
    add x1, x1, linkbuf@PAGEOFF
    mov x2, #64
    mov x16, #58
    svc #0x80
    b.cs fail
    cmp x0, #16
    b.ne fail
    adrp x9, linkbuf@PAGE
    add x9, x9, linkbuf@PAGEOFF
    ldrb w10, [x9]
    cmp w10, #'s'
    b.ne fail

    mov x21, #10
    adrp x0, symlink_path@PAGE
    add x0, x0, symlink_path@PAGEOFF
    mov x16, #10
    svc #0x80
    b.cs fail

    mov x21, #83
    mov x0, #0
    adrp x1, timer_zero@PAGE
    add x1, x1, timer_zero@PAGEOFF
    adrp x2, timer_old@PAGE
    add x2, x2, timer_old@PAGEOFF
    mov x16, #83
    svc #0x80
    b.cs fail

    mov x21, #86
    mov x0, #0
    adrp x1, timer_old@PAGE
    add x1, x1, timer_old@PAGEOFF
    mov x16, #86
    svc #0x80
    b.cs fail

    mov x21, #75
    adrp x0, pagebuf@PAGE
    add x0, x0, pagebuf@PAGEOFF
    mov x1, #4096
    mov x2, #0
    mov x16, #75
    svc #0x80
    b.cs fail

    mov x21, #78
    adrp x0, pagebuf@PAGE
    add x0, x0, pagebuf@PAGEOFF
    mov x1, #4096
    adrp x2, mincore_vec@PAGE
    add x2, x2, mincore_vec@PAGEOFF
    mov x16, #78
    svc #0x80
    b.cs fail

    mov x21, #41
    mov x0, #1
    mov x16, #41
    svc #0x80
    b.cs fail
    mov x19, x0

    mov x21, #90
    mov x0, x19
    mov x1, x19
    mov x16, #90
    svc #0x80
    b.cs fail
    cmp x0, x19
    b.ne fail

    mov x21, #35
    mov x0, x19
    mov x1, #0
    mov x16, #35
    svc #0x80
    b.cs fail

    mov x21, #92
    mov x0, x19
    mov x1, #1
    mov x16, #92
    svc #0x80
    b.cs fail

    mov x21, #4
    mov x0, x19
    adrp x1, message@PAGE
    add x1, x1, message@PAGEOFF
    mov x2, #12
    mov x16, #4
    svc #0x80
    b.cs fail
    cmp x0, #12
    b.ne fail

    mov x21, #6
    mov x0, x19
    mov x16, #6
    svc #0x80
    b.cs fail

    mov x21, #99
    mov x16, #99
    svc #0x80
    b.cc fail
    cmp x0, #78
    b.ne fail

    mov x21, #56
    adrp x0, access_path@PAGE
    add x0, x0, access_path@PAGEOFF
    mov x16, #56
    svc #0x80
    b.cc fail
    cmp x0, #45
    b.ne fail

    mov x21, #85
    adrp x0, access_path@PAGE
    add x0, x0, access_path@PAGEOFF
    mov x16, #85
    svc #0x80
    b.cc fail
    cmp x0, #1
    b.ne fail

    mov x0, #0
    mov x16, #1
    svc #0x80

fail:
    mov x0, x21
    mov x16, #1
    svc #0x80

.section __TEXT,__cstring
message:
    .asciz "range000099\n"
access_path:
    .asciz "syscall_data.txt"
symlink_target:
    .asciz "syscall_data.txt"
symlink_path:
    .asciz "range000099.link"
login_name:
    .asciz "machgate"
pipe_byte:
    .asciz "x"

.section __DATA,__data
.p2align 12
pagebuf:
    .space 4096
timer_zero:
    .space 32
timer_old:
    .space 32
linkbuf:
    .space 64
mincore_vec:
    .space 1
statfsbuf:
    .space 2168
wait_status:
    .long 0
login_buffer:
    .space 32
ioctl_count:
    .long 0
select_readfds:
    .long 0
select_timeout:
    .quad 0
    .long 0
    .long 0
pipe_readbuf:
    .space 1
.p2align 3
sigignore_action:
    .quad 1
    .long 0
    .long 0
sigdefault_action:
    .quad 0
    .long 0
    .long 0
sigignore_flags_action:
    .quad 1
    .long 0x20000000
    .long 0x7f
sigguest_handler_action:
    .quad 2
    .long 0
    .long 0
sigold_action:
    .space 16
sigquery_action:
    .space 16
sigold_default_action:
    .space 16
sigold_flags_action:
    .space 16
sigquery_flags_action:
    .space 16
sigquery_after_guest_action:
    .space 16
sigusr1_set:
    .long 0x20000000
sigold_set:
    .long 0
sigpending_set:
    .long 0
.p2align 3
sigalt_old:
    .space 24

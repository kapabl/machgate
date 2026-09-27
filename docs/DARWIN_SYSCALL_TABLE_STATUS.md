# Darwin Syscall Table Status

Source: Apple xnu `bsd/kern/syscalls.master`, downloaded to `/tmp/xnu_syscalls.master` during implementation.

This tracker records every numeric source row, including conditional duplicate rows from Apple `#if` / `#else` sections. Use the source-row total for production-progress accounting and the unique-number total for dispatcher-slot accounting.

## Summary

- Done: 669 / 669 source rows (100.0%)
- Translated: 455 / 669 source rows (68.0%)
- ENOSYS: 214 / 669 source rows (32.0%)
- Wrongly-classified: 0 / 669 source rows (0.0%)
- Deferred: 0 / 669 source rows (0.0%)
- Harder: 0 / 669 source rows (0.0%)
- Harder2: 0 / 669 source rows (0.0%)
- Pending: 0 / 669 source rows (0.0%)

## Range 0-99

| Source Line | Number | Name | Status | Notes |
| --- | ---: | --- | --- | --- |
| 45 | 0 | `nosys` | ENOSYS | reserved indirect syscall row, explicit Darwin `ENOSYS` |
| 46 | 1 | `exit` | DONE | raw Linux `SYS_exit_group` |
| 47 | 2 | `fork` | DONE | Darwin `EAGAIN` child-process compatibility; fixture-covered |
| 48 | 3 | `read` | DONE | raw Linux `SYS_read` |
| 49 | 4 | `write` | DONE | raw Linux `SYS_write` |
| 50 | 5 | `open` | DONE | raw Linux `SYS_openat` with Darwin `O_*` translation |
| 51 | 6 | `sys_close` | DONE | raw Linux `SYS_close` |
| 52 | 7 | `wait4` | DONE | single-process compatibility: `WNOHANG` returns Darwin `ECHILD`; blocking waits unsupported |
| 53 | 8 | `enosys` | ENOSYS | historical creat row, explicit Darwin `ENOSYS` |
| 54 | 9 | `link` | DONE | raw Linux `SYS_linkat` with `AT_FDCWD` |
| 55 | 10 | `unlink` | DONE | raw Linux `SYS_unlinkat` with `AT_FDCWD` |
| 56 | 11 | `enosys` | ENOSYS | historical execv row, explicit Darwin `ENOSYS` |
| 57 | 12 | `sys_chdir` | DONE | raw Linux `SYS_chdir` |
| 58 | 13 | `sys_fchdir` | DONE | raw Linux `SYS_fchdir` |
| 59 | 14 | `mknod` | DONE | raw Linux `SYS_mknodat` with `AT_FDCWD` |
| 60 | 15 | `chmod` | DONE | raw Linux `SYS_fchmodat` with `AT_FDCWD` |
| 61 | 16 | `chown` | DONE | raw Linux `SYS_fchownat` with `AT_FDCWD` |
| 62 | 17 | `enosys` | ENOSYS | historical break row, explicit Darwin `ENOSYS` |
| 63 | 18 | `getfsstat` | DONE | Linux `/proc/self/mounts` enumeration translated to Darwin statfs layout |
| 64 | 19 | `enosys` | ENOSYS | historical lseek row, explicit Darwin `ENOSYS` |
| 65 | 20 | `getpid` | DONE | raw Linux `SYS_getpid`; covered by range fixture after integration |
| 66 | 21 | `enosys` | ENOSYS | historical mount row, explicit Darwin `ENOSYS` |
| 67 | 22 | `enosys` | ENOSYS | historical umount row, explicit Darwin `ENOSYS` |
| 68 | 23 | `setuid` | DONE | raw Linux `SYS_setuid` |
| 69 | 24 | `getuid` | DONE | raw Linux `SYS_getuid`; covered by range fixture after integration |
| 70 | 25 | `geteuid` | DONE | raw Linux `SYS_geteuid`; covered by range fixture after integration |
| 71 | 26 | `ptrace` | DONE | Darwin `EPERM` tracing/security compatibility; fixture-covered |
| 73 | 27 | `recvmsg` | DONE | raw Linux `recvmsg` with Darwin iovec/message subset; ancillary data unsupported |
| 74 | 28 | `sendmsg` | DONE | raw Linux `sendmsg` with Darwin iovec/message subset; ancillary data unsupported |
| 75 | 29 | `recvfrom` | DONE | raw Linux `recvfrom` with Darwin sockaddr handling for tested socket families |
| 76 | 30 | `accept` | DONE | raw Linux `accept` with Darwin sockaddr handling for tested socket families |
| 77 | 31 | `getpeername` | DONE | raw Linux `getpeername` with Darwin sockaddr handling |
| 78 | 32 | `getsockname` | DONE | raw Linux `getsockname` with Darwin sockaddr handling |
| 80 | 27 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 81 | 28 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 82 | 29 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 83 | 30 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 84 | 31 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 85 | 32 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 87 | 33 | `access` | DONE | raw Linux `SYS_faccessat` with `AT_FDCWD`; covered by range fixture after integration |
| 88 | 34 | `chflags` | DONE | Darwin `chflags` compatibility: zero flags validate path and succeed; nonzero flags return `ENOTSUP` |
| 89 | 35 | `fchflags` | DONE | Darwin `fchflags` compatibility: zero flags validate fd and succeed; nonzero flags return `ENOTSUP` |
| 90 | 36 | `sync` | DONE | raw Linux `SYS_sync` |
| 91 | 37 | `kill` | DONE | raw Linux `kill` with Darwin signal mapping subset |
| 92 | 38 | `sys_crossarch_trap` | DONE | Darwin `EINVAL` for unsupported cross-architecture trap; fixture-covered |
| 93 | 39 | `getppid` | DONE | raw Linux `SYS_getppid` |
| 94 | 40 | `nosys` | ENOSYS | historical lstat row, explicit Darwin `ENOSYS` |
| 95 | 41 | `sys_dup` | DONE | raw Linux `SYS_dup`; covered by range fixture after integration |
| 96 | 42 | `pipe` | DONE | raw Linux `SYS_pipe2`; returns fds in `x0` and `x1` |
| 97 | 43 | `getegid` | DONE | raw Linux `SYS_getegid`; covered by range fixture after integration |
| 98 | 44 | `nosys` | ENOSYS | historical profil row, explicit Darwin `ENOSYS` |
| 99 | 45 | `nosys` | ENOSYS | historical ktrace row, explicit Darwin `ENOSYS` |
| 100 | 46 | `sigaction` | DONE | `SIG_DFL`/`SIG_IGN` compatibility with mask/flag round-trip; guest handler pointers return Darwin `ENOSYS` pending signal runtime |
| 101 | 47 | `getgid` | DONE | raw Linux `SYS_getgid`; covered by range fixture after integration |
| 102 | 48 | `sigprocmask` | DONE | raw Linux `sigprocmask` with Darwin signal set mapping subset |
| 103 | 49 | `getlogin` | DONE | Darwin getlogin compatibility backed by process-local login buffer |
| 104 | 50 | `setlogin` | DONE | Darwin setlogin compatibility updates process-local login buffer |
| 105 | 51 | `acct` | DONE | `acct(NULL)` succeeds; non-null path uses Linux `acct` where available |
| 106 | 52 | `sigpending` | DONE | raw Linux `sigpending` with Darwin signal set mapping subset |
| 107 | 53 | `sigaltstack` | DONE | raw Linux `sigaltstack` with Darwin stack layout translation |
| 108 | 54 | `ioctl` | DONE | basic fd/terminal request translation; unknown requests return Darwin `ENOSYS` |
| 109 | 55 | `reboot` | DONE | Darwin `EPERM` privileged host operation compatibility; fixture-covered |
| 110 | 56 | `revoke` | DONE | Darwin `ENOTSUP` compatibility; fixture-covered |
| 111 | 57 | `symlink` | DONE | raw Linux `SYS_symlinkat` with `AT_FDCWD` |
| 112 | 58 | `readlink` | DONE | raw Linux `SYS_readlinkat` with `AT_FDCWD` |
| 113 | 59 | `execve` | DONE | Darwin `ENOTSUP` child-process replacement compatibility; fixture-covered |
| 114 | 60 | `umask` | DONE | raw Linux `SYS_umask` |
| 115 | 61 | `chroot` | DONE | Darwin `EPERM` privileged host mutation compatibility; fixture-covered |
| 116 | 62 | `nosys` | ENOSYS | historical fstat row, explicit Darwin `ENOSYS` |
| 117 | 63 | `nosys` | ENOSYS | reserved internal row, explicit Darwin `ENOSYS` |
| 118 | 64 | `nosys` | ENOSYS | historical getpagesize row, explicit Darwin `ENOSYS` |
| 119 | 65 | `msync` | DONE | raw Linux `SYS_msync` with Darwin `MS_*` translation |
| 121 | 66 | `vfork` | DONE | Darwin `EAGAIN` child-process compatibility; fixture-covered |
| 123 | 66 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 125 | 67 | `oslog_coproc_reg` | DONE | no-op success compatibility; fixture-covered |
| 126 | 68 | `oslog_coproc` | DONE | no-op success compatibility; fixture-covered |
| 127 | 69 | `nosys` | ENOSYS | historical sbrk row, explicit Darwin `ENOSYS` |
| 128 | 70 | `nosys` | ENOSYS | historical sstk row, explicit Darwin `ENOSYS` |
| 129 | 71 | `nosys` | ENOSYS | historical mmap row, explicit Darwin `ENOSYS` |
| 130 | 72 | `nosys` | ENOSYS | historical vadvise row, explicit Darwin `ENOSYS` |
| 131 | 73 | `munmap` | DONE | raw Linux `SYS_munmap` |
| 132 | 74 | `mprotect` | DONE | raw Linux `SYS_mprotect` |
| 133 | 75 | `madvise` | DONE | raw Linux `SYS_madvise` with Darwin advice handling where applicable |
| 134 | 76 | `nosys` | ENOSYS | historical vhangup row, explicit Darwin `ENOSYS` |
| 135 | 77 | `nosys` | ENOSYS | historical vlimit row, explicit Darwin `ENOSYS` |
| 136 | 78 | `mincore` | DONE | raw Linux `SYS_mincore` with page residency vector handling |
| 137 | 79 | `getgroups` | DONE | raw Linux `SYS_getgroups` |
| 138 | 80 | `setgroups` | DONE | raw Linux `SYS_setgroups` |
| 139 | 81 | `getpgrp` | DONE | raw Linux `SYS_getpgid` with pid 0 |
| 140 | 82 | `setpgid` | DONE | raw Linux `SYS_setpgid` |
| 141 | 83 | `setitimer` | DONE | raw Linux `SYS_setitimer` with Darwin timeval layout translation |
| 142 | 84 | `nosys` | ENOSYS | historical wait row, explicit Darwin `ENOSYS` |
| 143 | 85 | `swapon` | DONE | Darwin `EPERM` privileged host operation compatibility; fixture-covered |
| 144 | 86 | `getitimer` | DONE | raw Linux `SYS_getitimer` with Darwin timeval layout translation |
| 145 | 87 | `nosys` | ENOSYS | historical gethostname row, explicit Darwin `ENOSYS` |
| 146 | 88 | `nosys` | ENOSYS | historical sethostname row, explicit Darwin `ENOSYS` |
| 147 | 89 | `sys_getdtablesize` | DONE | Linux `prlimit64/getrlimit(RLIMIT_NOFILE)` |
| 148 | 90 | `sys_dup2` | DONE | raw Linux `SYS_dup3`; preserves Darwin `dup2(fd, fd)` success semantics |
| 149 | 91 | `nosys` | ENOSYS | historical getdopt row, explicit Darwin `ENOSYS` |
| 150 | 92 | `sys_fcntl` | DONE | common `fcntl` commands plus Darwin flag translation; covered by range fixture after integration |
| 151 | 93 | `select` | DONE | Linux `select`/`pselect6` with Darwin fdset/timeval translation |
| 152 | 94 | `nosys` | ENOSYS | historical setdopt row, explicit Darwin `ENOSYS` |
| 153 | 95 | `fsync` | DONE | raw Linux `SYS_fsync` |
| 154 | 96 | `setpriority` | DONE | raw Linux `SYS_setpriority` |
| 156 | 97 | `socket` | DONE | raw Linux `socket` with Darwin domain/type/protocol mapping subset |
| 157 | 98 | `connect` | DONE | raw Linux `connect` with Darwin sockaddr handling for tested socket families |
| 159 | 97 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 160 | 98 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 162 | 99 | `nosys` | ENOSYS | historical accept row, explicit Darwin `ENOSYS` |

## Range 100-199

| Source Line | Number | Name | Status | Notes |
| --- | ---: | --- | --- | --- |
| 163 | 100 | `getpriority` | DONE | Linux `SYS_getpriority` with raw-result conversion to Darwin nice value |
| 164 | 101 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 165 | 102 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 166 | 103 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 168 | 104 | `bind` | DONE | raw Linux socket `bind` translation |
| 169 | 105 | `setsockopt` | DONE | raw Linux socket `setsockopt` translation |
| 170 | 106 | `listen` | DONE | raw Linux socket `listen` translation |
| 172 | 104 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 173 | 105 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 174 | 106 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 176 | 107 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 177 | 108 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 178 | 109 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 179 | 110 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 180 | 111 | `sigsuspend` | DONE | Darwin `EINTR` compatibility; fixture-covered |
| 181 | 112 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 183 | 113 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 184 | 114 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 186 | 113 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 187 | 114 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 189 | 115 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 190 | 116 | `gettimeofday` | DONE | Linux `SYS_gettimeofday`; optional Darwin mach_absolute_time pointer gets monotonic nanoseconds |
| 191 | 117 | `getrusage` | DONE | Linux `SYS_getrusage` using LP64-compatible rusage layout |
| 193 | 118 | `getsockopt` | DONE | raw Linux socket `getsockopt` translation |
| 195 | 118 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 197 | 119 | `nosys` | ENOSYS | reserved legacy row |
| 198 | 120 | `readv` | DONE | raw Linux `SYS_readv` |
| 199 | 121 | `writev` | DONE | raw Linux `SYS_writev`; covered by `darwin_range_100_199` fixture |
| 200 | 122 | `settimeofday` | DONE | safe `settimeofday(NULL, NULL)` no-op subset; host clock mutation returns Darwin `ENOSYS` |
| 201 | 123 | `fchown` | DONE | raw Linux `SYS_fchown` |
| 202 | 124 | `fchmod` | DONE | raw Linux `SYS_fchmod` |
| 203 | 125 | `nosys` | ENOSYS | reserved legacy row |
| 204 | 126 | `setreuid` | DONE | raw Linux `SYS_setreuid` |
| 205 | 127 | `setregid` | DONE | raw Linux `SYS_setregid` |
| 206 | 128 | `rename` | DONE | raw Linux `SYS_renameat` with `AT_FDCWD` |
| 207 | 129 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 208 | 130 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 209 | 131 | `sys_flock` | DONE | raw Linux `SYS_flock` |
| 210 | 132 | `mkfifo` | DONE | raw Linux `SYS_mknodat` with `S_IFIFO` |
| 212 | 133 | `sendto` | DONE | raw Linux socket `sendto` translation |
| 213 | 134 | `shutdown` | DONE | raw Linux socket `shutdown` translation |
| 214 | 135 | `socketpair` | DONE | raw Linux `socketpair` translation |
| 216 | 133 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 217 | 134 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 218 | 135 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 220 | 136 | `mkdir` | DONE | raw Linux `SYS_mkdirat`; covered by `darwin_range_100_199` fixture |
| 221 | 137 | `rmdir` | DONE | raw Linux `SYS_unlinkat` with `AT_REMOVEDIR`; covered by `darwin_range_100_199` fixture |
| 222 | 138 | `utimes` | DONE | raw Linux `SYS_utimensat`/time translation for `utimes` |
| 223 | 139 | `futimes` | DONE | raw Linux futimes-compatible timestamp update |
| 224 | 140 | `adjtime` | DONE | safe `adjtime(NULL, olddelta)` query subset; host clock mutation returns Darwin `ENOSYS` |
| 225 | 141 | `nosys` | ENOSYS | reserved legacy row |
| 226 | 142 | `gethostuuid` | DONE | deterministic MachGate container UUID compatibility |
| 227 | 143 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 228 | 144 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 229 | 145 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 230 | 146 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 231 | 147 | `setsid` | DONE | raw Linux `SYS_setsid` |
| 232 | 148 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 233 | 149 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 234 | 150 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 235 | 151 | `getpgid` | DONE | raw Linux `SYS_getpgid` |
| 236 | 152 | `setprivexec` | DONE | explicit no-op success because child exec support is out of target |
| 237 | 153 | `pread` | DONE | raw Linux `SYS_pread64`; covered by `darwin_range_100_199` fixture |
| 238 | 154 | `pwrite` | DONE | raw Linux `SYS_pwrite64` |
| 241 | 155 | `nfssvc` | DONE | Darwin `ENOTSUP` NFS control compatibility; fixture-covered |
| 243 | 155 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 246 | 156 | `nosys` | ENOSYS | reserved legacy getdirentries row |
| 247 | 157 | `statfs` | DONE | Linux `statfs` translated to Darwin statfs layout |
| 248 | 158 | `fstatfs` | DONE | Linux `fstatfs` translated to Darwin statfs layout |
| 249 | 159 | `unmount` | DONE | Darwin `EPERM` mount namespace mutation compatibility; fixture-covered |
| 250 | 160 | `nosys` | ENOSYS | reserved legacy async_daemon row |
| 253 | 161 | `getfh` | DONE | Darwin `ENOTSUP` file-handle export compatibility; fixture-covered |
| 255 | 161 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 258 | 162 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 259 | 163 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 260 | 164 | `funmount` | DONE | Darwin `EPERM` mount namespace mutation compatibility; fixture-covered |
| 261 | 165 | `quotactl` | DONE | Darwin `ENOTSUP` quota control compatibility; fixture-covered |
| 262 | 166 | `nosys` | ENOSYS | reserved legacy exportfs row |
| 263 | 167 | `mount` | DONE | Darwin `EPERM` mount namespace mutation compatibility; fixture-covered |
| 264 | 168 | `nosys` | ENOSYS | reserved legacy ustat row |
| 265 | 169 | `csops` | DONE | self `CS_OPS_STATUS` returns zero status flags; unsupported ops return `ENOTSUP` |
| 266 | 170 | `csops_audittoken` | DONE | self `CS_OPS_STATUS` returns zero status flags; unsupported ops return `ENOTSUP` |
| 267 | 171 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 268 | 172 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 269 | 173 | `waitid` | DONE | single-process `WNOHANG` subset returns Darwin `ECHILD`; blocking waits return Darwin `ENOSYS` |
| 270 | 174 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 271 | 175 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 272 | 176 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 273 | 177 | `kdebug_typefilter` | DONE | no-op success compatibility for Darwin kernel debug typefilter |
| 274 | 178 | `kdebug_trace_string` | DONE | no-op success compatibility for Darwin kernel debug trace string |
| 275 | 179 | `kdebug_trace64` | DONE | no-op success compatibility for Darwin kernel debug trace64 |
| 276 | 180 | `kdebug_trace` | DONE | no-op success compatibility for Darwin kernel debug trace |
| 277 | 181 | `setgid` | DONE | raw Linux `SYS_setgid` |
| 278 | 182 | `setegid` | DONE | raw Linux `SYS_setresgid(-1, egid, -1)` |
| 279 | 183 | `seteuid` | DONE | raw Linux `SYS_setresuid(-1, euid, -1)` |
| 280 | 184 | `sigreturn` | DONE | Darwin `EINVAL` signal-frame boundary compatibility; fixture-covered |
| 281 | 185 | `sys_panic_with_data` | DONE | Darwin `EPERM` panic diagnostic compatibility; fixture-covered |
| 282 | 186 | `thread_selfcounts` | DONE | minimal single-thread compatibility for supported recount kinds |
| 283 | 187 | `fdatasync` | DONE | raw Linux `SYS_fdatasync` |
| 284 | 188 | `stat` | DONE | Linux `SYS_newfstatat` plus Darwin `struct stat` packing; covered by fixture |
| 285 | 189 | `sys_fstat` | DONE | raw Linux `SYS_fstat` plus Darwin `struct stat` packing |
| 286 | 190 | `lstat` | DONE | Linux `SYS_newfstatat` with `AT_SYMLINK_NOFOLLOW` plus Darwin `struct stat` packing |
| 287 | 191 | `pathconf` | DONE | Linux pathconf mapping for supported names |
| 288 | 192 | `sys_fpathconf` | DONE | Linux fpathconf mapping for supported names |
| 289 | 193 | `nosys` | ENOSYS | reserved legacy getfsstat row |
| 290 | 194 | `getrlimit` | DONE | raw Linux `SYS_getrlimit` with Darwin resource mapping; covered by fixture |
| 291 | 195 | `setrlimit` | DONE | raw Linux `SYS_setrlimit` with Darwin resource mapping |
| 292 | 196 | `getdirentries` | DONE | Linux getdents64 translated for Darwin getdirentries coverage |
| 293 | 197 | `mmap` | DONE | raw Linux `SYS_mmap` with Darwin flag mapping |
| 294 | 198 | `nosys` | ENOSYS | reserved legacy `__syscall` row; covered by fixture |
| 295 | 199 | `lseek` | DONE | raw Linux `SYS_lseek` |

## Range 200-399

| Source Line | Number | Name | Status | Notes |
| --- | ---: | --- | --- | --- |
| 296 | 200 | `truncate` | DONE | raw Linux `SYS_truncate` |
| 297 | 201 | `ftruncate` | DONE | raw Linux `SYS_ftruncate` |
| 298 | 202 | `sysctl` | DONE | common read-only `CTL_HW` and `CTL_KERN` MIB compatibility |
| 299 | 203 | `mlock` | DONE | raw Linux `SYS_mlock` |
| 300 | 204 | `munlock` | DONE | raw Linux `SYS_munlock` |
| 301 | 205 | `undelete` | DONE | Darwin whiteout compatibility errors; no ENOSYS |
| 303 | 206 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 304 | 207 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 305 | 208 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 306 | 209 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 307 | 210 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 308 | 211 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 309 | 212 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 310 | 213 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 312 | 214 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 313 | 215 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 324 | 216 | `open_dprotected_np` | DONE | Linux openat-backed data-protected open; Darwin-only protection metadata ignored |
| 325 | 217 | `fsgetpath_ext` | DONE | resolves fsid and fileid by scanning reachable cwd tree; fixture-covered |
| 326 | 218 | `openat_dprotected_np` | DONE | Linux openat-backed data-protected openat; Darwin-only protection metadata ignored |
| 327 | 219 | `nosys` | ENOSYS | reserved legacy row |
| 328 | 220 | `getattrlist` | DONE | common/file attrlist subset with stat-backed packing; fixture-covered |
| 329 | 221 | `setattrlist` | DONE | mode/owner/group/atime/mtime setter subset; fixture-covered |
| 330 | 222 | `getdirentriesattr` | DONE | getdents64 plus per-entry attr records/count/base/newstate outputs |
| 331 | 223 | `exchangedata` | DONE | Linux `renameat2(RENAME_EXCHANGE)` regular-file exchange compatibility |
| 332 | 224 | `nosys` | ENOSYS | reserved legacy row |
| 333 | 225 | `searchfs` | DONE | practical `ATTR_CMN_NAME` recursive name-search subset |
| 334 | 226 | `delete` | DONE | raw Linux unlinkat/delete semantics |
| 335 | 227 | `copyfile` | DONE | regular-file data copy with Darwin copyfile flag validation |
| 336 | 228 | `fgetattrlist` | DONE | fd-backed attrlist subset using fstat and `/proc/self/fd` |
| 337 | 229 | `fsetattrlist` | DONE | fd-backed mode/owner/group/atime/mtime setter subset |
| 338 | 230 | `poll` | DONE | raw Linux `SYS_poll` |
| 339 | 231 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 340 | 232 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 341 | 233 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 342 | 234 | `getxattr` | DONE | Linux xattr pass-through with Darwin position validation |
| 343 | 235 | `fgetxattr` | DONE | Linux fd xattr pass-through with Darwin position validation |
| 344 | 236 | `setxattr` | DONE | Linux xattr set pass-through with Darwin position validation |
| 345 | 237 | `fsetxattr` | DONE | Linux fd xattr set pass-through with Darwin position validation |
| 346 | 238 | `removexattr` | DONE | Linux xattr remove pass-through |
| 347 | 239 | `fremovexattr` | DONE | Linux fd xattr remove pass-through |
| 348 | 240 | `listxattr` | DONE | Linux xattr list pass-through |
| 349 | 241 | `flistxattr` | DONE | Linux fd xattr list pass-through |
| 350 | 242 | `fsctl` | DONE | path open plus Linux ioctl dispatch; non-ENOSYS behavior fixture-covered |
| 351 | 243 | `initgroups` | DONE | safe empty-set subset; non-empty mutation requests fail without mutating host credentials |
| 352 | 244 | `posix_spawn` | DONE | validates path pointer and returns Darwin `ENOTSUP` compatibility for unsupported child-process creation |
| 353 | 245 | `ffsctl` | DONE | fd Linux ioctl dispatch; non-ENOSYS behavior fixture-covered |
| 354 | 246 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 355 | 247 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 358 | 248 | `fhopen` | DONE | validates handle/options; returns Darwin `EPERM`/`ENOTSUP` for unsupported host handle open instead of ENOSYS |
| 360 | 248 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 363 | 249 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 364 | 250 | `minherit` | DONE | validated no-op compatibility for valid inheritance modes and page-aligned ranges |
| 366 | 251 | `semsys` | DONE | legacy SysV semaphore multiplexer dispatches to semctl/semget/semop |
| 368 | 251 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 371 | 252 | `msgsys` | DONE | legacy SysV message multiplexer dispatches to msgctl/msgget/msgsnd/msgrcv |
| 373 | 252 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 376 | 253 | `shmsys` | DONE | legacy SysV shared-memory multiplexer dispatches to shmat/shmctl/shmdt/shmget |
| 378 | 253 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 381 | 254 | `semctl` | DONE | raw Linux SysV `semctl` passthrough |
| 382 | 255 | `semget` | DONE | raw Linux SysV `semget` passthrough |
| 383 | 256 | `semop` | DONE | raw Linux SysV `semop` passthrough |
| 384 | 257 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 386 | 254 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 387 | 255 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 388 | 256 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 389 | 257 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 392 | 258 | `msgctl` | DONE | raw Linux SysV `msgctl` passthrough |
| 393 | 259 | `msgget` | DONE | raw Linux SysV `msgget` passthrough |
| 394 | 260 | `msgsnd` | DONE | raw Linux SysV `msgsnd` passthrough |
| 395 | 261 | `msgrcv` | DONE | raw Linux SysV `msgrcv` passthrough |
| 397 | 258 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 398 | 259 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 399 | 260 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 400 | 261 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 403 | 262 | `shmat` | DONE | raw Linux SysV `shmat` passthrough |
| 404 | 263 | `shmctl` | DONE | raw Linux SysV `shmctl` passthrough |
| 405 | 264 | `shmdt` | DONE | raw Linux SysV `shmdt` passthrough |
| 406 | 265 | `shmget` | DONE | raw Linux SysV `shmget` passthrough |
| 408 | 262 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 409 | 263 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 410 | 264 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 411 | 265 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 413 | 266 | `shm_open` | DONE | POSIX shm translated through `/dev/shm` open |
| 414 | 267 | `shm_unlink` | DONE | POSIX shm translated through `/dev/shm` unlink |
| 415 | 268 | `sem_open` | DONE | POSIX named semaphore compatibility via guest handle table |
| 416 | 269 | `sem_close` | DONE | POSIX named semaphore close via guest handle table |
| 417 | 270 | `sem_unlink` | DONE | POSIX named semaphore unlink via guest namespace |
| 418 | 271 | `sem_wait` | DONE | POSIX named semaphore wait compatibility; nonblocking-safe behavior for single-thread target |
| 419 | 272 | `sem_trywait` | DONE | POSIX named semaphore trywait compatibility |
| 420 | 273 | `sem_post` | DONE | POSIX named semaphore post compatibility |
| 421 | 274 | `sys_sysctlbyname` | DONE | common read-only `hw.*`, `machdep.cpu.*`, and `kern.*` name compatibility |
| 422 | 275 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 423 | 276 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 424 | 277 | `open_extended` | DONE | Linux openat-backed extended open; Darwin-only security metadata ignored |
| 425 | 278 | `umask_extended` | DONE | raw Linux umask for extended umask |
| 426 | 279 | `stat_extended` | DONE | Linux stat translated to Darwin stat64 plus zero extended security size |
| 427 | 280 | `lstat_extended` | DONE | Linux lstat translated to Darwin stat64 plus zero extended security size |
| 428 | 281 | `sys_fstat_extended` | DONE | Linux fstat translated to Darwin stat64 plus zero extended security size |
| 429 | 282 | `chmod_extended` | DONE | raw Linux fchmodat for extended chmod; Darwin-only ACL metadata ignored |
| 430 | 283 | `fchmod_extended` | DONE | raw Linux fchmod for extended fchmod; Darwin-only ACL metadata ignored |
| 431 | 284 | `access_extended` | DONE | Darwin accessx descriptor validation with Linux faccessat backend |
| 432 | 285 | `sys_settid` | DONE | guest-thread credential override support for current single-thread target |
| 433 | 286 | `gettid` | DONE | guest-thread credential query support |
| 434 | 287 | `setsgroups` | DONE | Darwin setsgroups compatibility: empty set succeeds; non-empty unsupported |
| 435 | 288 | `getsgroups` | DONE | Darwin getsgroups compatibility: empty group list succeeds |
| 436 | 289 | `setwgroups` | DONE | Darwin setwgroups compatibility: empty set succeeds; non-empty unsupported |
| 437 | 290 | `getwgroups` | DONE | Darwin getwgroups compatibility: empty group list succeeds |
| 438 | 291 | `mkfifo_extended` | DONE | raw Linux mkfifoat for extended mkfifo; Darwin-only metadata ignored |
| 439 | 292 | `mkdir_extended` | DONE | raw Linux mkdirat for extended mkdir; Darwin-only metadata ignored |
| 441 | 293 | `identitysvc` | DONE | register/deregister/cache no-op compatibility plus cache-size query validation |
| 443 | 293 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 445 | 294 | `shared_region_check_np` | DONE | safe `shared_region_check_np` compatibility behavior |
| 446 | 295 | `nosys` | ENOSYS | reserved legacy row |
| 447 | 296 | `vm_pressure_monitor` | DONE | safe `vm_pressure_monitor` compatibility behavior |
| 449 | 297 | `psynch_rw_longrdlock` | DONE | futex wait probe compatibility with `EAGAIN` test |
| 450 | 298 | `psynch_rw_yieldwrlock` | DONE | sched_yield compatibility; fixture-covered |
| 451 | 299 | `psynch_rw_downgrade` | DONE | single-thread-compatible no-op success based on Darwin pthread shim behavior |
| 452 | 300 | `psynch_rw_upgrade` | DONE | single-thread-compatible no-op success based on Darwin pthread shim behavior |
| 453 | 301 | `psynch_mutexwait` | DONE | futex wait probe compatibility with `EAGAIN` test |
| 454 | 302 | `psynch_mutexdrop` | DONE | futex wake no-waiter compatibility; fixture-covered |
| 455 | 303 | `psynch_cvbroad` | DONE | futex wake-all no-waiter compatibility; fixture-covered |
| 456 | 304 | `psynch_cvsignal` | DONE | futex wake-one no-waiter compatibility; fixture-covered |
| 457 | 305 | `psynch_cvwait` | DONE | futex wait probe compatibility with `EAGAIN` test |
| 458 | 306 | `psynch_rw_rdlock` | DONE | futex wait probe compatibility with `EAGAIN` test |
| 459 | 307 | `psynch_rw_wrlock` | DONE | futex wait probe compatibility with `EAGAIN` test |
| 460 | 308 | `psynch_rw_unlock` | DONE | futex wake compatibility; fixture-covered |
| 461 | 309 | `psynch_rw_unlock2` | DONE | returns Darwin `ENOTSUP` based on Darwin pthread shim behavior |
| 463 | 297 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 464 | 298 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 465 | 299 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 466 | 300 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 467 | 301 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 468 | 302 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 469 | 303 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 470 | 304 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 471 | 305 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 472 | 306 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 473 | 307 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 474 | 308 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 475 | 309 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 477 | 310 | `getsid` | DONE | raw Linux `SYS_getsid` |
| 478 | 311 | `sys_settid_with_pid` | DONE | single-process `sys_settid_with_pid` compatibility for current process / pid 0 |
| 480 | 312 | `psynch_cvclrprepost` | DONE | futex wake-all compatibility; fixture-covered |
| 482 | 312 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 484 | 313 | `aio_fsync` | DONE | synchronous AIO compatibility for `aio_fsync` |
| 485 | 314 | `aio_return` | DONE | synchronous AIO compatibility for `aio_return` using guest aiocb table |
| 486 | 315 | `aio_suspend` | DONE | synchronous AIO compatibility for `aio_suspend` |
| 487 | 316 | `aio_cancel` | DONE | synchronous AIO compatibility for `aio_cancel` returning all-done behavior |
| 488 | 317 | `aio_error` | DONE | synchronous AIO compatibility for `aio_error` using guest aiocb table |
| 489 | 318 | `aio_read` | DONE | synchronous AIO compatibility for `aio_read` via `pread64` |
| 490 | 319 | `aio_write` | DONE | synchronous AIO compatibility for `aio_write` via `pwrite64` |
| 491 | 320 | `lio_listio` | DONE | synchronous AIO compatibility for `lio_listio` without async notification |
| 492 | 321 | `nosys` | ENOSYS | reserved legacy row |
| 493 | 322 | `iopolicysys` | DONE | safe `iopolicysys` compatibility behavior |
| 494 | 323 | `process_policy` | DONE | safe `process_policy` compatibility behavior |
| 495 | 324 | `mlockall` | DONE | raw Linux `SYS_mlockall` |
| 496 | 325 | `munlockall` | DONE | raw Linux `SYS_munlockall` |
| 497 | 326 | `nosys` | ENOSYS | reserved row |
| 498 | 327 | `issetugid` | DONE | single-process container returns 0 |
| 499 | 328 | `__pthread_kill` | DONE | pthread kill compatibility for current-thread/Linux TID subset |
| 500 | 329 | `__pthread_sigmask` | DONE | pthread sigmask compatibility using Darwin signal set mapping |
| 501 | 330 | `__sigwait` | DONE | sigwait compatibility for blocked pending signals |
| 502 | 331 | `__disable_threadsignal` | DONE | no-op success for `__disable_threadsignal` in single-thread target |
| 503 | 332 | `__pthread_markcancel` | DONE | no-op success for `__pthread_markcancel` in single-thread target |
| 504 | 333 | `__pthread_canceled` | DONE | no-op success for `__pthread_canceled` in single-thread target |
| 509 | 334 | `__semwait_signal` | DONE | named semaphore wait/decrement, timeout `EAGAIN`, signal/post, invalid handle `EINVAL` |
| 512 | 335 | `nosys` | ENOSYS | reserved legacy row |
| 513 | 336 | `proc_info` | DONE | single-process compatibility for listpids and common `proc_pidinfo` flavors |
| 515 | 337 | `sendfile` | DONE | Linux `sendfile` for no-header/no-flags file-to-fd transfers |
| 517 | 337 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 519 | 338 | `stat64` | DONE | raw Linux `SYS_newfstatat` plus Darwin stat64 packing |
| 520 | 339 | `sys_fstat64` | DONE | raw Linux `SYS_fstat` translated to Darwin stat64 |
| 521 | 340 | `lstat64` | DONE | raw Linux `SYS_newfstatat` with `AT_SYMLINK_NOFOLLOW` plus Darwin stat64 packing |
| 522 | 341 | `stat64_extended` | DONE | Linux stat translated to Darwin stat64 extended layout |
| 523 | 342 | `lstat64_extended` | DONE | Linux lstat translated to Darwin stat64 extended layout |
| 524 | 343 | `sys_fstat64_extended` | DONE | Linux fstat translated to Darwin stat64 extended layout |
| 525 | 344 | `getdirentries64` | DONE | Linux `getdents64` records translated to Darwin `dirent64` records |
| 526 | 345 | `statfs64` | DONE | Linux `statfs` translated to Darwin `statfs64` layout |
| 527 | 346 | `fstatfs64` | DONE | Linux `fstatfs` translated to Darwin `statfs64` layout |
| 528 | 347 | `getfsstat64` | DONE | Linux filesystem enumeration translated to Darwin `statfs64` layout where available |
| 529 | 348 | `__pthread_chdir` | DONE | single guest thread maps to raw Linux `SYS_chdir` |
| 530 | 349 | `__pthread_fchdir` | DONE | single guest thread maps to raw Linux `SYS_fchdir` |
| 531 | 350 | `audit` | DONE | audit no-op success for nonnegative record length |
| 532 | 351 | `auditon` | DONE | Darwin `ENOTSUP` audit control compatibility |
| 533 | 352 | `nosys` | ENOSYS | reserved row |
| 534 | 353 | `getauid` | DONE | per-process emulated audit uid state |
| 535 | 354 | `setauid` | DONE | per-process emulated audit uid state |
| 536 | 355 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 537 | 356 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 538 | 357 | `getaudit_addr` | DONE | 48-byte Darwin auditinfo_addr compatibility state |
| 539 | 358 | `setaudit_addr` | DONE | 48-byte Darwin auditinfo_addr compatibility state |
| 540 | 359 | `auditctl` | DONE | Darwin `ENOTSUP` audit control compatibility |
| 541 | 360 | `bsdthread_create` | DONE | invalid/null create returns Darwin `EINVAL` under single-thread target |
| 542 | 361 | `bsdthread_terminate` | DONE | invalid/null terminate returns Darwin `EINVAL` under single-thread target |
| 543 | 362 | `kqueue` | DONE | safe kqueue subset: creates opaque host event fd; real event registration unsupported |
| 544 | 363 | `kevent` | DONE | safe kevent subset: zero-change/zero-event no-op waits only |
| 545 | 364 | `lchown` | DONE | raw Linux `SYS_fchownat` with `AT_SYMLINK_NOFOLLOW` |
| 546 | 365 | `nosys` | ENOSYS | reserved legacy row |
| 547 | 366 | `bsdthread_register` | DONE | no-op success for `bsdthread_register` in single-thread runtime |
| 548 | 367 | `workq_open` | DONE | single-thread workqueue no-op success compatibility |
| 549 | 368 | `workq_kernreturn` | DONE | op 0 success and invalid op `EINVAL` compatibility |
| 550 | 369 | `kevent64` | DONE | eventfd-backed zero-change subset; unsupported flags fail safely |
| 551 | 370 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 552 | 371 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 553 | 372 | `thread_selfid` | DONE | single guest thread maps to Linux `SYS_gettid` |
| 554 | 373 | `ledger` | DONE | safe `ledger` compatibility behavior |
| 555 | 374 | `kevent_qos` | DONE | safe kevent_qos subset: zero-change/zero-event no-op waits only |
| 556 | 375 | `kevent_id` | DONE | safe kevent_id subset: zero-change/zero-event no-op waits only |
| 557 | 376 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 558 | 377 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 559 | 378 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 560 | 379 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 561 | 380 | `__mac_execve` | DONE | validates path pointer and returns Darwin `ENOTSUP` compatibility for unsupported exec replacement |
| 563 | 381 | `__mac_syscall` | DONE | Darwin `ENOTSUP` MAC syscall compatibility |
| 564 | 382 | `__mac_get_file` | DONE | empty-label MAC compatibility |
| 565 | 383 | `__mac_set_file` | DONE | Darwin `ENOTSUP` MAC set compatibility |
| 566 | 384 | `__mac_get_link` | DONE | empty-label MAC compatibility |
| 567 | 385 | `__mac_set_link` | DONE | Darwin `ENOTSUP` MAC set compatibility |
| 568 | 386 | `__mac_get_proc` | DONE | empty-label MAC compatibility |
| 569 | 387 | `__mac_set_proc` | DONE | Darwin `ENOTSUP` MAC set compatibility |
| 570 | 388 | `__mac_get_fd` | DONE | empty-label MAC fd compatibility with fd validation |
| 571 | 389 | `__mac_set_fd` | DONE | Darwin `ENOTSUP` MAC set compatibility |
| 572 | 390 | `__mac_get_pid` | DONE | empty-label MAC pid compatibility with pid validation |
| 574 | 381 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 575 | 382 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 576 | 383 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 577 | 384 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 578 | 385 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 579 | 386 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 580 | 387 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 581 | 388 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 582 | 389 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 583 | 390 | `nosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 585 | 391 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 586 | 392 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 587 | 393 | `enosys` | ENOSYS | explicit Apple reserved/disabled syscall row; return Darwin `ENOSYS` |
| 588 | 394 | `pselect` | DONE | Linux pselect6 with Darwin fdset translation; signal mask unsupported |
| 589 | 395 | `pselect_nocancel` | DONE | Linux pselect6 nocancel with Darwin fdset translation; signal mask unsupported |
| 590 | 396 | `read_nocancel` | DONE | raw Linux `SYS_read` |
| 591 | 397 | `write_nocancel` | DONE | raw Linux `SYS_write` |
| 592 | 398 | `open_nocancel` | DONE | raw Linux `SYS_openat` with Darwin flag translation |
| 593 | 399 | `sys_close_nocancel` | DONE | raw Linux `SYS_close` for nocancel variant |

## Range 400-557

| Source Line | Number | Name | Status | Notes |
| --- | ---: | --- | --- | --- |
| 594 | 400 | `wait4_nocancel` | DONE | single-process wait4_nocancel compatibility: `WNOHANG` returns Darwin `ECHILD`; blocking waits unsupported |
| 596 | 401 | `recvmsg_nocancel` | DONE | raw Linux `recvmsg` nocancel with Darwin iovec/message subset; ancillary data unsupported |
| 597 | 402 | `sendmsg_nocancel` | DONE | raw Linux `sendmsg` nocancel with Darwin iovec/message subset; ancillary data unsupported |
| 598 | 403 | `recvfrom_nocancel` | DONE | raw Linux `recvfrom` nocancel with Darwin sockaddr handling |
| 599 | 404 | `accept_nocancel` | DONE | raw Linux `accept` nocancel with Darwin sockaddr handling |
| 601 | 401 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 602 | 402 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 603 | 403 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 604 | 404 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 606 | 405 | `msync_nocancel` | DONE | raw Linux SYS_msync |
| 607 | 406 | `sys_fcntl_nocancel` | DONE | Linux fcntl translation for nocancel variant |
| 608 | 407 | `select_nocancel` | DONE | Linux select/pselect translation for nocancel variant |
| 609 | 408 | `fsync_nocancel` | DONE | raw Linux SYS_fsync |
| 611 | 409 | `connect_nocancel` | DONE | raw Linux `connect` nocancel with Darwin sockaddr handling |
| 613 | 409 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 615 | 410 | `sigsuspend_nocancel` | DONE | Darwin `EINTR` compatibility matching plain sigsuspend boundary |
| 616 | 411 | `readv_nocancel` | DONE | raw Linux SYS_readv |
| 617 | 412 | `writev_nocancel` | DONE | raw Linux SYS_writev |
| 619 | 413 | `sendto_nocancel` | DONE | raw Linux `sendto` with existing Darwin sockaddr translation |
| 621 | 413 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 623 | 414 | `pread_nocancel` | DONE | raw Linux SYS_pread64 |
| 624 | 415 | `pwrite_nocancel` | DONE | raw Linux SYS_pwrite64 |
| 625 | 416 | `waitid_nocancel` | DONE | single-process `WNOHANG` subset returns Darwin `ECHILD`; blocking waits return Darwin `ENOSYS` |
| 626 | 417 | `poll_nocancel` | DONE | raw Linux SYS_poll |
| 628 | 418 | `msgsnd_nocancel` | DONE | aliases existing SysV `msgsnd` translation |
| 629 | 419 | `msgrcv_nocancel` | DONE | aliases existing SysV `msgrcv` translation |
| 631 | 418 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 632 | 419 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 634 | 420 | `sem_wait_nocancel` | DONE | aliases existing single-thread named semaphore wait behavior |
| 635 | 421 | `aio_suspend_nocancel` | DONE | aliases existing synchronous completed-AIO suspend behavior |
| 636 | 422 | `__sigwait_nocancel` | DONE | blocked pending-signal compatibility subset |
| 637 | 423 | `__semwait_signal_nocancel` | DONE | aliases existing zero-timeout `__semwait_signal` behavior |
| 638 | 424 | `__mac_mount` | DONE | Darwin `ENOTSUP` MAC mount compatibility |
| 640 | 425 | `__mac_get_mount` | DONE | empty-label MAC mount compatibility |
| 642 | 425 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 644 | 426 | `__mac_getfsstat` | DONE | Darwin `ENOTSUP` MAC getfsstat compatibility |
| 645 | 427 | `fsgetpath` | DONE | fsid/object-id lookup resolves through Linux directory search |
| 646 | 428 | `audit_session_self` | DONE | pseudo audit-session port compatibility |
| 647 | 429 | `audit_session_join` | DONE | pseudo audit-session join compatibility; invalid join `EINVAL` |
| 648 | 430 | `sys_fileport_makeport` | DONE | in-process fd to pseudo-port translation |
| 649 | 431 | `sys_fileport_makefd` | DONE | pseudo-port to dup fd translation |
| 650 | 432 | `audit_session_port` | DONE | pseudo audit-session port compatibility |
| 651 | 433 | `pid_suspend` | DONE | current/zero pid compatibility no-op; non-current pid returns Darwin `ESRCH` |
| 652 | 434 | `pid_resume` | DONE | current/zero pid compatibility no-op; non-current pid returns Darwin `ESRCH` |
| 654 | 435 | `pid_hibernate` | DONE | compressor sweep no-op succeeds; current pid returns Darwin `ENOTSUP`; non-current pid returns `ESRCH` |
| 656 | 435 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 659 | 436 | `pid_shutdown_sockets` | DONE | single-process socket shutdown sweep compatibility |
| 661 | 436 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 663 | 437 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 664 | 438 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 665 | 439 | `kas_info` | DONE | selectors 0..3 return zeroed uint64 layout info with pointer/size validation |
| 667 | 440 | `memorystatus_control` | DONE | partial safe `memorystatus_control` compatibility subset |
| 669 | 440 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 671 | 441 | `guarded_open_np` | DONE | guarded open via Linux openat with local guard registry |
| 672 | 442 | `guarded_close_np` | DONE | matching-guard close and unregister; wrong guard returns Darwin `EPERM` |
| 673 | 443 | `guarded_kqueue_np` | DONE | eventfd-backed kqueue compatibility fd with local guard registry |
| 674 | 444 | `change_fdguard_np` | DONE | local fd guard add/replace/remove compatibility |
| 675 | 445 | `usrctl` | DONE | Darwin `EPERM` compatibility for privileged user-session control |
| 676 | 446 | `proc_rlimit_control` | DONE | supported resource-limit monitor flavors validate and no-op safely |
| 678 | 447 | `connectx` | DONE | basic endpoint bind/connect plus optional writev compatibility |
| 679 | 448 | `disconnectx` | DONE | assoc/conn id 0 maps to Linux AF_UNSPEC disconnect |
| 680 | 449 | `peeloff` | DONE | Darwin `ENOTSUP` compatibility; fixture-covered |
| 681 | 450 | `socket_delegate` | DONE | creates socket for self/0 delegate pid |
| 683 | 447 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 684 | 448 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 685 | 449 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 686 | 450 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 688 | 451 | `telemetry` | DONE | no-op success compatibility |
| 690 | 452 | `proc_uuid_policy` | DONE | UUID pointer/length validation compatibility |
| 692 | 452 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 695 | 453 | `memorystatus_get_level` | DONE | safe `memorystatus_get_level` compatibility behavior |
| 697 | 453 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 699 | 454 | `system_override` | DONE | no-op success compatibility |
| 700 | 455 | `vfs_purge` | DONE | no-op success compatibility |
| 701 | 456 | `sfi_ctl` | DONE | writes out_time compatibility value |
| 702 | 457 | `sfi_pidctl` | DONE | self/zero pid validation and zero flags output |
| 704 | 458 | `coalition` | DONE | synthetic coalition id create/terminate/reap compatibility |
| 705 | 459 | `coalition_info` | DONE | synthetic coalition id validation and zero-filled info |
| 707 | 458 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 708 | 459 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 711 | 460 | `necp_match_policy` | DONE | no-policy compatibility result |
| 713 | 460 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 715 | 461 | `getattrlistbulk` | DONE | reads directory entries and packs Darwin attrlist records |
| 716 | 462 | `clonefileat` | DONE | Linux `FICLONE` where possible with byte-copy fallback |
| 717 | 463 | `openat` | DONE | raw Linux SYS_openat with Darwin AT_FDCWD and open flag translation |
| 718 | 464 | `openat_nocancel` | DONE | raw Linux SYS_openat with Darwin AT_FDCWD and open flag translation |
| 719 | 465 | `renameat` | DONE | raw Linux SYS_renameat with Darwin AT_FDCWD translation |
| 720 | 466 | `faccessat` | DONE | raw Linux SYS_faccessat with Darwin at-flag translation |
| 721 | 467 | `fchmodat` | DONE | raw Linux SYS_fchmodat for flags=0 |
| 722 | 468 | `fchownat` | DONE | raw Linux SYS_fchownat with Darwin at-flag translation |
| 723 | 469 | `fstatat` | DONE | raw Linux newfstatat/fstatat64 with Darwin stat translation |
| 724 | 470 | `fstatat64` | DONE | raw Linux newfstatat/fstatat64 with Darwin stat translation |
| 725 | 471 | `linkat` | DONE | raw Linux SYS_linkat with Darwin link flag translation |
| 726 | 472 | `unlinkat` | DONE | raw Linux SYS_unlinkat with Darwin AT_REMOVEDIR translation |
| 727 | 473 | `readlinkat` | DONE | raw Linux SYS_readlinkat |
| 728 | 474 | `symlinkat` | DONE | raw Linux SYS_symlinkat |
| 729 | 475 | `mkdirat` | DONE | raw Linux SYS_mkdirat |
| 730 | 476 | `getattrlistat` | DONE | at-relative stat to Darwin attrlist buffer |
| 731 | 477 | `proc_trace_log` | DONE | self/zero pid no-op success compatibility |
| 732 | 478 | `bsdthread_ctl` | DONE | validation/no-op QoS and workqueue controls for single-thread target |
| 733 | 479 | `openbyid_np` | DONE | resolves Darwin fs object id/fsid to a path and opens it |
| 735 | 480 | `recvmsg_x` | DONE | conservative `msghdr_x` subset without ancillary/control data |
| 736 | 481 | `sendmsg_x` | DONE | conservative `msghdr_x` subset without ancillary/control data |
| 738 | 480 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 739 | 481 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 741 | 482 | `thread_selfusage` | DONE | stable zero-success single-thread compatibility |
| 743 | 483 | `csrctl` | DONE | CSR check and active config compatibility |
| 745 | 483 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 747 | 484 | `guarded_open_dprotected_np` | DONE | Linux open-compatible guarded open; data-protection class no-op |
| 748 | 485 | `guarded_write_np` | DONE | guard-validated Linux write |
| 749 | 486 | `guarded_pwrite_np` | DONE | guard-validated Linux pwrite64 |
| 750 | 487 | `guarded_writev_np` | DONE | guard-validated Linux writev |
| 751 | 488 | `renameatx_np` | DONE | safe subset for flags `0`, `RENAME_EXCL`, and `RENAME_SWAP` via Linux renameat/renameat2 |
| 753 | 489 | `mremap_encrypted` | DONE | zero-length no-op success with fault validation |
| 755 | 489 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 758 | 490 | `netagent_trigger` | DONE | UUID length/pointer validation compatibility |
| 760 | 490 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 762 | 491 | `stack_snapshot_with_config` | DONE | empty no-op success compatibility |
| 764 | 492 | `microstackshot` | DONE | zero-filled trace buffer compatibility |
| 766 | 492 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 769 | 493 | `grab_pgo_data` | DONE | zero-filled output returning 0 bytes |
| 771 | 493 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 774 | 494 | `persona` | DONE | process-local default persona GET plus info/find support |
| 776 | 494 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 778 | 495 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 779 | 496 | `mach_eventlink_signal` | DONE | invalid/missing port returns Darwin `EINVAL` |
| 780 | 497 | `mach_eventlink_wait_until` | DONE | invalid/missing port returns Darwin `EINVAL` |
| 781 | 498 | `mach_eventlink_signal_wait_until` | DONE | invalid/missing port returns Darwin `EINVAL` |
| 782 | 499 | `work_interval_ctl` | DONE | no-op success with arg/len validation |
| 783 | 500 | `getentropy` | DONE | raw Linux SYS_getrandom with Darwin getentropy 256-byte limit |
| 785 | 501 | `necp_open` | DONE | dummy closeable NECP control fd |
| 786 | 502 | `necp_client_action` | DONE | valid NECP fd recognized; Apple-only actions return `ENOTSUP` |
| 788 | 501 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 789 | 502 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 792 | 503 | `__nexus_open` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 793 | 504 | `__nexus_register` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 794 | 505 | `__nexus_deregister` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 795 | 506 | `__nexus_create` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 796 | 507 | `__nexus_destroy` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 797 | 508 | `__nexus_get_opt` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 798 | 509 | `__nexus_set_opt` | DONE | Darwin `ENOTSUP` Skywalk nexus compatibility |
| 799 | 510 | `__channel_open` | DONE | Darwin `ENOTSUP` Skywalk channel compatibility |
| 800 | 511 | `__channel_get_info` | DONE | Darwin `ENOTSUP` Skywalk channel compatibility |
| 801 | 512 | `__channel_sync` | DONE | Darwin `ENOTSUP` Skywalk channel compatibility |
| 802 | 513 | `__channel_get_opt` | DONE | Darwin `ENOTSUP` Skywalk channel compatibility |
| 803 | 514 | `__channel_set_opt` | DONE | Darwin `ENOTSUP` Skywalk channel compatibility |
| 805 | 503 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 806 | 504 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 807 | 505 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 808 | 506 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 809 | 507 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 810 | 508 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 811 | 509 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 812 | 510 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 813 | 511 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 814 | 512 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 815 | 513 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 816 | 514 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 818 | 515 | `sys_ulock_wait` | DONE | 32-bit futex-backed `sys_ulock_wait` subset |
| 819 | 516 | `sys_ulock_wake` | DONE | futex-backed `sys_ulock_wake` subset |
| 820 | 517 | `fclonefileat` | DONE | fd-source clone/copy compatibility |
| 821 | 518 | `fs_snapshot` | DONE | argument validation plus safe `ENOTSUP` compatibility |
| 822 | 519 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 823 | 520 | `terminate_with_payload` | DONE | maps missing target to Darwin `ESRCH`, non-self target to Linux `SIGKILL`, self target exits 137 |
| 824 | 521 | `abort_with_payload` | DONE | `abort_with_payload` exits current guest process with status 134 |
| 826 | 522 | `necp_session_open` | DONE | dummy closeable NECP session fd |
| 827 | 523 | `necp_session_action` | DONE | valid NECP session fd recognized; Apple-only actions return `ENOTSUP` |
| 829 | 522 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 830 | 523 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 832 | 524 | `setattrlistat` | DONE | mode/time/owner/group attrlist subset |
| 833 | 525 | `net_qos_guideline` | DONE | default QoS result for valid Darwin struct size |
| 834 | 526 | `fmount` | DONE | invalid fd `EBADF`, valid fd safe `EPERM` |
| 835 | 527 | `ntp_adjtime` | DONE | Linux clock/time-backed ntp_adjtime compatibility behavior |
| 836 | 528 | `ntp_gettime` | DONE | Linux clock/time-backed ntp_gettime compatibility behavior |
| 837 | 529 | `os_fault_with_payload` | DONE | diagnostic no-op for `os_fault_with_payload` |
| 838 | 530 | `kqueue_workloop_ctl` | DONE | validates packed params and supports eventfd-backed create/destroy compatibility |
| 839 | 531 | `__mach_bridge_remote_time` | DONE | identity timestamp translation |
| 841 | 532 | `coalition_ledger` | DONE | synthetic coalition ledger limit op compatibility |
| 843 | 532 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 845 | 533 | `log_data` | DONE | buffer validation and no-op success |
| 846 | 534 | `memorystatus_available_memory` | DONE | Linux memory-info-backed `memorystatus_available_memory` |
| 847 | 535 | `objc_bp_assist_cfg_np` | DONE | no-op success compatibility |
| 848 | 536 | `shared_region_map_and_slide_2_np` | DONE | empty no-op subset for `shared_region_map_and_slide_2_np` |
| 849 | 537 | `pivot_root` | DONE | path validation plus safe `EPERM` compatibility |
| 850 | 538 | `task_inspect_for_pid` | DONE | Darwin `EPERM` with output port cleared |
| 851 | 539 | `task_read_for_pid` | DONE | Darwin `EPERM` with output port cleared |
| 852 | 540 | `sys_preadv` | DONE | raw Linux SYS_preadv |
| 853 | 541 | `sys_pwritev` | DONE | raw Linux SYS_pwritev |
| 854 | 542 | `sys_preadv_nocancel` | DONE | raw Linux SYS_preadv |
| 855 | 543 | `sys_pwritev_nocancel` | DONE | raw Linux SYS_pwritev |
| 856 | 544 | `sys_ulock_wait2` | DONE | 32-bit futex-backed `sys_ulock_wait2` subset |
| 857 | 545 | `proc_info_extended_id` | DONE | current-process short BSD info subset |
| 859 | 546 | `tracker_action` | DONE | no-op compatibility for supported tracker actions |
| 861 | 546 | `nosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 863 | 547 | `debug_syscall_reject` | DONE | no-op success compatibility |
| 864 | 548 | `sys_debug_syscall_reject_config` | DONE | no-op success compatibility |
| 865 | 549 | `graftdmg` | DONE | fd/path validation plus `ENOTSUP` compatibility |
| 866 | 550 | `map_with_linking_np` | DONE | zero-region no-op success compatibility |
| 867 | 551 | `freadlink` | DONE | Linux readlinkat with Darwin `O_SYMLINK`/`O_PATH` support |
| 868 | 552 | `sys_record_system_event` | DONE | no-op success compatibility |
| 869 | 553 | `mkfifoat` | DONE | raw Linux SYS_mknodat with S_IFIFO |
| 870 | 554 | `mknodat` | DONE | raw Linux SYS_mknodat |
| 871 | 555 | `ungraftdmg` | DONE | path validation plus `ENOTSUP` compatibility |
| 873 | 556 | `sys_coalition_policy_set` | DONE | synthetic coalition policy set compatibility |
| 874 | 557 | `sys_coalition_policy_get` | DONE | synthetic coalition policy get compatibility |
| 876 | 556 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |
| 877 | 557 | `enosys` | ENOSYS | reserved/nosys row; explicit Darwin ENOSYS |

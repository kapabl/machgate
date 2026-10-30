#ifndef _SYSCALL_GATE_H_
#define _SYSCALL_GATE_H_

#include <stdint.h>
#include <stddef.h>
#include "domain/guest_image.h"
#include "domain/guest_syscall.h"

struct iovec;

int syscall_gate_patch(struct machgate_guest_image* image);
void syscall_gate_dispatch(struct machgate_syscall_frame* state);
long machgate_syscall_write_no_sigpipe(int fd, const void* buffer, size_t size);
long machgate_syscall_writev_no_sigpipe(int fd, const struct iovec* iov, int iovcnt);

#endif /* _SYSCALL_GATE_H_ */

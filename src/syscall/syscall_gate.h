#ifndef _SYSCALL_GATE_H_
#define _SYSCALL_GATE_H_

#include <stdint.h>
#include <stddef.h>
#include "loader.h"

struct iovec;

struct syscall_gate_state {
	uint64_t x[19];
	uint64_t x30;
	uint64_t nzcv;
	uint64_t pc;
};

int syscall_gate_patch(struct load_results* lr);
void syscall_gate_dispatch(struct syscall_gate_state* state);
long machgate_syscall_write_no_sigpipe(int fd, const void* buffer, size_t size);
long machgate_syscall_writev_no_sigpipe(int fd, const struct iovec* iov, int iovcnt);

#endif /* _SYSCALL_GATE_H_ */

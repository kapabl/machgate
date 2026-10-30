#ifndef MACHGATE_DOMAIN_GUEST_SYSCALL_H
#define MACHGATE_DOMAIN_GUEST_SYSCALL_H

#include <stdint.h>

struct machgate_syscall_frame {
	uint64_t x[19];
	uint64_t x30;
	uint64_t nzcv;
	uint64_t pc;
};

struct machgate_guest_syscall {
	uint64_t number;
	uint64_t arguments[8];
	uint64_t pc;
};

struct machgate_syscall_outcome {
	uint64_t value;
	int darwin_errno;
	int failed;
};

struct machgate_guest_syscall machgate_guest_syscall_from_frame(
	const struct machgate_syscall_frame* frame);
struct machgate_syscall_outcome machgate_syscall_succeeded(uint64_t value);
struct machgate_syscall_outcome machgate_syscall_failed(int darwin_errno);
void machgate_syscall_apply_outcome(
	struct machgate_syscall_frame* frame,
	struct machgate_syscall_outcome outcome);

#endif

#include "domain/guest_syscall.h"

#define MACHGATE_SYSCALL_CARRY (1ull << 29)

struct machgate_guest_syscall machgate_guest_syscall_from_frame(
	const struct machgate_syscall_frame* frame)
{
	struct machgate_guest_syscall result = {
		.number = frame->x[16],
		.pc = frame->pc,
	};
	for (int argument_index = 0; argument_index < 8; argument_index++)
		result.arguments[argument_index] = frame->x[argument_index];
	return result;
}

struct machgate_syscall_outcome machgate_syscall_succeeded(uint64_t value)
{
	return (struct machgate_syscall_outcome){
		.value = value,
	};
}

struct machgate_syscall_outcome machgate_syscall_failed(int darwin_errno)
{
	return (struct machgate_syscall_outcome){
		.darwin_errno = darwin_errno,
		.failed = 1,
	};
}

void machgate_syscall_apply_outcome(
	struct machgate_syscall_frame* frame,
	struct machgate_syscall_outcome outcome)
{
	if (outcome.failed) {
		frame->x[0] = (uint64_t)outcome.darwin_errno;
		frame->nzcv |= MACHGATE_SYSCALL_CARRY;
		return;
	}
	frame->x[0] = outcome.value;
	frame->nzcv &= ~MACHGATE_SYSCALL_CARRY;
}

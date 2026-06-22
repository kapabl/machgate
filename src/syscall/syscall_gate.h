#ifndef _SYSCALL_GATE_H_
#define _SYSCALL_GATE_H_

#include <stdint.h>
#include "loader.h"

struct syscall_gate_state {
	uint64_t x[19];
	uint64_t x30;
	uint64_t nzcv;
	uint64_t pc;
};

int syscall_gate_patch(struct load_results* lr);
void syscall_gate_dispatch(struct syscall_gate_state* state);

#endif /* _SYSCALL_GATE_H_ */

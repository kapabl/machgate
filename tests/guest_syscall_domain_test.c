#include "domain/guest_syscall.h"

#include <stdint.h>

#define TEST_CARRY (1ull << 29)

int main(void)
{
	struct machgate_syscall_frame frame = {
		.x = {11, 22, 33, 44, 55, 66, 77, 88},
		.nzcv = TEST_CARRY,
		.pc = 0x1234,
	};
	frame.x[16] = 197;

	struct machgate_guest_syscall request =
		machgate_guest_syscall_from_frame(&frame);
	if (request.number != 197 || request.arguments[0] != 11 ||
	    request.arguments[7] != 88 || request.pc != 0x1234)
		return 1;

	machgate_syscall_apply_outcome(&frame, machgate_syscall_succeeded(99));
	if (frame.x[0] != 99 || (frame.nzcv & TEST_CARRY))
		return 2;

	machgate_syscall_apply_outcome(&frame, machgate_syscall_failed(78));
	if (frame.x[0] != 78 || !(frame.nzcv & TEST_CARRY))
		return 3;
	return 0;
}

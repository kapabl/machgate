#include "domain/guest_process.h"

#include <stdint.h>

int main(void)
{
	struct machgate_guest_process process = {0};
	struct machgate_guest_image* image = (void*)(uintptr_t)1;
	char* arguments[] = {(char*)"guest", NULL};
	char* environment[] = {NULL};
	char* apple[] = {(char*)"executable_path=/guest", NULL};

	if (machgate_guest_process_mark_initialized(&process) == 0)
		return 1;
	if (machgate_guest_process_accept_image(&process, image) != 0)
		return 2;
	if (machgate_guest_process_accept_image(&process, image) == 0)
		return 3;
	if (machgate_guest_process_prepare_runtime(
		    &process, 1, arguments, environment, apple, "/guest",
		    0x1000, (void*)(uintptr_t)0x2000, 4096) != 0)
		return 4;
	if (machgate_guest_process_mark_initialized(&process) != 0)
		return 5;
	if (machgate_guest_process_mark_entering(&process) != 0)
		return 6;
	if (process.stage != MACHGATE_GUEST_PROCESS_ENTERING)
		return 7;
	machgate_guest_process_reset(&process);
	return process.stage == MACHGATE_GUEST_PROCESS_EMPTY ? 0 : 8;
}

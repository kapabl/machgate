#include "domain/guest_process.h"

#include <string.h>

int machgate_guest_process_accept_image(
	struct machgate_guest_process* process,
	struct machgate_guest_image* image)
{
	if (!process || !image || process->stage != MACHGATE_GUEST_PROCESS_EMPTY)
		return -1;
	process->image = image;
	process->stage = MACHGATE_GUEST_PROCESS_IMAGE_PREPARED;
	return 0;
}

int machgate_guest_process_prepare_runtime(
	struct machgate_guest_process* process, int argument_count,
	char** arguments, char** environment, char** apple,
	const char* executable_path, uintptr_t stack_pointer,
	void* stack_top, size_t stack_size)
{
	if (!process || process->stage != MACHGATE_GUEST_PROCESS_IMAGE_PREPARED ||
	    argument_count < 0 || !arguments || !apple || !executable_path ||
	    !stack_pointer || !stack_top || !stack_size)
		return -1;

	process->runtime.argument_count = argument_count;
	process->runtime.arguments = arguments;
	process->runtime.environment = environment;
	process->runtime.executable_path = executable_path;
	process->runtime.main_stack_top = stack_top;
	process->runtime.main_stack_size = stack_size;
	process->stack_pointer = stack_pointer;
	process->apple = apple;
	process->stage = MACHGATE_GUEST_PROCESS_RUNTIME_PREPARED;
	return 0;
}

int machgate_guest_process_mark_initialized(
	struct machgate_guest_process* process)
{
	if (!process ||
	    process->stage != MACHGATE_GUEST_PROCESS_RUNTIME_PREPARED)
		return -1;
	process->stage = MACHGATE_GUEST_PROCESS_INITIALIZED;
	return 0;
}

int machgate_guest_process_mark_entering(
	struct machgate_guest_process* process)
{
	if (!process || process->stage != MACHGATE_GUEST_PROCESS_INITIALIZED)
		return -1;
	process->stage = MACHGATE_GUEST_PROCESS_ENTERING;
	return 0;
}

void machgate_guest_process_reset(struct machgate_guest_process* process)
{
	if (process)
		memset(process, 0, sizeof(*process));
}

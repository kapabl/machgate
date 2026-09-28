#ifndef MACHGATE_PROCESS_LAUNCH_CONTEXT_H
#define MACHGATE_PROCESS_LAUNCH_CONTEXT_H

#include <stddef.h>
#include <stdint.h>

struct machgate_process_launch_context {
	uintptr_t stack_pointer;
	char** arguments;
	char** environment;
	char** apple;
};

int machgate_prepare_process_launch_context(
	uintptr_t stack_top, size_t stack_size, int argument_count,
	char** arguments, char** environment, const char* executable_path,
	struct machgate_process_launch_context* context,
	char* error, size_t error_size);

#endif

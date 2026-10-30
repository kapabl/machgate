#ifndef MACHGATE_DOMAIN_GUEST_PROCESS_H
#define MACHGATE_DOMAIN_GUEST_PROCESS_H

#include "domain/guest_image.h"

#include <stddef.h>
#include <stdint.h>

#define MACHGATE_GUEST_PROCESS_RUNTIME_STATE_SYMBOL \
	"machgate_guest_process_runtime_state_v1"

enum machgate_guest_process_stage {
	MACHGATE_GUEST_PROCESS_EMPTY,
	MACHGATE_GUEST_PROCESS_IMAGE_PREPARED,
	MACHGATE_GUEST_PROCESS_RUNTIME_PREPARED,
	MACHGATE_GUEST_PROCESS_INITIALIZED,
	MACHGATE_GUEST_PROCESS_ENTERING,
};

struct machgate_guest_process_runtime_state {
	int argument_count;
	char** arguments;
	char** environment;
	const char* executable_path;
	void* main_stack_top;
	size_t main_stack_size;
};

typedef const struct machgate_guest_process_runtime_state*
	(*machgate_guest_process_runtime_state_function)(void);

struct machgate_guest_process {
	enum machgate_guest_process_stage stage;
	struct machgate_guest_image* image;
	struct machgate_guest_process_runtime_state runtime;
	uintptr_t stack_pointer;
	char** apple;
};

int machgate_guest_process_accept_image(
	struct machgate_guest_process* process,
	struct machgate_guest_image* image);
int machgate_guest_process_prepare_runtime(
	struct machgate_guest_process* process, int argument_count,
	char** arguments, char** environment, char** apple,
	const char* executable_path, uintptr_t stack_pointer,
	void* stack_top, size_t stack_size);
int machgate_guest_process_mark_initialized(
	struct machgate_guest_process* process);
int machgate_guest_process_mark_entering(
	struct machgate_guest_process* process);
void machgate_guest_process_reset(struct machgate_guest_process* process);

#endif

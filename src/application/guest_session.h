#ifndef MACHGATE_APPLICATION_GUEST_SESSION_H
#define MACHGATE_APPLICATION_GUEST_SESSION_H

#include "domain/guest_process.h"

#include <stddef.h>

int machgate_run_guest(const char* executable_path, int argument_count,
			       char** arguments, char** environment,
			       char* error, size_t error_size);
const struct machgate_guest_process_runtime_state*
machgate_guest_process_runtime_state_v1(void);

#endif

#include "guest_process_runtime.h"

#include <dlfcn.h>

const struct machgate_guest_process_runtime_state*
machgate_current_guest_process_runtime(void)
{
	static machgate_guest_process_runtime_state_function runtime_state;
	static int resolved;
	if (!resolved) {
		runtime_state =
			(machgate_guest_process_runtime_state_function)dlsym(
				RTLD_DEFAULT,
				MACHGATE_GUEST_PROCESS_RUNTIME_STATE_SYMBOL);
		resolved = 1;
	}
	return runtime_state ? runtime_state() : NULL;
}

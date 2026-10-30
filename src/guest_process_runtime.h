#ifndef MACHGATE_GUEST_PROCESS_RUNTIME_H
#define MACHGATE_GUEST_PROCESS_RUNTIME_H

#include "domain/guest_process.h"

const struct machgate_guest_process_runtime_state*
machgate_current_guest_process_runtime(void);

#endif

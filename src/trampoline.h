#ifndef MACHGATE_TRAMPOLINE_H
#define MACHGATE_TRAMPOLINE_H

#include "loader.h"

#include <stddef.h>

int machgate_apply_trampolines(struct machgate_guest_image* image, char* error,
			       size_t error_size);

#endif

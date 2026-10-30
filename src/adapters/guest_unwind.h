#ifndef MACHGATE_APPLICATION_GUEST_UNWIND_H
#define MACHGATE_APPLICATION_GUEST_UNWIND_H

#include "domain/guest_image.h"

#include <stddef.h>

int machgate_register_guest_unwind(struct machgate_guest_image* image,
				   char* error, size_t error_size);

#endif

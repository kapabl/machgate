#ifndef MACHGATE_INSTRUCTION_ADAPTATION_H
#define MACHGATE_INSTRUCTION_ADAPTATION_H

#include "domain/guest_image.h"

#include <stddef.h>

int machgate_adapt_guest_instructions(struct machgate_guest_image* image,
				      char* error, size_t error_size);
void* machgate_guest_thread_context(void);

#endif

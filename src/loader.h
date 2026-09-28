#ifndef MACHGATE_LOADER_H
#define MACHGATE_LOADER_H

#include "domain/guest_image.h"

#include <stddef.h>

int machgate_load_executable(const char* path,
			     struct machgate_guest_image* image,
			     char* error, size_t error_size);
void* machgate_pool_alloc(struct machgate_guest_image* image, size_t size);

#endif

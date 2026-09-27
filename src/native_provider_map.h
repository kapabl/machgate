#ifndef MACHGATE_NATIVE_PROVIDER_MAP_H
#define MACHGATE_NATIVE_PROVIDER_MAP_H

#include <stddef.h>

int machgate_native_provider_path(const char* install_name, char* path,
				  size_t path_size, char* error,
				  size_t error_size);

#endif

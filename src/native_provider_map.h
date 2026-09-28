#ifndef MACHGATE_NATIVE_PROVIDER_MAP_H
#define MACHGATE_NATIVE_PROVIDER_MAP_H

#include <stddef.h>

enum machgate_provider_decision_kind {
	MACHGATE_PROVIDER_UNMAPPED,
	MACHGATE_PROVIDER_MAP,
	MACHGATE_PROVIDER_SKIP,
	MACHGATE_PROVIDER_STUB,
	MACHGATE_PROVIDER_PARTIAL,
};

int machgate_native_provider_decision(
	const char* install_name, enum machgate_provider_decision_kind* kind,
	char* path, size_t path_size, char* error, size_t error_size);

#endif

#ifndef MACHGATE_NATIVE_PROVIDER_MAP_H
#define MACHGATE_NATIVE_PROVIDER_MAP_H

#include "domain/provider_decision.h"

#include <stddef.h>

int machgate_native_provider_decision(
	const char* install_name, enum machgate_provider_decision_kind* kind,
	char* path, size_t path_size, char* error, size_t error_size);

#endif

#include "native_provider_guest_import.h"
#include "resource_limit_compat.h"

#include <string.h>

int machgate_resolve_guest_import_v1(const char* name, void** address)
{
	if (!name || !address)
		return 0;

	if (strcmp(name, "getrlimit") == 0) {
		*address = machgate_darwin_getrlimit;
		return 1;
	}
	if (strcmp(name, "setrlimit") == 0) {
		*address = machgate_darwin_setrlimit;
		return 1;
	}
	return 0;
}

#include "native_provider_guest_import.h"

#include <sys/resource.h>

int getrlimit(__rlimit_resource_t resource, struct rlimit* limits)
{
	(void)resource;
	(void)limits;
	return 73;
}

__attribute__((visibility("hidden")))
int machgate_inherited_resolver_dependency(void)
{
	void* address = 0;
	return machgate_resolve_guest_import_v1("getrlimit", &address);
}

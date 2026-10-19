#ifndef MACHGATE_RESOURCE_LIMIT_COMPAT_H
#define MACHGATE_RESOURCE_LIMIT_COMPAT_H

#include <sys/resource.h>

int machgate_darwin_getrlimit(int darwin_resource, struct rlimit* limits);
int machgate_darwin_setrlimit(int darwin_resource,
			      const struct rlimit* limits);

#endif

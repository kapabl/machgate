#define _GNU_SOURCE

#include "resource_limit_compat.h"

#include <errno.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

#define DARWIN_RLIMIT_CPU 0
#define DARWIN_RLIMIT_FSIZE 1
#define DARWIN_RLIMIT_DATA 2
#define DARWIN_RLIMIT_STACK 3
#define DARWIN_RLIMIT_CORE 4
#define DARWIN_RLIMIT_AS 5
#define DARWIN_RLIMIT_MEMLOCK 6
#define DARWIN_RLIMIT_NPROC 7
#define DARWIN_RLIMIT_NOFILE 8

#define DARWIN_RLIM_INFINITY ((rlim_t)INT64_MAX)

static int linux_resource_for_darwin(int darwin_resource, int* linux_resource)
{
	switch (darwin_resource) {
	case DARWIN_RLIMIT_CPU:
		*linux_resource = RLIMIT_CPU;
		return 0;
	case DARWIN_RLIMIT_FSIZE:
		*linux_resource = RLIMIT_FSIZE;
		return 0;
	case DARWIN_RLIMIT_DATA:
		*linux_resource = RLIMIT_DATA;
		return 0;
	case DARWIN_RLIMIT_STACK:
		*linux_resource = RLIMIT_STACK;
		return 0;
	case DARWIN_RLIMIT_CORE:
		*linux_resource = RLIMIT_CORE;
		return 0;
	case DARWIN_RLIMIT_AS:
		*linux_resource = RLIMIT_AS;
		return 0;
	case DARWIN_RLIMIT_MEMLOCK:
		*linux_resource = RLIMIT_MEMLOCK;
		return 0;
	case DARWIN_RLIMIT_NPROC:
		*linux_resource = RLIMIT_NPROC;
		return 0;
	case DARWIN_RLIMIT_NOFILE:
		*linux_resource = RLIMIT_NOFILE;
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}

static rlim_t darwin_limit_from_linux(rlim_t linux_limit)
{
	if (linux_limit == RLIM_INFINITY)
		return DARWIN_RLIM_INFINITY;
	return linux_limit;
}

static rlim_t linux_limit_from_darwin(rlim_t darwin_limit)
{
	if (darwin_limit == DARWIN_RLIM_INFINITY)
		return RLIM_INFINITY;
	return darwin_limit;
}

int machgate_darwin_getrlimit(int darwin_resource, struct rlimit* limits)
{
	if (!limits) {
		errno = EFAULT;
		return -1;
	}

	int linux_resource;
	if (linux_resource_for_darwin(darwin_resource, &linux_resource) != 0)
		return -1;

	struct rlimit linux_limits;
	long result = syscall(SYS_prlimit64, 0, linux_resource, NULL,
	                      &linux_limits);
	if (result < 0)
		return -1;

	limits->rlim_cur = darwin_limit_from_linux(linux_limits.rlim_cur);
	limits->rlim_max = darwin_limit_from_linux(linux_limits.rlim_max);
	return 0;
}

int machgate_darwin_setrlimit(int darwin_resource,
			      const struct rlimit* limits)
{
	if (!limits) {
		errno = EFAULT;
		return -1;
	}

	int linux_resource;
	if (linux_resource_for_darwin(darwin_resource, &linux_resource) != 0)
		return -1;

	struct rlimit linux_limits = {
		.rlim_cur = linux_limit_from_darwin(limits->rlim_cur),
		.rlim_max = linux_limit_from_darwin(limits->rlim_max),
	};
	long result = syscall(SYS_prlimit64, 0, linux_resource, &linux_limits,
	                      NULL);
	if (result < 0)
		return -1;
	return 0;
}

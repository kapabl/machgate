#define _GNU_SOURCE

#include "host_facts.h"

#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>

typedef long (*host_sysconf_function)(int);

static long read_host_configuration(int selector)
{
	static host_sysconf_function host_sysconf;

	if (!host_sysconf)
		host_sysconf = (host_sysconf_function)dlsym(RTLD_NEXT, "sysconf");
	if (!host_sysconf)
		return -1;
	return host_sysconf(selector);
}

int machgate_host_cpu_count(void)
{
	long result = read_host_configuration(_SC_NPROCESSORS_ONLN);

	if (result < 1)
		return 1;
	if (result > INT_MAX)
		return INT_MAX;
	return (int)result;
}

long machgate_host_page_size(void)
{
	long result = read_host_configuration(_SC_PAGESIZE);

	if (result < 1)
		return 4096;
	return result;
}

uint64_t machgate_host_memory_size(void)
{
	long page_count = read_host_configuration(_SC_PHYS_PAGES);
	long page_size = machgate_host_page_size();

	if (page_count < 1 || page_size < 1)
		return 0;
	if ((uint64_t)page_count > UINT64_MAX / (uint64_t)page_size)
		return UINT64_MAX;
	return (uint64_t)page_count * (uint64_t)page_size;
}

#include "host_facts.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum darwin_sysctl_root {
	DARWIN_SYSCTL_KERN = 1,
	DARWIN_SYSCTL_HW = 6,
};

enum darwin_sysctl_value_kind {
	DARWIN_SYSCTL_ACTIVE_CPU_COUNT,
	DARWIN_SYSCTL_ARGUMENT_LIMIT,
	DARWIN_SYSCTL_BYTE_ORDER,
	DARWIN_SYSCTL_HOSTNAME,
	DARWIN_SYSCTL_MACHINE,
	DARWIN_SYSCTL_MEMORY_SIZE,
	DARWIN_SYSCTL_MODEL,
	DARWIN_SYSCTL_OPTIONAL_ARM64,
	DARWIN_SYSCTL_OS_RELEASE,
	DARWIN_SYSCTL_OS_TYPE,
	DARWIN_SYSCTL_OS_VERSION,
	DARWIN_SYSCTL_PAGE_SIZE,
	DARWIN_SYSCTL_PRODUCT_VERSION,
	DARWIN_SYSCTL_VERSION,
};

struct darwin_sysctl_entry {
	const char* name;
	int root;
	int selector;
	enum darwin_sysctl_value_kind value_kind;
	int name_has_mib;
};

static const struct darwin_sysctl_entry darwin_sysctl_entries[] = {
	{ "kern.ostype", 1, 1, DARWIN_SYSCTL_OS_TYPE, 1 },
	{ "kern.osrelease", 1, 2, DARWIN_SYSCTL_OS_RELEASE, 1 },
	{ "kern.version", 1, 4, DARWIN_SYSCTL_VERSION, 1 },
	{ "kern.argmax", 1, 8, DARWIN_SYSCTL_ARGUMENT_LIMIT, 1 },
	{ "kern.hostname", 1, 10, DARWIN_SYSCTL_HOSTNAME, 1 },
	{ "kern.osversion", 1, 65, DARWIN_SYSCTL_OS_VERSION, 1 },
	{ "kern.osproductversion", 1, 140, DARWIN_SYSCTL_PRODUCT_VERSION, 1 },
	{ "kern.osproductversioncompat", 1, 141, DARWIN_SYSCTL_PRODUCT_VERSION, 1 },
	{ "hw.machine", 6, 1, DARWIN_SYSCTL_MACHINE, 1 },
	{ "hw.model", 6, 2, DARWIN_SYSCTL_MODEL, 1 },
	{ "hw.ncpu", 6, 3, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 1 },
	{ "hw.byteorder", 6, 4, DARWIN_SYSCTL_BYTE_ORDER, 1 },
	{ "hw.memsize", 6, 24, DARWIN_SYSCTL_MEMORY_SIZE, 1 },
	{ "hw.pagesize", 6, 7, DARWIN_SYSCTL_PAGE_SIZE, 1 },
	{ "hw.logicalcpu", 6, 103, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 1 },
	{ "hw.physicalcpu", 6, 101, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 1 },
	{ "hw.activecpu", 6, 25, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 1 },
	{ "hw.availcpu", 6, 25, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 0 },
	{ "hw.logicalcpu_max", 0, 0, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 0 },
	{ "hw.physicalcpu_max", 0, 0, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 0 },
	{ "hw.optional.arm64", 0, 0, DARWIN_SYSCTL_OPTIONAL_ARM64, 0 },
	{ "machdep.cpu.core_count", 0, 0, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 0 },
	{ "machdep.cpu.thread_count", 0, 0, DARWIN_SYSCTL_ACTIVE_CPU_COUNT, 0 },
};

static const char darwin_machine[] = "arm64";
static const char darwin_model[] = "VirtualMac2,1";
static const char darwin_os_release[] = "24.6.0";
static const char darwin_os_type[] = "Darwin";
static const char darwin_os_version[] = "24G84";
static const char darwin_product_version[] = "15.6";
static const char darwin_version[] = "Darwin Kernel Version 24.6.0: MachGate";

static const struct darwin_sysctl_entry* find_named_entry(const char* name)
{
	if (!name)
		return NULL;

	for (size_t index = 0;
	     index < sizeof(darwin_sysctl_entries) / sizeof(darwin_sysctl_entries[0]);
	     index++) {
		if (strcmp(name, darwin_sysctl_entries[index].name) == 0)
			return &darwin_sysctl_entries[index];
	}
	return NULL;
}

static const struct darwin_sysctl_entry* find_mib_entry(int root, int selector)
{
	for (size_t index = 0;
	     index < sizeof(darwin_sysctl_entries) / sizeof(darwin_sysctl_entries[0]);
	     index++) {
		const struct darwin_sysctl_entry* entry = &darwin_sysctl_entries[index];

		if (entry->name_has_mib && entry->root == root &&
		    entry->selector == selector)
			return entry;
	}
	return NULL;
}

static int copy_result(const void* value, size_t value_size, void* output,
	                   size_t* output_size)
{
	if (!output_size) {
		errno = EINVAL;
		return -1;
	}
	if (!output) {
		*output_size = value_size;
		return 0;
	}
	if (*output_size < value_size) {
		*output_size = value_size;
		errno = ENOMEM;
		return -1;
	}

	memcpy(output, value, value_size);
	*output_size = value_size;
	return 0;
}

static int copy_integer(int value, void* output, size_t* output_size)
{
	return copy_result(&value, sizeof(value), output, output_size);
}

static int copy_memory_size(void* output, size_t* output_size)
{
	uint64_t value = machgate_host_memory_size();

	return copy_result(&value, sizeof(value), output, output_size);
}

static int copy_string(const char* value, void* output, size_t* output_size)
{
	return copy_result(value, strlen(value) + 1, output, output_size);
}

static int copy_hostname(void* output, size_t* output_size)
{
	char hostname[256];

	if (gethostname(hostname, sizeof(hostname)) != 0)
		memcpy(hostname, "machgate", sizeof("machgate"));
	hostname[sizeof(hostname) - 1] = '\0';
	return copy_string(hostname, output, output_size);
}

static int read_entry(const struct darwin_sysctl_entry* entry, void* output,
	                  size_t* output_size)
{
	switch (entry->value_kind) {
	case DARWIN_SYSCTL_ACTIVE_CPU_COUNT:
		return copy_integer(machgate_host_cpu_count(), output, output_size);
	case DARWIN_SYSCTL_ARGUMENT_LIMIT:
		return copy_integer(262144, output, output_size);
	case DARWIN_SYSCTL_BYTE_ORDER:
		return copy_integer(1234, output, output_size);
	case DARWIN_SYSCTL_HOSTNAME:
		return copy_hostname(output, output_size);
	case DARWIN_SYSCTL_MACHINE:
		return copy_string(darwin_machine, output, output_size);
	case DARWIN_SYSCTL_MEMORY_SIZE:
		return copy_memory_size(output, output_size);
	case DARWIN_SYSCTL_MODEL:
		return copy_string(darwin_model, output, output_size);
	case DARWIN_SYSCTL_OPTIONAL_ARM64:
		return copy_integer(1, output, output_size);
	case DARWIN_SYSCTL_OS_RELEASE:
		return copy_string(darwin_os_release, output, output_size);
	case DARWIN_SYSCTL_OS_TYPE:
		return copy_string(darwin_os_type, output, output_size);
	case DARWIN_SYSCTL_OS_VERSION:
		return copy_string(darwin_os_version, output, output_size);
	case DARWIN_SYSCTL_PAGE_SIZE:
		return copy_integer((int)machgate_host_page_size(), output, output_size);
	case DARWIN_SYSCTL_PRODUCT_VERSION:
		return copy_string(darwin_product_version, output, output_size);
	case DARWIN_SYSCTL_VERSION:
		return copy_string(darwin_version, output, output_size);
	}

	errno = ENOENT;
	return -1;
}

int sysctlnametomib(const char* name, int* mib, size_t* mib_length)
{
	if (!name || !mib_length) {
		errno = EINVAL;
		return -1;
	}

	const struct darwin_sysctl_entry* entry = find_named_entry(name);
	if (!entry || !entry->name_has_mib) {
		errno = ENOENT;
		return -1;
	}
	if (!mib || *mib_length < 2) {
		*mib_length = 2;
		errno = ENOMEM;
		return -1;
	}

	mib[0] = entry->root;
	mib[1] = entry->selector;
	*mib_length = 2;
	return 0;
}

int sysctl(const int* name, unsigned int name_length, void* output,
	   size_t* output_size, const void* replacement, size_t replacement_size)
{
	(void)replacement;
	(void)replacement_size;

	if (!name || name_length < 2) {
		errno = ENOENT;
		return -1;
	}

	const struct darwin_sysctl_entry* entry = find_mib_entry(name[0], name[1]);
	if (!entry) {
		errno = ENOENT;
		return -1;
	}
	return read_entry(entry, output, output_size);
}

int sysctlbyname(const char* name, void* output, size_t* output_size,
	         const void* replacement, size_t replacement_size)
{
	(void)replacement;
	(void)replacement_size;

	const struct darwin_sysctl_entry* entry = find_named_entry(name);
	if (!entry) {
		errno = ENOENT;
		return -1;
	}
	return read_entry(entry, output, output_size);
}

int uname(void* output)
{
	char* fields = output;

	if (!fields) {
		errno = EFAULT;
		return -1;
	}

	memset(fields, 0, 256 * 5);
	snprintf(fields, 256, "%s", darwin_os_type);
	if (gethostname(fields + 256, 256) != 0)
		snprintf(fields + 256, 256, "%s", "machgate");
	snprintf(fields + 512, 256, "%s", darwin_os_release);
	snprintf(fields + 768, 256, "%s", darwin_version);
	snprintf(fields + 1024, 256, "%s", darwin_machine);
	return 0;
}

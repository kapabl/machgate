#define _GNU_SOURCE

#include "variadic_abi_compat.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

struct aapcs64_argument_list {
	void* stack;
	void* general_register_top;
	void* vector_register_top;
	int general_register_offset;
	int vector_register_offset;
};

typedef int (*host_vfprintf_function)(
	FILE* stream, const char* format,
	struct aapcs64_argument_list* arguments);
typedef int (*host_vsnprintf_function)(
	char* buffer, size_t buffer_size, const char* format,
	struct aapcs64_argument_list* arguments);

static host_vfprintf_function find_host_vfprintf(void)
{
	union {
		void* object;
		host_vfprintf_function function;
	} symbol;

	symbol.object = dlvsym(RTLD_NEXT, "vfprintf", "GLIBC_2.17");
	if (!symbol.object)
		symbol.object = dlsym(RTLD_NEXT, "vfprintf");
	return symbol.function;
}

static host_vsnprintf_function find_host_vsnprintf(void)
{
	union {
		void* object;
		host_vsnprintf_function function;
	} symbol;

	symbol.object = dlvsym(RTLD_NEXT, "vsnprintf", "GLIBC_2.17");
	if (!symbol.object)
		symbol.object = dlsym(RTLD_NEXT, "vsnprintf");
	return symbol.function;
}

static int print_darwin_arguments(FILE* stream, const char* format,
	                          void* darwin_arguments)
{
	host_vfprintf_function host_vfprintf = find_host_vfprintf();
	if (!host_vfprintf) {
		errno = ENOSYS;
		return -1;
	}

	struct aapcs64_argument_list host_arguments = {
		.stack = darwin_arguments,
		.general_register_top = NULL,
		.vector_register_top = NULL,
		.general_register_offset = 0,
		.vector_register_offset = 0,
	};
	return host_vfprintf(stream, format, &host_arguments);
}

static int format_darwin_arguments(char* buffer, size_t buffer_size,
	                           const char* format,
	                           void* darwin_arguments)
{
	host_vsnprintf_function host_vsnprintf = find_host_vsnprintf();
	if (!host_vsnprintf) {
		errno = ENOSYS;
		return -1;
	}

	struct aapcs64_argument_list host_arguments = {
		.stack = darwin_arguments,
		.general_register_top = NULL,
		.vector_register_top = NULL,
		.general_register_offset = 0,
		.vector_register_offset = 0,
	};
	return host_vsnprintf(buffer, buffer_size, format, &host_arguments);
}

__attribute__((visibility("hidden")))
int machgate_darwin_printf_from_stack(const char* format,
	                              void* darwin_arguments)
{
	return print_darwin_arguments(stdout, format, darwin_arguments);
}

__attribute__((visibility("hidden")))
int machgate_darwin_vfprintf_from_stack(FILE* stream, const char* format,
	                               void* darwin_arguments)
{
	return print_darwin_arguments(stream, format, darwin_arguments);
}

__attribute__((visibility("hidden")))
int machgate_darwin_vsnprintf_from_stack(char* buffer, size_t buffer_size,
	                                const char* format,
	                                void* darwin_arguments)
{
	return format_darwin_arguments(
		buffer, buffer_size, format, darwin_arguments);
}

__attribute__((visibility("hidden")))
int machgate_darwin_snprintf_from_stack(char* buffer, size_t buffer_size,
	                               const char* format,
	                               void* darwin_arguments)
{
	return format_darwin_arguments(
		buffer, buffer_size, format, darwin_arguments);
}

__attribute__((visibility("hidden")))
int machgate_host_vsnprintf(char* buffer, size_t buffer_size,
	                    const char* format, va_list arguments)
{
	host_vsnprintf_function host_vsnprintf = find_host_vsnprintf();
	if (!host_vsnprintf) {
		errno = ENOSYS;
		return -1;
	}

	return host_vsnprintf(
		buffer, buffer_size, format,
		(struct aapcs64_argument_list*)&arguments);
}

__attribute__((visibility("hidden")))
int __wrap_snprintf(char* buffer, size_t buffer_size, const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	int result = machgate_host_vsnprintf(
		buffer, buffer_size, format, arguments);
	va_end(arguments);
	return result;
}

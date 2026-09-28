#ifndef MACHGATE_SHIM_VARIADIC_ABI_COMPAT_H
#define MACHGATE_SHIM_VARIADIC_ABI_COMPAT_H

#include <stdarg.h>
#include <stddef.h>

__attribute__((visibility("hidden")))
int machgate_host_vsnprintf(char* buffer, size_t buffer_size,
	                    const char* format, va_list arguments);

#endif

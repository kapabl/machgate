#ifndef MACHGATE_LOG_H
#define MACHGATE_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int machgate_verbose;

static inline int machgate_env_truthy(const char* value)
{
	return value && value[0] && strcmp(value, "0") != 0 &&
	       strcmp(value, "false") != 0 && strcmp(value, "FALSE") != 0 &&
	       strcmp(value, "no") != 0 && strcmp(value, "NO") != 0;
}

static inline int machgate_log_startup_enabled(void)
{
	return machgate_verbose ||
	       machgate_env_truthy(getenv("MACHGATE_VERBOSE")) ||
	       machgate_env_truthy(getenv("MACHGATE_LOG_STARTUP"));
}

static inline void machgate_log_startup(const char* format, ...)
{
	va_list args;

	if (!machgate_log_startup_enabled())
		return;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

#endif

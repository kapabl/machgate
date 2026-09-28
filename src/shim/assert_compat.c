#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

static const char* assertion_field(const char* value)
{
	return value ? value : "<unknown>";
}

static void write_assertion_diagnostic(const char* message, size_t length)
{
	while (length) {
		long result = syscall(SYS_write, STDERR_FILENO, message, length);
		if (result > 0) {
			message += result;
			length -= (size_t)result;
			continue;
		}
		if (result < 0 && errno == EINTR)
			continue;
		return;
	}
}

__attribute__((noreturn))
void __assert_rtn(const char* function, const char* file, int line,
		  const char* expression)
{
	char diagnostic[1024];
	int result = snprintf(
		diagnostic, sizeof(diagnostic),
		"machgate: assertion '%s' failed at %s:%d in %s\n",
		assertion_field(expression), assertion_field(file), line,
		assertion_field(function));
	if (result > 0) {
		size_t length = (size_t)result;
		if (length >= sizeof(diagnostic))
			length = sizeof(diagnostic) - 1;
		write_assertion_diagnostic(diagnostic, length);
	}
	abort();
}

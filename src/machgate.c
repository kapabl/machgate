#include "application/guest_session.h"
#include "vm_interpose.h"

#include <stdio.h>
#include <string.h>

int machgate_verbose;

static void print_usage(const char* program)
{
	fprintf(stderr, "usage: %s [-v] <arm64-mach-o> [arguments...]\n",
	        program);
}

int main(int argument_count, char** arguments, char** environment)
{
	machgate_ensure_vm_interpose(arguments, environment);

	int guest_index = 1;
	if (guest_index < argument_count &&
	    strcmp(arguments[guest_index], "-v") == 0) {
		machgate_verbose = 1;
		guest_index++;
	}
	if (guest_index >= argument_count) {
		print_usage(arguments[0]);
		return 2;
	}

	return machgate_run_guest(arguments[guest_index],
	                          argument_count - guest_index,
	                          &arguments[guest_index], environment);
}

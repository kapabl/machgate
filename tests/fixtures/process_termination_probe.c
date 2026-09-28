#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

typedef void (*termination_function)(int);

int main(int argument_count, char** arguments)
{
	if (argument_count != 3)
		return 2;

	void* handle = dlopen(arguments[1], RTLD_NOW | RTLD_LOCAL);
	if (!handle)
		return 3;

	union {
		void* object;
		termination_function function;
	} symbol;
	symbol.object = dlsym(handle, arguments[2]);
	if (!symbol.object)
		return 4;

	fputs("buffered", stdout);
	symbol.function(0);
	return 5;
}

#include "guest_allocator_bridge.h"

#include "guest_symbols.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*allocator_setter_fn)(
	void*, void*, void*, void*, void*, void*, void*, void*, void*, void*,
	void*, void*, void*, void*, void*, void*, void*, void*, void*, void*);

static allocator_setter_fn find_allocator_setter(void)
{
	const char* symbol_name = "machgate_shim_set_guest_cxx_allocators";
	allocator_setter_fn result =
		(allocator_setter_fn)dlsym(RTLD_DEFAULT, symbol_name);
	if (result)
		return result;

	const char* shim_path = getenv("MACHGATE_SYSTEM_SHIM_LIB");
	if (!shim_path || !*shim_path)
		return NULL;
	void* shim = dlopen(shim_path, RTLD_NOW | RTLD_GLOBAL);
	if (!shim)
		return NULL;
	return (allocator_setter_fn)dlsym(shim, symbol_name);
}

static void* find_operator(struct machgate_guest_image* image, const char* name)
{
	void* result = machgate_find_guest_definition(image, name);
	if (result || name[0] != '_')
		return result;

	size_t length = strlen(name);
	char* macho_name = malloc(length + 2);
	if (!macho_name)
		return NULL;
	macho_name[0] = '_';
	memcpy(macho_name + 1, name, length + 1);
	result = machgate_find_guest_definition(image, macho_name);
	free(macho_name);
	return result;
}

int machgate_configure_guest_allocator_bridge(struct machgate_guest_image* image)
{
	allocator_setter_fn setter = find_allocator_setter();
	if (!setter)
		return 0;

	void* operators[] = {
		find_operator(image, "_Znwm"),
		find_operator(image, "_Znam"),
		find_operator(image, "_ZnwmSt11align_val_t"),
		find_operator(image, "_ZnamSt11align_val_t"),
		find_operator(image, "_ZdlPv"),
		find_operator(image, "_ZdaPv"),
		find_operator(image, "_ZdlPvm"),
		find_operator(image, "_ZdaPvm"),
		find_operator(image, "_ZdlPvSt11align_val_t"),
		find_operator(image, "_ZdaPvSt11align_val_t"),
		find_operator(image, "_ZdlPvmSt11align_val_t"),
		find_operator(image, "_ZdaPvmSt11align_val_t"),
		find_operator(image, "_ZnwmRKSt9nothrow_t"),
		find_operator(image, "_ZnamRKSt9nothrow_t"),
		find_operator(image, "_ZnwmSt11align_val_tRKSt9nothrow_t"),
		find_operator(image, "_ZnamSt11align_val_tRKSt9nothrow_t"),
		find_operator(image, "_ZdlPvRKSt9nothrow_t"),
		find_operator(image, "_ZdaPvRKSt9nothrow_t"),
		find_operator(image, "_ZdlPvSt11align_val_tRKSt9nothrow_t"),
		find_operator(image, "_ZdaPvSt11align_val_tRKSt9nothrow_t"),
	};

	int configured = 0;
	for (size_t index = 0; index < sizeof(operators) / sizeof(operators[0]);
	     index++) {
		if (operators[index])
			configured++;
	}
	if (!configured)
		return 0;

	setter(operators[0], operators[1], operators[2], operators[3],
	       operators[4], operators[5], operators[6], operators[7],
	       operators[8], operators[9], operators[10], operators[11],
	       operators[12], operators[13], operators[14], operators[15],
	       operators[16], operators[17], operators[18], operators[19]);
	if (getenv("MACHGATE_TRACE_CXX_ALLOCATORS"))
		fprintf(stderr,
		        "machgate: configured %d guest C++ allocator hook%s\n",
		        configured, configured == 1 ? "" : "s");
	return configured;
}

#include <stddef.h>
#include <stdlib.h>

void* machgate_shim_malloc(size_t size);
void* machgate_shim_calloc(size_t count, size_t size);
void* machgate_shim_realloc(void* ptr, size_t size);
void machgate_shim_free(void* ptr);
int machgate_shim_posix_memalign(void** memptr, size_t alignment, size_t size);
void* machgate_shim_memalign(size_t alignment, size_t size);
void* machgate_shim_valloc(size_t size);

void* __wrap_malloc(size_t size)
{
	return machgate_shim_malloc(size);
}

void __wrap_free(void* ptr)
{
	machgate_shim_free(ptr);
}

void* __wrap_calloc(size_t count, size_t size)
{
	return machgate_shim_calloc(count, size);
}

void* __wrap_realloc(void* ptr, size_t size)
{
	return machgate_shim_realloc(ptr, size);
}

int __wrap_posix_memalign(void** memptr, size_t alignment, size_t size)
{
	return machgate_shim_posix_memalign(memptr, alignment, size);
}

void* __wrap_memalign(size_t alignment, size_t size)
{
	return machgate_shim_memalign(alignment, size);
}

void* __wrap_aligned_alloc(size_t alignment, size_t size)
{
	return machgate_shim_memalign(alignment, size);
}

void* __wrap_valloc(size_t size)
{
	return machgate_shim_valloc(size);
}

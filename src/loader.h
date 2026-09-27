#ifndef MACHGATE_LOADER_H
#define MACHGATE_LOADER_H

#include <stddef.h>
#include <stdint.h>

struct load_results {
	void* mh;
	intptr_t slide;
	uintptr_t mapping_start;
	uintptr_t mapping_end;
	uintptr_t entry_point;
};

int machgate_load_executable(const char* path, struct load_results* result,
			     char* error, size_t error_size);
void* machgate_pool_alloc(struct load_results* result, size_t size);

#endif

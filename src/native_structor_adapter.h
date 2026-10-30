#ifndef MACHGATE_NATIVE_STRUCTOR_ADAPTER_H
#define MACHGATE_NATIVE_STRUCTOR_ADAPTER_H

#include "domain/guest_image.h"

#include <stddef.h>
#include <stdint.h>

struct machgate_structor_adapters;

int machgate_native_symbol_has_structor_return(const char* guest_symbol);
int machgate_adapt_native_structor(
	struct machgate_guest_image* image,
	struct machgate_structor_adapters** adapters,
	const char* guest_symbol, uintptr_t native_target,
	uintptr_t* result, char* error, size_t error_size);
int machgate_finish_native_structor_adapters(
	struct machgate_structor_adapters* adapters,
	char* error, size_t error_size);
void machgate_discard_native_structor_adapters(
	struct machgate_structor_adapters* adapters);

#endif

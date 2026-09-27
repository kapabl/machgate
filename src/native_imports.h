#ifndef MACHGATE_NATIVE_IMPORTS_H
#define MACHGATE_NATIVE_IMPORTS_H

#include "loader.h"

int machgate_bind_native_imports(struct load_results* loaded, char* error,
				 size_t error_size);

#endif

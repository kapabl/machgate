#ifndef MACHGATE_GUEST_SYMBOLS_H
#define MACHGATE_GUEST_SYMBOLS_H

#include "loader.h"

void* machgate_find_guest_definition(struct machgate_guest_image* image,
				     const char* name);

#endif

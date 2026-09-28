#ifndef MACHGATE_DOMAIN_GUEST_IMAGE_H
#define MACHGATE_DOMAIN_GUEST_IMAGE_H

#include <stdint.h>

struct machgate_guest_image {
	void* header;
	intptr_t slide;
	uintptr_t mapping_start;
	uintptr_t mapping_end;
	uintptr_t entry_point;
};

#endif

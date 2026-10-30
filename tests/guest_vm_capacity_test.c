#include "syscall/guest_vm_track.h"

#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>

#define EXPECTED_TRACKED_MAPPINGS 64

int main(void)
{
	void* mappings[EXPECTED_TRACKED_MAPPINGS];
	for (size_t index = 0; index < EXPECTED_TRACKED_MAPPINGS; index++) {
		long result = machgate_darwin_mmap(
			NULL, 1, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (result < 0)
			return 1;
		mappings[index] = (void*)result;
	}

	errno = 0;
	long overflow = machgate_darwin_mmap(
		NULL, 1, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (overflow >= 0 || errno != ENOMEM)
		return 2;

	for (size_t index = 0; index < EXPECTED_TRACKED_MAPPINGS; index++) {
		if (machgate_darwin_munmap(mappings[index], 1) != 0)
			return 3;
	}
	return 0;
}


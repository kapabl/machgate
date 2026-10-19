#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

int machgate_structor_mapping_is_rx(void* address)
{
	FILE* maps = fopen("/proc/self/maps", "r");
	if (!maps)
		return 0;

	char line[512];
	uintptr_t target = (uintptr_t)address;
	int result = 0;
	while (fgets(line, sizeof(line), maps)) {
		unsigned long long start;
		unsigned long long end;
		char permissions[5];
		if (sscanf(line, "%llx-%llx %4s", &start, &end, permissions) != 3)
			continue;
		if (target < start || target >= end)
			continue;
		result = permissions[0] == 'r' && permissions[1] == '-' &&
		         permissions[2] == 'x';
		break;
	}
	fclose(maps);
	return result;
}

int machgate_structor_same_page(void* first, void* second)
{
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0)
		return 0;
	return (uintptr_t)first / (uintptr_t)page_size ==
	       (uintptr_t)second / (uintptr_t)page_size;
}

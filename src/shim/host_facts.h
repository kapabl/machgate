#ifndef MACHGATE_SHIM_HOST_FACTS_H
#define MACHGATE_SHIM_HOST_FACTS_H

#include <stdint.h>

__attribute__((visibility("hidden")))
int machgate_host_cpu_count(void);

__attribute__((visibility("hidden")))
uint64_t machgate_host_memory_size(void);

__attribute__((visibility("hidden")))
long machgate_host_page_size(void);

#endif

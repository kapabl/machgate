#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern void* __libc_malloc(size_t size);
extern void* __libc_calloc(size_t count, size_t size);
extern void __libc_free(void* pointer);

static atomic_int observe_allocator_calls;
static atomic_uint allocator_call_count;

void* malloc(size_t size)
{
	if (atomic_load_explicit(&observe_allocator_calls, memory_order_relaxed))
		atomic_fetch_add_explicit(&allocator_call_count, 1, memory_order_relaxed);
	return __libc_malloc(size);
}

void* calloc(size_t count, size_t size)
{
	if (atomic_load_explicit(&observe_allocator_calls, memory_order_relaxed))
		atomic_fetch_add_explicit(&allocator_call_count, 1, memory_order_relaxed);
	return __libc_calloc(count, size);
}

void free(void* pointer)
{
	if (atomic_load_explicit(&observe_allocator_calls, memory_order_relaxed))
		atomic_fetch_add_explicit(&allocator_call_count, 1, memory_order_relaxed);
	__libc_free(pointer);
}

struct tlv_descriptor {
	void* resolver;
	uintptr_t key;
	uintptr_t offset;
};

typedef void* (*tlv_bootstrap_function)(struct tlv_descriptor* descriptor);
typedef void (*tlv_atexit_function)(void (*function)(void*), void* argument);

static void unused_exit_callback(void* argument)
{
	(void)argument;
}

int main(int argc, char** argv)
{
	if (argc != 2)
		return 2;

	void* shim = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (!shim) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 3;
	}

	tlv_bootstrap_function bootstrap =
	    (tlv_bootstrap_function)dlsym(shim, "_tlv_bootstrap_impl");
	tlv_atexit_function register_exit_callback =
	    (tlv_atexit_function)dlsym(shim, "_tlv_atexit");
	if (!bootstrap || !register_exit_callback)
		return 4;

	struct tlv_descriptor descriptor = {0};
	atomic_store_explicit(&observe_allocator_calls, 1, memory_order_relaxed);
	void* storage = bootstrap(&descriptor);
	register_exit_callback(unused_exit_callback, NULL);
	atomic_store_explicit(&observe_allocator_calls, 0, memory_order_relaxed);

	if (!storage)
		return 5;
	unsigned calls = atomic_load_explicit(&allocator_call_count,
	                                     memory_order_relaxed);
	if (calls != 0) {
		fprintf(stderr,
		        "TLV bookkeeping invoked %u interposed allocator call%s\n",
		        calls, calls == 1 ? "" : "s");
		return 1;
	}
	return 0;
}

#include <dlfcn.h>
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>

typedef long (*machgate_darwin_mmap_fn)(void*, size_t, int, int, int, off_t);
typedef long (*machgate_darwin_munmap_fn)(void*, size_t);
typedef void* (*real_mmap_fn)(void*, size_t, int, int, int, off_t);
typedef int (*real_munmap_fn)(void*, size_t);

static machgate_darwin_mmap_fn guest_mmap_fn;
static machgate_darwin_munmap_fn guest_munmap_fn;
static real_mmap_fn real_mmap_fn_ptr;
static real_munmap_fn real_munmap_fn_ptr;
static int symbols_resolved;

static void resolve_symbols(void)
{
	if (symbols_resolved)
		return;
	symbols_resolved = 1;
	real_mmap_fn_ptr = (real_mmap_fn)dlsym(RTLD_NEXT, "mmap");
	real_munmap_fn_ptr = (real_munmap_fn)dlsym(RTLD_NEXT, "munmap");
	guest_mmap_fn = (machgate_darwin_mmap_fn)dlsym(RTLD_DEFAULT,
	                                               "machgate_darwin_mmap");
	guest_munmap_fn = (machgate_darwin_munmap_fn)dlsym(RTLD_DEFAULT,
	                                                   "machgate_darwin_munmap");
}

void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	void* result;

	resolve_symbols();
	if (guest_mmap_fn) {
		long mapped = guest_mmap_fn(addr, length, prot, flags, fd, offset);
		result = mapped < 0 ? MAP_FAILED : (void*)mapped;
	} else if (real_mmap_fn_ptr) {
		result = real_mmap_fn_ptr(addr, length, prot, flags, fd, offset);
	} else {
		errno = ENOSYS;
		result = MAP_FAILED;
	}
	return result;
}

int munmap(void* addr, size_t length)
{
	long result;

	resolve_symbols();
	if (guest_munmap_fn)
		result = guest_munmap_fn(addr, length);
	else if (real_munmap_fn_ptr)
		result = real_munmap_fn_ptr(addr, length);
	else {
		errno = ENOSYS;
		result = -1;
	}
	return (int)result;
}

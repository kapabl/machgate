#include "guest_vm_dispatch.h"
#include "guest_vm_track.h"

long machgate_dispatch_darwin_mmap(void* addr, size_t length, int prot, int flags,
                                   int fd, off_t offset)
{
	return machgate_darwin_mmap(addr, length, prot, flags, fd, offset);
}

long machgate_dispatch_darwin_munmap(void* addr, size_t length)
{
	return machgate_darwin_munmap(addr, length);
}
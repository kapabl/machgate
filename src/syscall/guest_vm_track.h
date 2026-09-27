#ifndef _GUEST_VM_TRACK_H_
#define _GUEST_VM_TRACK_H_

#include <stddef.h>
#include <sys/types.h>

long machgate_darwin_mmap(void* addr, size_t length, int prot, int flags,
                          int fd, off_t offset);
long machgate_darwin_munmap(void* addr, size_t length);

#endif /* _GUEST_VM_TRACK_H_ */
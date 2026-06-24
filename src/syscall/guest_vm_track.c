#include "guest_vm_track.h"

#include <errno.h>
#include <pthread.h>

#include <signal.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define GUEST_VM_TRACK_MAX 64

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

struct guest_vm_entry {
	void* addr;
	size_t mapped_size;
	int used;
};

static struct guest_vm_entry guest_vm_table[GUEST_VM_TRACK_MAX];
static pthread_mutex_t guest_vm_lock = PTHREAD_MUTEX_INITIALIZER;

static void guest_vm_track_add(void* addr, size_t length)
{
	pthread_mutex_lock(&guest_vm_lock);
	for (int index = 0; index < GUEST_VM_TRACK_MAX; index++) {
		if (guest_vm_table[index].used)
			continue;
		guest_vm_table[index].addr = addr;
		guest_vm_table[index].mapped_size = length;
		guest_vm_table[index].used = 1;
		pthread_mutex_unlock(&guest_vm_lock);
		return;
	}
	pthread_mutex_unlock(&guest_vm_lock);
}

static int guest_vm_lookup(void* addr, size_t* mapped_size)
{
	for (int index = 0; index < GUEST_VM_TRACK_MAX; index++) {
		if (!guest_vm_table[index].used)
			continue;
		if (guest_vm_table[index].addr != addr)
			continue;
		if (mapped_size)
			*mapped_size = guest_vm_table[index].mapped_size;
		return 1;
	}
	return 0;
}

static void guest_vm_forget(void* addr)
{
	pthread_mutex_lock(&guest_vm_lock);
	for (int index = 0; index < GUEST_VM_TRACK_MAX; index++) {
		if (!guest_vm_table[index].used)
			continue;
		if (guest_vm_table[index].addr != addr)
			continue;
		guest_vm_table[index].used = 0;
		break;
	}
	pthread_mutex_unlock(&guest_vm_lock);
}

long machgate_darwin_mmap(void* addr, size_t length, int prot, int flags,
                          int fd, off_t offset)
{
	errno = 0;
	long result = syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
	if (result >= 0 && !addr && length > 0 && length < PAGE_SIZE &&
	    (flags & MAP_ANONYMOUS))
		guest_vm_track_add((void*)result, length);
	return result;
}

long machgate_darwin_munmap(void* addr, size_t length)
{
	size_t mapped_size = 0;
	int tracked = guest_vm_lookup(addr, &mapped_size);

	if (tracked && length >= PAGE_SIZE) {
		errno = EINVAL;
		return -1;
	}

	sigset_t block_mask;
	sigset_t old_mask;
	sigfillset(&block_mask);
	pthread_sigmask(SIG_BLOCK, &block_mask, &old_mask);

	long result = -1;
	int tries = 0;
	do {
		errno = 0;
		result = syscall(SYS_munmap, addr, length);
		tries++;
	} while (result < 0 && errno == EINTR && tries < 8);

	pthread_sigmask(SIG_SETMASK, &old_mask, NULL);

	if (result < 0 && errno == EINTR) {
		if (tracked || length == PAGE_SIZE)
			errno = EINVAL;
		return -1;
	}

	if (result >= 0 && tracked)
		guest_vm_forget(addr);

	return result;
}
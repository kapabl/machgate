#include "guest_vm_track.h"

#include <errno.h>
#include <pthread.h>

#include <signal.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#define GUEST_SUBPAGE_MAPPING_MAX 64

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

struct guest_subpage_mapping {
	void* addr;
	size_t mapped_size;
	int used;
};

static struct guest_subpage_mapping guest_subpage_mappings[GUEST_SUBPAGE_MAPPING_MAX];
static pthread_mutex_t guest_vm_lock = PTHREAD_MUTEX_INITIALIZER;

static int guest_subpage_mapping_add(void* addr, size_t length)
{
	pthread_mutex_lock(&guest_vm_lock);
	for (int index = 0; index < GUEST_SUBPAGE_MAPPING_MAX; index++) {
		if (guest_subpage_mappings[index].used)
			continue;
		guest_subpage_mappings[index].addr = addr;
		guest_subpage_mappings[index].mapped_size = length;
		guest_subpage_mappings[index].used = 1;
		pthread_mutex_unlock(&guest_vm_lock);
		return 0;
	}
	pthread_mutex_unlock(&guest_vm_lock);
	return -1;
}

static int guest_subpage_mapping_lookup(void* addr, size_t* mapped_size)
{
	pthread_mutex_lock(&guest_vm_lock);
	for (int index = 0; index < GUEST_SUBPAGE_MAPPING_MAX; index++) {
		if (!guest_subpage_mappings[index].used)
			continue;
		if (guest_subpage_mappings[index].addr != addr)
			continue;
		if (mapped_size)
			*mapped_size = guest_subpage_mappings[index].mapped_size;
		pthread_mutex_unlock(&guest_vm_lock);
		return 1;
	}
	pthread_mutex_unlock(&guest_vm_lock);
	return 0;
}

static void guest_subpage_mapping_forget(void* addr)
{
	pthread_mutex_lock(&guest_vm_lock);
	for (int index = 0; index < GUEST_SUBPAGE_MAPPING_MAX; index++) {
		if (!guest_subpage_mappings[index].used)
			continue;
		if (guest_subpage_mappings[index].addr != addr)
			continue;
		guest_subpage_mappings[index].used = 0;
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
	    (flags & MAP_ANONYMOUS) &&
	    guest_subpage_mapping_add((void*)result, length) != 0) {
		syscall(SYS_munmap, (void*)result, length);
		errno = ENOMEM;
		return -1;
	}
	return result;
}

long machgate_darwin_munmap(void* addr, size_t length)
{
	size_t mapped_size = 0;
	int tracked = guest_subpage_mapping_lookup(addr, &mapped_size);

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
		guest_subpage_mapping_forget(addr);

	return result;
}

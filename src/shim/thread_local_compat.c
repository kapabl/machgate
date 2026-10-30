#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void* __tlv_image_base;
size_t __tlv_image_size;
size_t __tlv_bss_size;

struct machgate_tlv_descriptor {
	void* (*resolver)(struct machgate_tlv_descriptor*);
	uintptr_t key;
	uintptr_t offset;
};

struct machgate_tlv_exit_callback {
	void (*function)(void*);
	void* argument;
	struct machgate_tlv_exit_callback* next;
};

typedef int (*native_key_create_function)(pthread_key_t*, void (*)(void*));
typedef int (*native_setspecific_function)(pthread_key_t, const void*);
typedef void* (*native_getspecific_function)(pthread_key_t);
typedef void* (*native_malloc_function)(size_t);
typedef void* (*native_calloc_function)(size_t, size_t);
typedef void (*native_free_function)(void*);

static native_key_create_function native_key_create;
static native_setspecific_function native_setspecific;
static native_getspecific_function native_getspecific;
static native_malloc_function native_malloc;
static native_calloc_function native_calloc;
static native_free_function native_free;
static atomic_flag key_creation_lock = ATOMIC_FLAG_INIT;
static pthread_key_t exit_callback_key;
static int exit_callback_key_ready;

int machgate_configure_tlv_image(void* image_base, size_t image_size,
				 size_t bss_size)
{
	if ((image_size && !image_base) || image_size > SIZE_MAX - bss_size) {
		errno = EINVAL;
		return -1;
	}

	__tlv_image_base = image_base;
	__tlv_image_size = image_size;
	__tlv_bss_size = bss_size;
	return 0;
}

static void lock_key_creation(void)
{
	while (atomic_flag_test_and_set_explicit(&key_creation_lock,
	                                         memory_order_acquire))
		sched_yield();
}

static void unlock_key_creation(void)
{
	atomic_flag_clear_explicit(&key_creation_lock, memory_order_release);
}

static int resolve_native_thread_storage(void)
{
	if (!native_key_create)
		native_key_create = (native_key_create_function)dlsym(
			RTLD_NEXT, "pthread_key_create");
	if (!native_setspecific)
		native_setspecific = (native_setspecific_function)dlsym(
			RTLD_NEXT, "pthread_setspecific");
	if (!native_getspecific)
		native_getspecific = (native_getspecific_function)dlsym(
			RTLD_NEXT, "pthread_getspecific");
	if (!native_malloc)
		native_malloc = (native_malloc_function)dlsym(RTLD_NEXT, "malloc");
	if (!native_calloc)
		native_calloc = (native_calloc_function)dlsym(RTLD_NEXT, "calloc");
	if (!native_free)
		native_free = (native_free_function)dlsym(RTLD_NEXT, "free");
	if (!native_key_create || !native_setspecific || !native_getspecific ||
	    !native_malloc || !native_calloc || !native_free) {
		errno = ENOSYS;
		return -1;
	}
	return 0;
}

static int descriptor_storage_key(struct machgate_tlv_descriptor* descriptor,
	                              pthread_key_t* output)
{
	uintptr_t encoded = __atomic_load_n(&descriptor->key, __ATOMIC_ACQUIRE);

	if (encoded) {
		*output = (pthread_key_t)(encoded - 1);
		return 0;
	}

	lock_key_creation();
	encoded = __atomic_load_n(&descriptor->key, __ATOMIC_RELAXED);
	if (!encoded) {
		pthread_key_t new_key;
		int result = resolve_native_thread_storage();

		if (result == 0)
			result = native_key_create(&new_key, native_free);
		if (result != 0) {
			if (result > 0)
				errno = result;
			unlock_key_creation();
			return -1;
		}
		encoded = (uintptr_t)new_key + 1;
		__atomic_store_n(&descriptor->key, encoded, __ATOMIC_RELEASE);
	}
	unlock_key_creation();
	*output = (pthread_key_t)(encoded - 1);
	return 0;
}

static size_t required_storage_size(const struct machgate_tlv_descriptor* descriptor)
{
	if (__tlv_image_size > SIZE_MAX - __tlv_bss_size)
		return 0;

	size_t result = __tlv_image_size + __tlv_bss_size;
	if (descriptor->offset >= result) {
		if (descriptor->offset == UINTPTR_MAX)
			return 0;
		result = (size_t)descriptor->offset + 1;
	}
	if (result < 4096)
		result = 4096;
	return result;
}

void* _tlv_bootstrap_impl(struct machgate_tlv_descriptor* descriptor)
{
	if (!descriptor) {
		errno = EINVAL;
		return NULL;
	}

	pthread_key_t storage_key;
	if (descriptor_storage_key(descriptor, &storage_key) != 0)
		return NULL;

	void* storage = native_getspecific(storage_key);
	if (!storage) {
		size_t storage_size = required_storage_size(descriptor);

		if (!storage_size) {
			errno = EOVERFLOW;
			return NULL;
		}
		storage = native_calloc(1, storage_size);
		if (!storage)
			return NULL;
		if (__tlv_image_base && __tlv_image_size)
			memcpy(storage, __tlv_image_base, __tlv_image_size);
		int result = native_setspecific(storage_key, storage);
		if (result != 0) {
			native_free(storage);
			errno = result;
			return NULL;
		}
	}
	return (unsigned char*)storage + descriptor->offset;
}

static void run_exit_callbacks(void* list_pointer)
{
	struct machgate_tlv_exit_callback* callback = list_pointer;

	while (callback) {
		struct machgate_tlv_exit_callback* next = callback->next;
		callback->function(callback->argument);
		native_free(callback);
		callback = next;
	}
}

static int ensure_exit_callback_key(void)
{
	if (__atomic_load_n(&exit_callback_key_ready, __ATOMIC_ACQUIRE))
		return 0;

	lock_key_creation();
	if (!exit_callback_key_ready) {
		int result = resolve_native_thread_storage();

		if (result == 0)
			result = native_key_create(&exit_callback_key, run_exit_callbacks);
		if (result != 0) {
			if (result > 0)
				errno = result;
			unlock_key_creation();
			return -1;
		}
		__atomic_store_n(&exit_callback_key_ready, 1, __ATOMIC_RELEASE);
	}
	unlock_key_creation();
	return 0;
}

void _tlv_atexit(void (*function)(void*), void* argument)
{
	if (!function || ensure_exit_callback_key() != 0)
		return;

	struct machgate_tlv_exit_callback* callback = native_malloc(sizeof(*callback));
	if (!callback)
		return;
	callback->function = function;
	callback->argument = argument;
	callback->next = native_getspecific(exit_callback_key);
	if (native_setspecific(exit_callback_key, callback) != 0)
		native_free(callback);
}

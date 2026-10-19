#define _GNU_SOURCE

#include "darwin_errno.h"

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

enum guest_pthread_signature {
	GUEST_MUTEX_ERROR_CHECK = 0x32AAABA1,
	GUEST_MUTEX_RECURSIVE = 0x32AAABA2,
	GUEST_MUTEX_FIRST_FIT = 0x32AAABA3,
	GUEST_MUTEX_DEFAULT = 0x32AAABA7,
	GUEST_CONDITION = 0x3CB0B1BB,
	GUEST_READ_WRITE_LOCK = 0x2DA8B3B4,
};

struct native_pthread_sync_api {
	int (*mutex_lock)(pthread_mutex_t*);
	int (*mutex_unlock)(pthread_mutex_t*);
	int (*mutex_trylock)(pthread_mutex_t*);
	int (*mutex_init)(pthread_mutex_t*, const pthread_mutexattr_t*);
	int (*mutex_destroy)(pthread_mutex_t*);
	int (*condition_wait)(pthread_cond_t*, pthread_mutex_t*);
	int (*condition_timedwait)(pthread_cond_t*, pthread_mutex_t*,
	                           const struct timespec*);
	int (*condition_clockwait)(pthread_cond_t*, pthread_mutex_t*, clockid_t,
	                           const struct timespec*);
	int (*condition_signal)(pthread_cond_t*);
	int (*condition_broadcast)(pthread_cond_t*);
	int (*condition_init)(pthread_cond_t*, const pthread_condattr_t*);
	int (*condition_destroy)(pthread_cond_t*);
	int (*read_lock)(pthread_rwlock_t*);
	int (*write_lock)(pthread_rwlock_t*);
	int (*read_write_unlock)(pthread_rwlock_t*);
	int (*read_write_init)(pthread_rwlock_t*, const pthread_rwlockattr_t*);
};

static struct native_pthread_sync_api native_api;
static _Atomic int native_api_state;
static atomic_flag object_initialization_lock = ATOMIC_FLAG_INIT;

static void* next_symbol(const char* name)
{
	return dlsym(RTLD_NEXT, name);
}

static int resolve_native_api(void)
{
	int state = atomic_load_explicit(&native_api_state, memory_order_acquire);

	if (state == 2)
		return 0;
	int expected = 0;
	if (atomic_compare_exchange_strong_explicit(
		    &native_api_state, &expected, 1, memory_order_acquire,
		    memory_order_relaxed)) {
		native_api.mutex_lock = next_symbol("pthread_mutex_lock");
		native_api.mutex_unlock = next_symbol("pthread_mutex_unlock");
		native_api.mutex_trylock = next_symbol("pthread_mutex_trylock");
		native_api.mutex_init = next_symbol("pthread_mutex_init");
		native_api.mutex_destroy = next_symbol("pthread_mutex_destroy");
		native_api.condition_wait = next_symbol("pthread_cond_wait");
		native_api.condition_timedwait = next_symbol("pthread_cond_timedwait");
		native_api.condition_clockwait = next_symbol("pthread_cond_clockwait");
		native_api.condition_signal = next_symbol("pthread_cond_signal");
		native_api.condition_broadcast = next_symbol("pthread_cond_broadcast");
		native_api.condition_init = next_symbol("pthread_cond_init");
		native_api.condition_destroy = next_symbol("pthread_cond_destroy");
		native_api.read_lock = next_symbol("pthread_rwlock_rdlock");
		native_api.write_lock = next_symbol("pthread_rwlock_wrlock");
		native_api.read_write_unlock = next_symbol("pthread_rwlock_unlock");
		native_api.read_write_init = next_symbol("pthread_rwlock_init");
		if (!native_api.mutex_lock || !native_api.mutex_unlock ||
		    !native_api.mutex_trylock || !native_api.mutex_init ||
		    !native_api.mutex_destroy || !native_api.condition_wait ||
		    !native_api.condition_timedwait || !native_api.condition_signal ||
		    !native_api.condition_broadcast || !native_api.condition_init ||
		    !native_api.condition_destroy || !native_api.read_lock ||
		    !native_api.write_lock || !native_api.read_write_unlock ||
		    !native_api.read_write_init) {
			atomic_store_explicit(&native_api_state, 0, memory_order_release);
			return ENOSYS;
		}
		atomic_store_explicit(&native_api_state, 2, memory_order_release);
		return 0;
	}
	while (atomic_load_explicit(&native_api_state, memory_order_acquire) == 1)
		sched_yield();
	return atomic_load_explicit(&native_api_state, memory_order_acquire) == 2
		? 0 : ENOSYS;
}

static void lock_object_initialization(void)
{
	while (atomic_flag_test_and_set_explicit(&object_initialization_lock,
	                                         memory_order_acquire))
		sched_yield();
}

static void unlock_object_initialization(void)
{
	atomic_flag_clear_explicit(&object_initialization_lock,
	                           memory_order_release);
}

static int is_guest_mutex_signature(uint32_t signature)
{
	return signature == GUEST_MUTEX_ERROR_CHECK ||
	       signature == GUEST_MUTEX_RECURSIVE ||
	       signature == GUEST_MUTEX_FIRST_FIT ||
	       signature == GUEST_MUTEX_DEFAULT;
}

static int initialize_guest_mutex(pthread_mutex_t* mutex)
{
	uint32_t signature;
	lock_object_initialization();
	memcpy(&signature, mutex, sizeof(signature));
	int result = 0;
	if (is_guest_mutex_signature(signature)) {
		pthread_mutexattr_t attributes;
		result = pthread_mutexattr_init(&attributes);
		int attributes_initialized = result == 0;

		if (result == 0) {
			int type = PTHREAD_MUTEX_NORMAL;
			if (signature == GUEST_MUTEX_ERROR_CHECK)
				type = PTHREAD_MUTEX_ERRORCHECK;
			else if (signature == GUEST_MUTEX_RECURSIVE)
				type = PTHREAD_MUTEX_RECURSIVE;
			result = pthread_mutexattr_settype(&attributes, type);
		}
		if (result == 0) {
			memset(mutex, 0, sizeof(*mutex));
			result = native_api.mutex_init(mutex, &attributes);
		}
		if (attributes_initialized)
			pthread_mutexattr_destroy(&attributes);
	}
	unlock_object_initialization();
	return result;
}

static int initialize_guest_condition(pthread_cond_t* condition)
{
	uint32_t signature;
	lock_object_initialization();
	memcpy(&signature, condition, sizeof(signature));
	int result = 0;
	if (signature == GUEST_CONDITION) {
		memset(condition, 0, sizeof(*condition));
		result = native_api.condition_init(condition, NULL);
	}
	unlock_object_initialization();
	return result;
}

static int initialize_guest_read_write_lock(pthread_rwlock_t* lock)
{
	uint32_t signature;
	lock_object_initialization();
	memcpy(&signature, lock, sizeof(signature));
	int result = 0;
	if (signature == GUEST_READ_WRITE_LOCK) {
		memset(lock, 0, sizeof(*lock));
		result = native_api.read_write_init(lock, NULL);
	}
	unlock_object_initialization();
	return result;
}

int machgate_darwin_pthread_mutex_lock(pthread_mutex_t* mutex)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_mutex(mutex);
	if (result != 0)
		return result;
	return native_api.mutex_lock(mutex);
}

int machgate_darwin_pthread_mutex_unlock(pthread_mutex_t* mutex)
{
	int result = resolve_native_api();
	if (result != 0)
		return result;
	return native_api.mutex_unlock(mutex);
}

int machgate_darwin_pthread_mutex_trylock(pthread_mutex_t* mutex)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_mutex(mutex);
	if (result != 0)
		return result;
	return native_api.mutex_trylock(mutex);
}

int machgate_darwin_pthread_mutex_init(
	pthread_mutex_t* mutex, const pthread_mutexattr_t* attributes)
{
	int result = resolve_native_api();
	if (result != 0)
		return result;
	memset(mutex, 0, sizeof(*mutex));
	return native_api.mutex_init(mutex, attributes);
}

int machgate_darwin_pthread_mutex_destroy(pthread_mutex_t* mutex)
{
	int result = resolve_native_api();
	if (result != 0)
		return result;
	return native_api.mutex_destroy(mutex);
}

int machgate_darwin_pthread_cond_wait(pthread_cond_t* condition,
	                              pthread_mutex_t* mutex)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_condition(condition);
	if (result == 0)
		result = initialize_guest_mutex(mutex);
	if (result != 0)
		return result;
	return native_api.condition_wait(condition, mutex);
}

int machgate_darwin_pthread_cond_timedwait(
	pthread_cond_t* condition, pthread_mutex_t* mutex,
	const struct timespec* deadline)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_condition(condition);
	if (result == 0)
		result = initialize_guest_mutex(mutex);
	if (result != 0)
		return result;
	return native_api.condition_timedwait(condition, mutex, deadline);
}

int machgate_darwin_pthread_cond_clockwait(
	pthread_cond_t* condition, pthread_mutex_t* mutex, clockid_t clock,
	const struct timespec* deadline)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_condition(condition);
	if (result == 0)
		result = initialize_guest_mutex(mutex);
	if (result != 0)
		return result;
	if (!native_api.condition_clockwait)
		return ENOSYS;
	return native_api.condition_clockwait(condition, mutex, clock, deadline);
}

int machgate_darwin_pthread_cond_signal(pthread_cond_t* condition)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_condition(condition);
	if (result != 0)
		return result;
	return native_api.condition_signal(condition);
}

int machgate_darwin_pthread_cond_broadcast(pthread_cond_t* condition)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_condition(condition);
	if (result != 0)
		return result;
	return native_api.condition_broadcast(condition);
}

int pthread_cond_init(pthread_cond_t* condition,
	              const pthread_condattr_t* attributes)
{
	int result = resolve_native_api();
	if (result != 0)
		return machgate_darwin_errno_from_linux(result);
	memset(condition, 0, sizeof(*condition));
	return machgate_darwin_errno_from_linux(
		native_api.condition_init(condition, attributes));
}

int machgate_darwin_pthread_cond_destroy(pthread_cond_t* condition)
{
	int result = resolve_native_api();
	if (result != 0)
		return result;
	return native_api.condition_destroy(condition);
}

int pthread_mutex_lock(pthread_mutex_t* mutex)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_mutex_lock(mutex));
}

int pthread_mutex_unlock(pthread_mutex_t* mutex)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_mutex_unlock(mutex));
}

int pthread_mutex_trylock(pthread_mutex_t* mutex)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_mutex_trylock(mutex));
}

int pthread_mutex_init(pthread_mutex_t* mutex,
	               const pthread_mutexattr_t* attributes)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_mutex_init(mutex, attributes));
}

int pthread_mutex_destroy(pthread_mutex_t* mutex)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_mutex_destroy(mutex));
}

int pthread_cond_wait(pthread_cond_t* condition, pthread_mutex_t* mutex)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_cond_wait(condition, mutex));
}

int pthread_cond_timedwait(pthread_cond_t* condition, pthread_mutex_t* mutex,
	                   const struct timespec* deadline)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_cond_timedwait(condition, mutex, deadline));
}

int pthread_cond_signal(pthread_cond_t* condition)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_cond_signal(condition));
}

int pthread_cond_broadcast(pthread_cond_t* condition)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_cond_broadcast(condition));
}

int pthread_cond_destroy(pthread_cond_t* condition)
{
	return machgate_darwin_errno_from_linux(
		machgate_darwin_pthread_cond_destroy(condition));
}

int pthread_rwlock_rdlock(pthread_rwlock_t* lock)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_read_write_lock(lock);
	if (result != 0)
		return machgate_darwin_errno_from_linux(result);
	return machgate_darwin_errno_from_linux(native_api.read_lock(lock));
}

int pthread_rwlock_wrlock(pthread_rwlock_t* lock)
{
	int result = resolve_native_api();
	if (result == 0)
		result = initialize_guest_read_write_lock(lock);
	if (result != 0)
		return machgate_darwin_errno_from_linux(result);
	return machgate_darwin_errno_from_linux(native_api.write_lock(lock));
}

int pthread_rwlock_unlock(pthread_rwlock_t* lock)
{
	int result = resolve_native_api();
	if (result != 0)
		return machgate_darwin_errno_from_linux(result);
	return machgate_darwin_errno_from_linux(
		native_api.read_write_unlock(lock));
}

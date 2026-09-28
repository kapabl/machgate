#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum machgate_dispatch_kind {
	MACHGATE_DISPATCH_SEMAPHORE = 1,
	MACHGATE_DISPATCH_QUEUE = 2,
	MACHGATE_DISPATCH_DATA = 3,
};

struct machgate_dispatch_header {
	int kind;
	_Atomic int reference_count;
};

struct machgate_dispatch_semaphore {
	struct machgate_dispatch_header header;
	sem_t native_semaphore;
};

struct machgate_dispatch_queue {
	struct machgate_dispatch_header header;
	char label[64];
};

struct machgate_dispatch_data {
	struct machgate_dispatch_header header;
	const void* bytes;
	size_t size;
};

struct machgate_dispatch_source_type {
	int availability;
};

const struct machgate_dispatch_source_type _dispatch_source_type_mach_recv = {
	0,
};

struct machgate_block_literal {
	void* isa;
	uint32_t flags;
	uint32_t reserved;
	void (*invoke)(void*);
	void* descriptor;
};

_Static_assert(sizeof(struct machgate_dispatch_queue) == 72,
	       "dispatch queue compatibility object size must remain stable");

struct machgate_dispatch_queue _dispatch_main_q = {
	{ MACHGATE_DISPATCH_QUEUE, ATOMIC_VAR_INIT(INT_MAX) },
	"com.apple.main-thread",
};

static struct machgate_dispatch_queue global_default_queue = {
	{ MACHGATE_DISPATCH_QUEUE, ATOMIC_VAR_INIT(INT_MAX) },
	"com.apple.root.default",
};

static uint64_t read_clock_nanoseconds(clockid_t clock)
{
	struct timespec current;

	if (clock_gettime(clock, &current) != 0)
		return 0;
	return (uint64_t)current.tv_sec * 1000000000ULL +
	       (uint64_t)current.tv_nsec;
}

static struct timespec realtime_deadline(uint64_t remaining_nanoseconds)
{
	struct timespec result;

	clock_gettime(CLOCK_REALTIME, &result);
	result.tv_sec += (time_t)(remaining_nanoseconds / 1000000000ULL);
	result.tv_nsec += (long)(remaining_nanoseconds % 1000000000ULL);
	if (result.tv_nsec >= 1000000000L) {
		result.tv_sec++;
		result.tv_nsec -= 1000000000L;
	}
	return result;
}

void* dispatch_semaphore_create(long value)
{
	if (value < 0) {
		errno = EINVAL;
		return NULL;
	}

	struct machgate_dispatch_semaphore* result = calloc(1, sizeof(*result));
	if (!result)
		return NULL;
	result->header.kind = MACHGATE_DISPATCH_SEMAPHORE;
	atomic_init(&result->header.reference_count, 1);
	if (sem_init(&result->native_semaphore, 0, (unsigned int)value) != 0) {
		free(result);
		return NULL;
	}
	return result;
}

long dispatch_semaphore_wait(void* semaphore, uint64_t deadline)
{
	struct machgate_dispatch_semaphore* value = semaphore;

	if (!value) {
		errno = EINVAL;
		return -1;
	}
	if (deadline == 0)
		return sem_trywait(&value->native_semaphore);
	if (deadline == UINT64_MAX) {
		int result;
		do {
			result = sem_wait(&value->native_semaphore);
		} while (result != 0 && errno == EINTR);
		return result;
	}

	uint64_t now = read_clock_nanoseconds(CLOCK_MONOTONIC);
	if (deadline <= now)
		return sem_trywait(&value->native_semaphore);
	struct timespec absolute = realtime_deadline(deadline - now);
	int result;
	do {
		result = sem_timedwait(&value->native_semaphore, &absolute);
	} while (result != 0 && errno == EINTR);
	return result;
}

long dispatch_semaphore_signal(void* semaphore)
{
	struct machgate_dispatch_semaphore* value = semaphore;

	if (!value)
		return 0;
	return sem_post(&value->native_semaphore);
}

void* dispatch_get_global_queue(long priority, unsigned long flags)
{
	(void)priority;
	(void)flags;
	return &global_default_queue;
}

void* dispatch_queue_create(const char* label, void* attributes)
{
	(void)attributes;
	struct machgate_dispatch_queue* result = calloc(1, sizeof(*result));

	if (!result)
		return NULL;
	result->header.kind = MACHGATE_DISPATCH_QUEUE;
	atomic_init(&result->header.reference_count, 1);
	if (label) {
		size_t length = strnlen(label, sizeof(result->label) - 1);
		memcpy(result->label, label, length);
	}
	return result;
}

static void invoke_block(void* block_pointer)
{
	struct machgate_block_literal* block = block_pointer;

	if (block && block->invoke)
		block->invoke(block);
}

void dispatch_async(void* queue, void* block)
{
	(void)queue;
	invoke_block(block);
}

void dispatch_sync(void* queue, void* block)
{
	(void)queue;
	invoke_block(block);
}

static int begin_once(long* predicate)
{
	long expected = 0;

	if (__atomic_compare_exchange_n(predicate, &expected, 1, 0,
	                                __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
		return 1;
	while (__atomic_load_n(predicate, __ATOMIC_ACQUIRE) != -1)
		sched_yield();
	return 0;
}

static void complete_once(long* predicate)
{
	__atomic_store_n(predicate, -1, __ATOMIC_RELEASE);
}

void dispatch_once(long* predicate, void* block)
{
	if (!predicate)
		return;
	if (begin_once(predicate)) {
		invoke_block(block);
		complete_once(predicate);
	}
}

void dispatch_once_f(long* predicate, void* context,
	             void (*function)(void*))
{
	if (!predicate)
		return;
	if (begin_once(predicate)) {
		if (function)
			function(context);
		complete_once(predicate);
	}
}

void* dispatch_source_create(const void* type, uintptr_t handle,
	                     uintptr_t mask, void* queue)
{
	(void)type;
	(void)handle;
	(void)mask;
	(void)queue;
	errno = ENOTSUP;
	return NULL;
}

void dispatch_source_set_event_handler(void* source, void* handler)
{
	(void)source;
	(void)handler;
}

void dispatch_resume(void* object)
{
	(void)object;
}

uint64_t dispatch_time(uint64_t base, int64_t delta)
{
	if (base == UINT64_MAX)
		return UINT64_MAX;
	if (base == 0)
		base = read_clock_nanoseconds(CLOCK_MONOTONIC);
	if (delta >= 0) {
		uint64_t addition = (uint64_t)delta;
		if (addition > UINT64_MAX - base)
			return UINT64_MAX;
		return base + addition;
	}

	uint64_t subtraction = (uint64_t)(-(delta + 1)) + 1;
	if (subtraction >= base)
		return 1;
	return base - subtraction;
}

uint64_t dispatch_walltime(const struct timespec* time, int64_t delta)
{
	uint64_t base;

	if (time)
		base = (uint64_t)time->tv_sec * 1000000000ULL +
		       (uint64_t)time->tv_nsec;
	else
		base = read_clock_nanoseconds(CLOCK_REALTIME);
	return dispatch_time(base, delta);
}

void* dispatch_data_create(const void* buffer, size_t size, void* queue,
	                   void* destructor)
{
	(void)queue;
	(void)destructor;
	struct machgate_dispatch_data* result = calloc(1, sizeof(*result));

	if (!result)
		return NULL;
	result->header.kind = MACHGATE_DISPATCH_DATA;
	atomic_init(&result->header.reference_count, 1);
	result->bytes = buffer;
	result->size = size;
	return result;
}

void dispatch_retain(void* object)
{
	struct machgate_dispatch_header* header = object;

	if (!header || object == &_dispatch_main_q || object == &global_default_queue)
		return;
	atomic_fetch_add_explicit(&header->reference_count, 1,
	                          memory_order_relaxed);
}

void dispatch_release(void* object)
{
	struct machgate_dispatch_header* header = object;

	if (!header || object == &_dispatch_main_q || object == &global_default_queue)
		return;
	int previous = atomic_fetch_sub_explicit(&header->reference_count, 1,
	                                         memory_order_acq_rel);
	if (previous != 1)
		return;
	if (header->kind == MACHGATE_DISPATCH_SEMAPHORE) {
		struct machgate_dispatch_semaphore* semaphore = object;
		sem_destroy(&semaphore->native_semaphore);
	}
	free(object);
}

#include <errno.h>
#include <sched.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#define MACH_SYNC_SLOT_COUNT 256u
#define MACH_SYNC_SLOT_MASK 0xffffu

#define KERN_SUCCESS 0
#define KERN_RESOURCE_SHORTAGE 6
#define KERN_INVALID_ARGUMENT 4
#define KERN_FAILURE 5
#define KERN_ABORTED 14
#define KERN_OPERATION_TIMED_OUT 49

typedef uint32_t mach_port_name_t;
typedef int32_t kern_return_t;

typedef struct {
    uint32_t tv_sec;
    int32_t tv_nsec;
} mach_timespec_t;

typedef struct {
    unsigned char mig_version;
    unsigned char interface_version;
    unsigned char reserved;
    unsigned char protocol;
    unsigned char integer_representation;
    unsigned char character_representation;
    unsigned char float_representation;
    unsigned char trailing_reserved;
} mach_ndr_record_t;

typedef struct {
    sem_t semaphore;
    uint16_t generation;
    uint16_t references;
    int active;
} mach_sync_slot_t;

mach_ndr_record_t NDR_record = {
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    0,
};

static mach_sync_slot_t mach_sync_slots[MACH_SYNC_SLOT_COUNT];
static atomic_flag mach_sync_lock = ATOMIC_FLAG_INIT;

static void lock_mach_sync_slots(void)
{
    while (atomic_flag_test_and_set_explicit(&mach_sync_lock,
                                              memory_order_acquire)) {
        sched_yield();
    }
}

static void unlock_mach_sync_slots(void)
{
    atomic_flag_clear_explicit(&mach_sync_lock, memory_order_release);
}

static mach_port_name_t make_mach_sync_handle(uint32_t slot_index,
                                              uint16_t generation)
{
    return ((uint32_t)generation << 16) | (slot_index + 1u);
}

static mach_sync_slot_t* acquire_mach_sync_slot(mach_port_name_t handle)
{
    uint32_t encoded_slot = handle & MACH_SYNC_SLOT_MASK;
    uint16_t generation = (uint16_t)(handle >> 16);
    if (encoded_slot == 0 || encoded_slot > MACH_SYNC_SLOT_COUNT ||
        generation == 0) {
        return NULL;
    }

    mach_sync_slot_t* slot = &mach_sync_slots[encoded_slot - 1u];
    lock_mach_sync_slots();
    if (!slot->active || slot->generation != generation) {
        unlock_mach_sync_slots();
        return NULL;
    }
    slot->references++;
    unlock_mach_sync_slots();
    return slot;
}

static void release_mach_sync_slot(mach_sync_slot_t* slot)
{
    lock_mach_sync_slots();
    slot->references--;
    unlock_mach_sync_slots();
}

kern_return_t semaphore_create(mach_port_name_t task,
                               mach_port_name_t* semaphore,
                               int policy,
                               int value)
{
    (void)task;
    (void)policy;
    if (semaphore == NULL || value < 0) {
        return KERN_INVALID_ARGUMENT;
    }

    lock_mach_sync_slots();
    for (uint32_t slot_index = 0; slot_index < MACH_SYNC_SLOT_COUNT;
         slot_index++) {
        mach_sync_slot_t* slot = &mach_sync_slots[slot_index];
        if (slot->active) {
            continue;
        }
        if (sem_init(&slot->semaphore, 0, (unsigned int)value) != 0) {
            unlock_mach_sync_slots();
            return errno == ENOSPC ? KERN_RESOURCE_SHORTAGE :
                                     KERN_INVALID_ARGUMENT;
        }
        slot->generation++;
        if (slot->generation == 0) {
            slot->generation = 1;
        }
        slot->references = 0;
        slot->active = 1;
        *semaphore = make_mach_sync_handle(slot_index, slot->generation);
        unlock_mach_sync_slots();
        return KERN_SUCCESS;
    }
    unlock_mach_sync_slots();
    return KERN_RESOURCE_SHORTAGE;
}

kern_return_t semaphore_destroy(mach_port_name_t task,
                                mach_port_name_t semaphore)
{
    (void)task;
    uint32_t encoded_slot = semaphore & MACH_SYNC_SLOT_MASK;
    uint16_t generation = (uint16_t)(semaphore >> 16);
    if (encoded_slot == 0 || encoded_slot > MACH_SYNC_SLOT_COUNT ||
        generation == 0) {
        return KERN_INVALID_ARGUMENT;
    }

    lock_mach_sync_slots();
    mach_sync_slot_t* slot = &mach_sync_slots[encoded_slot - 1u];
    if (!slot->active || slot->generation != generation) {
        unlock_mach_sync_slots();
        return KERN_INVALID_ARGUMENT;
    }
    if (slot->references != 0) {
        unlock_mach_sync_slots();
        return KERN_FAILURE;
    }
    if (sem_destroy(&slot->semaphore) != 0) {
        unlock_mach_sync_slots();
        return KERN_FAILURE;
    }
    slot->active = 0;
    unlock_mach_sync_slots();
    return KERN_SUCCESS;
}

kern_return_t semaphore_signal(mach_port_name_t semaphore)
{
    mach_sync_slot_t* slot = acquire_mach_sync_slot(semaphore);
    if (slot == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    int result = sem_post(&slot->semaphore);
    release_mach_sync_slot(slot);
    return result == 0 ? KERN_SUCCESS : KERN_FAILURE;
}

kern_return_t semaphore_wait(mach_port_name_t semaphore)
{
    mach_sync_slot_t* slot = acquire_mach_sync_slot(semaphore);
    if (slot == NULL) {
        return KERN_INVALID_ARGUMENT;
    }
    int result;
    do {
        result = sem_wait(&slot->semaphore);
    } while (result != 0 && errno == EINTR);
    release_mach_sync_slot(slot);
    return result == 0 ? KERN_SUCCESS : KERN_ABORTED;
}

static int add_mach_duration(struct timespec* deadline,
                             mach_timespec_t duration)
{
    if (duration.tv_nsec < 0 || duration.tv_nsec >= 1000000000) {
        return -1;
    }
    deadline->tv_sec += (time_t)duration.tv_sec;
    deadline->tv_nsec += duration.tv_nsec;
    if (deadline->tv_nsec >= 1000000000) {
        deadline->tv_sec++;
        deadline->tv_nsec -= 1000000000;
    }
    return 0;
}

kern_return_t semaphore_timedwait(mach_port_name_t semaphore,
                                  mach_timespec_t wait_time)
{
    mach_sync_slot_t* slot = acquire_mach_sync_slot(semaphore);
    if (slot == NULL) {
        return KERN_INVALID_ARGUMENT;
    }

    struct timespec deadline;
    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0 ||
        add_mach_duration(&deadline, wait_time) != 0) {
        release_mach_sync_slot(slot);
        return KERN_INVALID_ARGUMENT;
    }

    int result;
    do {
        result = sem_timedwait(&slot->semaphore, &deadline);
    } while (result != 0 && errno == EINTR);
    int wait_errno = errno;
    release_mach_sync_slot(slot);
    if (result == 0) {
        return KERN_SUCCESS;
    }
    return wait_errno == ETIMEDOUT ? KERN_OPERATION_TIMED_OUT : KERN_ABORTED;
}

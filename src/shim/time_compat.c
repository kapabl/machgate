#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

struct machgate_timebase_info {
	uint32_t numer;
	uint32_t denom;
};

static int host_clock_gettime(clockid_t host_clock, struct timespec* now)
{
	return (int)syscall(SYS_clock_gettime, host_clock, now);
}

static int host_clock_getres(clockid_t host_clock, struct timespec* resolution)
{
	return (int)syscall(SYS_clock_getres, host_clock, resolution);
}

static uint64_t nanoseconds_from_host_clock(clockid_t host_clock)
{
	struct timespec now = {0};
	if (host_clock_gettime(host_clock, &now) != 0)
		return 0;

	const uint64_t nanoseconds_per_second = 1000000000u;
	return (uint64_t)now.tv_sec * nanoseconds_per_second +
	       (uint64_t)now.tv_nsec;
}

static clockid_t host_clock_for_darwin_clock(int darwin_clock)
{
	switch (darwin_clock) {
	case 0:
		return CLOCK_REALTIME;
	case 12:
		return CLOCK_PROCESS_CPUTIME_ID;
	case 16:
		return CLOCK_THREAD_CPUTIME_ID;
	default:
		return CLOCK_MONOTONIC;
	}
}

int clock_gettime(int darwin_clock, struct timespec* now)
{
	if (!now)
		return -1;
	return host_clock_gettime(host_clock_for_darwin_clock(darwin_clock),
	                          now);
}

int clock_getres(int darwin_clock, struct timespec* resolution)
{
	if (!resolution)
		return -1;
	return host_clock_getres(host_clock_for_darwin_clock(darwin_clock),
	                         resolution);
}

uint64_t mach_absolute_time(void)
{
	return nanoseconds_from_host_clock(CLOCK_MONOTONIC);
}

int mach_timebase_info(struct machgate_timebase_info* info)
{
	if (!info)
		return 4;

	struct machgate_timebase_info arm64_timebase = {
		.numer = 1,
		.denom = 1,
	};
	*info = arm64_timebase;
	return 0;
}

uint64_t clock_gettime_nsec_np(int darwin_clock)
{
	clockid_t host_clock = host_clock_for_darwin_clock(darwin_clock);
	return nanoseconds_from_host_clock(host_clock);
}

uint64_t mach_continuous_time(void)
{
	return mach_absolute_time();
}

uint64_t mach_approximate_time(void)
{
	return mach_absolute_time();
}

uint64_t mach_continuous_approximate_time(void)
{
	return mach_continuous_time();
}

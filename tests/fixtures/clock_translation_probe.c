#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef int (*clock_gettime_fn)(int, struct timespec*);
typedef int (*clock_getres_fn)(int, struct timespec*);
typedef unsigned long long (*nsec_fn)(int);

int main(void)
{
    void* shim = dlopen("libsystem_shim.so", RTLD_NOW | RTLD_LOCAL);
    if (!shim) {
        printf("dlopen failed: %s\n", dlerror());
        return 1;
    }

    clock_gettime_fn shim_clock_gettime =
        (clock_gettime_fn)dlsym(shim, "clock_gettime");
    clock_getres_fn shim_clock_getres =
        (clock_getres_fn)dlsym(shim, "clock_getres");
    nsec_fn shim_nsec = (nsec_fn)dlsym(shim, "clock_gettime_nsec_np");
    if (!shim_clock_gettime || !shim_clock_getres || !shim_nsec) {
        printf("shim time exports missing\n");
        return 1;
    }

    struct timespec resolution;
    memset(&resolution, 0, sizeof(resolution));
    if (shim_clock_getres(6, &resolution) != 0) {
        printf("clock_getres(Darwin CLOCK_MONOTONIC) failed\n");
        return 1;
    }
    if (resolution.tv_sec != 0 || resolution.tv_nsec != 1) {
        printf("clock_getres(6) resolution %lld.%09ld is not monotonic "
               "precision; Darwin clock id was not translated\n",
               (long long)resolution.tv_sec, resolution.tv_nsec);
        return 1;
    }

    struct timespec now;
    memset(&now, 0, sizeof(now));
    if (shim_clock_gettime(8, &now) != 0) {
        printf("clock_gettime(Darwin CLOCK_UPTIME_RAW) failed\n");
        return 1;
    }
    if (shim_nsec(8) == 0) {
        printf("clock_gettime_nsec_np returned no time\n");
        return 1;
    }

    printf("clock-translation-ok\n");
    return 0;
}

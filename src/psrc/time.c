#include "time.h"

#include <stddef.h>
#include <stdbool.h>
#if PLATFORM != PLAT_MACOS || 1 // TODO: detect macos version
    #include <time.h>
#else
    #include <mach/mach_time.h>
    #include <CoreServices/CoreServices.h>
#endif

#include "attribs.h"

#if (PLATFLAGS & PLATFLAG_WINDOWSLIKE)
    LARGE_INTEGER perfctfreq;
    uint64_t perfctmul = 1000000;
#endif

uint64_t altutime(void) {
    #if (PLATFLAGS & PLATFLAG_WINDOWSLIKE)
        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);
        return time.QuadPart * perfctmul / perfctfreq.QuadPart;
    #elif PLATFORM == PLAT_MACOS && 0 // TODO: detect macos version
        uint64_t time = mach_absolute_time();
        Nanoseconds nsec = AbsoluteToNanoseconds(*(AbsoluteTime*)&time);
        return (*(uint64_t*)&nsec) / 1000;
    #else
        struct timespec time;
        clock_gettime(CLOCK_MONOTONIC, &time);
        return time.tv_sec * 1000000 + time.tv_nsec / 1000;
    #endif
}

void microwait(uint64_t d) {
    #if (PLATFLAGS & PLATFLAG_WINDOWSLIKE)
        #if PSRC_MTLVL >= 2
        static THREADLOCAL HANDLE timer = NULL;
        #else
        static HANDLE timer = NULL;
        #endif
        if (!timer) timer = CreateWaitableTimer(NULL, true, NULL);
        LARGE_INTEGER tmpd = {.QuadPart = d * -10};
        SetWaitableTimer(timer, &tmpd, 0, NULL, NULL, false);
        WaitForSingleObject(timer, INFINITE);
    #elif PLATFORM == PLAT_MACOS && 0 // TODO: detect macos version
        d *= 1000;
        AbsoluteTime time = NanosecondsToAbsolute(*(Nanoseconds*)&d);
        mach_wait_until(*(uint64_t*)&time);
    #else
        struct timespec dts;
        dts.tv_sec = d / 1000000;
        dts.tv_nsec = (d % 1000000) * 1000;
        nanosleep(&dts, NULL);
    #endif
}

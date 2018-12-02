//
// Created by noisyfox on 2018/12/2.
//

#include "vm_thread.h"
#include <string.h>

#if defined(HAVE_GET_NPROCS)

#include <sys/sysinfo.h>

#elif defined(HAVE_SC_NPROCESSORS_ONLN)

#include <unistd.h>

#elif defined(HAVE_HW_AVAILCPU)

#include <sys/sysctl.h>

#elif defined(WIN32)

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#else
#error Not supported on this platform.
#endif

SystemProcessorInfo g_systemProcessorInfo = {0};


JAVA_BOOLEAN thread_init() {
    memset(&g_systemProcessorInfo, 0, sizeof(SystemProcessorInfo));

#if defined(HAVE_GET_NPROCS)

    int np = get_nprocs();
    if (np < 1) {
        np = 1;
    }
    g_systemProcessorInfo.numberOfProcessors = (uint32_t) np;

#elif defined(HAVE_SC_NPROCESSORS_ONLN)

    long np = sysconf(_SC_NPROCESSORS_ONLN);
    if (np < 1) {
        np = 1;
    }
    g_systemProcessorInfo.numberOfProcessors = (uint32_t) np;

#elif defined(HAVE_HW_AVAILCPU)

    int mib[4];
    int numCPU;
    size_t len = sizeof(numCPU);

    /* set the mib for hw.ncpu */
    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

    /* get the number of CPUs from the system */
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    if (numCPU < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &numCPU, &len, NULL, 0);
        if (numCPU < 1) {
            numCPU = 1;
        }
    }

    g_systemProcessorInfo.numberOfProcessors = (uint32_t) numCPU;

#elif defined(WIN32)

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    g_systemProcessorInfo.numberOfProcessors = si.dwNumberOfProcessors;

#endif

    return JAVA_TRUE;
}

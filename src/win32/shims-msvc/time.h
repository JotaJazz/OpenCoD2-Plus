/* MSVC-ABI <time.h>: the runtime's, plus POSIX clock_gettime (MinGW's winpthread
 * provides it; here pthread_msvc.c does). */
#include_next <time.h>

#ifndef COD2_MSVC_TIME_H
#define COD2_MSVC_TIME_H

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
typedef int clockid_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif
int clock_gettime(clockid_t clock, struct timespec *ts);
#ifdef __cplusplus
}
#endif

#endif

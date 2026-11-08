#ifndef COD2_GLIBC_COMPAT_H
#define COD2_GLIBC_COMPAT_H

/* Force-included in the Linux build (C and C++). g++ always defines _GNU_SOURCE, and
 * with it glibc 2.38+ sends strtol, strtoul and the scanf family to their C23
 * versions (__isoc23_*, which also take 0b prefixes). That raises the glibc a
 * binary needs from 2.34 to 2.38, so a server built on a new distribution no
 * longer starts on Debian 12 or Ubuntu 22.04. The C build never defines
 * _GNU_SOURCE; this keeps the C++ build on the same functions. In C,
 * <features.h> is left alone: included first, it would settle the features
 * before a source's own _GNU_SOURCE (sqlite3.c's mremap). */
#ifdef __cplusplus
#include <features.h>
#undef __GLIBC_USE_C23_STRTOL
#define __GLIBC_USE_C23_STRTOL 0
#undef __GLIBC_USE_C2X_STRTOL
#define __GLIBC_USE_C2X_STRTOL 0
#endif

/* glibc 2.43 gave i386's acosf, asinf, atan2f, log10f and sqrtf new
 * versions, which a binary linked there needs (GLIBC_2.43). Calls bind to
 * the versions every i386 glibc has, the x87 ones 1.3's binaries call. */
#if defined(__i386__)
__asm__(".symver acosf,acosf@GLIBC_2.0");
__asm__(".symver asinf,asinf@GLIBC_2.0");
__asm__(".symver atan2f,atan2f@GLIBC_2.0");
__asm__(".symver log10f,log10f@GLIBC_2.0");
__asm__(".symver sqrtf,sqrtf@GLIBC_2.0");
#endif

#endif

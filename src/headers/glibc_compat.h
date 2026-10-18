#ifndef COD2_GLIBC_COMPAT_H
#define COD2_GLIBC_COMPAT_H

/* Force-included in the Linux C++ build. g++ always defines _GNU_SOURCE, and
 * with it glibc 2.38+ sends strtol, strtoul and the scanf family to their C23
 * versions (__isoc23_*, which also take 0b prefixes). That raises the glibc a
 * binary needs from 2.34 to 2.38, so a server built on a new distribution no
 * longer starts on Debian 12 or Ubuntu 22.04. The C build never defines
 * _GNU_SOURCE; this keeps the C++ build on the same functions. */
#include <features.h>
#undef __GLIBC_USE_C23_STRTOL
#define __GLIBC_USE_C23_STRTOL 0
#undef __GLIBC_USE_C2X_STRTOL
#define __GLIBC_USE_C2X_STRTOL 0

#endif

#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#ifndef __LONG32
#define __LONG32 long   /* MinGW's _mingw.h macro; OpenCoD2-Plus Server's hexrays_defs.h types DWORD/LONG with it */
#endif

#include <io.h>
#include <direct.h>

#ifndef COD2_NO_MKDIR_SHIM
#define mkdir(path, mode) _mkdir(path)
#endif

static __inline int pthread_main_np(void) { return 1; }

#endif
#ifdef __cplusplus
}
#endif
#endif

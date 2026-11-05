#ifndef WIN32_COMPAT_H
#define WIN32_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#if defined(_MSC_VER) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES   /* M_PI and friends from <math.h> */
#endif

#ifndef __LONG32
#define __LONG32 long   /* MinGW's _mingw.h macro; OpenCoD2-Plus Server's hexrays_defs.h types DWORD/LONG with it */
#endif

#include <io.h>
#include <direct.h>

#if defined(_MSC_VER) && defined(__clang__) && !defined(__PRFCHWINTRIN_H)
#define __PRFCHWINTRIN_H   /* SDL 2.30's SDL_endian.h defines _m_prefetch behind this guard; recent clang has it built in */
#endif

#ifdef _MSC_VER
/* the MSVC runtime spells POSIX and MinGW's extras its own way */
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#ifndef strcasecmp
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
#ifndef alloca
#define alloca _alloca
#endif
#define popen _popen
#define pclose _pclose

/* cl ignores GCC's __attribute__((constructor)). COD2_CONSTRUCTOR(fn){..} registers fn in the CRT's .CRT$XCU init
 * array so it runs before main, as the GCC constructor would.
 *   usage: COD2_CONSTRUCTOR(init_rune_locale) { ... }   (replaces the attribute) */
#ifdef __clang__
/* clang drops the unused static below; its own constructor attribute registers fn */
#define COD2_CONSTRUCTOR(fn) static void __attribute__((constructor)) fn(void)
#else
#pragma section(".CRT$XCU", read)   /* declare the CRT init array section once */
#define COD2_CONSTRUCTOR(fn)                                                 \
    static void fn(void);                                                   \
    __declspec(allocate(".CRT$XCU")) static void (*fn##__crtreg)(void) = fn;\
    static void fn(void)
#endif

/* COD2_ALT(lhs, rhs): emit a linker /alternatename directive from UNDECORATED C
 * symbol names. x86 cl prefixes C symbols with one leading '_'; x64 cl uses no
 * prefix. The alias-seam lists (msvc_alias_pragmas.h, msvc_seam_aliases.h,
 * msvc_link_glue.c) pass bare names so this adds the right decoration per arch.
 * (Adjacent string-literal concatenation inside __pragma is well-defined.) */
#if defined(_M_X64) || defined(_M_ARM64)
#define COD2_ALT_U_ ""
#else
#define COD2_ALT_U_ "_"
#endif
#define COD2_ALT(lhs, rhs) \
    __pragma(comment(linker, "/alternatename:" COD2_ALT_U_ lhs "=" COD2_ALT_U_ rhs))

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#endif

#ifndef COD2_NO_MKDIR_SHIM
#define mkdir(path, mode) _mkdir(path)
#endif

static __inline int pthread_main_np(void) { return 1; }

#endif
#ifdef __cplusplus
}
#endif
#endif

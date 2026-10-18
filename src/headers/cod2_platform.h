#ifndef CLEAN_COD2_PLATFORM
#define CLEAN_COD2_PLATFORM

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <pthread.h>
#include <stdio.h>
#include "cod2_feature_config.h"

#if defined(__i386__) && defined(__GNUC__) && !defined(__clang__) && !defined(__EMSCRIPTEN__)
#define COD2_GCC_X86_ABI 1
#else
#define COD2_GCC_X86_ABI 0
#endif

/* The original binary is a Windows build, where 8-byte scalars (long long,
 * double) are 8-byte aligned inside structs; the i386 System V ABI aligns them
 * to 4. Fields marked with this keep the Windows layout on Linux. */
#if COD2_GCC_X86_ABI && !defined(_WIN32)
#define COD2_WIN_ALIGN8 __attribute__((aligned(8)))
#else
#define COD2_WIN_ALIGN8
#endif

/* Windows calling-convention keywords used in shared code; MSVC and MinGW
 * know them, GCC on Linux does not. */
#if !defined(_MSC_VER) && !defined(_WIN32)
#ifndef __cdecl
#define __cdecl __attribute__((cdecl))
#endif
#ifndef __stdcall
#define __stdcall __attribute__((stdcall))
#endif
#endif

#if COD2_GCC_X86_ABI && defined(__APPLE_CC__)
#define COD2_APPLE_GCC_X86_ABI 1
#else
#define COD2_APPLE_GCC_X86_ABI 0
#endif

#ifdef __EMSCRIPTEN__
#define __attribute_regparm__(n)
#define __attribute_sseregparm__
#else
#define __attribute_regparm__(n) __attribute__((regparm(n)))
#if COD2_GCC_X86_ABI
#define __attribute_sseregparm__ __attribute__((sseregparm))
#else
#define __attribute_sseregparm__
#endif
#endif

#if COD2_GCC_X86_ABI
#define COD2_REGPARM(n) __attribute__((regparm(n)))
#define COD2_SSEREGPARM __attribute__((sseregparm))
#define COD2_CDECL __attribute__((cdecl))
#define COD2_STDCALL __attribute__((stdcall))
#define COD2_FASTCALL __attribute__((fastcall))
#define COD2_FORCE_ALIGN_ARG_POINTER __attribute__((force_align_arg_pointer))
#define COD2_ALIGNED(n) __attribute__((aligned(n)))
#else
#define COD2_REGPARM(n)
#define COD2_SSEREGPARM
#define COD2_CDECL
#define COD2_STDCALL
#define COD2_FASTCALL
#define COD2_FORCE_ALIGN_ARG_POINTER
#define COD2_ALIGNED(n)
#endif

#if COD2_APPLE_GCC_X86_ABI
#define COD2_REGPARM_AND_STACKPARM __attribute__((regparmandstackparm))
#else
#define COD2_REGPARM_AND_STACKPARM
#endif

#endif

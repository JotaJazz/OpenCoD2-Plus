#ifndef CLEAN_COD2_PLATFORM
#define CLEAN_COD2_PLATFORM

#ifdef __cplusplus
extern "C" {
#endif

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

/* An empty parameter list, int f(), declares a function without a prototype
 * in C but one that takes no arguments in C++. COD2_KR keeps the C meaning in
 * C++: the call passes the same arguments with the same default promotions. */
#ifndef COD2_KR
#ifdef __cplusplus
#define COD2_KR ...
#else
#define COD2_KR
#endif
#endif

/* C++ has no forward-declared enums without an underlying type, which GNU C
 * accepts: COD2_ENUM_DECL(X) declares enum X ahead of its definition in C++,
 * and COD2_ENUM_INT gives that definition the matching int base (the size the
 * enums already have with GCC and MSVC). */
#ifdef __cplusplus
#define COD2_ENUM_DECL(X) enum X : int;
#define COD2_ENUM_INT : int
#define COD2_ENUM_DECL_UINT(X) enum X : unsigned int;
#define COD2_ENUM_UINT : unsigned int
#else
#define COD2_ENUM_DECL(X)
#define COD2_ENUM_INT
#define COD2_ENUM_DECL_UINT(X)
#define COD2_ENUM_UINT
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

/* the order of two pointers for a sort: 1.3's (int)(a - b) on 32 bits; on x64 that difference
   can pass an int, so it compares (-1, 0, 1) */
#ifdef COD2_X64
#define COD2_PTR_ORDER(a, b) (((uintptr_t)(a) > (uintptr_t)(b)) - ((uintptr_t)(a) < (uintptr_t)(b)))
#else
#define COD2_PTR_ORDER(a, b) ((int)((uintptr_t)(a) - (uintptr_t)(b)))
#endif

/* a size 1.3 hardcodes for a type with pointers: on 32 bits the literal, and the build fails if
   sizeof(T) differs from it; on x64 sizeof(T) */
#ifdef COD2_X64
#define COD2_SIZEOF_1_3(T, n) sizeof(T)
#else
#define COD2_SIZEOF_1_3(T, n) (sizeof(char[sizeof(T) == (n) ? 1 : -1]) * (n))
#endif

/* the same for a field's offset: 1.3's literal on 32 bits (checked), offsetof on x64 */
#ifdef COD2_X64
#define COD2_OFFSETOF_1_3(T, f, n) offsetof(T, f)
#else
#define COD2_OFFSETOF_1_3(T, f, n) (sizeof(char[offsetof(T, f) == (n) ? 1 : -1]) * (n))
#endif

/* a size 1.3 hardcodes for an object our headers do not describe whole (its fields are read at
   1.3's offsets): the literal on 32 bits; on x64 the literal plus room for its pointers to grow.
   shortcut: 0x100 covers 32 grown pointers, replace with sizeof once the type is complete */
#ifdef COD2_X64
#define COD2_SIZE_1_3_PADDED(n) ((n) + 0x100)
#else
#define COD2_SIZE_1_3_PADDED(n) (n)
#endif

#ifdef __cplusplus
}
#endif
#endif

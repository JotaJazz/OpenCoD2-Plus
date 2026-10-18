#ifndef IMPORTS_H
#define IMPORTS_H

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

#ifdef __cplusplus
extern "C" {
#endif

#include "imports/agl.h"
#include "imports/audiotoolbox.h"
#include "imports/carbon.h"
#include "imports/corefoundation.h"
#include "imports/iokit.h"
#include "imports/opengl.h"
#include "imports/quicktime.h"
#include "imports/libc.h"
#include "imports/libstdcxx.h"
#include "generated_syms.h"

#ifdef GFX_REAL_D3D9
#define D3DVTCC __attribute__((stdcall))
#else
#define D3DVTCC
#endif

#ifdef __cplusplus
}
#endif
#endif

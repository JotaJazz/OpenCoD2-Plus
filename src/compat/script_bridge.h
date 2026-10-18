#ifndef COD2_COMPAT_SCRIPT_BRIDGE_H
#define COD2_COMPAT_SCRIPT_BRIDGE_H

/* Shared by the generated script_exports.cpp and script_imports.cpp. */
#include <stdarg.h>
#include <stdio.h>
#include <utility>

/* The assembler name of a C symbol: MinGW (i686) prefixes an underscore. */
#define COD2_SYM_STR2(x) #x
#define COD2_SYM_STR(x) COD2_SYM_STR2(x)
#define COD2_SYM(name) COD2_SYM_STR(__USER_LABEL_PREFIX__) name

#endif

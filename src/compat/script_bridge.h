#ifndef COD2_COMPAT_SCRIPT_BRIDGE_H
#define COD2_COMPAT_SCRIPT_BRIDGE_H

/* Shared by the generated script_exports.cpp and script_imports.cpp. */
#include <stdarg.h>
#include <stdio.h>
#include <utility>
#include <type_traits>
#include <stdint.h>
#include <string.h>

/* Exports must return what the engine reads from eax: a scalar or nothing. */
template<class T> struct cod2_bridge_returns_scalar
	: std::integral_constant<bool, std::is_scalar<T>::value || std::is_void<T>::value> {};

/* The assembler name of a C symbol: MinGW (i686) prefixes an underscore. */
#define COD2_SYM_STR2(x) #x
#define COD2_SYM_STR(x) COD2_SYM_STR2(x)
#define COD2_SYM(name) COD2_SYM_STR(__USER_LABEL_PREFIX__) name

#endif

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

/* The ABI of one side, as scripts/engine_abi.py records the engine's: the
   return class (v void, i integer or pointer in eax, f floating in st0, s
   struct or union through a hidden pointer), its size, and the stack slot a
   parameter takes (references are pointers). */
template<class T> constexpr char cod2_abi_class()
{
	return std::is_void<T>::value ? 'v' : std::is_floating_point<T>::value ? 'f'
		: (std::is_class<T>::value || std::is_union<T>::value) ? 's' : 'i';
}
template<class T> constexpr unsigned cod2_abi_size()
{
	if constexpr (std::is_void<T>::value) return 0; else return sizeof(T);
}
template<class T> constexpr unsigned cod2_abi_slot()
{
	return std::is_reference<T>::value ? sizeof(void *) : (sizeof(T) + 3) / 4 * 4;
}

/* An integer return the two sides give different sizes travels as the
   engine's: the unsigned type of that size, converted to and from the
   CoD2rev-Plus type, so no side reads bits of eax the other did not set. */
template<unsigned N> struct cod2_abi_uint;
template<> struct cod2_abi_uint<1> { typedef unsigned char type; };
template<> struct cod2_abi_uint<2> { typedef unsigned short type; };
template<> struct cod2_abi_uint<4> { typedef unsigned int type; };
template<> struct cod2_abi_uint<8> { typedef unsigned long long type; };
template<class T, unsigned N> using cod2_abi_int =
	typename std::conditional<sizeof(T) == N, T, typename cod2_abi_uint<N>::type>::type;

/* The assembler name of a C symbol: MinGW (i686) prefixes an underscore. */
#define COD2_SYM_STR2(x) #x
#define COD2_SYM_STR(x) COD2_SYM_STR2(x)
#define COD2_SYM(name) COD2_SYM_STR(__USER_LABEL_PREFIX__) name

#endif

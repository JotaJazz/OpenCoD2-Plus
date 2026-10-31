// Functions CoD2rev-Plus's script VM calls that this engine does not have,
// for the executables without CoD2rev-Plus's core (universal, qcommon, xanim:
// the client), which defines them in the dedicated server. Same behaviour as
// CoD2rev-Plus's.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "script_bridge.h"
#include <math.h>

extern "C" void *cod2c_Hunk_AllocAlignInternal(int size, int alignment) __asm__(COD2_SYM("Hunk_AllocAlignInternal"));

// xanim.cpp
void *Hunk_AllocXAnimTreePrecache(int size)
{
	return cod2c_Hunk_AllocAlignInternal(size, 4);
}

// qvsnprintf.cpp is CoD2rev-Plus's own vsnprintf; the C library's does the same.
int Q_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	return vsnprintf(str, size, format, args);
}

// q_shared.cpp
void Sys_UnimplementedFunctionInternal(const char *function)
{
	Com_Printf("FUNCTION NOT IMPLEMENTED!!! (%s)\n", function);
}

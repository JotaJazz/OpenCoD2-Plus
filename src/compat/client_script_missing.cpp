// Functions CoD2rev-Plus's script VM calls that this engine does not have,
// for the executables without CoD2rev-Plus's universal and qcommon (the client):
// in the dedicated server those define them. Same behaviour as CoD2rev-Plus's.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "script_bridge.h"
#include <math.h>

// com_math.cpp
int Q_rint(float in)
{
	return floor(in + 0.5);
}

int VectorCompare(const vec3_t v1, const vec3_t v2)
{
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2])
		return 0;

	return 1;
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

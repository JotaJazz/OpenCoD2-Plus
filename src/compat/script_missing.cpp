// Functions CoD2rev-Plus's script VM calls that this engine does not have
// (scripts/gen_script_bridge.py, PROVIDED). Same behaviour as CoD2rev-Plus's.
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

// The data blobs refer to VM_Execute by the original binary's C++ name, whose
// third argument is unsigned int; CoD2rev-Plus's takes unsigned long. The
// label is the raw assembler name, as src/PC/script had it: MinGW's --defsym
// list (build/win_gen/cxx_defsyms_c.txt) aliases the prefixed one to it.
extern "C" unsigned int cod2x_VM_Execute_orig(unsigned int localId, const char *pos, unsigned int numArgs)
	__asm__("_Z10VM_ExecutejPKcj");
unsigned int cod2x_VM_Execute_orig(unsigned int localId, const char *pos, unsigned int numArgs)
{
	return VM_Execute(localId, pos, (unsigned long)numArgs);
}

// MinGW's data blob (build/win_gen/data.win.S, the _functions table of
// script builtins) names "int" through a string src/PC/script defined.
extern "C" const char cod2x_str_int[] __asm__(COD2_SYM("str_0021d27c"));
const char cod2x_str_int[] = "int";

// The engine's game code compares Scr_GetPointerType with these.
#include "../headers/scr_vartypes.h"
static_assert(COD2_VAR_DEAD_ENTITY == VAR_DEAD_ENTITY && COD2_VAR_ENTITY == VAR_ENTITY
	&& COD2_VAR_ARRAY == VAR_ARRAY && COD2_VAR_DEAD_THREAD == VAR_DEAD_THREAD,
	"src/headers/scr_vartypes.h does not match CoD2rev-Plus's VAR_ types");

// CMake derives PROTOCOL_VERSION from COD2_FEATURE_CFLAGS; the engine reads
// COD2_PATCH_10 wherever it is set. Both must name the same game version.
#include "../headers/cod2_feature_config.h"
static_assert(PROTOCOL_VERSION == COD2_PROTOCOL_VERSION,
	"PROTOCOL_VERSION and COD2_PATCH_10 name different game versions; set COD2_PATCH_10 through COD2_FEATURE_CFLAGS");

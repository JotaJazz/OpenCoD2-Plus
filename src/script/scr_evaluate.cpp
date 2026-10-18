#include "../qcommon/qcommon.h"
#include "script_public.h"

/*
==============
Scr_InitEvaluate
==============
*/
void Scr_InitEvaluate()
{
	scrVarPub.canonicalStrMark = Hunk_SetMark();
	scrCompilePub.archivedCanonicalStringsBuf = (uint16_t *)Hunk_AllocInternal(0x20000);
	scrVarPub.canonicalStrCount = 0;
}

/*
==============
Scr_EndLoadEvaluate
==============
*/
void Scr_EndLoadEvaluate()
{
	scrCompilePub.archivedCanonicalStringsBuf = 0;
	Hunk_ClearHigh(scrVarPub.canonicalStrMark);
}

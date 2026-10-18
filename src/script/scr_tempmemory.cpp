#include "../qcommon/qcommon.h"
#include "script_public.h"

unsigned int currentPos;

/*
==============
Scr_Allign2
==============
*/
int Scr_Allign2( int pos )
{
	UNIMPLEMENTED(__FUNCTION__);
	return 0;
}

/*
==============
Scr_Allign4
==============
*/
int Scr_Allign4( int pos )
{
	UNIMPLEMENTED(__FUNCTION__);
	return 0;
}

/*
==============
Scr_Allign2Strict
==============
*/
int Scr_Allign2Strict( int pos )
{
	UNIMPLEMENTED(__FUNCTION__);
	return 0;
}

/*
==============
Scr_Allign4Strict
==============
*/
int Scr_Allign4Strict( int pos )
{
	UNIMPLEMENTED(__FUNCTION__);
	return 0;
}

/*
==============
TempMemoryReset
==============
*/
void TempMemoryReset()
{
	currentPos = 0;
}

/*
==============
TempMalloc
==============
*/
char* TempMalloc( int len )
{
	char *buf = (char *)Hunk_ReallocateTempMemoryInternal( currentPos + len ) + currentPos;
	currentPos += len;

	return buf;
}

/*
==============
TempMemorySetPos
==============
*/
void TempMemorySetPos( char *pos )
{
	currentPos -= (char *)TempMalloc( 0 ) - pos;
	Hunk_ReallocateTempMemoryInternal( currentPos );
}

/*
==============
TempMallocAlignStrict
==============
*/
char* TempMallocAlignStrict( int len )
{
	return (char *)TempMalloc( len );
}

/*
==============
TempMallocAlign
==============
*/
char* TempMallocAlign( int len )
{
	return (char *)TempMalloc( len );
}

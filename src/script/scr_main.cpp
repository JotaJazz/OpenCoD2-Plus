#include "../qcommon/qcommon.h"
#include "script_public.h"

/*
============
Scr_PostCompileScripts
============
*/
void Scr_PostCompileScripts()
{
	assert( scrCompilePub.programLen == static_cast< uint >( TempMalloc( 0 ) - scrVarPub.programBuffer ) );
	assert( !scrAnimPub.animTreeNames );

	Hunk_ConvertTempToPermLowInternal();
}

/*
============
Scr_IsIdentifier
============
*/
bool Scr_IsIdentifier( const char *token )
{
	while ( token[0] )
	{
		if ( !isalnum(token[0]) && token[0] != '_' )
		{
			return false;
		}

		token++;
	}

	return true;
}

/*
============
Scr_IsInScriptMemory
============
*/
int Scr_IsInScriptMemory( const char *pos )
{
	assert(scrVarPub.programBuffer);
	assert(pos);
	return pos - scrVarPub.programBuffer < scrCompilePub.programLen;
}

/*
============
SL_TransferToCanonicalString
============
*/
unsigned int SL_TransferToCanonicalString( unsigned int stringValue )
{
	assert(stringValue);

	SL_TransferRefToUser(stringValue, 2);

	if ( scrCompilePub.archivedCanonicalStringsBuf[stringValue] )
	{
		return scrCompilePub.archivedCanonicalStringsBuf[stringValue];
	}

	scrVarPub.canonicalStrCount++;
	scrCompilePub.archivedCanonicalStringsBuf[stringValue] = scrVarPub.canonicalStrCount;

	return scrVarPub.canonicalStrCount;
}

/*
============
Scr_BeginLoadAnimTrees
============
*/
void Scr_BeginLoadAnimTrees( int user )
{
	assert(!scrAnimPub.animtree_loading);
	scrAnimPub.animtree_loading = true;

	scrAnimPub.xanim_num[user] = 0;
	scrAnimPub.xanim_lookup[user][0].anims = NULL;

	assert(!scrAnimPub.animtrees);
	scrAnimPub.animtrees = Scr_AllocArray();
	scrAnimPub.animtree_node = 0;

	scrCompilePub.developer_statement = SCR_DEV_NO;
}

/*
============
Scr_BeginLoadScriptsRemote
============
*/
void Scr_BeginLoadScriptsRemote( void )
{
	scrCompilePub.script_loading = true;
	Scr_InitOpcodeLookup();
	scrCompilePub.loadedscripts = Scr_AllocArray();
}

/*
============
SL_GetCanonicalString
============
*/
unsigned int SL_GetCanonicalString( const char *str )
{
	unsigned int stringValue = SL_FindString(str);

	if ( scrCompilePub.archivedCanonicalStringsBuf[stringValue] )
	{
		return scrCompilePub.archivedCanonicalStringsBuf[stringValue];
	}

	return SL_TransferToCanonicalString( SL_GetString_( str, 0 ) );
}

/*
============
Scr_EndLoadScripts
============
*/
void Scr_EndLoadScripts()
{
	Scr_EndLoadEvaluate();
	SL_ShutdownSystem(2);

	scrCompilePub.script_loading = false;

	assert(scrCompilePub.loadedscripts);
	ClearObject(scrCompilePub.loadedscripts);
	RemoveRefToObject(scrCompilePub.loadedscripts);
	scrCompilePub.loadedscripts = 0;

	assert(scrCompilePub.scriptsPos);
	ClearObject(scrCompilePub.scriptsPos);
	RemoveRefToObject(scrCompilePub.scriptsPos);
	scrCompilePub.scriptsPos = 0;

	assert(scrCompilePub.builtinFunc);
	ClearObject(scrCompilePub.builtinFunc);
	RemoveRefToObject(scrCompilePub.builtinFunc);
	scrCompilePub.builtinFunc = 0;

	assert(scrCompilePub.builtinMeth);
	ClearObject(scrCompilePub.builtinMeth);
	RemoveRefToObject(scrCompilePub.builtinMeth);
	scrCompilePub.builtinMeth = 0;
}

/*
============
Scr_EndLoadAnimTrees
============
*/
void Scr_EndLoadAnimTrees()
{
	assert(scrAnimPub.animtrees);
	ClearObject(scrAnimPub.animtrees);
	RemoveRefToObject(scrAnimPub.animtrees);
	scrAnimPub.animtrees = 0;

	if ( scrAnimPub.animtree_node )
	{
		RemoveRefToObject(scrAnimPub.animtree_node);
	}

	SL_ShutdownSystem(2);
	scrVarPub.endScriptBuffer = (const char *)Hunk_AllocLowInternal(0);

	scrAnimPub.animtree_loading = false;
}

/*
============
Scr_FreeScripts
============
*/
void Scr_FreeScripts( void )
{
	if ( scrCompilePub.script_loading )
	{
		scrCompilePub.script_loading = 0;
		Scr_EndLoadScripts();
	}

	if ( scrAnimPub.animtree_loading )
	{
		scrAnimPub.animtree_loading = 0;
		Scr_EndLoadAnimTrees();
	}

	SL_ShutdownSystem(1);
	Scr_ShutdownOpcodeLookup();

	scrVarPub.programBuffer = NULL;
	scrCompilePub.programLen = 0;

	scrVarPub.endScriptBuffer = NULL;
	scrVarPub.checksum = 0;
}

/*
============
Scr_BeginLoadScripts
============
*/
void Scr_BeginLoadScripts()
{
	assert(!scrCompilePub.script_loading);
	scrCompilePub.script_loading = true;

	Scr_InitOpcodeLookup();

	assert(!scrCompilePub.loadedscripts);
	scrCompilePub.loadedscripts = Scr_AllocArray();

	assert(!scrCompilePub.scriptsPos);
	scrCompilePub.scriptsPos = Scr_AllocArray();

	assert(!scrCompilePub.builtinFunc);
	scrCompilePub.builtinFunc = Scr_AllocArray();

	assert(!scrCompilePub.builtinMeth);
	scrCompilePub.builtinMeth = Scr_AllocArray();

	scrVarPub.programBuffer = (const char *)Hunk_AllocLowInternal(0);

	scrCompilePub.programLen = 0;
	scrVarPub.endScriptBuffer = 0;

	Scr_InitEvaluate();

	scrVarPub.fieldBuffer = NULL;
	scrCompilePub.value_count = 0;

	Scr_ClearErrorMessage();

	scrCompilePub.func_table_size = 0;

	Scr_BeginLoadAnimTrees(1);

	TempMemoryReset();
	//assert((!((int)scrVarPub.programBuffer & 31)));
}

/*
============
Scr_GetFunctionHandle
============
*/
int Scr_GetFunctionHandle( const char *filename, const char *name )
{
	VariableValue pos;
	unsigned int name2, str, id, posId, filePosId;

	assert(scrCompilePub.scriptsPos);
	assert(strlen( filename ) < MAX_QPATH);

	name2 = Scr_CreateCanonicalFilename(filename);
	filePosId = FindVariable(scrCompilePub.scriptsPos, name2);
	SL_RemoveRefToString(name2);

	if ( !filePosId )
	{
		return 0;
	}

	id = FindObject(filePosId);
	assert(id);
	str = SL_FindLowercaseString(name);

	if ( !str )
	{
		return 0;
	}

	posId = FindVariable(id, str);

	if ( !posId )
	{
		return 0;
	}

	if ( GetObjectType(posId) != VAR_POINTER )
	{
		return 0;
	}

	pos = Scr_EvalVariable( FindVariable( FindObject( posId ), 1 ) );
	assert(pos.type == VAR_CODEPOS || pos.type == VAR_DEVELOPER_CODEPOS);

	if ( !Scr_IsInScriptMemory( pos.u.codePosValue ) )
	{
		return 0;
	}

	assert(pos.u.codePosValue - scrVarPub.programBuffer);
	return pos.u.codePosValue - scrVarPub.programBuffer;
}

/*
============
Scr_PrecacheAnimTrees
============
*/
void Scr_PrecacheAnimTrees( void *(*Alloc)(int), int user )
{
	for ( int i = 1; i <= scrAnimPub.xanim_num[user]; i++ )
	{
		Scr_LoadAnimTreeAtIndex(i, Alloc, user);
	}
}

/*
============
Scr_LoadScript
============
*/
unsigned int Scr_LoadScript( const char *filename )
{
	unsigned int filePosPtr, filePosId, scriptId, name;
	const char *oldSourceBuf, *oldFilename, *sourceBuffer;
	char extFilename[MAX_QPATH];
	sval_u parseData;

	assert(scrCompilePub.script_loading);
	assert(strlen( filename ) < MAX_QPATH);

	name = Scr_CreateCanonicalFilename(filename);

	if ( FindVariable(scrCompilePub.loadedscripts, name) )
	{
		SL_RemoveRefToString(name);
		filePosPtr = FindVariable(scrCompilePub.scriptsPos, name);

		if ( filePosPtr )
		{
			return FindObject(filePosPtr);
		}

		return 0;
	}

	scriptId = GetNewVariable(scrCompilePub.loadedscripts, name);
	SL_RemoveRefToString(name);

	Com_sprintf(extFilename, sizeof(extFilename), "%s.gsc", SL_ConvertToString(name));
	oldSourceBuf = scrParserPub.sourceBuf;

	sourceBuffer = Scr_AddSourceBuffer(SL_ConvertToString(name), extFilename, TempMalloc(0), true);

	if ( !sourceBuffer )
	{
		return 0;
	}

	scrAnimPub.animTreeNames = 0;
	scrCompilePub.far_function_count = 0;

	oldFilename = scrParserPub.scriptfilename;
	scrParserPub.scriptfilename = extFilename;

	scrCompilePub.in_ptr = "+";
	scrCompilePub.parseBuf = sourceBuffer;

	ScriptParse(&parseData, 0);

	filePosId = GetObjectA(GetVariable(scrCompilePub.scriptsPos, name));

	ScriptCompile(parseData, filePosId, scriptId);

	scrParserPub.scriptfilename = oldFilename;
	scrParserPub.sourceBuf = oldSourceBuf;

	return filePosId;
}

/*
============
Scr_ScanFile
============
*/
int Scr_ScanFile( char *buf, int max_size )
{
	int c, n;

	c = '*';

	for ( n = 0; n < max_size; n++ )
	{
		c = *scrCompilePub.in_ptr++;

		if ( !c || c == 10 )
		{
			break;
		}

		buf[n] = c;
	}

	if ( c == 10 )
	{
		buf[n] = 10;
		n++;
	}
	else if ( !c )
	{
		if ( scrCompilePub.parseBuf )
		{
			scrCompilePub.in_ptr = scrCompilePub.parseBuf;
			scrCompilePub.parseBuf = NULL;
		}
		else
		{
			scrCompilePub.in_ptr--;
		}
	}

	return n;
}
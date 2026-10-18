#include "../qcommon/qcommon.h"
#include "script_public.h"

scrAnimGlob_t scrAnimGlob;
scrAnimPub_t scrAnimPub;

/*
============
Scr_GetAnims
============
*/
XAnim* Scr_GetAnims( int index )
{
	assert(index > 0 && index < MAX_XANIMTREE_NUM);
	return scrAnimPub.xanim_lookup[SCR_XANIM_SERVER][index].anims;
}

/*
============
Scr_GetAnimsIndex
============
*/
int Scr_GetAnimsIndex( const XAnim *anims )
{
	UNIMPLEMENTED(__FUNCTION__);
	return 0;
}

/*
============
SetAnimCheck
============
*/
void SetAnimCheck( int bAnimCheck )
{
	scrAnimGlob.bAnimCheck = bAnimCheck;
}

/*
============
Scr_FindAnimTree
============
*/
scr_animtree_t Scr_FindAnimTree( const char *filename )
{
	VariableValue tempValue;
	unsigned int fileId, filenameId, xanimId;
	scr_animtree_t tree;

	assert( scrAnimPub.animtree_loading );

	filenameId = Scr_CreateCanonicalFilename(filename);
	fileId = FindVariable(scrAnimPub.animtrees, filenameId);

	SL_RemoveRefToString(filenameId);

	if ( !fileId )
	{
		tree.anims = NULL;
		return tree;
	}

	assert( GetVariableName( fileId ) < SL_MAX_STRING_INDEX );
	filenameId = GetVariableName(fileId);
	fileId = FindObject(fileId);
	assert( fileId );
	xanimId = FindVariable(fileId, ANIMTREE_XANIM);

	if ( !xanimId )
	{
		tree.anims = NULL;
		return tree;
	}

	tempValue = Scr_EvalVariable(xanimId);
	assert( tempValue.type == VAR_CODEPOS );
	tree.anims = (XAnim *)tempValue.u.codePosValue;

	return tree;
}

/*
============
Scr_FindAnim
============
*/
void Scr_FindAnim( const char *filename, const char *animName, scr_anim_s *anim, int user )
{
	unsigned int index, name;

	name = SL_GetLowercaseString_(animName, 0);
	Scr_EmitAnimationInternal( (char *)anim, name, Scr_UsingTreeInternal( filename, &index, user ) );
	SL_RemoveRefToString(name);
}

/*
============
Scr_UsingTree
============
*/
void Scr_UsingTree( const char *filename, unsigned int sourcePos )
{
	if ( !Scr_IsIdentifier(filename) )
	{
		CompileError(sourcePos, "bad anim tree name");
		return;
	}

	scrAnimPub.animTreeNames = Scr_UsingTreeInternal(filename, &scrAnimPub.animTreeIndex, 1);
}

/*
============
Scr_EmitAnimation
============
*/
void Scr_EmitAnimation( char *pos, unsigned int animName, unsigned int sourcePos )
{
	if ( !scrAnimPub.animTreeNames )
	{
		CompileError(sourcePos, "#using_animtree was not specified");
		return;
	}

	Scr_EmitAnimationInternal(pos, animName, scrAnimPub.animTreeNames);
}

/*
============
Scr_LoadAnimTreeAtIndex
============
*/
void Scr_LoadAnimTreeAtIndex( int index, void *(*Alloc)(int), int user )
{
	unsigned int id, fileId, filenameId, names, name;
	int size, size2;
	scr_animtree_t animtree;
	VariableValue tempValue;

	id = scrAnimGlob.using_xanim_lookup[user][index];
	assert(scrAnimPub.animtree_loading);

	filenameId = GetVariableName(id);

	fileId = FindObject(id);
	assert(fileId);

	if ( FindVariable(fileId, ANIMTREE_XANIM) )
	{
		return;
	}

	names = FindVariable(fileId, ANIMTREE_NAMES);

	if ( !names )
	{
		scrAnimPub.xanim_lookup[user][index].anims = NULL;
		return;
	}

	animtree.anims = NULL;
	names = FindObject(names);

	assert(names);
	assert(!scrAnimPub.animtree_node);

	scrAnimPub.animtree_node = Scr_AllocArray();

	if ( !Scr_LoadAnimTreeInternal( SL_ConvertToString( filenameId ), scrAnimPub.animtree_node, names ) )
	{
		Com_Error(ERR_DROP, "unknown anim tree '%s'", SL_ConvertToString(filenameId));
	}

	size = Scr_GetAnimTreeSize(scrAnimPub.animtree_node);
	assert(size);

	animtree.anims = XAnimCreateAnims( SL_ConvertToString( filenameId ), size, Alloc );

	name = SL_GetString_("root", 0);
	ConnectScriptToAnim(names, 0, filenameId, name, index);
	SL_RemoveRefToString(name);

	Scr_PrecacheAnimationTree(scrAnimPub.animtree_node);

	size2 = Scr_CreateAnimationTree(scrAnimPub.animtree_node, names, animtree.anims, 1, "root", 0, filenameId, index);
	assert(size == size2);

	Scr_CheckAnimsDefined(names, filenameId);

	RemoveVariable(fileId, 0);
	RemoveRefToObject(scrAnimPub.animtree_node);

	scrAnimPub.animtree_node = 0;

	tempValue.type = VAR_CODEPOS;
	tempValue.u.codePosValue = (const char *)animtree.anims;

	SetVariableValue(GetVariable(fileId, 1), &tempValue);

	XAnimSetupSyncNodes(animtree.anims);
	scrAnimPub.xanim_lookup[user][index] = animtree;
}

/*
============
Scr_GetAnimTreeSize
============
*/
int Scr_GetAnimTreeSize( unsigned int parentNode )
{
	unsigned int node, arrayNode;
	int size = 0;

	for ( node = FindNextSibling(parentNode); node; node = FindNextSibling(node) )
	{
		if ( GetVariableName(node) >= SL_MAX_STRING_INDEX )
		{
			continue;
		}

		if ( GetObjectType(node) == VAR_POINTER )
		{
			arrayNode = FindObject(node);
			size += Scr_GetAnimTreeSize(arrayNode);
			continue;
		}

		size++;
	}

	if ( size )
	{
		size++;
	}

	return size;
}

/*
============
Scr_EmitAnimationInternal
============
*/
void Scr_EmitAnimationInternal( char *pos, unsigned int animName, unsigned int names )
{
	VariableUnion *value;
	unsigned int animId;
	VariableValue tempValue;

	assert(names);
	animId = FindVariable(names, animName);

	if ( animId )
	{
		value = GetVariableValueAddress(animId);
		*(const char **)pos = value->codePosValue;
		value->codePosValue = pos;
		return;
	}

	animId = GetNewVariable(names, animName);

	*(const char **)pos = NULL;
	tempValue.type = VAR_CODEPOS;
	tempValue.u.codePosValue = pos;

	SetVariableValue(animId, &tempValue);
}

/*
============
ConnectScriptToAnim
============
*/
void ConnectScriptToAnim( unsigned int names, int index, unsigned int filename, unsigned int name, int treeIndex )
{
	scr_anim_s anim;
	const char *codePos, *nextCodePos;
	VariableUnion *value;
	unsigned int animId;

	animId = FindVariable(names, name);

	if ( !animId )
	{
		return;
	}

	value = GetVariableValueAddress(animId);

	if ( !value->codePosValue )
	{
		Com_Error(ERR_DROP, "duplicate animation '%s' in 'animtrees/%s.atr'", SL_ConvertToString(name), SL_ConvertToString(filename));
	}

	anim.index = index;
	anim.tree = treeIndex;

	for ( codePos = value->codePosValue; codePos; codePos = nextCodePos )
	{
		nextCodePos = *(const char **)codePos;
		*(const char **)codePos = anim.linkPointer;
	}

	value->codePosValue = NULL;
}

/*
============
Scr_UsingTreeInternal
============
*/
unsigned int Scr_UsingTreeInternal( const char *filename, unsigned int *index, int user )
{
	unsigned int id, fileId, names, name;

	assert( scrAnimPub.animtree_loading );
	assert( Scr_IsIdentifier( filename ) );

	name = Scr_CreateCanonicalFilename(filename);
	id = FindVariable(scrAnimPub.animtrees, name);

	if ( id )
	{
		fileId = FindObject(id);
		*index = 0;

		for ( int i = 1; i <= scrAnimPub.xanim_num[user]; i++ )
		{
			if ( scrAnimGlob.using_xanim_lookup[user][i] == id )
			{
				*index = i;
				break;
			}
		}

		assert(index);
	}
	else
	{
		id = GetNewVariable(scrAnimPub.animtrees, name);
		fileId = GetObjectA(id);

		scrAnimPub.xanim_num[user]++;
		assert( scrAnimPub.xanim_num[user] < MAX_XANIMTREE_NUM );

		scrAnimGlob.using_xanim_lookup[user][scrAnimPub.xanim_num[user]] = id;
		*index = scrAnimPub.xanim_num[user];
	}

	assert( !FindVariable( fileId, ANIMTREE_XANIM ) );

	names = GetArray(GetVariable(fileId, 0));
	SL_RemoveRefToString(name);

	return names;
}

/*
============
Scr_CreateAnimationTree
============
*/
int Scr_CreateAnimationTree( unsigned int parentNode, unsigned int names, XAnim *anims,
                             unsigned int childIndex, const char *parentName, unsigned int parentIndex,
                             unsigned int filename, int treeIndex )
{
	unsigned int index, size = 0, name, nodeRef;
	unsigned short flags = 0;

	for ( nodeRef = FindNextSibling(parentNode); nodeRef; nodeRef = FindNextSibling(nodeRef) )
	{
		name = GetVariableName(nodeRef);

		if ( name >= SL_MAX_STRING_INDEX )
		{
			continue;
		}

		size++;
	}

	assert( parentIndex == (unsigned short)parentIndex );
	assert( childIndex == (unsigned short)childIndex );
	assert( size == (unsigned short)size );

	index = FindArrayVariable(parentNode, 0);

	if ( index )
	{
		flags = GetVariableValueAddress(index)->intValue;
	}

	scrVarPub.checksum *= 31;
	scrVarPub.checksum += parentIndex;

	scrVarPub.checksum *= 31;
	scrVarPub.checksum += childIndex;

	scrVarPub.checksum *= 31;
	scrVarPub.checksum += size;

	scrVarPub.checksum *= 31;
	scrVarPub.checksum += flags;

	XAnimBlend(anims, parentIndex, parentName, childIndex, size, flags);

	parentIndex = childIndex;
	childIndex += size;

	for ( nodeRef = FindNextSibling(parentNode); nodeRef; nodeRef = FindNextSibling(nodeRef) )
	{
		name = GetVariableName(nodeRef);

		if ( name >= SL_MAX_STRING_INDEX )
		{
			continue;
		}

		ConnectScriptToAnim(names, parentIndex, filename, name, treeIndex);

		if ( GetObjectType(nodeRef) == VAR_POINTER )
		{
			childIndex = Scr_CreateAnimationTree(FindObject(nodeRef), names, anims, childIndex, SL_ConvertToString(name), parentIndex, filename, treeIndex);
		}
		else
		{
			assert( parentIndex == (unsigned short)parentIndex );

			scrVarPub.checksum *= 31;
			scrVarPub.checksum += parentIndex;

			XAnimCreate(anims, parentIndex, SL_ConvertToString(name));
		}

		parentIndex++;
	}

	return childIndex;
}

/*
============
Scr_CheckAnimsDefined
============
*/
void Scr_CheckAnimsDefined( unsigned int names, unsigned int filename )
{
	VariableUnion *value;
	unsigned int animId, name;

	for ( animId = FindNextSibling(names); animId; animId = FindNextSibling(animId) )
	{
		name = GetVariableName(animId);
		assert(name < SL_MAX_STRING_INDEX);
		value = GetVariableValueAddress(animId);

		if ( !value->codePosValue )
		{
			continue;
		}

		if ( Scr_IsInScriptMemory(value->codePosValue) )
		{
			CompileError2(value->codePosValue, "%s", va("animation '%s' not defined in anim tree '%s'", SL_ConvertToString(name), SL_ConvertToString(filename)));
		}

		Com_Error(ERR_DROP, "%s", va("animation '%s' not defined in anim tree '%s'", SL_ConvertToString(name), SL_ConvertToString(filename)));
	}
}

/*
============
AnimTreeCompileError
============
*/
void AnimTreeCompileError( const char *msg )
{
	const char *pos = Com_GetLastTokenPos();

	Com_EndParseSession();
	CompileError(pos - scrAnimGlob.start, "%s", msg);
}

const char *propertyNames[] =
{
	"loopsync",
	"nonloopsync",
	"complete"
};

/*
============
GetAnimTreeParseProperties
============
*/
int GetAnimTreeParseProperties()
{
	int flags = 0, i;
	const char *token;

	while ( 1 )
	{
		token = Com_ParseOnLine(&scrAnimGlob.pos);

		if ( !token[0] )
		{
			return flags;
		}

		for ( i = 0; i < ARRAY_COUNT( propertyNames ); i++ )
		{
			if ( !strcasecmp(token, propertyNames[i]) )
			{
				break;
			}
		}

		switch ( i )
		{
		case 0:
			flags |= ANIM_FLAG_LOOPSYNC;
			break;

		case 1:
			flags |= ANIM_FLAG_NONLOOPSYNC;
			break;

		case 2:
			flags |= ANIM_FLAG_COMPLETE;
			break;

		default:
			AnimTreeCompileError("unknown anim property");
			break;
		}
	}

	return flags;
}

/*
============
AnimTreeParseInternal
============
*/
bool AnimTreeParseInternal( unsigned int parentNode, unsigned int names, bool bIncludeParent, bool bLoop, bool bComplete )
{
	VariableValue tempValue;
	unsigned int node, index = 0, currentAnim = 0;
	const char *token;
	int flags = 0;
	bool bIgnore = false;

	tempValue.type = VAR_INTEGER;

	while ( 1 )
	{
		while ( 1 )
		{
			while ( 1 )
			{
				token = Com_Parse(&scrAnimGlob.pos);

				if ( !scrAnimGlob.pos )
				{
					if ( bIgnore )
					{
						RemoveVariable(parentNode, index);
					}

					if ( bIncludeParent && !GetArraySize(parentNode) )
					{
						index = SL_GetString_(bLoop != false ? "void_loop" : "void", 0);
						GetVariable(parentNode, index);
						SL_RemoveRefToString(index);
					}

					return true;
				}

				if ( !Scr_IsIdentifier(token) )
				{
					break;
				}

				if ( bIgnore )
				{
					RemoveVariable(parentNode, index);
				}

				index = SL_GetLowercaseString_(token, 2);

				if ( FindVariable(parentNode, index) )
				{
					AnimTreeCompileError("duplicate animation");
				}

				currentAnim = GetVariable(parentNode, index);
				bIgnore = false;

				if ( !bComplete && !FindVariable(names, index) )
				{
					bIgnore = scrAnimGlob.bAnimCheck == false;
				}

				flags = 0;
				token = Com_ParseOnLine(&scrAnimGlob.pos);

				if ( !token[0] )
				{
					continue;
				}

				if ( Scr_IsIdentifier(token) )
				{
					AnimTreeCompileError("FIXME: aliases not yet implemented");
				}

				if ( token[0] != ':' || token[1] )
				{
					AnimTreeCompileError("bad token");
				}

				flags = GetAnimTreeParseProperties();
				token = Com_Parse(&scrAnimGlob.pos);

				if ( token[0] != '{' || token[1] )
				{
					AnimTreeCompileError("properties cannot be applied to primitive animations");
				}

				break;
			}

			if ( token[0] != '{' )
			{
				break;
			}

			if ( token[1] )
			{
				AnimTreeCompileError("bad token");
			}

			if ( *Com_ParseOnLine(&scrAnimGlob.pos) )
			{
				AnimTreeCompileError("token not allowed after '{'");
			}

			if ( !currentAnim )
			{
				AnimTreeCompileError("no animation specified for this block");
			}

			node = GetArray(currentAnim);

			if ( bComplete || flags & ANIM_FLAG_COMPLETE && !bIgnore )
			{
				if ( AnimTreeParseInternal(node, names, !bIgnore, flags & ANIM_FLAG_LOOPSYNC, true) )
				{
					AnimTreeCompileError("unexpected end of file");
				}
			}
			else
			{
				if ( AnimTreeParseInternal(node, names, !bIgnore, flags & ANIM_FLAG_LOOPSYNC, false) )
				{
					AnimTreeCompileError("unexpected end of file");
				}
			}

			if ( GetArraySize(node) )
			{
				tempValue.u.intValue = flags;
				SetVariableValue(GetArrayVariable(node, 0), &tempValue);
			}
			else
			{
				RemoveVariable(parentNode, index);
			}

			currentAnim = 0;
			bIgnore = false;
		}

		if ( token[0] == '}' )
		{
			break;
		}

		AnimTreeCompileError("bad token");
	}

	if ( token[1] )
	{
		AnimTreeCompileError("bad token");
	}

	if ( *Com_ParseOnLine(&scrAnimGlob.pos) )
	{
		AnimTreeCompileError("token not allowed after '}'");
	}

	if ( bIgnore )
	{
		RemoveVariable(parentNode, index);
	}

	if ( bIncludeParent && !GetArraySize(parentNode) )
	{
		index = SL_GetString_(bLoop != false ? "void_loop" : "void", 0);
		GetVariable(parentNode, index);
		SL_RemoveRefToString(index);
	}

	return false;
}

/*
============
Scr_AnimTreeParse
============
*/
void Scr_AnimTreeParse( const char *pos, unsigned int parentNode, unsigned int names )
{
	Com_BeginParseSession("Scr_AnimTreeParse");

	scrAnimGlob.pos = pos;
	scrAnimGlob.start = pos;

	if ( !AnimTreeParseInternal(parentNode, names, true, false, false) )
	{
		AnimTreeCompileError("bad token");
	}

	Com_EndParseSession();
}

/*
============
Scr_LoadAnimTreeInternal
============
*/
bool Scr_LoadAnimTreeInternal( const char *filename, unsigned int parentNode, unsigned int names )
{
	const char *oldSourceBuf, *oldFilename, *sourceBuffer;
	char extFilename[MAX_QPATH];

	Com_sprintf(extFilename, sizeof(extFilename), "animtrees/%s.atr", filename);
	oldSourceBuf = scrParserPub.sourceBuf;

	sourceBuffer = Scr_AddSourceBuffer(NULL, extFilename, NULL, true);

	if ( !sourceBuffer )
	{
		return false;
	}

	oldFilename = scrParserPub.scriptfilename;
	scrParserPub.scriptfilename = extFilename;

	Scr_AnimTreeParse(sourceBuffer, parentNode, names);

	scrParserPub.scriptfilename = oldFilename;
	scrParserPub.sourceBuf = oldSourceBuf;

	Hunk_ClearTempMemoryHighInternal();

	return GetArraySize(parentNode) != 0;
}

/*
============
Scr_PrecacheAnimationTree
============
*/
void Scr_PrecacheAnimationTree( unsigned int parentNode )
{
	unsigned int name, node;

	for ( node = FindNextSibling(parentNode); node; node = FindNextSibling(node) )
	{
		name = GetVariableName(node);

		if ( name >= SL_MAX_STRING_INDEX )
		{
			continue;
		}

		if ( GetObjectType(node) == VAR_POINTER )
		{
			Scr_PrecacheAnimationTree(FindObject(node));
			continue;
		}

		XAnimPrecache(SL_ConvertToString(name), Hunk_AllocXAnimTreePrecache);
	}
}

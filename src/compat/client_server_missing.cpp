// CoD2rev-Plus functions that its server, game and bgame call and the
// engine's core does not have, for the client, which runs them on that core
// (the dedicated server has CoD2rev-Plus's own). The bodies are CoD2rev-Plus's
// (the file named above each one); where it reaches into its own qcommon, the
// engine's public functions stand in.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "../server/server.h"
#include "../game/g_shared.h"
#include "server_decls.h"
#include "../qcommon/cm_local.h"
#include "script_bridge.h"

extern searchpath_t *fs_searchpaths;  // src/PC/universal/com_files.c
extern char fs_gamedir[];             // src/PC/universal/com_files.c
extern dvar_t *fs_homepath;           // src/PC/universal/com_files.c

// universal/q_shared.cpp
short BigShort( short l )
{
	return (short)( ( ( l & 0xff ) << 8 ) | ( ( l >> 8 ) & 0xff ) );  // _BigShort on a little-endian host
}

// universal/q_shared.cpp
int I_strcmpConstantTime(const char *s0, const char *s1)
{
	size_t len0, len1, len, i;
	unsigned char diff;

	assert(s0);
	assert(s1);

	len0 = strlen(s0);
	len1 = strlen(s1);
	len = len0 > len1 ? len0 : len1;
	diff = len0 != len1;

	for ( i = 0; i < len; i++ )
	{
		diff |= (unsigned char)( i < len0 ? s0[i] : 0 ) ^ (unsigned char)( i < len1 ? s1[i] : 0 );
	}

	return diff;
}

// universal/q_shared.cpp
bool Com_BitCheck( const int array[], int bitNum )
{
	return (array[bitNum >> 5] >> (bitNum & 31)) & 1;
}

// universal/q_shared.cpp
void Com_BitSet( int array[], int bitNum )
{
	array[bitNum >> 5] |= 1 << (bitNum & 31);
}

// universal/q_shared.cpp
void Com_BitClear( int array[], int bitNum )
{
	array[bitNum >> 5] &= ~(1 << (bitNum & 31));
}

// universal/com_math.cpp
void VectorInverse( vec3_t v )
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

// universal/com_math.cpp
vec_t VectorLength( const vec3_t v )
{
	return I_sqrt( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

// universal/com_math.cpp
vec_t Vec2Length( const vec2_t v )
{
	return I_sqrt( v[0] * v[0] + v[1] * v[1] );
}

// universal/com_math.cpp
vec_t Vec2LengthSq( const vec2_t v )
{
	return ( v[0] * v[0] + v[1] * v[1] );
}

// universal/com_math.cpp
vec_t VectorLengthSquared( const vec3_t v )
{
	return ( v[0] * v[0] + v[1] * v[1] + v[2] * v[2] );
}

// universal/com_math.cpp
void Vec3Lerp(const vec3_t start, const vec3_t end, float fraction, vec3_t endpos)
{
	endpos[0] = (end[0] - start[0]) * fraction + start[0];
	endpos[1] = (end[1] - start[1]) * fraction + start[1];
	endpos[2] = (end[2] - start[2]) * fraction + start[2];
}

// universal/com_math.cpp
void RoundFloatArray(vec3_t x, vec3_t y)
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		if ( x[i] < y[i] )
			x[i] = ceil(x[i]);
		else
			x[i] = floor(x[i]);
	}
}

// universal/q_shared.cpp
void SetConfigString(char **ppszConfigString, const char *pszKeyValue)
{
	size_t len;
	char *dest;

	if ( *pszKeyValue )
	{
		len = strlen(pszKeyValue);
		dest = (char *)Hunk_AllocLowAlignInternal(len + 1, 1);
		strcpy(dest, pszKeyValue);
		*ppszConfigString = dest;
	}
	else
	{
		*ppszConfigString = "";
	}
}

// universal/q_shared.cpp
void SetConfigString2(unsigned char *pMember, const char *pszKeyValue)
{
	SetConfigString((char **)pMember, pszKeyValue);
}

// universal/com_files.cpp
void FS_RotateHomeFile(const char *filename, int count)
{
	char from[MAX_OSPATH];
	char to[MAX_OSPATH];
	int i;

	if (count <= 0)
	{
		return;
	}

	FS_BuildOSPath(fs_homepath->current.string, fs_gamedir, va("%s.%i", filename, count), from);
	remove(from);

	for (i = count - 1; i >= 0; i--)
	{
		FS_BuildOSPath(fs_homepath->current.string, fs_gamedir, i ? va("%s.%i", filename, i) : filename, from);
		FS_BuildOSPath(fs_homepath->current.string, fs_gamedir, va("%s.%i", filename, i + 1), to);
		rename(from, to);
	}
}

// universal/com_files.cpp: the engine has no FS_FileOpenReadBinary or
// FS_FileGetFileSize, so stdio stands in.
int FS_HomeFileLength(const char *filename)
{
	char ospath[MAX_OSPATH];
	FILE *f;
	int len;

	FS_BuildOSPath(fs_homepath->current.string, fs_gamedir, filename, ospath);
	f = fopen(ospath, "rb");

	if (!f)
	{
		return -1;
	}

	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fclose(f);

	return len;
}

// universal/com_math.cpp
void SnapAngles(vec3_t angles)
{
	float delta;
	int r;
	int i;

	for ( i = 0; i < 3; ++i )
	{
		r = Q_rint(angles[i]);
		delta = (float)r - angles[i];

		if ( Square(delta) < 0.0000010000001 )
			angles[i] = (float)r;
	}
}

// qcommon/cmd.cpp, through Cmd_Argc and Cmd_Argv: the arguments live in the
// engine's tokenizer, which SV_ExecuteClientCommand has just filled.
void Cmd_ReplaceControlChars( void )
{
	int i;
	char *s;

	for ( i = 0; i < Cmd_Argc(); i++ )
	{
		s = (char *)Cmd_Argv(i);

		if ( i == 1 && *s >= 0x14 && *s <= 0x16 )
			s++;

		for ( ; *s; s++ )
		{
			if ( (unsigned char)*s < ' ' )
				*s = '.';
		}
	}
}

// xanim/dobj.cpp
int DObjSetControlRotTransIndex( const DObj *obj, const int *partBits, int boneIndex )
{
	assert(obj);
	assert(&obj->skel->Mat);
	assert(boneIndex >= 0);
	assert(boneIndex < obj->numBones);

	int boneIndexHigh = boneIndex >> 5;
	int boneIndexLow = 1 << (boneIndex & 0x1F);

	if ( !( partBits[boneIndex >> 5] & boneIndexLow ) )
	{
		return 0;
	}

	DSkel_t *skel = obj->skel;

	if ( skel->partBits.skel[boneIndexHigh] & boneIndexLow )
	{
		return 0;
	}

	assert(!(skel->partBits.skel[boneIndexHigh] & boneIndexLow));
	skel->partBits.control[boneIndexHigh] |= boneIndexLow;
	skel->partBits.anim[boneIndexHigh] |= boneIndexLow;

	return 1;
}

// xanim/dobj.cpp
int DObjSetRotTransIndex( const DObj *obj, const int *partBits, int boneIndex )
{
	assert(obj);
	assert(&obj->skel->Mat);
	assert(boneIndex >= 0);
	assert(boneIndex < obj->numBones);

	int boneIndexHigh = boneIndex >> 5;
	int boneIndexLow = 1 << (boneIndex & 0x1F);

	if ( !( partBits[boneIndex >> 5] & boneIndexLow ) )
	{
		return 0;
	}

	DSkel_t *skel = obj->skel;

	if ( skel->partBits.skel[boneIndexHigh] & boneIndexLow )
	{
		return 0;
	}

	assert(!(skel->partBits.skel[boneIndexHigh] & boneIndexLow));
	skel->partBits.anim[boneIndexHigh] |= boneIndexLow;

	return 1;
}

// universal/com_files.cpp (CVE-2006-2082): the requested iwd against the
// names FS_ReferencedIwdNames builds. The engine's searchpath_s and pack_t
// have the layout of CoD2rev-Plus's searchpath_t and iwd_t.
qboolean FS_VerifyIwd( const char *iwd )
{
	char teststring[ BIG_INFO_STRING ];
	searchpath_t    *search;

	for ( search = fs_searchpaths ; search ; search = search->next )
	{
		if ( search->iwd )
		{
			Q_strncpyz( teststring, search->iwd->iwdGamename, sizeof( teststring ) );
			Q_strncat( teststring, sizeof( teststring ), "/" );
			Q_strncat( teststring, sizeof( teststring ), search->iwd->iwdBasename );
			Q_strncat( teststring, sizeof( teststring ), ".iwd" );
			if ( !Q_stricmp( teststring, iwd ) )
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}

// qcommon/msg_mp.cpp: the engine's versions take no buffer and return a
// static one, and keep '%' and control characters.
char *MSG_ReadStringLine( msg_t *msg, char *string, unsigned int maxChars )
{
	int c;
	unsigned int l;

	for ( l = 0; ; l++ )
	{
		c = MSG_ReadByte(msg); // use ReadByte so -1 is out of bounds

		// translate all fmt spec to avoid crash bugs
		if ( c == '%' )
		{
			c = '.';
		}
		else if ( c == '\n' || c == -1 )
		{
			c = 0;
		}

		if ( l < maxChars )
		{
			string[l] = I_CleanChar(c);
		}

		if ( c == 0 )
		{
			break;
		}
	}

	string[maxChars - 1] = 0;

	return string;
}

// qcommon/msg_mp.cpp
char *MSG_ReadString( msg_t *msg, char *string, unsigned int maxChars )
{
	int c;
	unsigned int l;

	assert(string);

	for ( l = 0; ; l++ )
	{
		c = MSG_ReadByte(msg);

		if ( c == -1 )
		{
			c = 0;
		}

		if ( l < maxChars )
		{
			string[l] = I_CleanChar(c);
		}

		if ( c == 0 )
		{
			break;
		}
	}

	string[maxChars - 1] = 0;

	return string;
}

// qcommon/common.cpp. In the client the engine's Com_*WeaponInfoMemory belong
// to cgame's bgame (src/PC/bgame); the local server's weapon defs are in
// CoD2rev-Plus's bgame, so it keeps its own owner and frees its own defs.
// Otherwise a full map_restart found the old weapon count, did not register
// the weapons again, and the anim script named weapons it did not know.
static int iWeaponInfoSource;

void Com_SetWeaponInfoMemory( int iSource )
{
	iWeaponInfoSource = iSource;
}

void Com_FreeWeaponInfoMemory( int iSource )
{
	if ( iSource == iWeaponInfoSource )
	{
		iWeaponInfoSource = 0;
		BG_ShutdownWeaponDefFiles();
	}
}

// The client runs CoD2rev-Plus's collision (qcommon/cm_*) too, on the engine's
// BSP loader and memory.

// qcommon/threads.cpp: one per thread context; q_shared.c points the thread
// value 3 at the context's entry, and CM_InitThreadData fills it
TraceThreadInfo g_traceThreadInfo[NUMTHREADS];

// qcommon/com_bsp_load_obj.cpp, which only defines it there
struct lump_t
{
	int filelen;
	int fileofs;
};

struct dheader_t
{
	int ident;
	int version;
	lump_t lumps[LUMP_COUNT];
};

// qcommon/com_bsp_load_obj.cpp: over the BSP the engine's Com_LoadBsp loaded
byte* Com_GetBspLump( int type, int elemSize, int *count )
{
	int fileSize;
	dheader_t *header = Com_GetBsp(&fileSize, NULL);
	int offset = header->lumps[type].fileofs;
	int length = header->lumps[type].filelen;

	if ( length )
	{
		if ( type == LUMP_ENTITIES && length + offset > fileSize )
		{
			*count = ( length / elemSize ) + ( fileSize - ( length + offset ) );
			return (byte *)header + offset;
		}

		if ( length + offset > fileSize )
		{
			Com_Error(ERR_DROP, "CM_LoadMap: lump %i extends past end of file", type);
		}

		*count = length / elemSize;

		if ( elemSize * *count != length )
		{
			Com_Error(ERR_DROP, "CM_LoadMap: lump %i has funny size", type);
		}

		return (byte *)header + offset;
	}

	*count = 0;
	return NULL;
}

// qcommon/com_bsp_load_obj.cpp
bool Com_BspHasLump( int type )
{
	int count;

	Com_GetBspLump(type, 1, &count);

	return count != 0;
}

// universal/q_shared.cpp: a little-endian host (the engine's LittleShort and
// LittleLong are function pointers, which an import would call as code)
short LittleShort( short l )
{
	return l;
}

// universal/q_shared.cpp
int LittleLong( int l )
{
	return l;
}

// universal/q_shared.cpp
float LittleFloat( float l )
{
	return l;
}

// universal/com_memory.cpp: the engine's low hunk keeps its temporary
// allocations itself
extern "C" void cod2_engine_Hunk_ClearTempMemory( void ) __asm__(COD2_SYM("Hunk_ClearTempMemory"));
void Hunk_ClearTempMemoryInternal()
{
	cod2_engine_Hunk_ClearTempMemory();
}

// universal/com_math.cpp
void CreateRotationMatrix( const vec3_t angles, vec3_t matrix[3] )
{
	AngleVectors( angles, matrix[0], matrix[1], matrix[2] );
	VectorInverse( matrix[1] );
}

// universal/com_math.cpp
void TransposeMatrix( const vec3_t matrix[3], vec3_t transpose[3] )
{
	int i, j;

	for ( i = 0; i < 3; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			transpose[i][j] = matrix[j][i];
		}
	}
}

// universal/com_math.cpp
void RotatePoint( vec3_t point, const vec3_t matrix[3] )
{
	vec3_t tvec;

	VectorCopy( point, tvec );

	point[0] = DotProduct( matrix[0], tvec );
	point[1] = DotProduct( matrix[1], tvec );
	point[2] = DotProduct( matrix[2], tvec );
}

// The engine's collision functions CoD2rev-Plus does not have, which the
// client's renderer and the blobs' imp_ pointers call by their C names.

// src/PC/qcommon/cm_load_obj.c
extern "C" cplane_t *CM_GetPlaneNum( int planeNum )
{
	return &cm.planes[planeNum];
}

// src/PC/qcommon/cm_load.c rewrites a lump of the map's BSP: a developer tool
// only the imp_ pointers name, which nothing runs
extern "C" void CM_SaveLump( int lumpnum, byte *newLump, int size, int *checksum )
{
	Com_Error(ERR_FATAL, "CM_SaveLump is not in this build");
}

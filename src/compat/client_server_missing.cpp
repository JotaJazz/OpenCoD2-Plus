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

// universal/q_shared.cpp (the engine's has C linkage; plusid.cpp declares this one)
void Info_RemoveKey( char *s, const char *key )
{
	char	*start;
	char	pkey[MAX_INFO_KEY];
	char	value[MAX_INFO_VALUE];
	char	*o;

	if ( strlen( s ) >= MAX_INFO_STRING )
	{
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring" );
	}

	if (strchr (key, '\\'))
	{
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			memmove (start, s, strlen(s) + 1);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

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

// universal/q_parse.cpp, with a buffer of its own: the engine has
// Com_ParseOnLine, not the parse thread info CoD2rev-Plus keeps the line in
const char *Com_ParseRestOfLine( const char **data_p )
{
	static char line[MAX_TOKEN_CHARS];
	const char *token;

	line[0] = 0;
	while ( 1 )
	{
		token = Com_ParseOnLine( data_p );
		if ( !token[0] )
		{
			break;
		}
		if ( line[0] )
		{
			Q_strncat( line, MAX_TOKEN_CHARS, " " );
		}
		Q_strncat( line, MAX_TOKEN_CHARS, token );
	}

	return line;
}

#ifdef COD2_PLUS
// xanim/xanim.cpp, for the Plus antilag's rewind of a player's anim tree: the
// engine's XAnimAllocInfoForTree and XAnimFreeInfoToList are static inline,
// over the same g_xAnimInfo free list
XAnimInfo* XAnimAllocInfo( XAnimTree_s *tree, unsigned int infoIndex )
{
	unsigned int next = g_xAnimInfo->next;

	if ( !next )
	{
		Com_Error(ERR_DROP, "exceeded maximum number of anim info");
	}

	g_xAnimInfo->next = g_xAnimInfo[next].next;
	g_xAnimInfo[g_xAnimInfo->next].prev = 0;

	tree->infoArray[infoIndex] = next;
	return &g_xAnimInfo[next];
}

// xanim/xanim.cpp, with XAnimClearServerNotify
void XAnimFreeInfo( XAnimTree_s *tree, unsigned int infoIndex )
{
	XAnimInfo *info = &g_xAnimInfo[infoIndex];

	(void)tree;
	if ( info->notifyName )
	{
		SL_RemoveRefToString( info->notifyName );
		info->notifyName = 0;
	}
	info->notifyIndex = -1;

	info->prev = 0;
	info->next = g_xAnimInfo->next;

	g_xAnimInfo[g_xAnimInfo->next].prev = infoIndex;
	g_xAnimInfo->next = infoIndex;
}
#endif

#if defined(COD2_PLUS) && !defined(_WIN32)
#include <arpa/inet.h>
#include <netdb.h>

// unix/unix_net.cpp, for the Plus server's own HTTP downloads
// (server/sv_www_plus.cpp), which bind the game's net_ip
qboolean Sys_StringToSockaddr( const char *s, struct sockaddr *sadr )
{
	struct hostent *h;

	memset (sadr, 0, sizeof(*sadr));
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	((struct sockaddr_in *)sadr)->sin_port = 0;

	if ( s[0] >= '0' && s[0] <= '9' )
	{
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(s);
	}
	else
	{
		if ( !(h = gethostbyname(s)) )
			return qfalse;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}

	return qtrue;
}
#endif

#ifdef LIBCOD
// What libcod calls of CoD2rev-Plus's platform code (src/unix/unix_threads.cpp,
// src/win32/win_threads.cpp), which the client does not link: its critical
// sections, its threads and its sleep. The engine's own threads keep the
// engine's primitives; these serve libcod's sections (CRITSECT_EXEC,
// _MYSQL, _SQLITE, _SOUND) only.
#ifdef _WIN32
static CRITICAL_SECTION crit_sections[CRITSECT_COUNT];
static INIT_ONCE crit_sectionsOnce = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK Sys_InitializeCriticalSections( PINIT_ONCE, PVOID, PVOID * )
{
	for ( int i = 0; i < CRITSECT_COUNT; i++ )
		InitializeCriticalSection( &crit_sections[i] );
	return TRUE;
}

// win32/win_threads.cpp, the sections made on first use: the client has no
// Sys_InitMainThread of CoD2rev-Plus's
void Sys_EnterCriticalSection( int section )
{
	InitOnceExecuteOnce( &crit_sectionsOnce, Sys_InitializeCriticalSections, NULL, NULL );
	EnterCriticalSection( &crit_sections[section] );
}

void Sys_LeaveCriticalSection( int section )
{
	LeaveCriticalSection( &crit_sections[section] );
}

struct sysThreadStart_t
{
	void *(*func)( void * );
	void *arg;
};

static DWORD WINAPI Sys_ThreadStart( LPVOID param )
{
	sysThreadStart_t start = *(sysThreadStart_t *)param;

	free( param );
	start.func( start.arg );
	return 0;
}

// win32/win_threads.cpp (Sys_CreateThreadWithHandle)
qboolean Sys_CreateNewThread( void *(*ThreadMain)( void * ), threadid_t *tid, void *arg )
{
	sysThreadStart_t *start = (sysThreadStart_t *)malloc( sizeof( *start ) );
	HANDLE thid;

	if ( start == NULL )
	{
		Com_Printf("Failed to start thread!\n");
		return qfalse;
	}

	start->func = ThreadMain;
	start->arg = arg;

	thid = CreateThread( NULL, 0, Sys_ThreadStart, start, 0, tid );
	if ( thid == NULL )
	{
		free( start );
		Com_Printf("Failed to start thread with error %lu\n", GetLastError());
		return qfalse;
	}

	CloseHandle( thid );
	return qtrue;
}

// win32/win_threads.cpp
void Sys_SleepMSec( int msec )
{
	Sleep( msec );
}
#else
static pthread_mutex_t crit_sections[CRITSECT_COUNT];
static pthread_once_t crit_sectionsOnce = PTHREAD_ONCE_INIT;

static void Sys_InitializeCriticalSections( void )
{
	pthread_mutexattr_t muxattr;

	pthread_mutexattr_init(&muxattr);
	pthread_mutexattr_settype(&muxattr, PTHREAD_MUTEX_RECURSIVE);

	for ( int i = 0; i < CRITSECT_COUNT; i++ )
		pthread_mutex_init( &crit_sections[i], &muxattr );

	pthread_mutexattr_destroy(&muxattr);
}

// unix/unix_threads.cpp, the sections made on first use: the client has no
// Sys_InitMainThread of CoD2rev-Plus's
void Sys_EnterCriticalSection( int section )
{
	pthread_once( &crit_sectionsOnce, Sys_InitializeCriticalSections );
	pthread_mutex_lock(&crit_sections[section]);
}

void Sys_LeaveCriticalSection( int section )
{
	pthread_mutex_unlock(&crit_sections[section]);
}

// unix/unix_threads.cpp, without the alternate signal stack it gives each
// worker for CoD2rev-Plus's crash handler, which the client does not have
int Sys_CreatePthread( pthread_t *thread, const pthread_attr_t *attr, void *(*func)( void * ), void *arg )
{
	return pthread_create( thread, attr, func, arg );
}

#define MIN_STACKSIZE 256*1024
// unix/unix_threads.cpp
qboolean Sys_CreateNewThread( void *(*ThreadMain)( void * ), threadid_t *tid, void *arg )
{
	int err;
	pthread_attr_t tattr;

	err = pthread_attr_init(&tattr);
	if(err != 0)
	{
		Com_Printf("pthread_attr_init(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}

	err = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_Printf("pthread_attr_setdetachstate(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}

	err = pthread_attr_setstacksize(&tattr, MIN_STACKSIZE);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_Printf("pthread_attr_setstacksize(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}

	err = Sys_CreatePthread(tid, &tattr, ThreadMain, arg);
	if(err != 0)
	{
		pthread_attr_destroy(&tattr);
		Com_Printf("pthread_create(): Thread creation failed with the following error: %s\n", strerror(err));
		return qfalse;
	}
	pthread_attr_destroy(&tattr);
	return qtrue;
}

// unix/unix_threads.cpp
void Sys_SleepMSec( int msec )
{
	struct timespec ts;
	ts.tv_sec = msec / 1000;
	ts.tv_nsec = (msec % 1000) * 1000000;
	nanosleep(&ts, NULL);
}
#endif
#endif

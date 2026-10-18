// CoD2rev-Plus functions that src/server calls and this engine does not
// have. The bodies are CoD2rev-Plus's (the file named above each one); where
// it reaches into its own qcommon, the engine's public functions stand in.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "../server/server.h"
#include "../game/g_shared.h"

extern searchpath_t *fs_searchpaths;  // src/PC/universal/com_files.c

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

// game/g_load_utils.cpp
qboolean G_GetEntityToken( char *buffer, int bufferSize )
{
	const char* s;

	// Parse the next token from sv.entityParsePoint
	s = Com_Parse(&sv.entityParsePoint);

	// Copy the parsed token into the destination buffer
	Q_strncpyz(buffer, s, bufferSize);

	// If sv.entityParsePoint is NULL and the parsed token is empty,
	// we've reached the end (return qfalse), otherwise qtrue
	if (!sv.entityParsePoint && !s[0])
	{
		return qfalse;
	}

	return qtrue;
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

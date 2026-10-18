#include "../qcommon/qcommon.h"
#include "g_shared.h"

/*
===================
Svcmd_EntityList_f
===================
*/
void    Svcmd_EntityList_f( void )
{
	int e;
	gentity_t       *check;

	check = g_entities + 1;
	for ( e = 1; e < level.num_entities ; e++, check++ )
	{
		if ( !check->r.inuse )
		{
			continue;
		}
		Com_Printf( "%3i:", e );
		switch ( check->s.eType )
		{
		case ET_GENERAL:
			Com_Printf( "ET_GENERAL          " );
			break;
		case ET_PLAYER:
			Com_Printf( "ET_PLAYER           " );
			break;
		case ET_ITEM:
			Com_Printf( "ET_ITEM             " );
			break;
		case ET_MISSILE:
			Com_Printf( "ET_MISSILE          " );
			break;
		case ET_INVISIBLE:
			Com_Printf( "ET_INVISIBLE        " );
			break;
		case ET_SCRIPTMOVER:
			Com_Printf( "ET_SCRIPTMOVER      " );
			break;
		default:
			Com_Printf( "%3i                 ", check->s.eType );
			break;
		}

		if ( check->classname )
		{
			Com_Printf( "%s", SL_ConvertToString( check->classname ) );
		}
		Com_Printf( "\n" );
	}
}

#define MAX_CVAR_VALUE_STRING   256

typedef struct ipFilter_s
{
	unsigned mask;
	unsigned compare;
} ipFilter_t;

#define MAX_IPFILTERS   1024

typedef struct ipFilterList_s
{
	ipFilter_t ipFilters[MAX_IPFILTERS];
	int numIPFilters;
} ipFilterList_t;

static ipFilterList_t ipFilters;

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter( const char *s, ipFilter_t *f )
{
	char num[128];
	int i, j;
	byte b[4];
	byte m[4];

	for ( i = 0 ; i < 4 ; i++ )
	{
		b[i] = 0;
		m[i] = 0;
	}

	for ( i = 0 ; i < 4 ; i++ )
	{
		if ( *s < '0' || *s > '9' )
		{
			if ( *s == '*' )   // 'match any'
			{
				// b[i] and m[i] to 0
				s++;
				if ( !*s )
				{
					break;
				}
				s++;
				continue;
			}
			Com_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while ( *s >= '0' && *s <= '9' )
		{
			if ( j == 3 )
			{
				Com_Printf( "Bad filter address: %s\n", s );
				return qfalse;
			}
			num[j++] = *s++;
		}
		num[j] = 0;
		if ( atoi( num ) > 255 )
		{
			Com_Printf( "Bad filter address: %s\n", num );
			return qfalse;
		}
		b[i] = atoi( num );
		m[i] = 255;

		if ( !*s )
		{
			break;
		}
		s++;
	}

	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return qtrue;
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket( const byte *ip )
{
	unsigned in;
	int i;

	in = *(const unsigned *)ip;

	for ( i = 0 ; i < ipFilters.numIPFilters ; i++ )
	{
		if ( ( in & ipFilters.ipFilters[i].mask ) == ipFilters.ipFilters[i].compare )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
UpdateIPBans
=================
*/
void UpdateIPBans( void )
{
	byte b[4];
	byte m[4];
	int i,j;
	char iplist_final[MAX_CVAR_VALUE_STRING];
	char ip[64];

	*iplist_final = 0;
	for ( i = 0 ; i < ipFilters.numIPFilters ; i++ )
	{
		if ( ipFilters.ipFilters[i].compare == 0xffffffff )
		{
			continue;
		}

		*(unsigned *)b = ipFilters.ipFilters[i].compare;
		*(unsigned *)m = ipFilters.ipFilters[i].mask;
		*ip = 0;
		for ( j = 0; j < 4 ; j++ )
		{
			if ( m[j] != 255 )
			{
				Q_strncat( ip, sizeof( ip ), "*" );
			}
			else
			{
				Q_strncat( ip, sizeof( ip ), va( "%i", b[j] ) );
			}
			Q_strncat( ip, sizeof( ip ), ( j < 3 ) ? "." : " " );
		}
		if ( strlen( iplist_final ) + strlen( ip ) < MAX_CVAR_VALUE_STRING )
		{
			Q_strncat( iplist_final, sizeof( iplist_final ), ip );
		}
	}

	Dvar_SetString(g_banIPs, iplist_final);
}

/*
=================
AddIP
=================
*/
void AddIP( const char *str )
{
	int i;

	for ( i = 0; i < ipFilters.numIPFilters; i++ )
	{
		if (  ipFilters.ipFilters[i].compare == 0xffffffff )
		{
			break;      // free spot
		}
	}

	if ( i == ipFilters.numIPFilters )
	{
		if ( ipFilters.numIPFilters == MAX_IPFILTERS )
		{
			Com_Printf( "IP filter list is full\n" );
			return;
		}
		ipFilters.numIPFilters++;
	}

	if ( !StringToFilter( str, &ipFilters.ipFilters[i] ) )
	{
		ipFilters.ipFilters[i].compare = 0xffffffffu;
	}

	UpdateIPBans();
}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f( void )
{
	ipFilter_t f;
	int i;
	char str[MAX_TOKEN_CHARS];

	if ( SV_Cmd_Argc() < 2 )
	{
		Com_Printf( "Usage:  sv removeip <ip-mask>\n" );
		return;
	}

	SV_Cmd_ArgvBuffer( 1, str, sizeof( str ) );

	if ( !StringToFilter( str, &f ) )
	{
		return;
	}

	for ( i = 0 ; i < ipFilters.numIPFilters ; i++ )
	{
		if ( ipFilters.ipFilters[i].mask == f.mask   &&
		        ipFilters.ipFilters[i].compare == f.compare )
		{
			ipFilters.ipFilters[i].compare = 0xffffffffu;
			Com_Printf( "Removed.\n" );

			UpdateIPBans();
			return;
		}
	}

	Com_Printf( "Didn't find %s.\n", str );
}

/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f( void )
{
	char str[MAX_TOKEN_CHARS];

	if ( SV_Cmd_Argc() < 2 )
	{
		Com_Printf( "Usage:  addip <ip-mask>\n" );
		return;
	}

	SV_Cmd_ArgvBuffer( 1, str, sizeof( str ) );

	AddIP( str );
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans( void )
{
	char *s, *t;
	char str[MAX_CVAR_VALUE_STRING];

	ipFilters.numIPFilters = 0;

	// AddIP sets g_banIPs again, so walk a copy of it
	Q_strncpyz( str, g_banIPs->current.string, sizeof( str ) );

	for ( t = s = str; *t; /* */ )
	{
		s = strchr( s, ' ' );
		if ( !s )
		{
			AddIP( t );
			break;
		}
		while ( *s == ' ' )
			*s++ = 0;
		if ( *t )
		{
			AddIP( t );
		}
		t = s;
	}
}

// fretn, note: if a player is called '3' and there are only 2 players
// on the server (clientnum 0 and 1)
// this function will say 'client 3 is not connected'
// solution: first check for usernames, if none is found, check for slotnumbers
gclient_t   *ClientForString( const char *s )
{
	gclient_t   *cl;
	int i;
	int idnum;

	// check for a name match
	for ( i = 0 ; i < level.maxclients ; i++ )
	{
		cl = &level.clients[i];
		if ( cl->sess.connected == CON_DISCONNECTED )
		{
			continue;
		}
		if ( !Q_stricmp( cl->sess.cs.name, s ) )
		{
			return cl;
		}
	}

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' )
	{
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients )
		{
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->sess.connected == CON_DISCONNECTED )
		{
			Com_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	Com_Printf( "User %s is not on the server\n", s );

	return NULL;
}

/*
=================
ConsoleCommand
=================
*/
qboolean ConsoleCommand()
{
	char cmd[MAX_TOKEN_CHARS];

	SV_Cmd_ArgvBuffer( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "entitylist" ) == 0 )
	{
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "addip" ) == 0 )
	{
		Svcmd_AddIP_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "removeip" ) == 0 )
	{
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "listip" ) == 0 )
	{
		Cbuf_ExecuteText(EXEC_INSERT, "g_banIPs\n");
		return qtrue;
	}

	// if ( g_dedicated.integer ) {
	if ( Q_stricmp( cmd, "say" ) == 0 )
	{
		SV_GameSendServerCommand(-1, SV_CMD_CAN_IGNORE, va("%c \"GAME_SERVER\x15: %s\"", 101, ConcatArgs(1)));
		return qtrue;
	}

	Com_Printf("Unknown command \"%s\"\n", cmd);
	return qfalse;
}

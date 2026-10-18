#include "../qcommon/qcommon.h"

/*
==============
SV_FlushRedirect
==============
*/
void SV_FlushRedirect( char *outputbuf )
{
	char buf[FRAGMENT_SIZE];
	char c;
	size_t len;

	len = strlen(outputbuf);

	while ( len > sizeof(buf) - 6 )
	{
		c = outputbuf[sizeof(buf) - 6];
		outputbuf[sizeof(buf) - 6] = 0;
		Com_sprintf(buf, sizeof(buf), "print\n%s", outputbuf);
		NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, buf);
		len -= sizeof(buf) - 6;
		outputbuf += sizeof(buf) - 6;
		*outputbuf = c;
	}

	Com_sprintf(buf, sizeof(buf), "print\n%s", outputbuf);
	NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, buf);
}

/*
==============================================================================

RCON BRUTE FORCE PROTECTION

Every address gets a few attempts. After sv_rconMaxAttempts wrong passwords
inside sv_rconLockTime seconds the address is ignored by rcon for
sv_rconLockTime seconds. A correct password clears the counter. Addresses on the
server machine itself (127.x.x.x) and those in sv_rconWhitelist are never locked.
sv_rconMaxAttempts 0 disables the lock.

==============================================================================
*/
#define MAX_RCON_TRACKED 64

typedef struct
{
	netadr_t adr;
	int failures;
	int firstFailureTime;
	int lockedUntil;
	int lastSeen;
	bool inUse;
} rconAttempt_t;

static rconAttempt_t rconAttempts[MAX_RCON_TRACKED];

static rconAttempt_t *SV_RconAttemptForAddress( netadr_t from, bool create )
{
	int now = Sys_Milliseconds();
	int i;

	for ( i = 0; i < MAX_RCON_TRACKED; i++ )
	{
		if ( rconAttempts[i].inUse && NET_CompareBaseAdr( rconAttempts[i].adr, from ) )
		{
			rconAttempts[i].lastSeen = now;
			return &rconAttempts[i];
		}
	}

	if ( !create )
	{
		return NULL;
	}

	// Take a free slot, otherwise recycle the least recently seen address,
	// preferring addresses that aren't currently locked
	rconAttempt_t *slot = NULL;

	for ( i = 0; i < MAX_RCON_TRACKED; i++ )
	{
		rconAttempt_t *entry = &rconAttempts[i];

		if ( !entry->inUse )
		{
			slot = entry;
			break;
		}

		if ( !slot )
		{
			slot = entry;
			continue;
		}

		bool entryLocked = entry->lockedUntil - now > 0;
		bool slotLocked = slot->lockedUntil - now > 0;

		if ( entryLocked != slotLocked )
		{
			if ( slotLocked )
			{
				slot = entry;
			}
		}
		else if ( entry->lastSeen - slot->lastSeen < 0 )
		{
			slot = entry;
		}
	}

	memset( slot, 0, sizeof( *slot ) );
	slot->adr = from;
	slot->inUse = true;
	slot->lastSeen = now;

	return slot;
}

static bool SV_RconIsWhitelisted( netadr_t from )
{
	const char *list = sv_rconWhitelist->current.string;
	char token[32];
	netadr_t adr;

	while ( *list )
	{
		int len = strcspn( list, " ," );

		// IP addresses only, a host name would block the server on a lookup
		if ( len > 0 && len < (int)sizeof( token ) && (int)strspn( list, "0123456789." ) == len )
		{
			memcpy( token, list, len );
			token[len] = 0;

			if ( NET_StringToAdr( token, &adr ) && NET_CompareBaseAdr( adr, from ) )
			{
				return true;
			}
		}

		list += len;
		list += strspn( list, " ," );
	}

	return false;
}

static bool SV_RconIsExempt( netadr_t from )
{
	// Tools running on the server machine itself (127.x.x.x) are trusted, and
	// so are the admin's addresses, which spoofed bad guesses could lock out
	return NET_IsLocalAddress( from ) || ( from.type == NA_IP && from.ip[0] == 127 ) || SV_RconIsWhitelisted( from );
}

static bool SV_RconIsLocked( netadr_t from )
{
	if ( !sv_rconMaxAttempts->current.integer || SV_RconIsExempt( from ) )
	{
		return false;
	}

	rconAttempt_t *entry = SV_RconAttemptForAddress( from, false );

	return entry && entry->lockedUntil && entry->lockedUntil - Sys_Milliseconds() > 0;
}

static void SV_RconRegisterResult( netadr_t from, bool valid )
{
	if ( !sv_rconMaxAttempts->current.integer || SV_RconIsExempt( from ) )
	{
		return;
	}

	if ( valid )
	{
		rconAttempt_t *entry = SV_RconAttemptForAddress( from, false );

		if ( entry )
		{
			memset( entry, 0, sizeof( *entry ) );
		}

		return;
	}

	rconAttempt_t *entry = SV_RconAttemptForAddress( from, true );

	if ( !entry )
	{
		return;
	}

	int now = Sys_Milliseconds();
	int window = sv_rconLockTime->current.integer * 1000;

	// Start counting again once the previous burst of failures is old enough
	if ( !entry->failures || now - entry->firstFailureTime > window )
	{
		entry->failures = 0;
		entry->firstFailureTime = now;
	}

	entry->failures++;

	if ( entry->failures >= sv_rconMaxAttempts->current.integer )
	{
		entry->lockedUntil = now + window;
		entry->failures = 0;

		Com_Printf( "Rcon: %s locked for %i seconds after %i failed attempts\n",
		            NET_AdrToString( from ), sv_rconLockTime->current.integer, sv_rconMaxAttempts->current.integer );
	}
}

// show_bug.cgi?id=376
// if we send an OOB print message this size, 1.31 clients die in a Com_Printf buffer overflow
// the buffer overflow will be fixed in > 1.31 clients
// but we want a server side fix
// we must NEVER send an OOB message that will be > 1.31 MAXPRINTMSG (4096)
#define SV_OUTPUTBUF_LENGTH ( 256 * MAX_CLIENTS - 16 )

/*
===============
SV_ExecuteRemoteCommand
Runs an accepted rcon command and sends its output back to from
===============
*/
void SV_ExecuteRemoteCommand( netadr_t from, const char *command )
{
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	// FIXME TTimo our rcon redirection could be improved
	//   big rcon commands such as status lead to sending
	//   out of band packets on every single call to Com_Printf
	//   which leads to client overflows
	//   see show_bug.cgi?id=51
	//     (also a Q3 issue)
	Com_BeginRedirect( sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect );
	SV_Cmd_ExecuteString( command );
	Com_EndRedirect();
}

/*
===============
SVC_RemoteCommand
An rcon packet arrived from the network.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand( netadr_t from, msg_t *msg )
{
	qboolean valid;
	int time;
	char remaining[1024];
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	static int lasttime = 0;

#ifdef LIBCOD
	if (!sv_allowRcon->current.boolean)
		return;
#endif

#if LIBCOD_COMPILE_RATELIMITER == 1
	static leakyBucket_t bucket;
	// Prevent using rcon as an amplifier and make dictionary attacks impractical
	if ( SVC_RateLimitAddress( from, 10, 1000 ) )
	{
		Com_DPrintf( "SVC_RemoteCommand: rate limit from %s exceeded, dropping request\n", NET_AdrToString( from ) );
		return;
	}
#endif

	// Ignore locked addresses before they can use up the global limits below
	if ( SV_RconIsLocked( from ) )
	{
		Com_DPrintf( "SVC_RemoteCommand: %s is locked out, dropping request\n", NET_AdrToString( from ) );
		return;
	}

	const char *password = Cmd_Argv( 1 );

	valid = strlen( rcon_password->current.string ) &&
	        !I_strcmpConstantTime( password, rcon_password->current.string );

	// Count every guess, also those the global limits below drop, or
	// requests from many addresses would keep each one from being locked
	if ( rcon_password->current.string[0] )
	{
		SV_RconRegisterResult( from, valid );
	}

	// The global limits only apply to wrong passwords, so that bad requests
	// sent from many addresses can't keep the admin from using rcon
	if ( !valid )
	{
#if LIBCOD_COMPILE_RATELIMITER == 1
		// Make DoS via rcon impractical
		if ( SVC_RateLimit( &bucket, 10, 1000 ) )
		{
			Com_DPrintf( "SVC_RemoteCommand: rate limit exceeded, dropping request\n" );
			return;
		}
#endif

		// TTimo - show_bug.cgi?id=534
		time = Com_Milliseconds();
		if ( lasttime && time - lasttime < 500 )
		{
			return;
		}
		lasttime = time;

		Com_Printf( "Bad rcon from %s:\n%s\n", NET_AdrToString( from ), Cmd_Argv( 2 ) );
	}
	else if ( sv_logRcon->current.boolean )
	{
		Com_Printf( "Rcon from %s:\n%s\n", NET_AdrToString( from ), Cmd_Argv( 2 ) );
	}

#ifdef LIBCOD
	// CodeCallback_RemoteCommand decides what to do with accepted commands
	if ( valid && rcon_password->current.string[0] && Scr_RemoteCommandCallback( from ) )
	{
		return;
	}
#endif

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	// FIXME TTimo our rcon redirection could be improved
	//   big rcon commands such as status lead to sending
	//   out of band packets on every single call to Com_Printf
	//   which leads to client overflows
	//   see show_bug.cgi?id=51
	//     (also a Q3 issue)
	Com_BeginRedirect( sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect );

	if ( !rcon_password->current.string[0] )
	{
		Com_Printf("The server must set 'rcon_password' for clients to use 'rcon'.\n");
	}
	else if ( valid )
	{
		// ATVI Wolfenstein Misc #284
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		int len = 0;
		int maxlen = 1024;

		for ( int i = 2; i < SV_Cmd_Argc(); ++i )
		{
			len = Com_AddToString(SV_Cmd_Argv(i), remaining, len, maxlen, 1);
			len = Com_AddToString(" ", remaining, len, maxlen, 0);
		}

		if ( len < maxlen )
		{
			remaining[len] = 0;
			SV_Cmd_ExecuteString(remaining);
		}
	}
	else if ( password[0] )
	{
		Com_Printf("Invalid password.\n");
	}
	else
	{
		Com_Printf("You must log in with 'rcon login <password>' before using 'rcon'.\n");
	}

	Com_EndRedirect();
}

void SV_MatchEnd( void )
{
	;
}

/*
================
SV_MasterAddress
================
*/
netadr_t *SV_MasterAddress()
{
	static netadr_t sv_master_address;
	unsigned short port;

	// Resolve only once: the lookup blocks the whole server, and this is called
	// for every challenge that is waiting on the authorize server.
	if ( sv_master_address.type != NA_BOT )
	{
		return &sv_master_address;
	}

	Com_Printf("Resolving %s\n", MASTER_SERVER_NAME);

	if ( !NET_StringToAdr(MASTER_SERVER_NAME, &sv_master_address) )
	{
		Com_Printf("Couldn't resolve address: %s\n", MASTER_SERVER_NAME);
		return &sv_master_address;
	}

	if ( !strstr(":", MASTER_SERVER_NAME) )
	{
		sv_master_address.port = BigShort(PORT_MASTER);
	}

	port = BigShort(sv_master_address.port);

	Com_Printf(
	    "%s resolved to %i.%i.%i.%i:%i\n",
	    MASTER_SERVER_NAME,
	    sv_master_address.ip[0],
	    sv_master_address.ip[1],
	    sv_master_address.ip[2],
	    sv_master_address.ip[3],
	    port);

	return &sv_master_address;
}

/*
=================
SV_MasterGameCompleteStatus
NERVE - SMF - Sends gameCompleteStatus messages to all master servers
=================
*/
void SV_MasterGameCompleteStatus()
{
	netadr_t *adr;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if ( !com_dedicated || com_dedicated->current.integer != 2 )
	{
		return;     // only dedicated servers send master game status
	}

	adr = SV_MasterAddress();

	if ( adr->type != NA_BAD )
	{
		Com_Printf("Sending gameCompleteStatus to %s\n", MASTER_SERVER_NAME);
		SVC_GameCompleteStatus(*adr);
	}
}

void SV_MasterHeartbeat( const char *hbname )
{
	netadr_t *adr;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if ( !com_dedicated || com_dedicated->current.integer != 2 )
	{
		return;     // only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if ( svs.time >= svs.nextHeartbeatTime )
	{
		svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

		adr = SV_MasterAddress();

		if ( adr->type != NA_BAD )
		{
			Com_Printf("Sending heartbeat to %s\n", MASTER_SERVER_NAME);
			NET_OutOfBandPrint(NS_SERVER, *adr, va("heartbeat %s\n", hbname));
		}
	}

	// if not time yet, don't send anything
	if ( svs.time >= svs.nextStatusResponseTime )
	{
		svs.nextStatusResponseTime = svs.time + STATUS_MSEC;

		adr = SV_MasterAddress();

		if ( adr->type != NA_BAD )
		{
			SVC_Status(*adr);
		}
	}
}

/*
=================
SV_MasterShutdown

Informs all masters that this server is going down
=================
*/
void SV_MasterShutdown( void )
{
	// send a hearbeat right now
	svs.nextHeartbeatTime = -1;

#ifdef LIBCOD
	SV_MasterHeartbeat_libcod( HEARTBEAT_DEAD );
#else
	SV_MasterHeartbeat( HEARTBEAT_DEAD );               // NERVE - SMF - changed to flatline
#endif
}
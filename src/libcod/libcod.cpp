#include "gsc.hpp"

#ifdef LIBCOD

dvar_t *sv_master[MAX_MASTER_SERVERS];
dvar_t *fs_library;
dvar_t *g_playerCollision;
dvar_t *g_playerEject;
dvar_t *sv_allowRcon;
dvar_t *sv_downloadMessage;
dvar_t *sv_fastDownload;
dvar_t *sv_cracked;
dvar_t *sv_kickbots;
dvar_t *g_safePrecache;
dvar_t *g_reservedModels;
dvar_t *logErrors;
dvar_t *g_corpseHit;
dvar_t *g_spectateBots;
dvar_t *g_logPickup;
dvar_t *sv_botKickMessages;
dvar_t *sv_noAuthorize;
dvar_t *sv_authorizeTimeout;
dvar_t *sv_limitLocalRcon;

// set while the map, gametype main and startupgametype scripts run, where
// g_reservedModels does not apply
bool libcod_precaching;
dvar_t *jump_bounceEnable;
dvar_t *g_mantleBlockEnable;
dvar_t *g_fixedWeaponSpreads;
dvar_t *g_dropGrenadeOnDeath;
dvar_t *sv_botUseTriggerUse;
dvar_t *g_bulletDrop;
dvar_t *g_bulletDropMaxTime;
#if LIBCOD_COMPILE_GRAPH == 1
dvar_t *g_debugGraph;
dvar_t *sv_graphAstarMaxIterations;
dvar_t *sv_graphAstarWeight;
dvar_t *sv_graphMaxGraphs;
dvar_t *sv_graphMaxNodes;
#endif

int codecallback_playercommand = 0;
int codecallback_userinfochanged = 0;
int codecallback_fire_grenade = 0;
int codecallback_vid_restart = 0;
int codecallback_error = 0;
int codecallback_hitchwarning = 0;
int codecallback_pickup = 0;
int codecallback_suicide = 0;
int codecallback_weapon_change = 0;
int codecallback_remotecommand = 0;

char consolePrefix[64] = "console: ";
char testClientName[MAX_NAME_LENGTH];

// CodeCallback_<name>Button runs when the button goes down; names and order
// are the same as in zk_libcod
static struct
{
	const char *name;
	int buttons;
	int handle;
} buttonCallbacks[] =
{
	{ "CodeCallback_AttackButton", BUTTON_ATTACK, 0 },
	{ "CodeCallback_MeleeButton", BUTTON_MELEE, 0 },
	{ "CodeCallback_UseButton", BUTTON_USE, 0 },
	{ "CodeCallback_ReloadButton", BUTTON_RELOAD, 0 },
	{ "CodeCallback_LeanLeftButton", BUTTON_LEANLEFT, 0 },
	{ "CodeCallback_LeanRightButton", BUTTON_LEANRIGHT, 0 },
	{ "CodeCallback_ProneButton", BUTTON_PRONE, 0 },
	{ "CodeCallback_CrouchButton", BUTTON_CROUCH, 0 },
	{ "CodeCallback_StandButton", BUTTON_JUMP, 0 },
	{ "CodeCallback_AdsButton", BUTTON_ADS, 0 },
	{ "CodeCallback_MeleeBreathButton", BUTTON_MELEE | BUTTON_HOLDBREATH, 0 },
	{ "CodeCallback_HoldBreathButton", BUTTON_HOLDBREATH, 0 },
	{ "CodeCallback_FragButton", BUTTON_FRAG, 0 },
	{ "CodeCallback_SmokeButton", BUTTON_SMOKE, 0 },
};

// Script errors are kept until the next frame: running script from inside the
// VM's error handling is not safe
#define MAX_QUEUED_SCRIPT_ERRORS 32
static char queuedScriptErrors[MAX_QUEUED_SCRIPT_ERRORS][256];
static char queuedScriptErrorPositions[MAX_QUEUED_SCRIPT_ERRORS][MAX_QPATH + 16];
static int numQueuedScriptErrors;
static bool runningErrorCallback;

int hitchFrameTime;

// weapon + 1 that PM_BeginWeaponChange started to switch to, 0 when none
int libcod_weaponChange[MAX_CLIENTS];

int client_challenge_ping[MAX_CLIENTS] = {0};

void RegisterLibcodDvars()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
		Libcod_ResetClient(i);

	sv_master[0] = Dvar_RegisterString("sv_master1", MASTER_SERVER_NAME, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_master[1] = Dvar_RegisterString("sv_master2", COMMUNITY_MASTER_SERVER_NAME, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_master[2] = Dvar_RegisterString("sv_master3", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_master[3] = Dvar_RegisterString("sv_master4", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_master[4] = Dvar_RegisterString("sv_master5", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);

	fs_library = Dvar_RegisterString("fs_library", "", DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);

	g_playerCollision = Dvar_RegisterBool("g_playerCollision", true, DVAR_CHANGEABLE_RESET);
	g_playerEject = Dvar_RegisterBool("g_playerEject", true, DVAR_CHANGEABLE_RESET);

	sv_allowRcon = Dvar_RegisterBool("sv_allowRcon", true, DVAR_CHANGEABLE_RESET);
	sv_downloadMessage = Dvar_RegisterString("sv_downloadMessage", "", DVAR_CHANGEABLE_RESET);
	sv_fastDownload = Dvar_RegisterBool("sv_fastDownload", false, DVAR_CHANGEABLE_RESET);
	sv_cracked = Dvar_RegisterBool("sv_cracked", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_kickbots = Dvar_RegisterBool("sv_kickbots", false, DVAR_CHANGEABLE_RESET);
	logErrors = Dvar_RegisterBool("logErrors", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	Libcod_RegisterCallbackDvars();

	g_corpseHit = Dvar_RegisterBool("g_corpseHit", true, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	g_spectateBots = Dvar_RegisterBool("g_spectateBots", true, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	g_logPickup = Dvar_RegisterBool("g_logPickup", true, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_botKickMessages = Dvar_RegisterBool("sv_botKickMessages", true, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_noAuthorize = Dvar_RegisterBool("sv_noAuthorize", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_authorizeTimeout = Dvar_RegisterInt("sv_authorizeTimeout", AUTHORIZE_TIMEOUT, 0, 1200000, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	sv_limitLocalRcon = Dvar_RegisterBool("sv_limitLocalRcon", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);

	// only from the command line; kept in a variable because setCvar from a script can change any dvar
	libcod_unsafeBuiltins = Dvar_RegisterBool("sv_libcodUnsafe", false, DVAR_INIT | DVAR_CHANGEABLE_RESET)->current.boolean;
	if (libcod_unsafeBuiltins)
		Com_Printf("^3sv_libcodUnsafe is on: scripts can run shell commands, open any file and read or write raw memory\n");

	jump_bounceEnable = Dvar_RegisterBool("jump_bounceEnable", false, DVAR_CHEAT | DVAR_CODINFO | DVAR_CHANGEABLE_RESET);
	g_mantleBlockEnable = Dvar_RegisterBool("g_mantleBlockEnable", true, DVAR_CHANGEABLE_RESET);

	g_fixedWeaponSpreads = Dvar_RegisterBool("g_fixedWeaponSpreads", false, DVAR_CHANGEABLE_RESET);
	g_dropGrenadeOnDeath = Dvar_RegisterBool("g_dropGrenadeOnDeath", true, DVAR_CHANGEABLE_RESET);
	sv_botUseTriggerUse = Dvar_RegisterBool("sv_botUseTriggerUse", false, DVAR_CHANGEABLE_RESET);
	g_bulletDrop = Dvar_RegisterBool("g_bulletDrop", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	g_bulletDropMaxTime = Dvar_RegisterInt("g_bulletDropMaxTime", 10000, 50, 60000, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);

#if LIBCOD_COMPILE_JSON == 1
	gsc_json_register_dvars();
#endif

#if LIBCOD_COMPILE_HTTP == 1
	gsc_http_register_dvars();
#endif

#if LIBCOD_COMPILE_GRAPH == 1
	g_debugGraph = Dvar_RegisterBool("g_debugGraph", false, DVAR_CHANGEABLE_RESET);
	sv_graphAstarMaxIterations = Dvar_RegisterInt("sv_graphAstarMaxIterations", 0, 0, 100000, DVAR_CHANGEABLE_RESET);
	sv_graphAstarWeight = Dvar_RegisterFloat("sv_graphAstarWeight", 1.0, 1.0, 2.0, DVAR_CHANGEABLE_RESET);
	sv_graphMaxGraphs = Dvar_RegisterInt("sv_graphMaxGraphs", 8, 1, 64, DVAR_CHANGEABLE_RESET);
	sv_graphMaxNodes = Dvar_RegisterInt("sv_graphMaxNodes", 16384, 64, 65535, DVAR_CHANGEABLE_RESET);
#endif
}

// Called by SV_SpawnServer once the configstrings are empty. The precache
// dvars are latched, so registering them again applies a new value from this
// map on. With g_safePrecache, index 1 of models and effects holds a stand-in
// for what does not fit.
void Libcod_SpawnServerConfigstrings()
{
	g_reservedModels = Dvar_RegisterInt("g_reservedModels", 0, 0, 255, DVAR_ARCHIVE | DVAR_LATCH | DVAR_CHANGEABLE_RESET);
	g_safePrecache = Dvar_RegisterBool("g_safePrecache", false, DVAR_ARCHIVE | DVAR_LATCH | DVAR_CHANGEABLE_RESET);

	if ( g_safePrecache->current.boolean )
	{
		// the client shows fx/misc/missing_fx.efx for an effect it can't
		// load; effect index 0 can't be played, so the stand-in is index 1
		SV_SetConfigstring(CS_EFFECT_NAMES + 1, "0");
		G_SetModelIndex(1, "xmodel/default_static_model");
	}
}

void InitLibcodCallbacks()
{
	codecallback_playercommand = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_PlayerCommand", 0);
	codecallback_userinfochanged = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_UserInfoChanged", 0);
	codecallback_fire_grenade = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_FireGrenade", 0);
	codecallback_vid_restart = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_VidRestart", 0);
	codecallback_error = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_Error", 0);
	codecallback_hitchwarning = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_HitchWarning", 0);
	codecallback_pickup = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_Pickup", 0);
	codecallback_suicide = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_Suicide", 0);
	codecallback_weapon_change = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_WeaponChange", 0);
	codecallback_remotecommand = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", "CodeCallback_RemoteCommand", 0);

	for (size_t i = 0; i < ARRAY_COUNT(buttonCallbacks); i++)
		buttonCallbacks[i].handle = GScr_LoadScriptAndLabel("maps/mp/gametypes/_callbacksetup", buttonCallbacks[i].name, 0);

	Libcod_LoadCallbacks();

}

// Script objects and threads of the previous level are gone, which async
// work checks against this; levelId is no use, a freed id is often reused
unsigned int libcodLevelGeneration = 1;

// Called from G_InitGame on every level start, map_restart and
// fast_restart included
void Libcod_InitLevel()
{
	libcodLevelGeneration++;

	Libcod_ClearDroppingBullets();
#if LIBCOD_COMPILE_ENTITY == 1
	gsc_entity_restoreitemquantities();
#endif

	// objective indexes belong to the map
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		for (int j = 0; j < MAX_OBJECTIVES; j++)
			ClearObjective(&libcodClients[i].objectives[j]);
	}

	// errors of the previous map are not reported to the new one
	numQueuedScriptErrors = 0;
	hitchFrameTime = 0;

#if LIBCOD_COMPILE_HTTP == 1
	gsc_http_cleanup_on_level_load();
#endif
}

// Called from G_ShutdownGame while the level's script objects still exist,
// so pending async tasks can drop the references they hold. A map change
// reinitializes the variable system, after which those ids mean nothing.
void Libcod_ShutdownLevel()
{
	Libcod_ShutdownCallbacks();
#if LIBCOD_COMPILE_EXEC == 1
	gsc_exec_async_shutdown_level();
#endif
#if LIBCOD_COMPILE_SQLITE == 1
	gsc_async_sqlite_shutdown_level();
#endif
}

// position is "file:line", or NULL outside developer mode, where the
// engine has no positions
void Scr_QueueErrorCallback(const char *message, const char *position)
{
	if (!codecallback_error || runningErrorCallback || numQueuedScriptErrors == MAX_QUEUED_SCRIPT_ERRORS)
		return;

	I_strncpyz(queuedScriptErrors[numQueuedScriptErrors], message, sizeof(queuedScriptErrors[0]));
	I_strncpyz(queuedScriptErrorPositions[numQueuedScriptErrors], position ? position : "", sizeof(queuedScriptErrorPositions[0]));
	numQueuedScriptErrors++;
}

// Called at the start of every game frame
void Scr_RunFrameCallbacks()
{
	if (!Scr_IsSystemActive())
		return;

	if (codecallback_hitchwarning && hitchFrameTime)
	{
		stackPushInt(hitchFrameTime);
		short ret = Scr_ExecThread(codecallback_hitchwarning, 1);
		Scr_FreeThread(ret);
	}
	hitchFrameTime = 0;

#if LIBCOD_COMPILE_SPEEX == 1
	Libcod_RunSoundCallbacks();
#endif

	// CodeCallback_Error(critical, internal_function, message, position);
	// errors that stop the map never get here, so critical is always 0.
	// position ("file:line") is undefined outside developer mode (developer 2
	// reports errors without stopping the map), and
	// callbacks written for zk_libcod's three parameters just don't see it.
	runningErrorCallback = true;
	for (int i = 0; i < numQueuedScriptErrors; i++)
	{
		if (queuedScriptErrorPositions[i][0])
			stackPushString(queuedScriptErrorPositions[i]);
		else
			stackPushUndefined();
		stackPushString(queuedScriptErrors[i]);
		stackPushString("RuntimeError");
		stackPushInt(0);
		short ret = Scr_ExecThread(codecallback_error, 4);
		Scr_FreeThread(ret);
	}
	runningErrorCallback = false;
	numQueuedScriptErrors = 0;

	Libcod_RunCallbacksFrame();

#if LIBCOD_COMPILE_HTTP == 1
	gsc_http_frame();
#endif
}

// Called after each usercmd of an active client. The callbacks can drop the
// client, so the state is checked before each one.
void Scr_ClientThinkCallbacks(client_t *cl, int oldButtons)
{
	int num = cl - svs.clients;
	int weapon = libcod_weaponChange[num] - 1;

	libcod_weaponChange[num] = 0;

	if (!Scr_IsSystemActive() || !cl->gentity)
		return;

	gentity_t *ent = cl->gentity;

	// CodeCallback_WeaponChange(index, name), when the switch starts
	if (codecallback_weapon_change && weapon >= 0)
	{
		stackPushString(BG_GetWeaponDef(weapon)->szInternalName);
		stackPushInt(weapon);
		short ret = Scr_ExecEntThread(ent, codecallback_weapon_change, 2);
		Scr_FreeThread(ret);
	}

	int pressed = cl->lastUsercmd.buttons;

	for (size_t i = 0; i < ARRAY_COUNT(buttonCallbacks); i++)
	{
		if (cl->state != CS_ACTIVE)
			return;

		if (!buttonCallbacks[i].handle || !(pressed & buttonCallbacks[i].buttons) || (oldButtons & buttonCallbacks[i].buttons))
			continue;

		short ret = Scr_ExecEntThread(ent, buttonCallbacks[i].handle, 0);
		Scr_FreeThread(ret);
	}
}

static void Com_Assert_f( void )
{
	assert(0);
}

void SV_AddLibcodCommands()
{
	if ( com_developer->current.integer )
	{
		Cmd_AddCommand("assert", Com_Assert_f);
	}
}

const char* NET_AdrToStringNoPort( netadr_t a )
{
	static char s[64];

	if ( a.type == NA_LOOPBACK )
	{
		Com_sprintf( s, sizeof( s ), "loopback" );
	}
	else if ( a.type == NA_BOT )
	{
		Com_sprintf( s, sizeof( s ), "bot" );
	}
	else if ( a.type == NA_IP )
	{
		Com_sprintf( s, sizeof( s ), "%i.%i.%i.%i",
		             a.ip[0], a.ip[1], a.ip[2], a.ip[3] );
	}
	else
	{
		Com_sprintf( s, sizeof( s ), "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x",
		             a.ipx[0], a.ipx[1], a.ipx[2], a.ipx[3], a.ipx[4], a.ipx[5], a.ipx[6], a.ipx[7], a.ipx[8], a.ipx[9] );
	}

	return s;
}

void manymaps_prepare(const char *mapname, int read)
{
	char map_check[MAX_OSPATH];
	char library_path[MAX_OSPATH];

	dvar_t *fs_homepath = Dvar_FindVar("fs_homepath");
	dvar_t *fs_game = Dvar_FindVar("fs_game");
	dvar_t *map = Dvar_FindVar("mapname");

	if (strlen(fs_library->current.string))
		I_strncpyz(library_path, fs_library->current.string, sizeof(library_path));
	else
		snprintf(library_path, sizeof(library_path), "%s/%s/Library", fs_homepath->current.string, fs_game->current.string);

	Com_sprintf(map_check, MAX_OSPATH, "%s/%s.iwd", library_path, mapname);

#if PROTOCOL_VERSION == 115
	const char *stock_maps[] = { "mp_breakout", "mp_brecourt", "mp_burgundy", "mp_carentan", "mp_dawnville", "mp_decoy", "mp_downtown", "mp_farmhouse", "mp_leningrad", "mp_matmata", "mp_railyard", "mp_toujane", "mp_trainstation" };
#else
	const char *stock_maps[] = { "mp_breakout", "mp_brecourt", "mp_burgundy", "mp_carentan", "mp_dawnville", "mp_decoy", "mp_downtown", "mp_farmhouse", "mp_leningrad", "mp_matmata", "mp_railyard", "mp_toujane", "mp_trainstation", "mp_rhine", "mp_harbor" };
#endif

	bool map_found = false, from_stock_map = false;

	for (int i = 0; i < int( sizeof(stock_maps) / sizeof(stock_maps[0]) ); i++)
	{
		if (strcmp(map->current.string, stock_maps[i]) == 0)
		{
			from_stock_map = true;
			break;
		}
	}

	for (int i = 0; i < int( sizeof(stock_maps) / sizeof(stock_maps[0]) ); i++)
	{
		if (strcmp(mapname, stock_maps[i]) == 0)
		{
			map_found = true;

			if (from_stock_map) // When changing from stock map to stock map do not trigger manymap
				return;
			else
				break;
		}
	}

	int map_exists = access(map_check, F_OK) != -1;

	if (!map_exists && !map_found)
		return;

	DIR *dir;
	struct dirent *dir_ent;

	dir = opendir(library_path);

	if (!dir)
		return;

	while ((dir_ent = readdir(dir)) != NULL)
	{
		if (strcmp(dir_ent->d_name, ".") == 0 || strcmp(dir_ent->d_name, "..") == 0)
			continue;

		char fileDelete[MAX_OSPATH];
		Com_sprintf(fileDelete, MAX_OSPATH, "%s/%s/%s", fs_homepath->current.string, fs_game->current.string, dir_ent->d_name);

		if (access(fileDelete, F_OK) != -1)
		{
			int unlink_success = unlink(fileDelete) == 0;
			Com_Printf("manymaps> REMOVED OLD LINK: %s result of unlink: %s\n", fileDelete, unlink_success?"success":"failed");
		}
	}

	closedir(dir);

	if (map_exists)
	{
		char src[MAX_OSPATH];
		char dst[MAX_OSPATH];

		Com_sprintf(src, MAX_OSPATH, "%s/%s.iwd", library_path, mapname);
		Com_sprintf(dst, MAX_OSPATH, "%s/%s/%s.iwd", fs_homepath->current.string, fs_game->current.string, mapname);

		if (access(src, F_OK) != -1)
		{
#ifdef _WIN32
			int link_success = CreateHardLinkA(dst, src, NULL) != 0;
#else
			int link_success = symlink(src, dst) == 0;
#endif
			Com_Printf("manymaps> NEW LINK: src=%s dst=%s result of link: %s\n", src, dst, link_success?"success":"failed");

			if (link_success && read == -1) // FS_LoadDir is needed when empty.iwd is missing (then .d3dbsp isn't referenced anywhere)
				FS_AddIwdFilesForGameDirectory(fs_homepath->current.string, fs_game->current.string);
		}
	}
}

// ioquake3 rate limit connectionless requests
// https://github.com/ioquake/ioq3/blob/master/code/server/sv_main.c
// This is deliberately quite large to make it more of an effort to DoS
#define MAX_BUCKETS	16384
#define MAX_HASHES 1024

static leakyBucket_t buckets[ MAX_BUCKETS ];
static leakyBucket_t* bucketHashes[ MAX_HASHES ];
leakyBucket_t outboundLeakyBucket;

static long SVC_HashForAddress( netadr_t address )
{
	unsigned char *ip = address.ip;
	int	i;
	long hash = 0;

	for ( i = 0; i < 4; i++ )
	{
		hash += (long)( ip[ i ] ) * ( i + 119 );
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	hash &= ( MAX_HASHES - 1 );

	return hash;
}

static leakyBucket_t *SVC_BucketForAddress( netadr_t address, int burst, int period )
{
	leakyBucket_t *bucket = NULL;
	int	i;
	long hash = SVC_HashForAddress( address );
	uint64_t now = Sys_Milliseconds64();

	for ( bucket = bucketHashes[ hash ]; bucket; bucket = bucket->next )
	{
		if ( memcmp( bucket->adr, address.ip, 4 ) == 0 )
		{
			return bucket;
		}
	}

	for ( i = 0; i < MAX_BUCKETS; i++ )
	{
		int interval;

		bucket = &buckets[ i ];
		interval = now - bucket->lastTime;

		// Reclaim expired buckets
		if ( bucket->lastTime > 0 && ( interval > ( burst * period ) ||
		                               interval < 0 ) )
		{
			if ( bucket->prev != NULL )
			{
				bucket->prev->next = bucket->next;
			}
			else
			{
				bucketHashes[ bucket->hash ] = bucket->next;
			}

			if ( bucket->next != NULL )
			{
				bucket->next->prev = bucket->prev;
			}

			memset( bucket, 0, sizeof( leakyBucket_t ) );
		}

		if ( bucket->type == 0 )
		{
			bucket->type = address.type;
			memcpy( bucket->adr, address.ip, 4 );

			bucket->lastTime = now;
			bucket->burst = 0;
			bucket->hash = hash;

			// Add to the head of the relevant hash chain
			bucket->next = bucketHashes[ hash ];
			if ( bucketHashes[ hash ] != NULL )
			{
				bucketHashes[ hash ]->prev = bucket;
			}

			bucket->prev = NULL;
			bucketHashes[ hash ] = bucket;

			return bucket;
		}
	}

	// Couldn't allocate a bucket for this address
	return NULL;
}

bool SVC_RateLimit( leakyBucket_t *bucket, int burst, int period )
{
	if ( bucket != NULL )
	{
		uint64_t now = Sys_Milliseconds64();
		int interval = now - bucket->lastTime;
		int expired = interval / period;
		int expiredRemainder = interval % period;

		if ( expired > bucket->burst || interval < 0 )
		{
			bucket->burst = 0;
			bucket->lastTime = now;
		}
		else
		{
			bucket->burst -= expired;
			bucket->lastTime = now - expiredRemainder;
		}

		if ( bucket->burst < burst )
		{
			bucket->burst++;

			return false;
		}
	}

	return true;
}

bool SVC_RateLimitAddress( netadr_t from, int burst, int period, bool limitLAN )
{
	if (!limitLAN && Sys_IsLANAddress(from))
		return false;

	leakyBucket_t *bucket = SVC_BucketForAddress( from, burst, period );

	return SVC_RateLimit( bucket, burst, period );
}

int hook_findMap(const char *qpath, void **buffer)
{
	int read = FS_ReadFile(qpath, buffer);
	manymaps_prepare(Cmd_Argv(1), read);

	if (read != -1)
		return read;
	else
		return FS_ReadFile(qpath, buffer);
}

bool hook_SV_MapExists(const char *mapname)
{
	bool map_exists = SV_MapExists(mapname);

	if (map_exists)
	{
		return map_exists;
	}
	else
	{
		char map_check[MAX_OSPATH];
		char library_path[MAX_OSPATH];

		dvar_t *fs_homepath = Dvar_FindVar("fs_homepath");
		dvar_t *fs_game = Dvar_FindVar("fs_game");

		if (strlen(fs_library->current.string))
			I_strncpyz(library_path, fs_library->current.string, sizeof(library_path));
		else
			snprintf(library_path, sizeof(library_path), "%s/%s/Library", fs_homepath->current.string, fs_game->current.string);

		Com_sprintf(map_check, MAX_OSPATH, "%s/%s.iwd", library_path, mapname);

		return access(map_check, F_OK) != -1;
	}
}

#define IP_BAN_LIST_NAME "iplist.txt"
bool SV_IsBannedIp(netadr_t adr)
{
	bool banned;
	char *file;
	const char *token;
	const char *text;

	if ( FS_ReadFile(IP_BAN_LIST_NAME, (void **)&file) < 0 )
		return false;

	text = file;
	banned = false;

	while ( 1 )
	{
		token = Com_Parse(&text);

		if ( !token[0] )
			break;

		if ( strcmp(token, NET_AdrToStringNoPort(adr)) == 0 )
		{
			banned = true;
			break;
		}

		Com_SkipRestOfLine(&text);
	}

	FS_FreeFile(file);

	return banned;
}

int clientfps[MAX_CLIENTS] = {0};
int clientframes[MAX_CLIENTS] = {0};
uint64_t clientframetime[MAX_CLIENTS] = {0};
void SV_ClientThink_libcod(client_t *cl, usercmd_t *cmd)
{
	int oldButtons = cl->lastUsercmd.buttons;
	cl->lastUsercmd = *cmd;

	if ( cl->state != CS_ACTIVE )
	{
		return;     // may have been kicked during the last usercmd
	}

	int clientnum = cl - svs.clients;
	clientframes[clientnum]++;

	if (Sys_Milliseconds64() - clientframetime[clientnum] >= 1000)
	{
		if (clientframes[clientnum] > 1000)
			clientframes[clientnum] = 1000;

		clientfps[clientnum] = clientframes[clientnum];
		clientframetime[clientnum] = Sys_Milliseconds64();
		clientframes[clientnum] = 0;
	}

	G_SetLastServerTime(cl - svs.clients, cmd->serverTime);
	ClientThink(cl - svs.clients);

	Scr_ClientThinkCallbacks(cl, oldButtons);
}

int bot_buttons[MAX_CLIENTS] = {0};
int bot_weapon[MAX_CLIENTS] = {0};
char bot_forwardmove[MAX_CLIENTS] = {0};
char bot_rightmove[MAX_CLIENTS] = {0};

libcodClient_t libcodClients[MAX_CLIENTS];
libcodEntity_t libcodEntities[MAX_GENTITIES];

int libcodPmoveClient = -1;

// Whether two players walk through each other because of setCollisionTeam
bool Libcod_SkipCollision(int clientNum, int otherNum)
{
	if (clientNum < 0 || clientNum >= MAX_CLIENTS || otherNum < 0 || otherNum >= MAX_CLIENTS)
		return false;

	gclient_t *a = g_entities[clientNum].client;
	gclient_t *b = g_entities[otherNum].client;
	if (!a || !b || a->sess.connected != CON_CONNECTED || b->sess.connected != CON_CONNECTED)
		return false;

	int teamA = libcodClients[clientNum].collisionTeam;
	int teamB = libcodClients[otherNum].collisionTeam;

	if (teamA == COLLISION_TEAM_NONE || teamB == COLLISION_TEAM_NONE)
		return true;
	if ((teamA == COLLISION_TEAM_AXIS && b->sess.cs.team != TEAM_AXIS) || (teamA == COLLISION_TEAM_ALLIES && b->sess.cs.team != TEAM_ALLIES))
		return true;
	if ((teamB == COLLISION_TEAM_AXIS && a->sess.cs.team != TEAM_AXIS) || (teamB == COLLISION_TEAM_ALLIES && a->sess.cs.team != TEAM_ALLIES))
		return true;

	return false;
}

// enableSilent: the other players don't get this player's steps, jumps,
// landings, foliage sounds and empty clicks. The events are only left out of
// the entity state the others receive; the player's own playerState keeps
// them, so its prediction is unchanged.
bool Libcod_IsSilencedEvent(int entityNum, int event)
{
	return entityNum >= 0 && entityNum < MAX_CLIENTS && libcodClients[entityNum].silent
	       && ((event >= EV_FOOTSTEP_RUN_DEFAULT && event <= EV_FOLIAGE_SOUND) || event == EV_NOAMMO);
}

bool Libcod_IsNotSolidFor(int entityNum, int clientNum)
{
	return clientNum >= 0 && clientNum < MAX_CLIENTS
	       && (libcodEntities[entityNum].notSolidMask[clientNum >> 5] & (1 << (clientNum & 0x1F)));
}

void Libcod_FreeEntity(int entityNum)
{
	memset(&libcodEntities[entityNum], 0, sizeof(libcodEntities[entityNum]));
}

// Called when a client slot is freed: what scripts set for the old player
// must not carry over to the next one in that slot
void Libcod_ResetClient(int clientNum)
{
	libcodClient_t *client = &libcodClients[clientNum];
	int generation = client->generation;

	// entities hidden from the old player are shown to the next one
	int word = clientNum >> 5, bit = 1 << (clientNum & 0x1F);
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		if (libcodEntities[i].hiddenMask[word] & bit)
		{
			libcodEntities[i].hiddenMask[word] &= ~bit;
			g_entities[i].r.clientMask[word] &= ~bit;
		}
		libcodEntities[i].notSolidMask[word] &= ~bit;
	}

	// nor is the next player muted by those who muted the old one
	for (int i = 0; svs.clients && i < sv_maxclients->current.integer; i++)
		svs.clients[i].muteList[clientNum] = false;

	memset(client, 0, sizeof(*client));
	client->generation = generation + 1;
	client->bulletDrag = 0.01f;
	client->bulletVelocity = 31500;
	client->weaponSpreadScale = 1.0f;
	client->turretSpreadScale = 1.0f;
	client->meleeRangeScale = 1.0f;
	client->meleeWidthScale = 1.0f;
	client->meleeHeightScale = 1.0f;
	client->fireRangeScale = 1.0f;
	for (int i = 0; i < MAX_OBJECTIVES; i++)
		ClearObjective(&client->objectives[i]);

	libcod_weaponChange[clientNum] = 0;
	bot_buttons[clientNum] = 0;
	bot_weapon[clientNum] = 0;
	bot_forwardmove[clientNum] = 0;
	bot_rightmove[clientNum] = 0;

#if LIBCOD_COMPILE_PLAYER == 1
	extern int custom_animation[];
	extern int player_g_speed[];
	extern int player_g_gravity[];
	extern int player_disableitempickup[];

	player_g_speed[clientNum] = 0;
	player_g_gravity[clientNum] = 0;
	player_disableitempickup[clientNum] = 0;
	custom_animation[clientNum] = 0;
#endif
}

void SV_BotUserMove_libcod(client_t *client)
{
	int num;
	usercmd_t ucmd = {0};

	if (client->gentity == NULL)
		return;

	num = client - svs.clients;
	ucmd.serverTime = svs.time;

	playerState_t *ps = SV_GameClientNum(num);
	gentity_t *ent = SV_GentityNum(num);

	if (bot_weapon[num])
		ucmd.weapon = (byte)(bot_weapon[num] & 0xFF);
	else
		ucmd.weapon = (byte)(ps->weapon & 0xFF);

	// as SV_UserMove does for real clients
	if (!BG_IsWeaponValid(ps, ucmd.weapon))
		ucmd.weapon = ps->weapon;
	ucmd.offHandIndex = ps->offHandIndex;

	if (ent->client == NULL)
		return;

	if (ent->client->sess.archiveTime == 0)
	{
		ucmd.buttons = bot_buttons[num];

		ucmd.forwardmove = bot_forwardmove[num];
		ucmd.rightmove = bot_rightmove[num];

		ucmd.angles[0] = ent->client->sess.cmd.angles[0];
		ucmd.angles[1] = ent->client->sess.cmd.angles[1];
		ucmd.angles[2] = ent->client->sess.cmd.angles[2];
	}

	client->deltaMessage = client->netchan.outgoingSequence - 1;
	SV_ClientThink_libcod(client, &ucmd);
}

void SV_ResetPureClient_libcod(client_t *cl)
{
	cl->pureAuthentic = 0;

	if (codecallback_vid_restart)
	{
		if (!Scr_IsSystemActive())
			return;

		if (cl->gentity == NULL)
			return;

		stackPushInt(cl - svs.clients);
		short ret = Scr_ExecEntThread(cl->gentity, codecallback_vid_restart, 1);
		Scr_FreeThread(ret);
	}
}

// The client whose command CodeCallback_PlayerCommand is looking at; only
// that client's clientCommand() may run it, once
int playerCommandClient = -1;

void hook_ClientCommand(int clientNum)
{
	if ( ! codecallback_playercommand)
	{
		ClientCommand(clientNum);
		return;
	}

	if (!Scr_IsSystemActive())
		return;

	stackPushArray();
	int args = Cmd_Argc();
	for (int i = 0; i < args; i++)
	{
		char tmp[COD2_MAX_STRINGLENGTH];
		SV_Cmd_ArgvBuffer(i, tmp, sizeof(tmp));
		if(i == 1 && tmp[0] >= 20 && tmp[0] <= 22)
		{
			char *part = strtok(tmp + 1, " ");
			while(part != NULL)
			{
				stackPushString(part);
				stackPushArrayLast();
				part = strtok(NULL, " ");
			}
		}
		else
		{
			stackPushString(tmp);
			stackPushArrayLast();
		}
	}

	playerCommandClient = clientNum;
	short ret = Scr_ExecEntThread(&g_entities[clientNum], codecallback_playercommand, 1);
	Scr_FreeThread(ret);
	playerCommandClient = -1;
}

// The rcon command CodeCallback_RemoteCommand is looking at; processRemoteCommand
// runs it, at most once
static struct
{
	bool pending;
	bool deferred;
	netadr_t from;
	char command[1024];
} remoteCommand;

// Hands an accepted rcon command to CodeCallback_RemoteCommand(from, args).
// Returns false when there is no callback and the server should run it.
bool Scr_RemoteCommandCallback(netadr_t from)
{
	if (!codecallback_remotecommand || !Scr_IsSystemActive())
		return false;

	// the same text SVC_RemoteCommand would run
	int len = 0;
	for (int i = 2; i < SV_Cmd_Argc(); i++)
	{
		len = Com_AddToString(SV_Cmd_Argv(i), remoteCommand.command, len, sizeof(remoteCommand.command), 1);
		len = Com_AddToString(" ", remoteCommand.command, len, sizeof(remoteCommand.command), 0);
	}
	if (len >= (int)sizeof(remoteCommand.command))
		return true;
	remoteCommand.command[len] = 0;
	remoteCommand.from = from;
	remoteCommand.pending = true;

	stackPushArray();
	for (int i = 2; i < SV_Cmd_Argc(); i++)
	{
		stackPushString(SV_Cmd_Argv(i));
		stackPushArrayLast();
	}
	stackPushString(NET_AdrToString(from));

	remoteCommand.deferred = false;
	short ret = Scr_ExecThread(codecallback_remotecommand, 2);
	Scr_FreeThread(ret);

	remoteCommand.pending = false;
	if (remoteCommand.deferred)
	{
		remoteCommand.deferred = false;
		SV_ExecuteRemoteCommand(remoteCommand.from, remoteCommand.command);
	}
	return true;
}

bool Scr_ProcessRemoteCommand()
{
	if (!remoteCommand.pending)
		return false;

	remoteCommand.pending = false;

	// a map change frees the scripts that are running, the callback included,
	// so it waits until the callback thread is done
	if (ContainsMapCommand(remoteCommand.command))
		remoteCommand.deferred = true;
	else
		SV_ExecuteRemoteCommand(remoteCommand.from, remoteCommand.command);
	return true;
}

void hook_ClientUserinfoChanged(int clientNum)
{
	if ( ! codecallback_userinfochanged)
	{
		ClientUserinfoChanged(clientNum);
		return;
	}

	if (!Scr_IsSystemActive())
		return;

	stackPushInt(clientNum); // one parameter is required
	short ret = Scr_ExecEntThread(&g_entities[clientNum], codecallback_userinfochanged, 1);
	Scr_FreeThread(ret);
}

/*
=================
SV_ResolveMaster_libcod

Resolves sv_master<index+1> when needed. Returns true when adr holds a
usable address. A failed lookup is retried on the next cycle instead of
wiping the dvar, so a temporary DNS problem doesn't drop the master for
the rest of the session.
=================
*/
static bool SV_ResolveMaster_libcod( int index, netadr_t *adr )
{
	dvar_t *master = sv_master[index];

	if ( !master->current.string[0] )
	{
		return false;
	}

	if ( master->modified || adr->type == NA_BOT || adr->type == NA_BAD )
	{
		master->modified = qfalse;

		Com_Printf( "Resolving %s\n", master->current.string );
		if ( !NET_StringToAdr( master->current.string, adr ) )
		{
			Com_Printf( "Couldn't resolve address: %s (will retry later)\n", master->current.string );
			adr->type = NA_BAD;
			return false;
		}

		// Use the default master port unless one was given as "host:port"
		if ( !strchr( master->current.string, ':' ) )
		{
			adr->port = BigShort( PORT_MASTER );
		}

		Com_Printf( "%s resolved to %i.%i.%i.%i:%i\n", master->current.string,
		            adr->ip[0], adr->ip[1], adr->ip[2], adr->ip[3],
		            (unsigned short)BigShort( adr->port ) );
	}

	return true;
}

void SV_MasterHeartbeat_libcod(const char *hbname)
{
	static netadr_t	adr[MAX_MASTER_SERVERS];
	int	i;

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if ( !com_dedicated || com_dedicated->current.integer != 2 )
	{
		return;     // only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if ( svs.time >= svs.nextHeartbeatTime )
	{
		svs.nextHeartbeatTime = svs.time + HEARTBEAT_MSEC;

		// send to group masters
		for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ )
		{
			if ( !SV_ResolveMaster_libcod( i, &adr[i] ) )
			{
				continue;
			}

			Com_Printf( "Sending heartbeat to %s\n", sv_master[i]->current.string );
			NET_OutOfBandPrint( NS_SERVER, adr[i], va( "heartbeat %s\n", hbname ) );
		}
	}

	// if not time yet, don't send anything
	if ( svs.time >= svs.nextStatusResponseTime )
	{
		svs.nextStatusResponseTime = svs.time + STATUS_MSEC;

		// send to group masters
		for ( i = 0 ; i < MAX_MASTER_SERVERS ; i++ )
		{
			if ( !SV_ResolveMaster_libcod( i, &adr[i] ) )
			{
				continue;
			}

			SVC_Status( adr[i] );
		}
	}
}

// Adds bot pings and removes spam on 1.2 and 1.3
void SV_CalcPings_libcod( void )
{
	int i, j;
	client_t *cl;
	int total, count;
	int delta;

	for ( i = 0 ; i < sv_maxclients->current.integer ; i++ )
	{
		cl = &svs.clients[i];

		if ( cl->state != CS_ACTIVE )
		{
			cl->ping = -1;
			continue;
		}

		if ( !cl->gentity )
		{
			cl->ping = -1;
			continue;
		}

		if ( cl->netchan.remoteAddress.type == NA_BOT )
		{
			cl->ping = 0;
			cl->lastPacketTime = svs.time;
			continue;
		}

		total = 0;
		count = 0;

		for ( j = 0 ; j < PACKET_BACKUP ; j++ )
		{
			if ( cl->frames[j].messageAcked == 0xFFFFFFFF )
				continue;

			delta = cl->frames[j].messageAcked - cl->frames[j].messageSent;
			count++;
			total += delta;
		}

		if ( !count )
			cl->ping = 999;
		else
		{
			cl->ping = total / count;

			if ( cl->ping > 999 )
				cl->ping = 999;
		}
	}
}

void SV_CheckTimeouts_libcod( void )
{
	int	i;
	client_t *cl;
	int	droppoint;
	int	zombiepoint;

	droppoint = svs.time - 1000 * sv_timeout->current.integer;
	zombiepoint = svs.time - 1000 * sv_zombietime->current.integer;

	for ( i = 0, cl = svs.clients; i < sv_maxclients->current.integer; i++, cl++ )
	{
		// message times may be wrong across a changelevel
		if (cl->lastPacketTime > svs.time)
			cl->lastPacketTime = svs.time;

		if (cl->state == CS_ZOMBIE && cl->lastPacketTime < zombiepoint)
		{
			cl->state = CS_FREE; // can now be reused
			continue;
		}

		if (cl->state >= CS_CONNECTED && cl->lastPacketTime < droppoint)
		{
			// wait several frames so a debugger session doesn't
			// cause a timeout
			if ( ++cl->timeoutCount > 5 )
			{
				SV_DropClient(cl, "EXE_TIMEDOUT");
				cl->state = CS_FREE; // don't bother with zombie state
			}
		}
		else
			cl->timeoutCount = 0;
	}
}

void PM_ProjectVelocity(const float *velIn, const float *normal, float *velOut)
{
	float lengthSq2D;
	float adjusted;
	float newZ;
	float lengthScale;

	if (!jump_bounceEnable->current.boolean)
	{
		PM_ClipVelocity(velIn, normal, velOut);
		return;
	}

	lengthSq2D = (float)(velIn[0] * velIn[0]) + (float)(velIn[1] * velIn[1]);

	if ( I_fabs(normal[2]) < 0.001 || lengthSq2D == 0.0 )
	{
		velOut[0] = velIn[0];
		velOut[1] = velIn[1];
		velOut[2] = velIn[2];
	}
	else
	{
		newZ = (float)-(float)((float)(velIn[0] * normal[0]) + (float)(velIn[1] * normal[1])) / normal[2];
		adjusted = velIn[1];
		lengthScale = I_sqrt((float)((float)(velIn[2] * velIn[2]) + lengthSq2D) / (float)((float)(newZ * newZ) + lengthSq2D));

		if ( lengthScale < 1.0 || newZ < 0.0 || velIn[2] > 0.0 )
		{
			velOut[0] = lengthScale * velIn[0];
			velOut[1] = lengthScale * adjusted;
			velOut[2] = lengthScale * newZ;
		}
	}
}

const char *SV_ModifyConfigstringIwdChkSum( client_t *client, int index )
{
	if ( index == CS_SYSTEMINFO && client->netchan.protocol == 119 )
	{
		// the systeminfo string can be up to BIG_INFO_STRING long
		char tmp[BIG_INFO_STRING];
		static char info[BIG_INFO_STRING];

		memset( info, 0, sizeof( info ) );

		I_strncpyz(tmp, sv.configstrings[index], sizeof(tmp));

		char *part = strtok(tmp, " ");

		while(part != NULL)
		{
			if ( strcmp(part, "1053665859") == 0 ) // iw_06.iwd
				I_strncat( info, sizeof( info ), va( "%s ", "-141992458" ) );
			else if ( strcmp(part, "1046874969") == 0 ) // iw_07.iwd
				I_strncat( info, sizeof( info ), va( "%s ", "840608716" ) );
			else
				I_strncat( info, sizeof( info ), va( "%s ", part ) );

			part = strtok(NULL, " ");
		}

		return info;
	}

	return sv.configstrings[index];
}

#endif
#include "gsc.hpp"

#include <vector>

#ifdef LIBCOD

// Callbacks from zk_libcod (same names and parameters): events, notifies,
// blocked movers, the map's weapons and turrets, debug prints and request
// spam. Each is looked up in _callbacksetup.gsc (or fs_callbacks) when the
// scripts load.

int codecallback_playerevent = 0;
int codecallback_entityevent = 0;
int codecallback_notify = 0;
int codecallback_notifydebug = 0;
int codecallback_moverblockage = 0;
int codecallback_map_weapons_load = 0;
int codecallback_map_turrets_load = 0;
int codecallback_dprintf = 0;
int codecallback_clspam = 0;

dvar_t *g_noMoverBlockage;
dvar_t *g_spawnMapWeapons;
dvar_t *g_spawnMapTurrets;

void Libcod_RegisterCallbackDvars()
{
	g_noMoverBlockage = Dvar_RegisterBool("g_noMoverBlockage", false, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	g_spawnMapWeapons = Dvar_RegisterBool("g_spawnMapWeapons", true, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
	g_spawnMapTurrets = Dvar_RegisterBool("g_spawnMapTurrets", true, DVAR_ARCHIVE | DVAR_CHANGEABLE_RESET);
}

void Libcod_LoadCallbacks()
{
	const char *file = Libcod_CallbacksScript();

	codecallback_playerevent = GScr_LoadScriptAndLabel(file, "CodeCallback_PlayerEvent", 0);
	codecallback_entityevent = GScr_LoadScriptAndLabel(file, "CodeCallback_EntityEvent", 0);
	codecallback_notify = GScr_LoadScriptAndLabel(file, "CodeCallback_Notify", 0);
	codecallback_notifydebug = GScr_LoadScriptAndLabel(file, "CodeCallback_NotifyDebug", 0);
	codecallback_moverblockage = GScr_LoadScriptAndLabel(file, "CodeCallback_MoverBlockage", 0);
	codecallback_map_weapons_load = GScr_LoadScriptAndLabel(file, "CodeCallback_MapWeaponsLoad", 0);
	codecallback_map_turrets_load = GScr_LoadScriptAndLabel(file, "CodeCallback_MapTurretsLoad", 0);
	codecallback_dprintf = GScr_LoadScriptAndLabel(file, "CodeCallback_DPrintf", 0);
	codecallback_clspam = GScr_LoadScriptAndLabel(file, "CodeCallback_CLSpam", 0);
}

// A script value kept past the stack it came from; it holds a reference
// until it is pushed back or released
struct SavedValue
{
	VariableValue value;
};

static void SaveValue(SavedValue *saved, const VariableValue *value)
{
	saved->value = *value;
	AddRefToValue(saved->value.type, saved->value.u);
}

// Pushes the value onto the stack, which takes over its reference
static void PushSavedValue(SavedValue *saved)
{
	IncInParam();
	*scrVmPub.top = saved->value;
}

static void ReleaseSavedValue(SavedValue *saved)
{
	RemoveRefToValue(saved->value.type, saved->value.u);
}

// Pushes an array of the values, or undefined when there are none
static void PushSavedValueArray(SavedValue *values, int count)
{
	if ( !count )
	{
		Scr_AddUndefined();
		return;
	}

	Scr_MakeArray();
	for ( int i = 0; i < count; i++ )
	{
		PushSavedValue(&values[i]);
		Scr_AddArray();
	}
}

/*
 * CodeCallback_PlayerEvent(event, parm) on the player, for each predictable
 * player event, and CodeCallback_EntityEvent(event, parm) on the entity for
 * each event added with G_AddEvent. A callback that adds events itself does
 * not call itself again.
 */
static bool runningEventCallback;

void Libcod_PlayerEvent(const playerState_t *ps, int event, int eventParm)
{
	if ( !codecallback_playerevent || runningEventCallback || !Scr_IsSystemActive() )
		return;

	if ( ps->clientNum < 0 || ps->clientNum >= MAX_CLIENTS || !g_entities[ps->clientNum].r.inuse )
		return;

	runningEventCallback = true;
	Scr_AddInt(eventParm);
	Scr_AddInt(event);
	Scr_FreeThread(Scr_ExecEntThread(&g_entities[ps->clientNum], codecallback_playerevent, 2));
	runningEventCallback = false;
}

void Libcod_EntityEvent(gentity_t *ent, int event, int eventParm)
{
	if ( !codecallback_entityevent || runningEventCallback || !Scr_IsSystemActive() || !ent->r.inuse )
		return;

	runningEventCallback = true;
	Scr_AddInt(eventParm);
	Scr_AddInt(event);
	Scr_FreeThread(Scr_ExecEntThread(ent, codecallback_entityevent, 2));
	runningEventCallback = false;
}

/*
 * CodeCallback_Notify(message, args) on the entity, for each notify the
 * engine sends to an entity (damage, trigger, menuresponse...), right after
 * the notify. args is an array of the notify's arguments, or undefined.
 */
#define MAX_NOTIFY_CALLBACK_ARGS 16

void Libcod_Notify(gentity_t *ent, unsigned int stringValue, unsigned int paramcount)
{
	SavedValue args[MAX_NOTIFY_CALLBACK_ARGS];
	int count = 0;
	bool call = codecallback_notify && Scr_IsSystemActive();

	// the first argument is on top of the stack
	if ( call )
	{
		for ( unsigned int i = 0; i < paramcount && count < MAX_NOTIFY_CALLBACK_ARGS; i++ )
			SaveValue(&args[count++], scrVmPub.top - i);
		SL_AddRefToString(stringValue);
	}

	Scr_NotifyNum(ent->s.number, CLASS_NUM_ENTITY, stringValue, paramcount);

	if ( !call )
		return;

	// a script woken by the notify may have freed the entity
	if ( !ent->r.inuse || !Scr_IsSystemActive() )
	{
		for ( int i = 0; i < count; i++ )
			ReleaseSavedValue(&args[i]);
		SL_RemoveRefToString(stringValue);
		return;
	}

	PushSavedValueArray(args, count);
	Scr_AddConstString(stringValue);
	SL_RemoveRefToString(stringValue);
	Scr_FreeThread(Scr_ExecEntThread(ent, codecallback_notify, 2));
}

/*
 * CodeCallback_NotifyDebug(object, message, args) for every notify, script
 * ones included, at the start of the next frame. At most
 * MAX_NOTIFY_DEBUG_QUEUE notifies a frame are kept.
 */
#define MAX_NOTIFY_DEBUG_QUEUE 4096

struct QueuedNotify
{
	unsigned int objectId;
	unsigned int stringValue;
	int count;
	SavedValue args[MAX_NOTIFY_CALLBACK_ARGS];
};

static std::vector<QueuedNotify> notifyQueue;
static bool notifyQueueFullWarned;

void Libcod_QueueNotifyDebug(unsigned int objectId, unsigned int stringValue, VariableValue *top)
{
	if ( !codecallback_notifydebug )
		return;

	if ( notifyQueue.size() >= MAX_NOTIFY_DEBUG_QUEUE )
	{
		if ( !notifyQueueFullWarned )
			Com_Printf("WARNING: more than %i notifies in a frame, CodeCallback_NotifyDebug misses the rest\n", MAX_NOTIFY_DEBUG_QUEUE);
		notifyQueueFullWarned = true;
		return;
	}

	notifyQueue.push_back(QueuedNotify());
	QueuedNotify *n = &notifyQueue.back();
	n->objectId = objectId;
	AddRefToObject(objectId);
	n->stringValue = stringValue;
	SL_AddRefToString(stringValue);
	n->count = 0;

	for ( VariableValue *arg = top; arg->type != VAR_PRECODEPOS && n->count < MAX_NOTIFY_CALLBACK_ARGS; arg-- )
		SaveValue(&n->args[n->count++], arg);
}

static void RunNotifyDebugCallbacks()
{
	// notifies sent by the callbacks wait for the next frame
	std::vector<QueuedNotify> queue;
	queue.swap(notifyQueue);
	notifyQueueFullWarned = false;

	for ( size_t i = 0; i < queue.size(); i++ )
	{
		QueuedNotify *n = &queue[i];

		PushSavedValueArray(n->args, n->count);
		Scr_AddConstString(n->stringValue);
		Scr_AddObject(n->objectId);
		SL_RemoveRefToString(n->stringValue);
		RemoveRefToObject(n->objectId);
		Scr_FreeThread(Scr_ExecThread(codecallback_notifydebug, 3));
	}
}

/*
 * CodeCallback_MoverBlockage(mover) on an entity that blocks a mover. With
 * g_noMoverBlockage the mover goes on and the entity stays where it is.
 * Returns true when the mover is not blocked.
 */
bool Libcod_MoverBlocked(gentity_t *check, gentity_t *pusher)
{
	if ( codecallback_moverblockage && Scr_IsSystemActive() && check->r.inuse )
	{
		Scr_AddEntity(pusher);
		Scr_FreeThread(Scr_ExecEntThread(check, codecallback_moverblockage, 1));
	}

	return g_noMoverBlockage->current.boolean;
}

/*
 * The map's weapons and turrets. With g_spawnMapWeapons or g_spawnMapTurrets
 * off they are neither spawned nor precached, and what the map had is passed
 * to CodeCallback_MapWeaponsLoad(num, info) and
 * CodeCallback_MapTurretsLoad(num, info) before the gametype's main.
 */
struct MapWeapon
{
	char classname[MAX_QPATH];
	vec3_t origin;
	vec3_t angles;
	int count;
};

struct MapTurret
{
	char classname[MAX_QPATH];
	vec3_t origin;
	vec3_t angles;
	int toparc, bottomarc, leftarc, rightarc;
	char model[MAX_QPATH];
	char weaponinfo[MAX_QPATH];
	int exportValue;
	char gameobjectname[MAX_QPATH];
};

static std::vector<MapWeapon> mapWeapons;
static std::vector<MapTurret> mapTurrets;

static const char *SpawnVarValue(const char *key)
{
	for ( int i = 0; i < level.spawnVar.numSpawnVars; i++ )
	{
		if ( !strcmp(level.spawnVar.spawnVars[i][0], key) )
			return level.spawnVar.spawnVars[i][1];
	}

	return NULL;
}

static int SpawnVarInt(const char *key, int defaultValue)
{
	const char *value = SpawnVarValue(key);
	return value ? atoi(value) : defaultValue;
}

static void SpawnVarString(const char *key, char *out, int size)
{
	const char *value = SpawnVarValue(key);
	I_strncpyz(out, value ? value : "", size);
}

void Libcod_ClearMapItems()
{
	mapWeapons.clear();
	mapTurrets.clear();
}

// Called by G_CallSpawn for each map entity; true when it is not spawned
bool Libcod_SkipMapEntity(const char *classname)
{
	bool weapon = !strncmp(classname, "weapon_", 7);
	bool turret = !strcmp(classname, "misc_mg42") || !strcmp(classname, "misc_turret");

	if ( !( weapon && !g_spawnMapWeapons->current.boolean ) && !( turret && !g_spawnMapTurrets->current.boolean ) )
		return false;

	// parse the fields as the entity would be, for origin, angles and count
	gentity_t *ent = G_Spawn();
	G_ParseEntityFields(ent);

	if ( weapon )
	{
		MapWeapon w;
		I_strncpyz(w.classname, classname, sizeof(w.classname));
		VectorCopy(ent->r.currentOrigin, w.origin);
		VectorCopy(ent->r.currentAngles, w.angles);
		w.count = ent->count;
		mapWeapons.push_back(w);
	}
	else
	{
		MapTurret t;
		I_strncpyz(t.classname, classname, sizeof(t.classname));
		VectorCopy(ent->r.currentOrigin, t.origin);
		VectorCopy(ent->r.currentAngles, t.angles);
		// -1 when the map does not set them, as zk_libcod
		t.toparc = SpawnVarInt("toparc", -1);
		t.bottomarc = SpawnVarInt("bottomarc", -1);
		t.leftarc = SpawnVarInt("leftarc", -1);
		t.rightarc = SpawnVarInt("rightarc", -1);
		SpawnVarString("model", t.model, sizeof(t.model));
		SpawnVarString("weaponinfo", t.weaponinfo, sizeof(t.weaponinfo));
		t.exportValue = SpawnVarInt("export", 0);
		SpawnVarString("script_gameobjectname", t.gameobjectname, sizeof(t.gameobjectname));
		mapTurrets.push_back(t);
	}

	G_FreeEntity(ent);
	return true;
}

// Called by Scr_LoadGameType before the gametype's main
void Libcod_MapItemsCallbacks()
{
	if ( codecallback_map_weapons_load )
	{
		if ( mapWeapons.empty() )
		{
			Scr_AddUndefined();
		}
		else
		{
			Scr_MakeArray();
			for ( size_t i = 0; i < mapWeapons.size(); i++ )
			{
				Scr_AddString(mapWeapons[i].classname);
				Scr_AddArray();
				Scr_AddVector(mapWeapons[i].origin);
				Scr_AddArray();
				Scr_AddVector(mapWeapons[i].angles);
				Scr_AddArray();
				Scr_AddInt(mapWeapons[i].count);
				Scr_AddArray();
			}
		}
		Scr_AddInt((int)mapWeapons.size());
		Scr_FreeThread(Scr_ExecThread(codecallback_map_weapons_load, 2));
	}

	if ( codecallback_map_turrets_load )
	{
		if ( mapTurrets.empty() )
		{
			Scr_AddUndefined();
		}
		else
		{
			Scr_MakeArray();
			for ( size_t i = 0; i < mapTurrets.size(); i++ )
			{
				const MapTurret &t = mapTurrets[i];
				Scr_AddString(t.classname);
				Scr_AddArray();
				Scr_AddVector(t.origin);
				Scr_AddArray();
				Scr_AddVector(t.angles);
				Scr_AddArray();
				Scr_AddInt(t.toparc);
				Scr_AddArray();
				Scr_AddInt(t.bottomarc);
				Scr_AddArray();
				Scr_AddInt(t.leftarc);
				Scr_AddArray();
				Scr_AddInt(t.rightarc);
				Scr_AddArray();
				Scr_AddString(t.model);
				Scr_AddArray();
				Scr_AddString(t.weaponinfo);
				Scr_AddArray();
				Scr_AddInt(t.exportValue);
				Scr_AddArray();
				Scr_AddString(t.gameobjectname);
				Scr_AddArray();
			}
		}
		Scr_AddInt((int)mapTurrets.size());
		Scr_FreeThread(Scr_ExecThread(codecallback_map_turrets_load, 2));
	}

	Libcod_ClearMapItems();
}

/*
 * CodeCallback_DPrintf(message) for each Com_DPrintf of the main thread,
 * developer or not. Unlike zk_libcod the message is still printed with
 * developer set. A print made by the callback does not call it again.
 */
void Libcod_DPrintf(const char *message)
{
	static bool running;

	if ( !codecallback_dprintf || running || !Sys_IsMainThread() || !Scr_IsSystemActive() )
		return;

	running = true;
	Scr_AddString(message);
	Scr_FreeThread(Scr_ExecThread(codecallback_dprintf, 1));
	running = false;
}

/*
 * CodeCallback_CLSpam(type, ip) when an address goes over a request rate
 * limit; type is "INFO:ADDRESS", "STATUS:ADDRESS", "CHALLENGE:ADDRESS",
 * "CONNECT:ADDRESS" or "RCON:ADDRESS". Returns true when the callback ran.
 */
bool Libcod_SpamCallback(const char *type, netadr_t from)
{
	if ( !codecallback_clspam || !Sys_IsMainThread() || !Scr_IsSystemActive() )
		return false;

	Scr_AddString(NET_AdrToString(from));
	Scr_AddString(type);
	Scr_FreeThread(Scr_ExecThread(codecallback_clspam, 2));
	return true;
}

// Called at the start of every game frame
void Libcod_RunCallbacksFrame()
{
	if ( !notifyQueue.empty() )
		RunNotifyDebugCallbacks();
}

// Called when the level's script objects are about to go
void Libcod_ShutdownCallbacks()
{
	for ( size_t i = 0; i < notifyQueue.size(); i++ )
	{
		QueuedNotify *n = &notifyQueue[i];
		for ( int j = 0; j < n->count; j++ )
			ReleaseSavedValue(&n->args[j]);
		SL_RemoveRefToString(n->stringValue);
		RemoveRefToObject(n->objectId);
	}
	notifyQueue.clear();
	notifyQueueFullWarned = false;

	Libcod_ClearMapItems();
}

#endif

#ifndef _LIBCOD_CLIENT_H_
#define _LIBCOD_CLIENT_H_

// Per-client state set by libcod script functions and read by the engine.
// It is cleared when a client slot is freed, so the next player in that slot
// starts with the defaults.
typedef struct
{
	bool hiddenFromScoreboard;
	bool hiddenFromServerStatus;
	bool notAllowingSpectators;

	// weapons; the scales start at 1
	float weaponSpreadScale;
	float turretSpreadScale;
	float meleeRangeScale;
	float meleeWidthScale;
	float meleeHeightScale;
	float fireRangeScale;
	bool overrideBulletMask;
	int bulletMask;
	bool fireThroughWalls;
	bool noBulletImpacts;

	// overrideContents
	bool overrideContents;
	int contents;

	bool noEarthquakes;
	bool silent;

	// setActivateOnUseButtonRelease: use when the button is let go
	bool activateOnUseButtonRelease;
	bool heldUseButton;

	// the authorize server's answer for this player's CD key
	char authorizationStatus[32];

	// setCollisionTeam: which players this one collides with
	int collisionTeam;

	// enableBulletDrop
	bool bulletDrop;
	float bulletDrag;
	float bulletVelocity;

	// playSoundFile: the sound playing (index, 0 for none), who it comes from,
	// the next packet and the packets due
	int soundIndex;
	int soundTalker;
	int soundPacket;
	float soundPending;

	// counts the players a slot has had, so what an old player left behind
	// (bullets in flight) can tell it is gone
	int generation;

	// objective_player_*: a slot that is not empty replaces the shared
	// objective of the same index for this player
	objective_t objectives[MAX_OBJECTIVES];
} libcodClient_t;

extern libcodClient_t libcodClients[];

// Per-entity state, cleared when the entity is freed
typedef struct
{
	// the r.clientMask bits set by hideFromPlayer, undone when that player's
	// slot is freed
	int hiddenMask[2];

	// notSolidForPlayer: the players who walk through this brush model
	int notSolidMask[2];

	// playFxOnTagForPlayer: per event slot, the event sequence it was added
	// with and the player (number + 1) it is for
	struct
	{
		int sequence;
		int target;
	} fxOnTag[4];

	// enableGravity
	int gravity;
	bool resting;
	bool locational;
	bool alignToGround;
	float maxVelocity;
	float parallelBounce;
	float perpendicularBounce;
} libcodEntity_t;

enum
{
	LIBCOD_GRAVITY_NONE,
	LIBCOD_GRAVITY_FALL,
	LIBCOD_GRAVITY_BOUNCE,
};

enum
{
	COLLISION_TEAM_AXIS_ALLIES,	// everyone (the default)
	COLLISION_TEAM_NONE,
	COLLISION_TEAM_AXIS,
	COLLISION_TEAM_ALLIES,
};

extern libcodEntity_t libcodEntities[];

// The client whose Pmove is running, or -1: per-player collision only
// applies to a player's own movement
extern int libcodPmoveClient;

void Libcod_ResetClient(int clientNum);
void Libcod_FreeEntity(int entityNum);
bool Libcod_SkipCollision(int clientNum, int otherNum);
bool Libcod_IsNotSolidFor(int entityNum, int clientNum);
bool Libcod_IsSilencedEvent(int entityNum, int event);

bool Libcod_RunEntityGravity(gentity_t *ent);
void Libcod_EnableGravity(gentity_t *ent, bool bounce, bool locational, bool alignToGround);
void Libcod_DisableGravity(gentity_t *ent);
void Libcod_EntityOriginSet(gentity_t *ent);
void Libcod_GetEntityVelocity(gentity_t *ent, vec3_t velocity);
void Libcod_SetEntityVelocity(gentity_t *ent, const vec3_t velocity);

extern float libcodBulletDistance;
extern bool libcodBulletHit;
void Libcod_ClearDroppingBullets();
bool Libcod_FireDroppingBullet(gentity_t *attacker, const weaponParms *wp, gentity_t *weaponEnt, int gameTime, const vec3_t end);
void Libcod_RunDroppingBullets();

void Libcod_RunSounds();
void Libcod_RunSoundCallbacks();

#endif

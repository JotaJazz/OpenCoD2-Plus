#include "gsc.hpp"

#ifdef LIBCOD

// Gravity for script models (enableGravity), on the trajectories the client
// already knows: TR_GRAVITY while falling, TR_LINEAR at the maximum speed and
// TR_STATIONARY at rest. The trajectory is only rewritten when the motion
// changes (a hit, a bounce, the speed limit), so the client draws the same
// path the server follows.

#define GRAVITY_MASK ((MASK_SHOT & ~CONTENTS_WATER) | CONTENTS_PLAYERCLIP)

// Surfaces flatter than this are ground; steeper ones are walls
#define GROUND_NORMAL 0.7f

// A bounce off the ground weaker than what gravity adds in this many frames
// would bounce again every frame: the entity comes to rest instead
#define BOUNCE_REST_FRAMES 1.5f

static void Libcod_StartMotion(gentity_t *ent, const vec3_t velocity)
{
	libcodEntity_t *le = &libcodEntities[ent->s.number];

	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;
	ent->s.pos.trDuration = 0;
	VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
	VectorCopy(velocity, ent->s.pos.trDelta);
	ent->s.groundEntityNum = ENTITYNUM_NONE;
	le->resting = false;

	// at the speed limit already: no acceleration
	if (VectorLength(velocity) > le->maxVelocity)
	{
		ent->s.pos.trType = TR_LINEAR;
		VectorScale(velocity, le->maxVelocity / VectorLength(velocity), ent->s.pos.trDelta);
	}
}

// The velocity now: what the trajectory gives, not the last frame's move
void Libcod_GetEntityVelocity(gentity_t *ent, vec3_t velocity)
{
	if (libcodEntities[ent->s.number].resting || ent->s.pos.trType == TR_STATIONARY)
		VectorClear(velocity);
	else
		BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
}

void Libcod_SetEntityVelocity(gentity_t *ent, const vec3_t velocity)
{
	Libcod_StartMotion(ent, velocity);
}

void Libcod_EnableGravity(gentity_t *ent, bool bounce, bool locational, bool alignToGround)
{
	libcodEntity_t *le = &libcodEntities[ent->s.number];
	vec3_t velocity;

	if (!le->gravity)
	{
		le->maxVelocity = 8192;
		le->parallelBounce = 0.5f;
		le->perpendicularBounce = 0.25f;
		VectorClear(velocity);
	}
	else
	{
		Libcod_GetEntityVelocity(ent, velocity);
	}

	le->gravity = bounce ? LIBCOD_GRAVITY_BOUNCE : LIBCOD_GRAVITY_FALL;
	le->locational = locational;
	le->alignToGround = alignToGround;
	ent->clipmask = GRAVITY_MASK;
	G_SetAngle(ent, ent->r.currentAngles);
	Libcod_StartMotion(ent, velocity);
}

// Script motion (moveTo, rotateTo, linkTo...) takes over from gravity
void Libcod_DisableGravity(gentity_t *ent)
{
	libcodEntity_t *le = &libcodEntities[ent->s.number];

	if (!le->gravity)
		return;

	le->gravity = LIBCOD_GRAVITY_NONE;
	le->resting = false;
	G_SetOrigin(ent, ent->r.currentOrigin);
}

// The origin was set by script: fall again from there
void Libcod_EntityOriginSet(gentity_t *ent)
{
	vec3_t velocity;

	if (!libcodEntities[ent->s.number].gravity)
		return;

	VectorClear(velocity);
	Libcod_StartMotion(ent, velocity);
}

static void Libcod_NotifyGround(gentity_t *ent, unsigned short name, const trace_t *trace)
{
	if (!Scr_IsSystemActive())
		return;

	if (trace->entityNum < ENTITYNUM_WORLD)
		Scr_AddEntity(&g_entities[trace->entityNum]);
	else
		Scr_AddUndefined();
	Scr_AddString(Com_SurfaceTypeToName(SURF_TYPEINDEX(trace->surfaceFlags)));
	Scr_Notify(ent, name, 2);
}

static void Libcod_Land(gentity_t *ent, const trace_t *trace, const vec3_t endpos)
{
	libcodEntity_t *le = &libcodEntities[ent->s.number];

	G_SetOrigin(ent, endpos);
	ent->s.groundEntityNum = trace->entityNum;
	le->resting = true;

	if (le->alignToGround)
	{
		// keep the yaw, tilt the model onto the surface
		vec3_t axis[3], angles, yaw = { 0, ent->r.currentAngles[1], 0 };

		VectorCopy(trace->normal, axis[2]);
		AngleVectors(yaw, axis[0], NULL, NULL);
		VectorMA(axis[0], -DotProduct(axis[0], axis[2]), axis[2], axis[0]);
		Vec3Normalize(axis[0]);
		Vec3Cross(axis[2], axis[0], axis[1]);
		AxisToAngles(axis, angles);
		G_SetAngle(ent, angles);
	}

	Libcod_NotifyGround(ent, scr_const.land, trace);
}

// Called for every entity each frame; true when the entity is under
// script gravity and was moved here
bool Libcod_RunEntityGravity(gentity_t *ent)
{
	libcodEntity_t *le = &libcodEntities[ent->s.number];
	trace_t trace;
	vec3_t target, velocity;

	if (!le->gravity)
		return false;

	if (le->resting)
	{
		// still standing on something?
		VectorCopy(ent->r.currentOrigin, target);
		target[2] -= 1.5f;
		SV_Trace(&trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, target, ent->s.number, ent->clipmask, le->locational, NULL, 1);

		if (trace.fraction == 1.0f && !trace.startsolid)
		{
			VectorClear(velocity);
			Libcod_StartMotion(ent, velocity);
		}

		G_RunThink(ent);
		return true;
	}

	BG_EvaluateTrajectory(&ent->s.pos, level.time, target);
	SV_Trace(&trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, target, ent->s.number, ent->clipmask, le->locational, NULL, 1);

	if (trace.startsolid)
	{
		// stuck in something: stop where it is
		trace.normal[0] = trace.normal[1] = 0;
		trace.normal[2] = 1;
		Libcod_Land(ent, &trace, ent->r.currentOrigin);
		SV_LinkEntity(ent);
		G_RunThink(ent);
		return true;
	}

	vec3_t endpos;
	Vec3Lerp(ent->r.currentOrigin, target, trace.fraction, endpos);
	VectorCopy(endpos, ent->r.currentOrigin);

	if (trace.fraction < 1.0f)
	{
		float into;

		BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
		into = DotProduct(velocity, trace.normal);

		if (le->gravity == LIBCOD_GRAVITY_BOUNCE)
		{
			vec3_t along, across;

			// the part across the surface is reflected and damped, the part
			// along it only damped
			VectorScale(trace.normal, into, across);
			VectorSubtract(velocity, across, along);
			VectorScale(along, le->parallelBounce, velocity);
			VectorMA(velocity, -le->perpendicularBounce, across, velocity);

			if (trace.normal[2] > GROUND_NORMAL
			        && DotProduct(velocity, trace.normal) < DEFAULT_GRAVITY * level.frameTime * 0.001f * BOUNCE_REST_FRAMES)
			{
				Libcod_Land(ent, &trace, endpos);
			}
			else
			{
				Libcod_StartMotion(ent, velocity);
				Libcod_NotifyGround(ent, scr_const.bounce, &trace);
			}
		}
		else if (trace.normal[2] > GROUND_NORMAL)
		{
			Libcod_Land(ent, &trace, endpos);
		}
		else
		{
			// a wall or a ceiling: slide along it
			VectorMA(velocity, -into, trace.normal, velocity);
			Libcod_StartMotion(ent, velocity);
		}
	}
	else if (ent->s.pos.trType == TR_GRAVITY)
	{
		// the speed limit: from here on at a constant speed
		BG_EvaluateTrajectoryDelta(&ent->s.pos, level.time, velocity);
		if (VectorLength(velocity) > le->maxVelocity)
			Libcod_StartMotion(ent, velocity);
	}

	SV_LinkEntity(ent);
	G_RunThink(ent);
	return true;
}

// Bullet drop (enableBulletDrop, with g_bulletDrop set): a player's bullets
// fly frame by frame, pulled down by gravity and slowed by drag, instead of
// hitting at once. Each step is a Bullet_Fire_Extended from the bullet's last
// position, so impacts, penetration and the player's bullet mask, impacts and
// range settings all apply; damage falls off with the whole distance flown.

#define MAX_DROPPING_BULLETS 256
#define MAX_PLAYER_BULLETS 32
#define BULLET_GRAVITY 386.09f	// 9.81 m/s2 in inches

typedef struct
{
	bool active;
	int attacker;
	int generation;
	int weaponEnt;
	int lagOffset;
	int startTime;
	float distance;
	float maxDistance;
	vec3_t origin;
	vec3_t velocity;
	weaponParms wp;
} droppingBullet_t;

static droppingBullet_t droppingBullets[MAX_DROPPING_BULLETS];

// Read by Bullet_Fire_Extended while a bullet step runs
float libcodBulletDistance;
bool libcodBulletHit;

void Libcod_ClearDroppingBullets()
{
	memset(droppingBullets, 0, sizeof(droppingBullets));
}

// One step of seconds; false once the bullet has hit something, or left
// its range
static bool Libcod_StepBullet(droppingBullet_t *b, float seconds, int gameTime)
{
	gentity_t *attacker = &g_entities[b->attacker];
	gentity_t *weaponEnt = &g_entities[b->weaponEnt];
	float drag = libcodClients[b->attacker].bulletDrag;
	vec3_t velocity, end, step;

	// drag is the part of the speed lost each second
	VectorScale(b->velocity, powf(1.0f - drag, seconds), velocity);
	velocity[2] -= BULLET_GRAVITY * seconds;

	// the mean velocity over the step gives the exact arc under constant
	// gravity
	VectorAdd(b->velocity, velocity, step);
	VectorScale(step, 0.5f * seconds, step);
	VectorAdd(b->origin, step, end);

	libcodBulletDistance = b->distance;
	libcodBulletHit = false;
	Bullet_Fire_Extended(weaponEnt, attacker, b->origin, end, 1.0, 0, &b->wp, weaponEnt, gameTime);
	libcodBulletDistance = 0;

	if (libcodBulletHit)
		return false;

	b->distance += VectorLength(step);
	VectorCopy(end, b->origin);
	VectorCopy(velocity, b->velocity);
	return b->distance < b->maxDistance;
}

// Called by Bullet_Fire with the players already rewound for the shot.
// Returns false when the shot should be fired the stock way.
bool Libcod_FireDroppingBullet(gentity_t *attacker, const weaponParms *wp, gentity_t *weaponEnt, int gameTime, const vec3_t end)
{
	extern dvar_t *g_bulletDrop;
	int num = attacker->s.number;
	int i, free = -1, count = 0;

	if (!g_bulletDrop->current.boolean || !attacker->client || num >= MAX_CLIENTS || !libcodClients[num].bulletDrop)
		return false;

	for (i = 0; i < MAX_DROPPING_BULLETS; i++)
	{
		if (!droppingBullets[i].active)
		{
			if (free < 0)
				free = i;
		}
		else if (droppingBullets[i].attacker == num)
		{
			count++;
		}
	}

	// too many in flight: this one hits at once
	if (free < 0 || count >= MAX_PLAYER_BULLETS)
		return false;

	droppingBullet_t *b = &droppingBullets[free];
	vec3_t dir;

	memset(b, 0, sizeof(*b));
	b->attacker = num;
	b->generation = libcodClients[num].generation;
	b->weaponEnt = weaponEnt->s.number;
	b->lagOffset = level.time - gameTime;
	b->startTime = level.time;
	b->maxDistance = 8192 * libcodClients[num].fireRangeScale;
	b->wp = *wp;
	VectorCopy(wp->muzzleTrace, b->origin);
	VectorSubtract(end, wp->muzzleTrace, dir);
	Vec3Normalize(dir);
	VectorScale(dir, libcodClients[num].bulletVelocity, b->velocity);

	// the first step right away, within the shot's rewind
	b->active = Libcod_StepBullet(b, level.frameTime * 0.001f, gameTime);
	return true;
}

// Moves the bullets in flight, once per game frame, each with the players
// rewound to where its shooter saw them
void Libcod_RunDroppingBullets()
{
	extern dvar_t *g_bulletDropMaxTime;
	AntilagClientStore antilagClients;

	for (int i = 0; i < MAX_DROPPING_BULLETS; i++)
	{
		droppingBullet_t *b = &droppingBullets[i];

		if (!b->active)
			continue;

		// the shooter left (or its slot has a new player), or time is up
		gentity_t *attacker = &g_entities[b->attacker];
		if (!attacker->r.inuse || !attacker->client || libcodClients[b->attacker].generation != b->generation
		        || level.time - b->startTime > g_bulletDropMaxTime->current.integer)
		{
			b->active = false;
			continue;
		}

		// a turret that is gone: the shooter fired it
		if (!g_entities[b->weaponEnt].r.inuse)
			b->weaponEnt = b->attacker;

		int gameTime = level.time - b->lagOffset;
		G_AntiLagRewindClientPos(gameTime, &antilagClients);
		b->active = Libcod_StepBullet(b, level.frameTime * 0.001f, gameTime);
		G_AntiLag_RestoreClientPos(&antilagClients);
	}
}

#endif

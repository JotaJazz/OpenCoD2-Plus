#include "../qcommon/qcommon.h"
#include "g_shared.h"

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker )
{
	assert(target);
	assert(attacker);

	if ( !target->takedamage )
	{
		return qfalse;
	}

	if ( target == attacker )
	{
		return qfalse;
	}

	if ( !target->client )
	{
		return qfalse;
	}

	if ( !attacker->client )
	{
		return qfalse;
	}

	if ( target->client->ps.pm_type >= PM_DEAD )
	{
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
Bullet_GetDamage
===============
*/
int Bullet_GetDamage( const weaponParms *wp, float dist )
{
	float lerpAmount;
	float range;

	if ( wp->weapDef->maxDamageRange > dist )
	{
		return wp->weapDef->damage;
	}

	if ( wp->weapDef->minDamageRange <= dist )
	{
		return wp->weapDef->minDamage;
	}

	range = wp->weapDef->minDamageRange - wp->weapDef->maxDamageRange;

	if ( range == 0 )
	{
		return wp->weapDef->damage;
	}

	lerpAmount = (dist - wp->weapDef->maxDamageRange) / range;
	assert(((lerpAmount >= 0.0) && (lerpAmount <= 1.0)));

	return lerp( wp->weapDef->damage, wp->weapDef->minDamage, lerpAmount );
}

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/
void SnapVectorTowards( vec3_t v, vec3_t to )
{
	int		i;

	for ( i = 0 ; i < 3 ; i++ )
	{
		if ( to[i] <= v[i] )
		{
			v[i] = (int)v[i];
		}
		else
		{
			v[i] = (int)v[i] + 1;
		}
	}
}
//unlagged - attack prediction #3

vec2_t fixed_spread_grid[] =
{
	{ 0.0, 0.0 },
	{ 1.0, 0.0 },
	{ -1.0, 0.0 },
	{ 0.0, 1.0 },
	{ 0.0, -1.0 },
	{ 1.0, 1.0 },
	{ -1.0, 1.0 },
	{ 1.0, -1.0 },
	{ -1.0, -1.0 },
};

#ifdef LIBCOD
// g_shotgun_spread_fix: CoD2x's two 8-pellet patterns, as fractions of the spread
static const vec2_t shotgun_spread_patterns[2][8] =
{
	{ { 0.0, -0.8 }, { 0.0, 0.0 }, { 0.0, 0.4 }, { 0.0, -0.4 }, { -0.4, 0.0 }, { 0.4, 0.0 }, { -0.6, 0.6 }, { 0.6, 0.6 } },
	{ { -0.1, 0.0 }, { 0.1, 0.0 }, { 0.0, 0.5 }, { 0.0, -0.5 }, { -0.8, 0.6 }, { 0.8, 0.6 }, { -0.8, -0.6 }, { 0.8, -0.6 } },
};
#endif

/*
===============
Bullet_Endpos
===============
*/
void Bullet_Endpos( float spread, vec3_t end, const weaponParms *wp, float maxRange, int shotIndex )
{
	float right;
	float up;

	assert(!IS_NAN(spread));
	assert(end);
	assert(wp);

	float aimOffset = tan( spread * DEG_TO_RAD ) * maxRange;
	assert(!IS_NAN(aimOffset));

#ifdef LIBCOD
	if (g_fixedWeaponSpreads->current.boolean && shotIndex != -1 && shotIndex < ARRAY_COUNT(fixed_spread_grid))
	{
		right = fixed_spread_grid[shotIndex][0];
		up = fixed_spread_grid[shotIndex][1];
	}
	else
#endif
		gunrandom(&right, &up);

	right *= aimOffset;
	up    *= aimOffset;
	assert(!IS_NAN(right));
	assert(!IS_NAN(up));

	assert(!IS_NAN((wp->muzzleTrace)[0]) && !IS_NAN((wp->muzzleTrace)[1]) && !IS_NAN((wp->muzzleTrace)[2]));

	assert(!IS_NAN((wp->forward)[0]) && !IS_NAN((wp->forward)[1]) && !IS_NAN((wp->forward)[2]));
	assert(!IS_NAN((wp->right)[0]) && !IS_NAN((wp->right)[1]) && !IS_NAN((wp->right)[2]));
	assert(!IS_NAN((wp->up)[0]) && !IS_NAN((wp->up)[1]) && !IS_NAN((wp->up)[2]));

	VectorMA(wp->muzzleTrace, maxRange, wp->forward, end);
	assert(!IS_NAN((end)[0]) && !IS_NAN((end)[1]) && !IS_NAN((end)[2]));

	VectorMA(end, right, wp->right, end);
	VectorMA(end, up, wp->up, end);
	assert(!IS_NAN((end)[0]) && !IS_NAN((end)[1]) && !IS_NAN((end)[2]));
}

/*
===============
G_SetEquippedOffHand
===============
*/
void G_SetEquippedOffHand( int clientNum, int offHandIndex )
{
	SV_GameSendServerCommand(clientNum, SV_CMD_RELIABLE, va("%c %i", 67, offHandIndex));
}

/*
===============
G_SelectWeaponIndex
===============
*/
void G_SelectWeaponIndex( int clientnum, int iWeaponIndex )
{
	SV_GameSendServerCommand(clientnum, SV_CMD_RELIABLE, va("%c %i", 97, iWeaponIndex));
}

/*
===============
G_GivePlayerWeapon
===============
*/
qboolean G_GivePlayerWeapon( playerState_t *pPS, int iWeaponIndex )
{
	int newOffHandIndex;
	int iCurrIndex;
	WeaponDef *oldWeapDef;
	WeaponDef *weapDef;

	assert(pPS);

	if ( Com_BitCheck(pPS->weapons, iWeaponIndex) )
	{
		return qfalse;
	}

	weapDef = BG_GetWeaponDef(iWeaponIndex);

	if ( weapDef->weaponClass == WEAPCLASS_TURRET )
	{
		return qfalse;
	}

	if ( weapDef->weaponClass == WEAPCLASS_NON_PLAYER )
	{
		return qfalse;
	}

	Com_BitSet(pPS->weapons, iWeaponIndex);
	Com_BitClear(pPS->weaponrechamber, iWeaponIndex);

	if ( weapDef->weaponClass == WEAPCLASS_ITEM )
	{
		return qtrue;
	}

	if ( weapDef->offhandClass != OFFHAND_CLASS_NONE )
	{
		if ( pPS->offHandIndex )
		{
			if ( BG_WeaponAmmo(pPS, pPS->offHandIndex) <= 0 )
			{
				oldWeapDef = BG_GetWeaponDef(pPS->offHandIndex);
				assert(oldWeapDef);
				newOffHandIndex = BG_GetFirstAvailableOffhand(pPS, oldWeapDef->offhandClass);

				if ( newOffHandIndex )
					pPS->offHandIndex = newOffHandIndex;
				else
					pPS->offHandIndex = iWeaponIndex;

				G_SetEquippedOffHand(pPS->clientNum, pPS->offHandIndex);
			}
		}
		else
		{
			pPS->offHandIndex = iWeaponIndex;
			G_SetEquippedOffHand(pPS->clientNum, pPS->offHandIndex);
		}

		return qtrue;
	}

	if ( weapDef->weaponSlot - 1 < SLOT_PRIMARYB )
	{
		if ( pPS->weaponslots[SLOT_PRIMARY] )
		{
			if ( !pPS->weaponslots[SLOT_PRIMARYB] )
			{
				pPS->weaponslots[SLOT_PRIMARYB] = iWeaponIndex;
			}
		}
		else
		{
			pPS->weaponslots[SLOT_PRIMARY] = iWeaponIndex;
		}
	}

	for ( iCurrIndex = weapDef->altWeaponIndex; iCurrIndex && !Com_BitCheck(pPS->weapons, iCurrIndex); iCurrIndex = BG_GetWeaponDef(iCurrIndex)->altWeaponIndex )
	{
		Com_BitSet(pPS->weapons, iCurrIndex);
		Com_BitClear(pPS->weaponrechamber, iWeaponIndex);
	}

	return qtrue;
}

/*
===============
G_GetWeaponIndexForName
===============
*/
int G_GetWeaponIndexForName( const char *name )
{
	if ( level.initializing )
	{
		return BG_GetWeaponIndexForName(name, G_RegisterWeapon);
	}

	return BG_FindWeaponIndexForName(name);
}

/*
===============
G_SetupWeaponDef
===============
*/
void G_SetupWeaponDef()
{
	Com_DPrintf("----------------------\n");
	Com_DPrintf("Game: G_SetupWeaponDef\n");

	if ( !bg_iNumWeapons )
	{
		SV_SetWeaponInfoMemory();
		ClearRegisteredItems();

		BG_ClearWeaponDef();
		BG_FillInAmmoItems(G_RegisterWeapon);

		G_GetWeaponIndexForName("defaultweapon_mp");
	}

	Com_DPrintf("----------------------\n");
}

/*
===============
Weapon_RocketLauncher_Fire
===============
*/
void Weapon_RocketLauncher_Fire( gentity_t *ent, float spread, weaponParms *wp )
{
	vec3_t launchpos, dir, kickBack;
	float fAimOffset;

	assert(ent);
	assert(wp);

	kickBack[2] = 16;
	fAimOffset = tan(spread * DEG_TO_RAD) * kickBack[2];

	gunrandom(&kickBack[1], &kickBack[0]);

	kickBack[1] *= fAimOffset;
	kickBack[0] *= fAimOffset;

	VectorScale(wp->forward, kickBack[2], dir);

	VectorMA(dir, kickBack[1], wp->right, dir);
	VectorMA(dir, kickBack[0], wp->up, dir);

	Vec3Normalize(dir);
	VectorCopy(wp->muzzleTrace, launchpos);

	fire_rocket(ent, launchpos, dir);

	if ( ent->client )
	{
		VectorMA(ent->client->ps.velocity, -64, wp->forward, ent->client->ps.velocity);
	}
}

/*
===============
weapon_grenadelauncher_fire
===============
*/
void weapon_grenadelauncher_fire( gentity_t *ent, int grenType, weaponParms *wp )
{
	vec3_t vTossVel;
	gentity_t *m;

	assert(ent);
	assert(wp);

	VectorScale(wp->forward, wp->weapDef->projectileSpeed, vTossVel);
	vTossVel[2] += wp->weapDef->projectileSpeedUp;

	m = fire_grenade(ent, wp->muzzleTrace, vTossVel, grenType, wp->weapDef->fuseTime);

	Vec3Normalize(vTossVel);
	VectorMA(m->s.pos.trDelta, DotProduct(ent->client->ps.velocity, vTossVel), vTossVel, m->s.pos.trDelta);
}

/*
===============
G_UseOffHand
===============
*/
void G_UseOffHand( gentity_t *ent )
{
	weaponParms wp;

	assert(ent->client);
	assert(ent->client->ps.offHandIndex != WP_NONE);

	wp.weapDef = BG_GetWeaponDef(ent->client->ps.offHandIndex);
	assert(wp.weapDef->weaponType == WEAPTYPE_GRENADE);

	CalcMuzzlePoints(ent, &wp);
	weapon_grenadelauncher_fire(ent, ent->client->ps.offHandIndex, &wp);
}

/*
===============
Weapon_Melee
===============
*/
void Weapon_Melee( gentity_t *ent, weaponParms *wp, float range, float width, float height )
{
	vec3_t hitOrigin;
	gentity_t *tent, *traceEnt;
	trace_t tr;

	assert(wp);
	assert(wp->weapDef);

	int damage = BG_GetWeaponDef(ent->s.weapon)->meleeDamage;

	if ( !Melee_Trace(ent, wp, damage, range, width, height, &tr, hitOrigin) )
	{
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	if ( traceEnt->client )
		tent = G_TempEntity(hitOrigin, EV_MELEE_HIT);
	else
		tent = G_TempEntity(hitOrigin, EV_MELEE_MISS);

	tent->s.otherEntityNum = tr.entityNum;
	tent->s.eventParm = DirToByte(tr.normal);
	tent->s.weapon = ent->s.weapon;

	if ( tr.entityNum == ENTITYNUM_WORLD )
	{
		return;
	}

	if ( !traceEnt->takedamage )
	{
		return;
	}

	G_Damage( traceEnt, ent, ent, wp->forward, hitOrigin, damage + rand() % 5, 0, MOD_MELEE, tr.partGroup, 0 );
}

/*
===============
FireWeaponMelee
===============
*/
void FireWeaponMelee( gentity_t *ent )
{
	weaponParms wp;

	assert(ent);
	assert(ent->client);

	if ( ent->client->ps.eFlags & EF_TURRET_ACTIVE && ent->active )
	{
		return;
	}

	wp.weapDef = BG_GetWeaponDef(ent->s.weapon);

	G_GetPlayerViewOrigin(ent, wp.muzzleTrace);
	G_GetPlayerViewDirection(ent, wp.forward, wp.right, wp.up);

	assert(player_meleeRange);
	assert(player_meleeWidth);
	assert(player_meleeHeight);

#ifdef LIBCOD
	libcodClient_t *lc = &libcodClients[ent->s.number];
	Weapon_Melee( ent, &wp, player_meleeRange->current.decimal * lc->meleeRangeScale,
	              player_meleeWidth->current.decimal * lc->meleeWidthScale, player_meleeHeight->current.decimal * lc->meleeHeightScale );
#else
	Weapon_Melee( ent, &wp, player_meleeRange->current.decimal, player_meleeWidth->current.decimal, player_meleeHeight->current.decimal );
#endif
}

/*
===============
Bullet_Fire
===============
*/
void Bullet_Fire( gentity_t *attacker, float spread, weaponParms *wp, gentity_t *weaponEnt, int gametime )
{
	AntilagClientStore antilagClients;
	vec3_t endpos;

	assert(attacker);
	assert(wp);
	assert(wp->weapDef);
	assert(wp->weapDef->weaponType == WEAPTYPE_BULLET);

	G_AntiLagRewindClientPos(gametime, &antilagClients);

	if ( wp->weapDef->weaponClass == WEAPCLASS_SPREAD )
	{
		G_BulletFireSpread(weaponEnt, attacker, wp, gametime, spread);
	}
	else
	{
		float range = 8192;
#ifdef LIBCOD
		if ( attacker->client )
			range *= libcodClients[attacker->s.number].fireRangeScale;
#endif
		Bullet_Endpos(spread, endpos, wp, range, -1);
#ifdef LIBCOD
		// enableBulletDrop: the bullet flies over the next frames
		if ( !Libcod_FireDroppingBullet(attacker, wp, weaponEnt, gametime, endpos) )
#endif
		Bullet_Fire_Extended(weaponEnt, attacker, wp->muzzleTrace, endpos, 1.0, 0, wp, weaponEnt, gametime);
	}

	G_AntiLag_RestoreClientPos(&antilagClients);
}

/*
===============
FireWeaponAntiLag
===============
*/
void FireWeaponAntiLag( gentity_t *ent, int gametime )
{
	float minSpread, maxSpread, fAimSpreadAmount, aimSpreadScale;
	weaponParms wp;

	if ( ent->client->ps.eFlags & EF_TURRET_ACTIVE && ent->active )
	{
		return;
	}

	wp.weapDef = BG_GetWeaponDef(ent->s.weapon);
	CalcMuzzlePoints(ent, &wp);

	aimSpreadScale = ent->client->currentAimSpreadScale;
	BG_GetSpreadForWeapon(&ent->client->ps, ent->s.weapon, &minSpread, &maxSpread);

	if ( ent->client->ps.fWeaponPosFrac == 1.0 )
		fAimSpreadAmount = (maxSpread - wp.weapDef->adsSpread) * aimSpreadScale + wp.weapDef->adsSpread;
	else
		fAimSpreadAmount = (maxSpread - minSpread) * aimSpreadScale + minSpread;

#ifdef LIBCOD
	fAimSpreadAmount *= libcodClients[ent->s.number].weaponSpreadScale;
#endif

	if ( wp.weapDef->weaponType == WEAPTYPE_BULLET )
	{
		Bullet_Fire(ent, fAimSpreadAmount, &wp, ent, gametime);
		return;
	}

	if ( wp.weapDef->weaponType == WEAPTYPE_GRENADE )
	{
		weapon_grenadelauncher_fire(ent, ent->s.weapon, &wp);
		return;
	}

	if ( wp.weapDef->weaponType == WEAPTYPE_PROJECTILE )
	{
		Weapon_RocketLauncher_Fire(ent, fAimSpreadAmount, &wp);
		return;
	}

	Com_Error(ERR_DROP, "Unknown weapon type %i for %s", wp.weapDef->weaponType, wp.weapDef->szInternalName);
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent )
{
	FireWeaponAntiLag(ent, level.time);
}

/*
===============
gunrandom
===============
*/
void gunrandom( float *x, float *y )
{
	float sinT, cosT, r, theta;

	theta = randomf() * 360;
	r = randomf();

	FastSinCos(theta * DEG_TO_RAD, &sinT, &cosT);

	*x = r * cosT;
	*y = r * sinT;
}

#ifdef COD2_PLUS
static void G_PlusSetPose( gentity_t *ent, const plusPose_t *pose );
static void G_PlusRestoreAnims( XAnimTree_s *tree, const plusPose_t *now, int numRewound, const uint16_t (*rewound)[2] );
#endif

/*
===============
G_AntiLag_RestoreClientPos
===============
*/
void G_AntiLag_RestoreClientPos( AntilagClientStore *antilagStore )
{
	if ( !g_antilag->current.boolean )
	{
		return;
	}

	assert(antilagStore);

	for ( int client = 0; client < level.maxclients; client++ )
	{
		if ( !antilagStore->clientMoved[client] )
		{
			continue;
		}

		assert(!IS_NAN(antilagStore->realClientPositions[client][0]) && !IS_NAN(antilagStore->realClientPositions[client][1]) && !IS_NAN(antilagStore->realClientPositions[client][2]));

		SV_UnlinkEntity(&g_entities[client]);
		memcpy(g_entities[client].r.currentOrigin, antilagStore->realClientPositions[client], sizeof(g_entities[client].r.currentOrigin));
		SV_LinkEntity(&g_entities[client]);

#ifdef COD2_PLUS
		if ( antilagStore->poseMoved[client] )
		{
			G_PlusSetPose(&g_entities[client], &antilagStore->realPoses[client]);

			if ( antilagStore->rewoundAnims[client] >= 0 )
				G_PlusRestoreAnims(level_bgs.clientinfo[client].pXAnimTree, &antilagStore->realPoses[client],
				                   antilagStore->rewoundAnims[client], antilagStore->rewound[client]);
		}
#endif
	}

#ifdef COD2_PLUS
	SV_ResetSkeletonCache();
#endif
}

/*
===============
CalcMuzzlePoints
===============
*/
void CalcMuzzlePoints( gentity_t *ent, weaponParms *wp )
{
	vec3_t viewang;

	if ( !ent->client )
	{
		return;
	}

	VectorCopy(ent->client->ps.viewangles, viewang);

	viewang[0] = ent->client->fGunPitch;
	viewang[1] = ent->client->fGunYaw;

	AngleVectors(viewang, wp->forward, wp->right, wp->up);
	G_GetPlayerViewOrigin(ent, wp->muzzleTrace);
}

#ifdef COD2_PLUS
// a second of frames at 125 Hz, more than the antilag goes back
#define PLUS_POSE_FRAMES 128
static plusPose_t plusPoses[MAX_CLIENTS][PLUS_POSE_FRAMES];
static int plusPoseNext[MAX_CLIENTS];

static void G_PlusGetPose( gentity_t *ent, plusPose_t *pose )
{
	clientInfo_t *ci = &level_bgs.clientinfo[ent->s.number];

	pose->time = level.time;
	VectorCopy(ent->r.currentAngles, pose->currentAngles);
	pose->eFlags = ent->s.eFlags;
	pose->fTorsoHeight = ent->s.fTorsoHeight;
	pose->fTorsoPitch = ent->s.fTorsoPitch;
	pose->fWaistPitch = ent->s.fWaistPitch;
	pose->legsYaw = ci->legs.yawAngle;
	pose->torsoYaw = ci->torso.yawAngle;
	pose->torsoPitch = ci->torso.pitchAngle;
	pose->lerpLean = ci->lerpLean;
	VectorCopy(ci->playerAngles, pose->playerAngles);
	pose->control = ci->control;

	XAnimTree_s *tree = ci->pXAnimTree;
	pose->numAnims = 0;

	for ( unsigned int i = 0; i < tree->anims->size; i++ )
	{
		unsigned int infoIndex = tree->infoArray[i];

		if ( !infoIndex )
			continue;

		if ( pose->numAnims == PLUS_POSE_ANIMS )
		{
			pose->numAnims = -1;
			return;
		}

		pose->anims[pose->numAnims].index = i;
		pose->anims[pose->numAnims].infoIndex = infoIndex;
		pose->anims[pose->numAnims].state = g_xAnimInfo[infoIndex].state;
		pose->numAnims++;
	}
}

/*
===============
G_PlusRewindAnims

Plus: puts back the tree's entries of then, with their states: the stance's
animation is the tree's, so a player who crouched since kept the crouch with
the flags of standing. The entries of now that then lacks are taken out, not
freed, for the restore; those of then that now lacks get a new info. Returns
how many entries it set, in rewound
===============
*/
static int G_PlusRewindAnims( XAnimTree_s *tree, const plusPose_t *then, const plusPose_t *now, uint16_t (*rewound)[2] )
{
	int k, j;

	for ( k = 0; k < then->numAnims; k++ )
	{
		if ( then->anims[k].index >= tree->anims->size )
			return -1; // of another tree
	}

	for ( k = 0; k < now->numAnims; k++ )
		tree->infoArray[now->anims[k].index] = 0;

	// both are in the order of the index
	for ( k = 0, j = 0; k < then->numAnims; k++ )
	{
		unsigned int index = then->anims[k].index;
		XAnimInfo *info;

		while ( j < now->numAnims && now->anims[j].index < index )
			j++;

		if ( j < now->numAnims && now->anims[j].index == index )
		{
			tree->infoArray[index] = now->anims[j].infoIndex;
			info = &g_xAnimInfo[now->anims[j].infoIndex];
		}
		else
		{
			info = XAnimAllocInfo(tree, index);
			info->notifyChild = 0;
			info->notifyIndex = -1;
			info->notifyName = 0;
			info->notifyType = 0;
		}

		info->state = then->anims[k].state;
		rewound[k][0] = index;
		rewound[k][1] = tree->infoArray[index];
	}

	return then->numAnims;
}

/*
===============
G_PlusRestoreAnims
===============
*/
static void G_PlusRestoreAnims( XAnimTree_s *tree, const plusPose_t *now, int numRewound, const uint16_t (*rewound)[2] )
{
	unsigned int i;
	int k, j;

	// the tree holds what the rewind set unless the shot changed it: a
	// script it ran may have dropped the player, which clears the tree
	for ( i = 0, k = 0; i < tree->anims->size; i++ )
	{
		if ( !tree->infoArray[i] )
			continue;

		if ( k == numRewound || rewound[k][0] != i || rewound[k][1] != tree->infoArray[i] )
			break;

		k++;
	}

	bool intact = i == tree->anims->size && k == numRewound;

	if ( intact )
	{
		// free the infos the rewind added
		for ( k = 0, j = 0; k < numRewound; k++ )
		{
			while ( j < now->numAnims && now->anims[j].index < rewound[k][0] )
				j++;

			if ( j == now->numAnims || now->anims[j].index != rewound[k][0] )
				XAnimFreeInfo(tree, rewound[k][1]);

			tree->infoArray[rewound[k][0]] = 0;
		}
	}

	for ( j = 0; j < now->numAnims; j++ )
	{
		if ( intact )
		{
			tree->infoArray[now->anims[j].index] = now->anims[j].infoIndex;
			g_xAnimInfo[now->anims[j].infoIndex].state = now->anims[j].state;
			continue;
		}

		for ( k = 0; k < numRewound && rewound[k][0] != now->anims[j].index; k++ )
			;

		// taken out by the rewind, it is in no tree now
		if ( k == numRewound )
			XAnimFreeInfo(tree, now->anims[j].infoIndex);
	}
}

static void G_PlusSetPose( gentity_t *ent, const plusPose_t *pose )
{
	clientInfo_t *ci = &level_bgs.clientinfo[ent->s.number];

	VectorCopy(pose->currentAngles, ent->r.currentAngles);
	ent->s.eFlags = pose->eFlags;
	ent->s.fTorsoHeight = pose->fTorsoHeight;
	ent->s.fTorsoPitch = pose->fTorsoPitch;
	ent->s.fWaistPitch = pose->fWaistPitch;
	ci->legs.yawAngle = pose->legsYaw;
	ci->torso.yawAngle = pose->torsoYaw;
	ci->torso.pitchAngle = pose->torsoPitch;
	ci->lerpLean = pose->lerpLean;
	VectorCopy(pose->playerAngles, ci->playerAngles);
	ci->control = pose->control;
}

/*
===============
G_PlusRecordPose

Plus: keeps a player's pose at the end of each frame. The antilag moved only
the origin: the yaw, lean, aim and stance flags of the hitboxes stayed those
of the present, so a shot at a player who leaned out and back missed what the
shooter saw. The bone controllers reach their angles over time, as the client
draws them, so their state is kept as it is, not rebuilt from a playerstate
(docs/architecture/plus-protocol.md#antilag)
===============
*/
void G_PlusRecordPose( gentity_t *ent )
{
	int clientNum = ent->s.number;

	G_PlusGetPose(ent, &plusPoses[clientNum][plusPoseNext[clientNum] % PLUS_POSE_FRAMES]);
	plusPoseNext[clientNum]++;
}

// the newest pose kept at gameTime or before it, or NULL
static const plusPose_t *G_PlusFindPose( int clientNum, int gameTime )
{
	for ( int i = 1; i <= PLUS_POSE_FRAMES && i <= plusPoseNext[clientNum]; i++ )
	{
		const plusPose_t *pose = &plusPoses[clientNum][(plusPoseNext[clientNum] - i) % PLUS_POSE_FRAMES];

		if ( pose->time > level.time )
			return NULL; // from an earlier map
		if ( pose->time <= gameTime )
			return pose;
	}

	return NULL;
}
#endif

/*
===============
G_AntiLagRewindClientPos
===============
*/
void G_AntiLagRewindClientPos( int gameTime, AntilagClientStore *antilagStore )
{
	vec3_t clientPosition;
	int snapshotTime;

	if ( !g_antilag->current.boolean )
	{
		return;
	}

	assert(antilagStore);
	memset(antilagStore, 0, sizeof(AntilagClientStore));
	assert(gameTime > 0);

	if ( level.time - gameTime <= 1000 / sv_fps->current.integer )
	{
		return;
	}

	for ( int client = 0; client < level.maxclients; client++ )
	{
		if ( level.clients[client].sess.connected != CON_CONNECTED )
		{
			continue;
		}

		if ( level.clients[client].sess.sessionState != SESS_STATE_PLAYING )
		{
			continue;
		}

		if ( !SV_GetClientPositionsAtTime(client, gameTime, clientPosition) )
		{
			continue;
		}

		assert(!IS_NAN(antilagStore->realClientPositions[client][0]) && !IS_NAN(antilagStore->realClientPositions[client][1]) && !IS_NAN(antilagStore->realClientPositions[client][2]));
		snapshotTime = gameTime;

		memcpy(antilagStore->realClientPositions[client], g_entities[client].r.currentOrigin, sizeof(antilagStore->realClientPositions[client]));

		SV_UnlinkEntity(&g_entities[client]);
		memcpy(g_entities[client].r.currentOrigin, clientPosition, sizeof(g_entities[client].r.currentOrigin));
		SV_LinkEntity(&g_entities[client]);

		antilagStore->clientMoved[client] = true;

#ifdef COD2_PLUS
		const plusPose_t *pose = G_PlusFindPose(client, gameTime);

		if ( pose )
		{
			plusPose_t *now = &antilagStore->realPoses[client];

			G_PlusGetPose(&g_entities[client], now);
			G_PlusSetPose(&g_entities[client], pose);
			antilagStore->poseMoved[client] = true;
			antilagStore->rewoundAnims[client] = -1;

			if ( pose->numAnims >= 0 && now->numAnims >= 0 )
				antilagStore->rewoundAnims[client] = G_PlusRewindAnims(level_bgs.clientinfo[client].pXAnimTree,
				                                                       pose, now, antilagStore->rewound[client]);
		}
#endif
	}

#ifdef COD2_PLUS
	// the skeletons computed this frame are of the present poses
	SV_ResetSkeletonCache();
#endif
}

/*
==============
Bullet_Fire_Extended
	A modified Bullet_Fire with more parameters.
	The original Bullet_Fire still passes through here and functions as it always has.

	uses for this include shooting through entities (windows, doors, other players, etc.) and reflecting bullets
==============
*/
#define MAX_BULLET_RESURSIONS 12
void Bullet_Fire_Extended( const gentity_t *source, gentity_t *attacker,
                           vec3_t start, vec3_t end, float spread, int resursion,
                           const weaponParms *wp, const gentity_t *weaponEnt, int gameTime )
{
	int mod, event, damage;
	int dflags = 0;
	float dot;
	vec3_t hitPos, reflect, dist;
	gentity_t *traceEnt, *tent;
	trace_t tr;

	if ( resursion > MAX_BULLET_RESURSIONS )
	{
		Com_DPrintf("Bullet_Fire_Extended: Too many resursions, bullet aborted\n");
		return;
	}

	if ( wp->weapDef->rifleBullet )
	{
		mod = MOD_RIFLE_BULLET;
		dflags = DAMAGE_PASSTHRU;
	}
	else
	{
		mod = MOD_PISTOL_BULLET;
	}

	if ( wp->weapDef->armorPiercing )
	{
		dflags |= DAMAGE_NO_ARMOR;
	}

	int contentMask = MASK_SHOT;
	bool impacts = true;

#ifdef LIBCOD
	if ( attacker->client )
	{
		libcodClient_t *lc = &libcodClients[attacker->s.number];

		if ( lc->overrideBulletMask )
			contentMask = lc->bulletMask;
		else if ( lc->fireThroughWalls )
			contentMask = CONTENTS_BODY;

		impacts = !lc->noBulletImpacts;
	}
#endif

	if ( wp->weapDef->rifleBullet )
		G_LocationalTrace(&tr, start, end, source->s.number, contentMask, riflePriorityMap);
	else
		G_LocationalTrace(&tr, start, end, source->s.number, contentMask, bulletPriorityMap);

	Vec3Lerp(start, end, tr.fraction, hitPos);
	G_CheckHitTriggerDamage(attacker, start, hitPos, wp->weapDef->damage, mod);

#ifdef LIBCOD
	// a dropping bullet ends at whatever it meets
	if ( tr.fraction < 1.0 )
		libcodBulletHit = true;
#endif

	traceEnt = &g_entities[tr.entityNum];

	VectorSubtract(end, start, reflect);
	Vec3Normalize(reflect);

	VectorMA(reflect, DotProduct(reflect, tr.normal) * -2.0, tr.normal, reflect);

	// send bullet impact
	if ( impacts && !( tr.surfaceFlags & SURF_SKY ) && !traceEnt->client && tr.fraction < 1.0 )
	{
		// legacy?
		if ( wp->weapDef->weaponClass == WEAPCLASS_SPREAD )
		{
			event = EV_BULLET_HIT_SMALL;
		}
		else if ( wp->weapDef->rifleBullet )
		{
			event = EV_SHOTGUN_HIT;
		}
		else
		{
			event = EV_BULLET_HIT_LARGE;
		}

		if ( wp->weapDef->rifleBullet )
			event = EV_SHOTGUN_HIT;
		else
			event = EV_BULLET_HIT_LARGE;

		tent = G_TempEntity(hitPos, event);

		tent->s.eventParm  = DirToByte(tr.normal);
		tent->s.eventParm2 = DirToByte(reflect);

#ifdef LIBCOD
		// g_corpseHit 0 hits a corpse as the surface under it
		if ( traceEnt->s.eType == ET_PLAYER_CORPSE && g_corpseHit->current.boolean )
#else
		if ( traceEnt->s.eType == ET_PLAYER_CORPSE )
#endif
			tent->s.surfType = SURF_TYPE_FLESH;
		else
			tent->s.surfType = SURF_TYPEINDEX( tr.surfaceFlags );

		tent->s.otherEntityNum = weaponEnt->s.number;
	}

	if ( tr.contents & SURF_NOIMPACT )
	{
		VectorSubtract(end, start, reflect);
		Vec3Normalize(reflect);

		dot = DotProduct(tr.normal, reflect);

		if ( -dot >= 0.125 )
			VectorMA(hitPos, 0.25 / -dot, reflect, start);
		else
			VectorMA(hitPos, 0, reflect, start);

		Bullet_Fire_Extended(source, attacker, start, end, spread, resursion + 1, wp, weaponEnt, gameTime);
		return;
	}

	if ( !traceEnt->takedamage )
	{
		return;
	}

	if ( traceEnt == attacker )
	{
		return;
	}

	VectorSubtract( start, hitPos, dist );
#ifdef LIBCOD
	// a dropping bullet's earlier steps count for the range falloff
	damage = Bullet_GetDamage( wp, VectorLength(dist) + libcodBulletDistance ) * spread;
#else
	damage = Bullet_GetDamage( wp, VectorLength(dist) ) * spread;
#endif

	G_Damage( traceEnt, attacker, attacker, wp->forward, hitPos, damage, dflags, mod, tr.partGroup, level.time - gameTime );

	if ( !traceEnt->client )
	{
		return;
	}

	// allow bullets to "pass through" func_explosives if they break by taking another simultanious shot
	if ( !( dflags & DAMAGE_PASSTHRU ) )
	{
		return;
	}

	if ( !Dvar_GetInt("scr_friendlyfire") && OnSameTeam(traceEnt, attacker) )
	{
		return;
	}

	// start new bullet at position this hit the bmodel and continue to the end position (ignoring shot-through bmodel in next trace)
	// spread = 0 as this is an extension of an already spread shot
	Bullet_Fire_Extended( traceEnt, attacker, hitPos, end, spread * 0.5, resursion + 1, wp, weaponEnt, gameTime );
}

vec2_t traceOffsets[] =
{
	{ 0.000000, 0.000000},
	{ 1.000000, 1.000000},
	{ 1.000000, -1.000000},
	{ -1.000000, 1.000000},
	{ -1.000000, -1.000000},
};

/*
===============
Melee_Trace
===============
*/
bool Melee_Trace( gentity_t *ent, weaponParms *wp, int damage, float range, float width, float height, trace_t *traceResult, vec3_t hitOrigin )
{
	int numTraces;
	int traceIndex;
	vec3_t endPos;

	assert(ent);
	assert(wp);
	assert(traceResult);
	assert(hitOrigin);

	if ( width <= 0 && height <= 0 )
		numTraces = 1;
	else
		numTraces = ARRAY_COUNT( traceOffsets );

	for ( traceIndex = 0; traceIndex < numTraces; traceIndex++ )
	{
		VectorMA(wp->muzzleTrace, range, wp->forward, endPos);

		VectorMA(endPos, width  * traceOffsets[traceIndex][0], wp->right, endPos);
		VectorMA(endPos, height * traceOffsets[traceIndex][1], wp->up,    endPos);

		G_LocationalTrace(traceResult, wp->muzzleTrace, endPos, ent->s.number, MASK_SHOT, bulletPriorityMap);
		Vec3Lerp(wp->muzzleTrace, endPos, traceResult->fraction, hitOrigin);

		if ( !traceIndex )
		{
			G_CheckHitTriggerDamage(ent, wp->muzzleTrace, hitOrigin, damage, MOD_MELEE);
		}

		if ( !( traceResult->surfaceFlags & SURF_NOIMPACT ) && traceResult->fraction != 1.0 )
		{
			return true;
		}
	}

	return false;
}

/*
===============
G_BulletFireSpread
===============
*/
void G_BulletFireSpread( const gentity_t *weaponEnt, gentity_t *attacker, const weaponParms *wp, int gameTime, float spread )
{
	vec3_t start;
	vec3_t end;
	int shotCount;

	VectorCopy(wp->muzzleTrace, start);
	shotCount = wp->weapDef->shotCount;

#ifdef LIBCOD
	if (g_fixedWeaponSpreads->current.boolean)
		shotCount++; // Extra bullet for a center shot.
#endif

	float range = wp->weapDef->minDamageRange;
#ifdef LIBCOD
	if ( attacker->client )
		range *= libcodClients[attacker->s.number].fireRangeScale;
#endif

#ifdef LIBCOD
	// every pellet on one of two fixed patterns, turned at random per shot, so
	// the same shot does not kill or miss by chance
	if ( g_shotgun_spread_fix->current.boolean && wp->weapDef->shotCount == 8 )
	{
		const vec2_t *pattern = shotgun_spread_patterns[randomf() < 0.5];
		float aimOffset = tan( spread * DEG_TO_RAD ) * range;
		float s, c;

		FastSinCos(randomf() * 2 * M_PI, &s, &c);

		for ( int i = 0; i < 8; i++ )
		{
			float right = (pattern[i][0] * c - pattern[i][1] * s) * aimOffset;
			float up = (pattern[i][0] * s + pattern[i][1] * c) * aimOffset;

			VectorMA(wp->muzzleTrace, range, wp->forward, end);
			VectorMA(end, right, wp->right, end);
			VectorMA(end, up, wp->up, end);
			Bullet_Fire_Extended(weaponEnt, attacker, start, end, 1.0, 0, wp, weaponEnt, gameTime);
		}
		return;
	}
#endif

	for ( int i = 0; i < shotCount; i++ )
	{
		Bullet_Endpos(spread, end, wp, range, i);
		Bullet_Fire_Extended(weaponEnt, attacker, start, end, 1.0, 0, wp, weaponEnt, gameTime);
	}
}

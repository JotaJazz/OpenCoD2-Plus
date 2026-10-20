#include "gsc_entity.hpp"

#if LIBCOD_COMPILE_ENTITY == 1

void gsc_entity_setalive(scr_entref_t id)
{
	int isAlive;

	if ( ! stackGetParams("i", &isAlive))
	{
		stackError("gsc_entity_setalive() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = GetEntity(id);

	entity->takedamage = isAlive;
	stackPushBool(qtrue);
}

void gsc_entity_setbounds(scr_entref_t id)
{
	float width, height;

	if ( ! stackGetParams("ff", &width, &height))
	{
		stackError("gsc_entity_setbounds() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *entity = GetEntity(id);

	vec3_t mins = {-height, -width, -width};
	vec3_t maxs = {width, width, height};

	VectorCopy(mins, entity->r.mins);
	VectorCopy(maxs, entity->r.maxs);

	stackPushBool(qtrue);
}

// Methods below are named and behave as in zk_libcod

// spawnGrenade(weapon, origin[, direction[, velocity[, fuse seconds]]]): a
// grenade thrown by this entity
void gsc_entity_spawngrenade(scr_entref_t id)
{
	int args = Scr_GetNumParam();
	const char *name;
	vec3_t origin;
	vec3_t dir = { 0, 0, 0 };
	vec3_t velocity = { 0, 0, 0 };

	if (args < 2 || args > 5 || !stackGetParamString(0, &name) || !stackGetParamVector(1, origin)
	        || (args > 2 && !stackGetParamVector(2, dir)) || (args > 3 && !stackGetParamVector(3, velocity)))
	{
		stackError("gsc_entity_spawngrenade() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	int weaponIndex = BG_FindWeaponIndexForName(name);
	if (!weaponIndex || BG_GetWeaponDef(weaponIndex)->weaponType != WEAPTYPE_GRENADE)
	{
		stackError("gsc_entity_spawngrenade() %s is not a loaded grenade", name);
		stackPushUndefined();
		return;
	}

	int fuseTime = BG_GetWeaponDef(weaponIndex)->fuseTime;
	if (args > 4)
	{
		float seconds;
		if (!stackGetParamFloat(4, &seconds))
		{
			stackError("gsc_entity_spawngrenade() fuse time has a wrong type");
			stackPushUndefined();
			return;
		}
		fuseTime = (int)(seconds * 1000);
	}

	gentity_t *owner = GetEntity(id);

	// fire_grenade takes over a grenade the owner is cooking; this one is
	// not that grenade
	int cooking = owner->client ? owner->client->ps.grenadeTimeLeft : 0;
	if (owner->client)
		owner->client->ps.grenadeTimeLeft = 0;

	gentity_t *grenade = fire_grenade(owner, origin, dir, weaponIndex, fuseTime);

	if (owner->client)
		owner->client->ps.grenadeTimeLeft = cooking;

	Vec3Normalize(dir);
	VectorMA(grenade->s.pos.trDelta, DotProduct(velocity, dir), dir, grenade->s.pos.trDelta);

	stackPushEntity(grenade);
}

static bool gsc_entity_isgrenade(scr_entref_t id, const char *function)
{
	if (GetEntity(id)->handler != ENT_HANDLER_GRENADE)
	{
		stackError("%s() entity %i is not a grenade", function, id.entnum);
		stackPushUndefined();
		return false;
	}
	return true;
}

// Seconds left before a thrown grenade explodes
void gsc_entity_getgrenadefusetime(scr_entref_t id)
{
	if (!gsc_entity_isgrenade(id, "gsc_entity_getgrenadefusetime"))
		return;

	int left = GetEntity(id)->nextthink - level.time;
	stackPushFloat(left < 0 ? 0 : left / 1000.0f);
}

// Adds (or with a negative value removes) seconds of fuse
void gsc_entity_addgrenadefusetime(scr_entref_t id)
{
	float seconds;

	if (!stackGetParams("f", &seconds))
	{
		stackError("gsc_entity_addgrenadefusetime() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!gsc_entity_isgrenade(id, "gsc_entity_addgrenadefusetime"))
		return;

	gentity_t *ent = GetEntity(id);
	int ms = (int)(seconds * 1000);

	ent->nextthink += ms - ms % FRAMETIME;
	if (ent->nextthink < level.time)
		ent->nextthink = level.time;

	stackPushBool(qtrue);
}

void gsc_entity_isturret(scr_entref_t id)
{
	stackPushBool(GetEntity(id)->pTurretInfo != NULL);
}

// The player on the turret, or undefined
void gsc_entity_getturretowner(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);

	if (!ent->pTurretInfo)
	{
		stackError("gsc_entity_getturretowner() entity %i is not a turret", id.entnum);
		stackPushUndefined();
		return;
	}

	if (ent->r.ownerNum == ENTITYNUM_NONE)
		stackPushUndefined();
	else
		stackPushEntity(&g_entities[ent->r.ownerNum]);
}

static const gitem_t *gsc_entity_getitem(scr_entref_t id, const char *function, int type1, int type2)
{
	gentity_t *ent = GetEntity(id);

	if (ent->s.eType != ET_ITEM || ent->s.index < 1 || ent->s.index >= bg_numItems
	        || (bg_itemlist[ent->s.index].giType != type1 && bg_itemlist[ent->s.index].giType != type2))
	{
		stackError("%s() entity %i is not an item of the right type", function, id.entnum);
		stackPushUndefined();
		return NULL;
	}

	return &bg_itemlist[ent->s.index];
}

// Ammo of a weapon lying on the ground (-1: none)
void gsc_entity_getweaponitemammo(scr_entref_t id)
{
	if (gsc_entity_getitem(id, "gsc_entity_getweaponitemammo", IT_WEAPON, IT_WEAPON))
		stackPushInt(GetEntity(id)->count);
}

void gsc_entity_setweaponitemammo(scr_entref_t id)
{
	int ammo;

	if (!stackGetParams("i", &ammo))
	{
		stackError("gsc_entity_setweaponitemammo() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (gsc_entity_getitem(id, "gsc_entity_setweaponitemammo", IT_WEAPON, IT_WEAPON))
	{
		GetEntity(id)->count = ammo;
		stackPushBool(qtrue);
	}
}

void gsc_entity_getweaponitemclipammo(scr_entref_t id)
{
	if (gsc_entity_getitem(id, "gsc_entity_getweaponitemclipammo", IT_WEAPON, IT_WEAPON))
		stackPushInt(GetEntity(id)->item.clipAmmoCount);
}

void gsc_entity_setweaponitemclipammo(scr_entref_t id)
{
	int ammo;

	if (!stackGetParams("i", &ammo))
	{
		stackError("gsc_entity_setweaponitemclipammo() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (gsc_entity_getitem(id, "gsc_entity_setweaponitemclipammo", IT_WEAPON, IT_WEAPON))
	{
		GetEntity(id)->item.clipAmmoCount = ammo;
		stackPushBool(qtrue);
	}
}

// How much an ammo or health item gives. This is per item type: it changes
// every item of that type.
void gsc_entity_getitemquantity(scr_entref_t id)
{
	const gitem_t *item = gsc_entity_getitem(id, "gsc_entity_getitemquantity", IT_AMMO, IT_HEALTH);
	if (item)
		stackPushInt(item->quantity);
}

// bg_itemlist outlives the map, so the stock quantities are restored on the
// next level load
static int *itemQuantities;

void gsc_entity_restoreitemquantities()
{
	if (!itemQuantities)
		return;

	for (int i = 0; i <= bg_numItems; i++)
		bg_itemlist[i].quantity = itemQuantities[i];
}

void gsc_entity_setitemquantity(scr_entref_t id)
{
	int quantity;

	if (!stackGetParams("i", &quantity))
	{
		stackError("gsc_entity_setitemquantity() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	const gitem_t *item = gsc_entity_getitem(id, "gsc_entity_setitemquantity", IT_AMMO, IT_HEALTH);
	if (item)
	{
		if (!itemQuantities)
		{
			itemQuantities = (int *)malloc((bg_numItems + 1) * sizeof(int));
			for (int i = 0; i <= bg_numItems; i++)
				itemQuantities[i] = bg_itemlist[i].quantity;
		}
		bg_itemlist[item - bg_itemlist].quantity = quantity;
		stackPushBool(qtrue);
	}
}

// The world position of a tag of the entity's model, or undefined when the
// model has no such tag
void gsc_entity_gettagorigin(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);
	unsigned int tag = Scr_GetConstLowercaseString(0);
	vec3_t origin;

	// "" is the model's origin
	if (!*SL_ConvertToString(tag))
	{
		stackPushVector(ent->r.currentOrigin);
		return;
	}

	if (!SV_DObjExists(ent) || SV_DObjGetBoneIndex(ent, tag) < 0 || !G_DObjGetWorldTagPos(ent, tag, origin))
	{
		stackPushUndefined();
		return;
	}

	stackPushVector(origin);
}

void gsc_entity_gettagangles(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);
	unsigned int tag = Scr_GetConstLowercaseString(0);
	float matrix[4][3];
	vec3_t angles;

	if (!*SL_ConvertToString(tag))
	{
		stackPushVector(ent->r.currentAngles);
		return;
	}

	if (!SV_DObjExists(ent) || SV_DObjGetBoneIndex(ent, tag) < 0 || !G_DObjGetWorldTagMatrix(ent, tag, matrix))
	{
		stackPushUndefined();
		return;
	}

	AxisToAngles(matrix, angles);
	stackPushVector(angles);
}

void gsc_entity_hastag(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);
	unsigned int tag = Scr_GetConstLowercaseString(0);

	stackPushBool(SV_DObjExists(ent) && SV_DObjGetBoneIndex(ent, tag) >= 0);
}

// The entity this one is linked to with linkTo, or undefined
void gsc_entity_islinkedto(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);

	if (ent->tagInfo && ent->tagInfo->parent)
		stackPushEntity(ent->tagInfo->parent);
	else
		stackPushUndefined();
}

void gsc_entity_getvmin(scr_entref_t id)
{
	stackPushVector(GetEntity(id)->r.mins);
}

void gsc_entity_getvmax(scr_entref_t id)
{
	stackPushVector(GetEntity(id)->r.maxs);
}

// What the entity's own movement collides with (missiles, items, movers;
// players move with their own mask)
void gsc_entity_getclipmask(scr_entref_t id)
{
	stackPushInt(GetEntity(id)->clipmask);
}

void gsc_entity_setclipmask(scr_entref_t id)
{
	int mask;

	if (!stackGetParams("i", &mask))
	{
		stackError("gsc_entity_setclipmask() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	GetEntity(id)->clipmask = mask;
	stackPushBool(qtrue);
}

// What the entity is made of for traces; a player's overrideContents value
// once it applies
void gsc_entity_getcontents(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);

	if (ent->s.number < MAX_CLIENTS && libcodClients[ent->s.number].overrideContents)
		stackPushInt(libcodClients[ent->s.number].contents);
	else
		stackPushInt(ent->r.contents);
}

// setLight(red, green, blue, intensity), each 0-255, on a script model
void gsc_entity_setlight(scr_entref_t id)
{
	int r, g, b, intensity;

	if (!stackGetParams("iiii", &r, &g, &b, &intensity))
	{
		stackError("gsc_entity_setlight() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *ent = GetEntity(id);
	if (ent->s.eType != ET_SCRIPTMOVER)
	{
		stackError("gsc_entity_setlight() entity %i is not a script model", id.entnum);
		stackPushUndefined();
		return;
	}

	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	intensity = intensity < 0 ? 0 : intensity > 255 ? 255 : intensity;
	ent->s.constantLight = r | (g << 8) | (b << 16) | (intensity << 24);
	stackPushBool(qtrue);
}

// The opposite of the stock showToPlayer: this player stops receiving the
// entity
void gsc_entity_hidefromplayer(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);
	gentity_t *player = Scr_GetEntity(0);

	if (!player || player->s.number >= MAX_CLIENTS)
	{
		stackError("gsc_entity_hidefromplayer() argument is not a player");
		stackPushUndefined();
		return;
	}

	int word = player->s.number >> 5, bit = 1 << (player->s.number & 0x1F);
	ent->r.clientMask[word] |= bit;
	libcodEntities[ent->s.number].hiddenMask[word] |= bit;

	if (ent->r.clientMask[0] == -1 && ent->r.clientMask[1] == -1)
		ent->flags |= FL_NODRAW;

	stackPushBool(qtrue);
}

static void gsc_entity_setsolidforplayer(scr_entref_t id, const char *function, bool solid)
{
	gentity_t *ent = GetEntity(id);
	gentity_t *player = Scr_GetEntity(0);

	if (!player || player->s.number >= MAX_CLIENTS)
	{
		stackError("%s() argument is not a player", function);
		stackPushUndefined();
		return;
	}

	if (!ent->r.bmodel)
	{
		stackError("%s() entity %i is not a brush model", function, id.entnum);
		stackPushUndefined();
		return;
	}

	int word = player->s.number >> 5, bit = 1 << (player->s.number & 0x1F);
	if (solid)
		libcodEntities[ent->s.number].notSolidMask[word] &= ~bit;
	else
		libcodEntities[ent->s.number].notSolidMask[word] |= bit;

	stackPushBool(qtrue);
}

// A brush model (door, wall) this player walks through, and sees as not
// solid; the others still collide with it
void gsc_entity_notsolidforplayer(scr_entref_t id)
{
	gsc_entity_setsolidforplayer(id, "gsc_entity_notsolidforplayer", false);
}

void gsc_entity_solidforplayer(scr_entref_t id)
{
	gsc_entity_setsolidforplayer(id, "gsc_entity_solidforplayer", true);
}

// Script model physics, as in zk_libcod: enableGravity makes a script model
// fall, slide off walls and land; enableBounce makes it bounce
static bool gsc_entity_isscriptmodel(gentity_t *ent, const char *function)
{
	if (ent->classname != scr_const.script_model)
	{
		stackError("%s() entity %i is not a script_model", function, ent->s.number);
		stackPushUndefined();
		return false;
	}
	return true;
}

// enableGravity([collideModels], [alignToGround]): collideModels traces
// against the hit boxes of models instead of their bounds
void gsc_entity_enablegravity(scr_entref_t id)
{
	int locational = 1, align = 1;
	int params = Scr_GetNumParam();

	if ((params > 0 && !stackGetParamInt(0, &locational)) || (params > 1 && !stackGetParamInt(1, &align)))
	{
		stackError("gsc_entity_enablegravity() argument has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *ent = GetEntity(id);
	if (!gsc_entity_isscriptmodel(ent, "gsc_entity_enablegravity"))
		return;

	Libcod_EnableGravity(ent, libcodEntities[ent->s.number].gravity == LIBCOD_GRAVITY_BOUNCE, locational != 0, align != 0);
	stackPushBool(qtrue);
}

void gsc_entity_disablegravity(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);
	if (!gsc_entity_isscriptmodel(ent, "gsc_entity_disablegravity"))
		return;

	bool was = libcodEntities[ent->s.number].gravity != LIBCOD_GRAVITY_NONE;
	Libcod_DisableGravity(ent);
	stackPushBool(was);
}

void gsc_entity_isgravityenabled(scr_entref_t id)
{
	stackPushBool(libcodEntities[GetEntity(id)->s.number].gravity != LIBCOD_GRAVITY_NONE);
}

// enableBounce([parallel], [perpendicular]): how much speed is kept along
// and across the surface at each bounce (0 to 1)
void gsc_entity_enablebounce(scr_entref_t id)
{
	float parallel = 0.5f, perpendicular = 0.25f;
	int params = Scr_GetNumParam();

	if ((params > 0 && !stackGetParamFloat(0, &parallel)) || (params > 1 && !stackGetParamFloat(1, &perpendicular)))
	{
		stackError("gsc_entity_enablebounce() argument has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *ent = GetEntity(id);
	if (!gsc_entity_isscriptmodel(ent, "gsc_entity_enablebounce"))
		return;

	libcodEntity_t *le = &libcodEntities[ent->s.number];
	if (!le->gravity)
	{
		stackPushBool(qfalse);
		return;
	}

	Libcod_EnableGravity(ent, true, le->locational, le->alignToGround);
	le->parallelBounce = parallel < 0 ? 0 : parallel > 1 ? 1 : parallel;
	le->perpendicularBounce = perpendicular < 0 ? 0 : perpendicular > 1 ? 1 : perpendicular;
	stackPushBool(qtrue);
}

void gsc_entity_disablebounce(scr_entref_t id)
{
	gentity_t *ent = GetEntity(id);
	if (!gsc_entity_isscriptmodel(ent, "gsc_entity_disablebounce"))
		return;

	libcodEntity_t *le = &libcodEntities[ent->s.number];
	if (!le->gravity)
	{
		stackPushBool(qfalse);
		return;
	}

	Libcod_EnableGravity(ent, false, le->locational, le->alignToGround);
	stackPushBool(qtrue);
}

static gentity_t *gsc_entity_withgravity(scr_entref_t id, const char *function)
{
	gentity_t *ent = GetEntity(id);
	if (!gsc_entity_isscriptmodel(ent, function))
		return NULL;
	return ent;
}

void gsc_entity_getentityvelocity(scr_entref_t id)
{
	gentity_t *ent = gsc_entity_withgravity(id, "gsc_entity_getentityvelocity");
	if (!ent)
		return;

	if (!libcodEntities[ent->s.number].gravity)
	{
		stackPushUndefined();
		return;
	}

	vec3_t velocity;
	Libcod_GetEntityVelocity(ent, velocity);
	stackPushVector(velocity);
}

static void gsc_entity_changevelocity(scr_entref_t id, const char *function, bool add)
{
	vec3_t velocity, current;

	if (!stackGetParams("v", velocity))
	{
		stackError("%s() argument is undefined or has a wrong type", function);
		stackPushUndefined();
		return;
	}

	gentity_t *ent = gsc_entity_withgravity(id, function);
	if (!ent)
		return;

	if (!libcodEntities[ent->s.number].gravity)
	{
		stackPushBool(qfalse);
		return;
	}

	if (add)
	{
		Libcod_GetEntityVelocity(ent, current);
		VectorAdd(velocity, current, velocity);
	}

	Libcod_SetEntityVelocity(ent, velocity);
	stackPushBool(qtrue);
}

void gsc_entity_setentityvelocity(scr_entref_t id)
{
	gsc_entity_changevelocity(id, "gsc_entity_setentityvelocity", false);
}

void gsc_entity_addentityvelocity(scr_entref_t id)
{
	gsc_entity_changevelocity(id, "gsc_entity_addentityvelocity", true);
}

void gsc_entity_setmaxentityvelocity(scr_entref_t id)
{
	float speed;

	if (!stackGetParams("f", &speed))
	{
		stackError("gsc_entity_setmaxentityvelocity() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	gentity_t *ent = gsc_entity_withgravity(id, "gsc_entity_setmaxentityvelocity");
	if (!ent)
		return;

	libcodEntity_t *le = &libcodEntities[ent->s.number];
	if (!le->gravity)
	{
		stackPushBool(qfalse);
		return;
	}

	vec3_t velocity;
	Libcod_GetEntityVelocity(ent, velocity);
	le->maxVelocity = speed < 0 ? 0 : speed;
	if (!le->resting)
		Libcod_SetEntityVelocity(ent, velocity);
	stackPushBool(qtrue);
}

void gsc_entity_getmaxentityvelocity(scr_entref_t id)
{
	gentity_t *ent = gsc_entity_withgravity(id, "gsc_entity_getmaxentityvelocity");
	if (!ent)
		return;

	if (!libcodEntities[ent->s.number].gravity)
		stackPushUndefined();
	else
		stackPushFloat(libcodEntities[ent->s.number].maxVelocity);
}

#endif

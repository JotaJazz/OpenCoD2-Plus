#include "gsc_weapons.hpp"

#if LIBCOD_COMPILE_WEAPONS == 1

qboolean isValidWeaponId(int id)
{
	int weps = BG_GetNumWeapons();

	if (id >= weps || id < 0 || weps == 0)
		return qfalse;

	return qtrue;
}

// The weapon named or numbered by the first argument, or NULL after a
// script error
static WeaponDef *gsc_weapons_getdef(const char *function)
{
	int id;
	const char *name;

	if (stackGetParams("s", &name))
	{
		id = BG_FindWeaponIndexForName(name);
		if (!id)
			id = -1; // unknown name, not weapon "none"
	}
	else if (!stackGetParams("i", &id))
	{
		stackError("%s() argument is undefined or has a wrong type", function);
		stackPushUndefined();
		return NULL;
	}

	if (!isValidWeaponId(id))
	{
		stackError("%s() weapon index is out of bounds", function);
		stackPushUndefined();
		return NULL;
	}

	return BG_GetWeaponDef(id);
}

// The same, for a setter: the weapon first, then the integer value
static WeaponDef *gsc_weapons_setdef(const char *function, int *value)
{
	int id;
	const char *name;

	if (stackGetParams("si", &name, value))
	{
		id = BG_FindWeaponIndexForName(name);
		if (!id)
			id = -1; // unknown name, not weapon "none"
	}
	else if (!stackGetParams("ii", &id, value))
	{
		stackError("%s() one or more arguments is undefined or has a wrong type", function);
		stackPushUndefined();
		return NULL;
	}

	if (!isValidWeaponId(id))
	{
		stackError("%s() weapon index is out of bounds", function);
		stackPushUndefined();
		return NULL;
	}

	return BG_GetWeaponDef(id);
}

void gsc_weapons_getweaponmaxammo()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponmaxammo");
	if (weapon)
		stackPushInt(weapon->maxAmmo);
}

void gsc_weapons_getweaponclipsize()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponclipsize");
	if (weapon)
		stackPushInt(weapon->clipSize);
}

void gsc_weapons_getweapondamage()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweapondamage");
	if (weapon)
		stackPushInt(weapon->damage);
}

void gsc_weapons_getweaponmeleedamage()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponmeleedamage");
	if (weapon)
		stackPushInt(weapon->meleeDamage);
}

void gsc_weapons_getweaponfiretime()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponfiretime");
	if (weapon)
		stackPushInt(weapon->fireTime);
}

void gsc_weapons_getweaponmeleetime()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponmeleetime");
	if (weapon)
		stackPushInt(weapon->meleeTime);
}

void gsc_weapons_getweaponreloadtime()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponreloadtime");
	if (weapon)
		stackPushInt(weapon->reloadTime);
}

void gsc_weapons_getweaponreloademptytime()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponreloademptytime");
	if (weapon)
		stackPushInt(weapon->reloadEmptyTime);
}

void gsc_weapons_getweaponcookable()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponcookable");
	if (weapon)
		stackPushInt(weapon->cookOffHold);
}

void gsc_weapons_setweapondamage()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweapondamage", &value);
	if (!weapon)
		return;

	weapon->damage = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponmaxammo()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponmaxammo", &value);
	if (!weapon)
		return;

	weapon->maxAmmo = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponclipsize()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponclipsize", &value);
	if (!weapon)
		return;

	weapon->clipSize = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponmeleedamage()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponmeleedamage", &value);
	if (!weapon)
		return;

	weapon->meleeDamage = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponfiretime()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponfiretime", &value);
	if (!weapon)
		return;

	weapon->fireTime = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponmeleetime()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponmeleetime", &value);
	if (!weapon)
		return;

	weapon->meleeTime = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponreloadtime()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponreloadtime", &value);
	if (!weapon)
		return;

	weapon->reloadTime = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponreloademptytime()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponreloademptytime", &value);
	if (!weapon)
		return;

	weapon->reloadEmptyTime = value;
	stackPushBool(qtrue);
}

void gsc_weapons_setweaponcookable()
{
	int value;
	WeaponDef *weapon = gsc_weapons_setdef("gsc_weapons_setweaponcookable", &value);
	if (!weapon)
		return;

	weapon->cookOffHold = value;
	stackPushBool(qtrue);
}

const char *hitlocs[] = { "none", "helmet", "head", "neck", "torso_upper", "torso_lower", "right_arm_upper", "right_arm_lower", "right_hand", "left_arm_upper", "left_arm_lower", "left_hand", "right_leg_upper", "right_leg_lower", "right_foot", "left_leg_upper", "left_leg_lower", "left_foot", "gun" };
int getHitLocOffset(const char *hitloc)
{
	int offset = -1;

	for (int i = 0; i < int( sizeof(hitlocs) / sizeof(hitlocs[0]) ); i++)
	{
		if (strcmp(hitlocs[i], hitloc) == 0)
		{
			offset = i;
			break;
		}
	}

	return offset;
}

void gsc_weapons_getweaponhitlocmultiplier()
{
	int id;
	const char *name;
	const char *hitloc;

	if (stackGetParams("ss", &name, &hitloc))
	{
		id = BG_FindWeaponIndexForName(name);
		if (!id)
			id = -1; // unknown name, not weapon "none"
	}
	else if ( ! stackGetParams("is", &id, &hitloc))
	{
		stackError("gsc_weapons_getweaponhitlocmultiplier() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!isValidWeaponId(id))
	{
		stackError("gsc_weapons_getweaponhitlocmultiplier() weapon index is out of bounds");
		stackPushUndefined();
		return;
	}

	WeaponDef *weapon = BG_GetWeaponDef(id);
	int offset = getHitLocOffset(hitloc);
	if (offset < 0)
	{
		stackError("gsc_weapons_getweaponhitlocmultiplier() unknown hit location '%s'", hitloc);
		stackPushUndefined();
		return;
	}

	stackPushFloat(weapon->locationDamageMultipliers[offset]);
}

void gsc_weapons_setweaponhitlocmultiplier()
{
	int id;
	const char *name;
	float multiplier;
	const char* hitloc;

	if (stackGetParams("ssf", &name, &hitloc, &multiplier))
	{
		id = BG_FindWeaponIndexForName(name);
		if (!id)
			id = -1; // unknown name, not weapon "none"
	}
	else if ( ! stackGetParams("isf", &id, &hitloc, &multiplier))
	{
		stackError("gsc_weapons_setweaponhitlocmultiplier() one or more arguments is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (!isValidWeaponId(id))
	{
		stackError("gsc_weapons_setweaponhitlocmultiplier() weapon index is out of bounds");
		stackPushUndefined();
		return;
	}

	WeaponDef *weapon = BG_GetWeaponDef(id);
	int offset = getHitLocOffset(hitloc);
	if (offset < 0)
	{
		stackError("gsc_weapons_setweaponhitlocmultiplier() unknown hit location '%s'", hitloc);
		stackPushUndefined();
		return;
	}

	weapon->locationDamageMultipliers[offset] = multiplier;
	stackPushBool(qtrue);
}

void gsc_weapons_getloadedweapons()
{
	int numweapons = BG_GetNumWeapons();

	stackPushArray();

	for (int i = 0; i < numweapons; i++)
	{
		WeaponDef *weapon = BG_GetWeaponDef(i);
		stackPushString(weapon->szInternalName);
		stackPushArrayLast();
	}
}

// Functions below are named and behave as in zk_libcod

void gsc_weapons_getweaponfusetime()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponfusetime");
	if (weapon)
		stackPushInt(weapon->fuseTime);
}

void gsc_weapons_getweaponraisetime()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponraisetime");
	if (weapon)
		stackPushInt(weapon->raiseTime);
}

void gsc_weapons_getweaponmovespeedscale()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweaponmovespeedscale");
	if (weapon)
		stackPushFloat(weapon->moveSpeedScale);
}

// A localized string, as the weapon files give it
void gsc_weapons_getweapondisplayname()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_getweapondisplayname");
	if (weapon)
		Scr_AddIString(weapon->displayName);
}

void gsc_weapons_issemiautoweapon()
{
	WeaponDef *weapon = gsc_weapons_getdef("gsc_weapons_issemiautoweapon");
	if (weapon)
		stackPushBool(weapon->semiAuto != 0);
}

#define MAX_IGNORED_WEAPONS 64
static char ignoredWeapons[MAX_IGNORED_WEAPONS][64];
static int numIgnoredWeapons;

// Weapons a mod asked not to load; BG_GetWeaponIndexForName gives them
// defaultweapon_mp instead
bool BG_IsWeaponIgnored(const char *name)
{
	for (int i = 0; i < numIgnoredWeapons; i++)
	{
		if (!I_stricmp(ignoredWeapons[i], name))
			return true;
	}
	return false;
}

void gsc_weapons_ignoreweapon()
{
	const char *name;

	if (!stackGetParams("s", &name))
	{
		stackError("gsc_weapons_ignoreweapon() argument is undefined or has a wrong type");
		stackPushUndefined();
		return;
	}

	if (strlen(name) >= sizeof(ignoredWeapons[0]))
	{
		stackError("gsc_weapons_ignoreweapon() weapon name is too long");
		stackPushUndefined();
		return;
	}

	if (numIgnoredWeapons == MAX_IGNORED_WEAPONS)
	{
		stackError("gsc_weapons_ignoreweapon() at most %i weapons can be ignored", MAX_IGNORED_WEAPONS);
		stackPushUndefined();
		return;
	}

	I_strncpyz(ignoredWeapons[numIgnoredWeapons++], name, sizeof(ignoredWeapons[0]));
	stackPushBool(qtrue);
}

void gsc_weapons_resetignoredweapons()
{
	numIgnoredWeapons = 0;
	stackPushBool(qtrue);
}

#endif

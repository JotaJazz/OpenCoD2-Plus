#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
/* dvar globals */
extern const dvar_t *bg_bobAmplitudeDucked;
extern const dvar_t *bg_bobAmplitudeProne;
extern const dvar_t *bg_bobAmplitudeStanding;

extern int bg_iNumWeapons;
extern WeaponDef *bg_weaponDefs[128];
static WeaponDef *bg_weapAmmoTypes[128];
static int bg_iNumAmmoTypes;
static WeaponDef *bg_sharedAmmoCaps[128];
static int bg_iNumSharedAmmoCaps;
static WeaponDef *bg_weapClips[128];
static int bg_iNumWeapClips;
static const char bg_emptyString[] = "";

void BG_ShutdownWeaponDefFiles(void);
WeaponDef *BG_GetWeaponDef(int iWeapon);
extern WeaponDef *BG_LoadDefaultWeaponDef(void);
extern WeaponDef *BG_LoadWeaponDefInternal(const char *folder, const char *name);
extern void BG_LoadPlayerAnimTypes(void);
extern void BG_InitWeaponStrings(void);
extern int I_stricmp(const char *, const char *);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int BG_AnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force);
extern void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
extern void PM_SetProneMovementOverride(playerState_t *ps);
extern qboolean Mantle_IsWeaponInactive(playerState_t *ps);
extern void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
extern const dvar_t *bg_aimSpreadMoveSpeedThreshold;
extern const dvar_t *player_scopeExitOnDamage;
extern const dvar_t *player_adsExitDelay;
extern const dvar_t *player_breath_fire_delay;
extern const dvar_t *player_breath_gasp_lerp;
extern const dvar_t *player_breath_gasp_scale;
extern const dvar_t *player_breath_gasp_time;
extern const dvar_t *player_breath_hold_lerp;
extern const dvar_t *player_breath_hold_time;
extern const dvar_t *player_toggleBinoculars;
extern const float AngleSubtract(const float a1, const float a2);
extern void AnglesSubtract(const vec_t *v1, const vec_t *v2, vec_t *v3);
extern float DiffTrack(float tgt, float cur, float rate, float deltaTime);
extern float DiffTrackAngle(float tgt, float cur, float rate, float deltaTime, float f, float granularity, float epsilon);
extern float GetLeanFraction(const float fFrac);
extern float randomf(void);
extern float sinf(float);
extern double sin(double);
void BG_ClearWeaponDef(void);
int BG_GetNumWeapons(void);
int BG_GetAmmoTypeMax(int iAmmoIndex);
int BG_GetAmmoClipSize(int iClipIndex);
int BG_FindWeaponIndexForName(const char *name);
WeaponDef *BG_LoadWeaponDef(const char *folder, const char *name);
int BG_SetupWeaponDef(WeaponDef *weapDef, void (*regWeap)(int));
qboolean BG_IsAnyEmptyPrimaryWeaponSlot(const playerState_t *ps);
float BG_GetBobCycle(const playerState_t *ps);
float BG_GetVerticalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp);
float BG_GetHorizontalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp);
static void BG_SetupClipIndexes(int weapIndex);
static void BG_SetupSharedAmmoIndexes(int weapIndex);
static void BG_SetupAmmoIndexes(int weapIndex);
static void BG_SetupWeaponItem(int weapIndex, WeaponDef *weapDef);
qboolean BG_IsAimDownSightWeapon(int iWeapon);
Bool BG_DoesWeaponRequireSlot(int weaponIndex);
Bool BG_DoesWeaponNeedSlot(int weapIndex);
int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex);
int BG_GetStackSlotForWeapon(const playerState_t *pPS, int iWeaponIndex, weapSlot_t preferedSlot);
qboolean BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex);
void BG_GetSpreadForWeapon(const playerState_t *ps, int weaponIndex, float *minSpread, float *maxSpread);
int BG_ClipForWeapon(int weapon);
int BG_AmmoForWeapon(int weapon);
qboolean BG_WeaponIsClipOnly(int weapon);
int BG_WeaponAmmo(const playerState_t *ps, int weapon);
int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode);
int BG_GetTotalAmmoReserve(const playerState_t *ps, int weaponIndex);
int BG_GetFirstEquippedOffhand(const playerState_t *ps, int offhandClass);
static qboolean BG_CalculateWeaponPosition_GunRecoil_SingleAngle(vec_t *offset, vec_t *speed,
                                                                 float fTimeStep, float fOfsCap, float fGunKickAccel, float fGunKickSpeedMax,
                                                                 float fGunKickSpeedDecay, float fGunKickStaticDecay);
int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
void BG_WeaponFireRecoil(const playerState_t *ps, vec_t *vGunSpeed, vec_t *kickAVel);
void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime);
int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass);
int BG_GetMaxPickupableAmmo(const playerState_t *ps, int weaponIndex);
Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex);
qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
int BG_GetWeaponIndexForName(const char *name, BG_RegisterWeapon regWeap);
void BG_FillInAmmoItems(BG_RegisterWeapon regWeap);
void CG_SetWeaponDefToDefaultWeapon(int iWeapon);
void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles);
void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles);

void BG_ShutdownWeaponDefFiles(void)
{
    bg_iNumWeapons = 0;
}

WeaponDef *BG_GetWeaponDef(int iWeapon)
{
    return bg_weaponDefs[iWeapon];
}

void BG_ClearWeaponDef(void)
{
    WeaponDef *defaultDef;
    gitem_t *itemList;
    int i;

    defaultDef = (WeaponDef *)BG_LoadDefaultWeaponDef();

    bg_weaponDefs[0] = defaultDef;

    bg_weapAmmoTypes[0] = defaultDef;
    bg_iNumAmmoTypes = 1;

    bg_sharedAmmoCaps[0] = defaultDef;
    bg_iNumSharedAmmoCaps = 1;

    bg_weapClips[0] = defaultDef;
    bg_iNumWeapClips = 1;

    itemList = (gitem_t *)((void *)imp_bg_itemlist);
    for (i = 1; i <= 128; i++) {
        itemList[i].giType = (itemType_t)(0);
    }

    BG_LoadPlayerAnimTypes();
    BG_InitWeaponStrings();
}

int BG_GetNumWeapons(void)
{
    return bg_iNumWeapons;
}

int BG_GetAmmoTypeMax(int iAmmoIndex)
{
    return bg_weapAmmoTypes[iAmmoIndex]->iMaxAmmo;
}

int BG_GetAmmoClipSize(int iClipIndex)
{
    return bg_weapClips[iClipIndex]->iClipSize;
}

int BG_FindWeaponIndexForName(const char *name)
{
    extern int bg_iNumWeapons;
    extern WeaponDef *bg_weaponDefs[128];
    extern int I_stricmp(const char *, const char *);
    int weapIndex;
    for (weapIndex = 1; weapIndex <= bg_iNumWeapons; weapIndex++) {
        const char *defName = *(const char **)bg_weaponDefs[weapIndex];
        if (I_stricmp(name, defName) == 0)
            return weapIndex;
    }
    return 0;
}

qboolean BG_IsAnyEmptyPrimaryWeaponSlot(const playerState_t *ps)
{
    return ps->weaponslots[1] == 0 || ps->weaponslots[2] == 0;
}

extern void PM_AddEvent(playerState_t *ps, int newEvent);

float BG_GetBobCycle(const playerState_t *ps)
{
    float val = (float)(*(byte *)&ps->bobCycle) / 255.0f;
    return (float)((double)val * 3.141592653589793 * 2.0 + 6.283185307179586);
}

static BM_ALWAYS_INLINE float BG_GetBobAmplitude(const playerState_t *ps, float speed, float maxAmp)
{
    const dvar_t *dvar;
    float amplitude;

    if (ps->viewHeightTarget == 11)
        dvar = bg_bobAmplitudeProne;
    else if (ps->viewHeightTarget == 40)
        dvar = bg_bobAmplitudeDucked;
    else
        dvar = bg_bobAmplitudeStanding;

    amplitude = speed * dvar->current.value;
    if (amplitude > maxAmp)
        amplitude = maxAmp;
    return amplitude;
}

float BG_GetVerticalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp)
{
    float amplitude = BG_GetBobAmplitude(ps, speed, maxAmp);
    float bob = sinf(cycle + cycle);

    bob += (float)sin((double)(cycle * 4.0f) + 1.5707963267948966) * 0.2f;
    return amplitude * (bob * 0.75f);
}

float BG_GetHorizontalBobFactor(const playerState_t *ps, float cycle, float speed, float maxAmp)
{
    return BG_GetBobAmplitude(ps, speed, maxAmp) * sinf(cycle);
}

static void BG_SetupClipIndexes(int weapIndex)
{
    WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    int iIndex;

    for (iIndex = 0; iIndex < bg_iNumWeapClips; iIndex++) {
        WeaponDef *clipDef = bg_weapClips[iIndex];

        if (strcmp(clipDef->szClipName, weapDef->szClipName) == 0) {
            weapDef->iClipIndex = iIndex;

            if (clipDef->iClipSize != weapDef->iClipSize && iIndex != 0 && weapIndex > 1) {
                int j;

                for (j = 1; j < weapIndex; j++) {
                    WeaponDef *otherWeapDef = bg_weaponDefs[j];

                    if (I_stricmp(clipDef->szClipName, otherWeapDef->szClipName) == 0 &&
                        clipDef->iClipSize == otherWeapDef->iClipSize) {
                        Com_Error(1, "\x15"
                                     "Clip Size mismatch for \"%s\" clip: '%s\" set it to %i, but \"%s\" already set it to %i.\n",
                                  weapDef->szAmmoName, weapDef->szInternalName,
                                  weapDef->iClipSize, otherWeapDef->szInternalName, otherWeapDef->iClipSize);
                    }
                }
            }

            return;
        }
    }

    bg_weapClips[bg_iNumWeapClips] = weapDef;
    weapDef->iClipIndex = bg_iNumWeapClips;
    bg_iNumWeapClips++;
}

static void BG_SetupSharedAmmoIndexes(int weapIndex)
{
    WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    int iIndex;

    weapDef->iSharedAmmoCapIndex = -1;
    if (!weapDef->szSharedAmmoCapName[0])
        return;

    Com_DPrintf("%s: %s\n", weapDef->szInternalName, weapDef->szSharedAmmoCapName);

    for (iIndex = 0; iIndex < bg_iNumSharedAmmoCaps; iIndex++) {
        WeaponDef *sharedAmmoCap = bg_sharedAmmoCaps[iIndex];

        if (I_stricmp(sharedAmmoCap->szSharedAmmoCapName, weapDef->szSharedAmmoCapName) == 0) {
            weapDef->iSharedAmmoCapIndex = iIndex;

            if (sharedAmmoCap->iSharedAmmoCap != weapDef->iSharedAmmoCap && iIndex != 0 && weapIndex > 1) {
                int j;

                for (j = 1; j < weapIndex; j++) {
                    WeaponDef *otherWeapDef = bg_weaponDefs[j];

                    if (I_stricmp(sharedAmmoCap->szSharedAmmoCapName, otherWeapDef->szSharedAmmoCapName) == 0 &&
                        sharedAmmoCap->iSharedAmmoCap == otherWeapDef->iSharedAmmoCap) {
                        Com_Error(1, "\x15Shared ammo cap mismatch for \"%s\" shared ammo cap: '%s\" set it to %i, but \"%s\" already set it to %i.\n", weapDef->szSharedAmmoCapName, weapDef->szInternalName,
                                  weapDef->iSharedAmmoCap, otherWeapDef->szInternalName, otherWeapDef->iSharedAmmoCap);
                    }
                }
            }

            return;
        }
    }

    bg_sharedAmmoCaps[bg_iNumSharedAmmoCaps] = weapDef;
    weapDef->iSharedAmmoCapIndex = bg_iNumSharedAmmoCaps;
    bg_iNumSharedAmmoCaps++;
}

static void BG_SetupAmmoIndexes(int weapIndex)
{
    WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    int iIndex;

    for (iIndex = 0; iIndex < bg_iNumAmmoTypes; iIndex++) {
        WeaponDef *ammoDef = bg_weapAmmoTypes[iIndex];

        if (strcmp(ammoDef->szAmmoName, weapDef->szAmmoName) == 0) {
            weapDef->iAmmoIndex = iIndex;

            if (ammoDef->iMaxAmmo != weapDef->iMaxAmmo && iIndex != 0 && weapIndex > 1) {
                int j;

                for (j = 1; j < weapIndex; j++) {
                    WeaponDef *otherWeapDef = bg_weaponDefs[j];

                    if (I_stricmp(ammoDef->szAmmoName, otherWeapDef->szAmmoName) == 0 &&
                        ammoDef->iMaxAmmo == otherWeapDef->iMaxAmmo) {
                        Com_Error(1, "\x15Max ammo mismatch for \"%s\" ammo: '%s\" set it to %i, but \"%s\" already set it to %i.\n", weapDef->szAmmoName, weapDef->szInternalName,
                                  weapDef->iMaxAmmo, otherWeapDef->szInternalName, otherWeapDef->iMaxAmmo);
                    }
                }
            }

            return;
        }
    }

    bg_weapAmmoTypes[bg_iNumAmmoTypes] = weapDef;
    weapDef->iAmmoIndex = bg_iNumAmmoTypes;
    bg_iNumAmmoTypes++;
}

static const char *BG_WeaponItemString(const char *string)
{
    return string ? string : bg_emptyString;
}

static void BG_SetupWeaponItem(int weapIndex, WeaponDef *weapDef)
{
    int *numItems = (int *)imp_bg_numItems;
    gitem_t *item = &((gitem_t *)imp_bg_itemlist)[weapIndex];

    item->classname = (char *)BG_WeaponItemString(weapDef->szInternalName);
    item->pickup_sound = (char *)BG_WeaponItemString(weapDef->szPickupSound);
    item->world_model[0] = (char *)BG_WeaponItemString(weapDef->szWorldModel);
    item->world_model[1] = (char *)bg_emptyString;
    item->icon = (char *)BG_WeaponItemString(weapDef->szHudIcon);
    item->pickup_name = (char *)BG_WeaponItemString(weapDef->szDisplayName);
    item->quantity = weapDef->iStartAmmo;
    item->giType = IT_WEAPON;
    item->giTag = weapIndex;
    item->giAmmoIndex = weapDef->iAmmoIndex;
    item->giClipIndex = weapDef->iClipIndex;

    if (*numItems <= weapIndex)
        *numItems = weapIndex + 1;
}

qboolean BG_IsAimDownSightWeapon(int iWeapon)
{
    return bg_weaponDefs[iWeapon]->bADSPositionInfo;
}

Bool BG_DoesWeaponRequireSlot(int weaponIndex)
{
    int val = bg_weaponDefs[weaponIndex]->weapSlot;
    return (unsigned int)(val - 1) <= 1;
}

Bool BG_DoesWeaponNeedSlot(int weapIndex)
{

    const WeaponDef *weapDef = bg_weaponDefs[weapIndex];
    if (weapDef->weapClass == 9)
        return 0;
    if (weapDef->offhandClass != 0)
        return 0;
    return 1;
}

int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex)
{
    const WeaponDef *weapDef = bg_weaponDefs[iWeaponIndex];

    if ((unsigned int)(weapDef->weapSlot - 1) > 1)
        return 0;
    if (!pPS->weaponslots[1])
        return 1;
    if (!pPS->weaponslots[2])
        return 2;
    return 0;
}

int BG_GetStackSlotForWeapon(const playerState_t *pPS, int iWeaponIndex, weapSlot_t preferedSlot)
{
    const WeaponDef *weapDef = bg_weaponDefs[iWeaponIndex];

    if (!weapDef->bSlotStackable || (unsigned int)(weapDef->weapSlot - 1) > 1)
        return 0;

    if ((unsigned int)(preferedSlot - 1) <= 1) {
        int weapon = pPS->weaponslots[preferedSlot];

        if (!weapon || bg_weaponDefs[weapon]->bSlotStackable)
            return preferedSlot;
    }

    if (!pPS->weaponslots[1] || bg_weaponDefs[(int)pPS->weaponslots[1]]->bSlotStackable)
        return 1;
    if (!pPS->weaponslots[2] || bg_weaponDefs[(int)pPS->weaponslots[2]]->bSlotStackable)
        return 2;
    return 0;
}

qboolean BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex)
{
    int altIndex = bg_weaponDefs[iWeaponIndex]->iAltWeaponIndex;

    while (altIndex) {
        if (altIndex == iAltIndex)
            return 1;
        if (altIndex == iWeaponIndex)
            break;
        altIndex = bg_weaponDefs[altIndex]->iAltWeaponIndex;
    }

    return 0;
}

void BG_GetSpreadForWeapon(const playerState_t *ps, int weaponIndex, float *minSpread, float *maxSpread)
{
    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];
    float frac;

    if (ps->viewHeightCurrent > 40.0f) {
        frac = (ps->viewHeightCurrent - 40.0f) / 20.0f;
        *minSpread = weapDef->fHipSpreadDuckedMin + (weapDef->fHipSpreadStandMin - weapDef->fHipSpreadDuckedMin) * frac;
        *maxSpread = weapDef->hipSpreadDuckedMax + (weapDef->hipSpreadStandMax - weapDef->hipSpreadDuckedMax) * frac;
    } else {
        frac = (ps->viewHeightCurrent - 11.0f) / 29.0f;
        *minSpread = weapDef->fHipSpreadProneMin + (weapDef->fHipSpreadDuckedMin - weapDef->fHipSpreadProneMin) * frac;
        *maxSpread = weapDef->hipSpreadProneMax + (weapDef->hipSpreadDuckedMax - weapDef->hipSpreadProneMax) * frac;
    }
}

int BG_ClipForWeapon(int weapon)
{
    return bg_weaponDefs[weapon]->iClipIndex;
}

int BG_AmmoForWeapon(int weapon)
{
    return bg_weaponDefs[weapon]->iAmmoIndex;
}

qboolean BG_WeaponIsClipOnly(int weapon)
{
    return bg_weaponDefs[weapon]->bClipOnly;
}

int BG_WeaponAmmo(const playerState_t *ps, int weapon)
{
    const WeaponDef *weapDef = bg_weaponDefs[weapon];
    int ammoIndex = weapDef->iAmmoIndex;
    int clipIndex = weapDef->iClipIndex;

    return ps->ammo[ammoIndex] + ps->ammoclip[clipIndex];
}

static BM_ALWAYS_INLINE int BG_IsPlayerWeaponInSlot_core(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode)
{
    int curWeaponIndex;

    if (!(pPS->weapons[iWeaponIndex >> 5] & (1 << (iWeaponIndex & 0x1f))))
        return 0;

    curWeaponIndex = iWeaponIndex;
    do {
        const WeaponDef *weapDef = bg_weaponDefs[curWeaponIndex];

        if ((unsigned int)(weapDef->weapSlot - 1) > 1)
            return 0;
        if ((int)pPS->weaponslots[1] == curWeaponIndex)
            return 1;
        if ((int)pPS->weaponslots[2] == curWeaponIndex)
            return 2;

        if (!bAnyMode)
            break;
        if (weapDef->iAltWeaponIndex)
            curWeaponIndex = weapDef->iAltWeaponIndex;
    } while (curWeaponIndex != iWeaponIndex);

    return 0;
}

int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode)
{
    return BG_IsPlayerWeaponInSlot_core(pPS, iWeaponIndex, bAnyMode);
}

int BG_GetTotalAmmoReserve(const playerState_t *ps, int weaponIndex)
{
    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];
    int ammoIndex = weapDef->iAmmoIndex;
    int clipIndex = weapDef->iClipIndex;
    int ammoCounted[128];
    int clipCounted[128];
    int ammo;
    int i;

    memset(ammoCounted, 0, sizeof(ammoCounted));
    memset(clipCounted, 0, sizeof(clipCounted));

    if (weapDef->iSharedAmmoCapIndex < 0) {
        if (weapDef->bClipOnly)
            return ps->ammoclip[clipIndex];
        return ps->ammo[ammoIndex];
    }

    ammo = 0;
    for (i = 1; i <= bg_iNumWeapons; i++) {
        const WeaponDef *otherWeapDef;

        if (!(ps->weapons[i >> 5] & (1 << (i & 0x1f))))
            continue;

        otherWeapDef = bg_weaponDefs[i];
        if (otherWeapDef->iSharedAmmoCapIndex != weapDef->iSharedAmmoCapIndex)
            continue;

        if (otherWeapDef->bClipOnly) {
            clipIndex = otherWeapDef->iClipIndex;
            if (clipCounted[clipIndex])
                continue;

            clipCounted[clipIndex] = 1;
            ammo += ps->ammoclip[clipIndex];
        } else {
            ammoIndex = otherWeapDef->iAmmoIndex;
            if (ammoCounted[ammoIndex])
                continue;

            ammoCounted[ammoIndex] = 1;
            ammo += ps->ammo[ammoIndex];
        }
    }

    return ammo;
}

int BG_GetFirstEquippedOffhand(const playerState_t *ps, int offhandClass)
{
    int weaponIndex;

    for (weaponIndex = 1; weaponIndex <= bg_iNumWeapons; weaponIndex++) {
        if (bg_weaponDefs[weaponIndex]->offhandClass != offhandClass)
            continue;
        if (ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f)))
            return weaponIndex;
    }

    return 0;
}

static qboolean BG_CalculateWeaponPosition_GunRecoil_SingleAngle(vec_t *offset, vec_t *speed,
                                                                 float fTimeStep, float fOfsCap, float fGunKickAccel, float fGunKickSpeedMax,
                                                                 float fGunKickSpeedDecay, float fGunKickStaticDecay)
{
    float newSpeed;

    if (__builtin_fabsf(*offset) < 0.25f && __builtin_fabsf(*speed) < 1.0f) {
        *offset = 0.0f;
        *speed = 0.0f;
        return 1;
    }

    *offset += *speed * fTimeStep;
    if (*offset > fOfsCap) {
        *offset = fOfsCap;
        if (*speed > 0.0f)
            *speed = 0.0f;
    } else if (*offset < -fOfsCap) {
        *offset = -fOfsCap;
        if (*speed < 0.0f)
            *speed = 0.0f;
    }

    if (*offset > 0.0f)
        *speed -= fGunKickAccel * fTimeStep;
    else if (*offset < 0.0f)
        *speed += fGunKickAccel * fTimeStep;

    newSpeed = *speed - *speed * fGunKickSpeedDecay * fTimeStep;
    *speed = newSpeed;

    if (newSpeed > 0.0f) {
        newSpeed -= fGunKickStaticDecay * fTimeStep;
        if (newSpeed < 0.0f)
            newSpeed = 0.0f;
    } else if (newSpeed < 0.0f) {
        newSpeed += fGunKickStaticDecay * fTimeStep;
        if (newSpeed > 0.0f)
            newSpeed = 0.0f;
    }

    if (newSpeed > fGunKickSpeedMax)
        newSpeed = fGunKickSpeedMax;
    if (newSpeed < -fGunKickSpeedMax)
        newSpeed = -fGunKickSpeedMax;

    *speed = newSpeed;
    return 0;
}

static BM_ALWAYS_INLINE int BG_GetViewmodelWeaponIndex_core(const playerState_t *ps)
{
    int weaponIndex;

    if (ps->pm_flags & 0x20000) {
        for (weaponIndex = 1; weaponIndex <= bg_iNumWeapons; weaponIndex++) {
            if (bg_weaponDefs[weaponIndex]->weapType == 3)
                return (unsigned char)weaponIndex;
        }

        return 0;
    }

    if (ps->pm_flags & 0x10)
        return ps->offHandIndex;

    return ps->weapon;
}

int BG_GetViewmodelWeaponIndex(const playerState_t *ps)
{
    return BG_GetViewmodelWeaponIndex_core(ps);
}

static BM_ALWAYS_INLINE float BG_RandomBetween(float min, float max)
{
    return min + (max - min) * randomf();
}

void BG_WeaponFireRecoil(const playerState_t *ps, vec_t *vGunSpeed, vec_t *kickAVel)
{
    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex_core(ps)];
    float fPosLerp = ps->fWeaponPosFrac;
    float fReducePercent = 1.0f;
    float pitch;
    float yaw;

    if (ps->weaponRestrictKickTime > 0) {
        if (fPosLerp == 1.0f)
            fReducePercent = 0.009999999776482582f * weapDef->adsGunKickReducedKickPercent;
        else
            fReducePercent = 0.009999999776482582f * weapDef->hipGunKickReducedKickPercent;
    }

    if (fPosLerp == 1.0f) {
        pitch = BG_RandomBetween(weapDef->fAdsViewKickPitchMin, weapDef->fAdsViewKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fAdsViewKickYawMin, weapDef->fAdsViewKickYawMax);
    } else {
        pitch = BG_RandomBetween(weapDef->fHipViewKickPitchMin, weapDef->fHipViewKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fHipViewKickYawMin, weapDef->fHipViewKickYawMax);
    }

    yaw *= fReducePercent;
    pitch *= fReducePercent;

    kickAVel[0] = -pitch;
    kickAVel[1] = yaw;
    kickAVel[2] = yaw * -0.5f;

    if (fPosLerp > 0.0f) {
        pitch = BG_RandomBetween(weapDef->fAdsGunKickPitchMin, weapDef->fAdsGunKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fAdsGunKickYawMin, weapDef->fAdsGunKickYawMax);
    } else {
        pitch = BG_RandomBetween(weapDef->fHipGunKickPitchMin, weapDef->fHipGunKickPitchMax);
        yaw = BG_RandomBetween(weapDef->fHipGunKickYawMin, weapDef->fHipGunKickYawMax);
    }

    vGunSpeed[0] += pitch * fReducePercent;
    vGunSpeed[1] += yaw * fReducePercent;
}

void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime)
{
    WeaponDef *weapDef;
    float f;
    float dt;
    float swayMaxAngle;
    float swayLerpSpeed;
    float swayPitchScale;
    float swayYawScale;
    float swayHorizScale;
    float swayVertScale;
    vec3_t deltaAngles;
    float scale;

    if (!frametime)
        return;

    weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];
    f = ps->fWeaponPosFrac;
    dt = (float)frametime * 0.0010000000474974513f;

    if (weapDef->bADSPositionInfo) {
        if (f > 0.0f && weapDef->overlayReticle)
            return;

        swayMaxAngle = weapDef->swayMaxAngle + (weapDef->adsSwayMaxAngle - weapDef->swayMaxAngle) * f;
        swayLerpSpeed = weapDef->swayLerpSpeed + (weapDef->adsSwayLerpSpeed - weapDef->swayLerpSpeed) * f;
        swayPitchScale = weapDef->swayPitchScale + (weapDef->adsSwayPitchScale - weapDef->swayPitchScale) * f;
        swayYawScale = weapDef->swayYawScale + (weapDef->adsSwayYawScale - weapDef->swayYawScale) * f;
        swayHorizScale = weapDef->swayHorizScale + (weapDef->adsSwayHorizScale - weapDef->swayHorizScale) * f;
        swayVertScale = weapDef->swayVertScale + (weapDef->adsSwayVertScale - weapDef->swayVertScale) * f;
    } else {
        swayMaxAngle = weapDef->swayMaxAngle;
        swayLerpSpeed = weapDef->swayLerpSpeed;
        swayPitchScale = weapDef->swayPitchScale;
        swayYawScale = weapDef->swayYawScale;
        swayHorizScale = weapDef->swayHorizScale;
        swayVertScale = weapDef->swayVertScale;
    }

    swayPitchScale *= ssSwayScale;
    swayYawScale *= ssSwayScale;
    swayHorizScale *= ssSwayScale;
    swayVertScale *= ssSwayScale;

    AnglesSubtract(ps->viewangles, swayViewAngles, deltaAngles);
    scale = 1.0f / (dt * 60.0f);
    deltaAngles[0] *= scale;
    deltaAngles[1] *= scale;

    if (deltaAngles[0] > swayMaxAngle)
        deltaAngles[0] = swayMaxAngle;
    if (deltaAngles[0] < -swayMaxAngle)
        deltaAngles[0] = -swayMaxAngle;
    if (deltaAngles[1] > swayMaxAngle)
        deltaAngles[1] = swayMaxAngle;
    if (deltaAngles[1] < -swayMaxAngle)
        deltaAngles[1] = -swayMaxAngle;

    swayOffset[1] = DiffTrack(deltaAngles[1] * swayVertScale, swayOffset[1], swayLerpSpeed, dt);
    swayOffset[2] = DiffTrack(deltaAngles[0] * swayHorizScale, swayOffset[2], swayLerpSpeed, dt);
    swayAngles[0] = DiffTrackAngle(deltaAngles[0] * swayPitchScale, swayAngles[0], swayLerpSpeed, dt, 0.0f, 0.0f, 0.0f);
    swayAngles[1] = DiffTrackAngle(deltaAngles[1] * swayYawScale, swayAngles[1], swayLerpSpeed, dt, 0.0f, 0.0f, 0.0f);

    swayViewAngles[0] = ps->viewangles[0];
    swayViewAngles[1] = ps->viewangles[1];
    swayViewAngles[2] = ps->viewangles[2];
}

int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass)
{
    int weaponIndex;

    for (weaponIndex = 1; weaponIndex <= bg_iNumWeapons; weaponIndex++) {
        const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];

        if (weapDef->offhandClass != offhandClass)
            continue;
        if (!(ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f))))
            continue;
        if (ps->ammo[weapDef->iAmmoIndex] + ps->ammoclip[weapDef->iClipIndex])
            return weaponIndex;
    }

    return 0;
}

int BG_GetMaxPickupableAmmo(const playerState_t *ps, int weaponIndex)
{
    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];
    int ammoCounted[128];
    int clipCounted[128];
    int ammoIndex = weapDef->iAmmoIndex;
    int clipIndex = weapDef->iClipIndex;
    int ammo;
    int i;

    memset(ammoCounted, 0, sizeof(ammoCounted));
    memset(clipCounted, 0, sizeof(clipCounted));

    if (weapDef->iSharedAmmoCapIndex < 0) {
        if (weapDef->bClipOnly)
            return bg_weapClips[clipIndex]->iClipSize - ps->ammoclip[clipIndex];
        return bg_weapAmmoTypes[ammoIndex]->iMaxAmmo - ps->ammo[ammoIndex];
    }

    ammo = bg_sharedAmmoCaps[weapDef->iSharedAmmoCapIndex]->iSharedAmmoCap;
    for (i = 1; i <= bg_iNumWeapons; i++) {
        const WeaponDef *otherWeapDef;

        if (!(ps->weapons[i >> 5] & (1 << (i & 0x1f))))
            continue;

        otherWeapDef = bg_weaponDefs[i];
        if (otherWeapDef->iSharedAmmoCapIndex != weapDef->iSharedAmmoCapIndex)
            continue;

        if (otherWeapDef->bClipOnly) {
            clipIndex = otherWeapDef->iClipIndex;
            if (clipCounted[clipIndex])
                continue;

            clipCounted[clipIndex] = 1;
            ammo -= ps->ammoclip[clipIndex];
        } else {
            ammoIndex = otherWeapDef->iAmmoIndex;
            if (ammoCounted[ammoIndex])
                continue;

            ammoCounted[ammoIndex] = 1;
            ammo -= ps->ammo[ammoIndex];
        }
    }

    return ammo;
}

Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex)
{
    const WeaponDef *weapDef;
    Bool owned;

    if (weaponIndex < 0 || weaponIndex > bg_iNumWeapons)
        return 0;

    owned = (ps->weapons[weaponIndex >> 5] & (1 << (weaponIndex & 0x1f))) != 0;
    if (!owned)
        return 0;

    weapDef = bg_weaponDefs[weaponIndex];
    if (weapDef->offhandClass)
        return owned;
    if ((int)ps->weaponslots[1] == weaponIndex)
        return owned;
    if ((int)ps->weaponslots[2] == weaponIndex)
        return owned;
    if (weapDef->iAltWeaponIndex == weaponIndex)
        return owned;

    return 0;
}

qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex)
{
    WeaponDef *weapDef;
    int slot;
    int altWeaponIndex;

    if (!(pPS->weapons[iWeaponIndex >> 5] & (1 << (iWeaponIndex & 0x1f))))
        return 0;

    weapDef = bg_weaponDefs[iWeaponIndex];
    slot = BG_IsPlayerWeaponInSlot_core(pPS, iWeaponIndex, 1);
    if (slot) {
        if (weapDef->bSlotStackable) {
            int replacement;

            pPS->weaponslots[slot] = 0;
            for (replacement = 1; replacement <= bg_iNumWeapons; replacement++) {
                if ((pPS->weapons[replacement >> 5] & (1 << (replacement & 0x1f))) &&
                    !BG_IsPlayerWeaponInSlot_core(pPS, replacement, 1)) {
                    pPS->weaponslots[slot] = (char)replacement;
                    break;
                }
            }
        } else {
            pPS->weaponslots[slot] = 0;
        }
    }

    pPS->weapons[iWeaponIndex >> 5] &= ~(1 << (iWeaponIndex & 0x1f));

    altWeaponIndex = weapDef->iAltWeaponIndex;
    while (altWeaponIndex &&
           (pPS->weapons[altWeaponIndex >> 5] & (1 << (altWeaponIndex & 0x1f)))) {
        pPS->weapons[altWeaponIndex >> 5] &= ~(1 << (altWeaponIndex & 0x1f));
        altWeaponIndex = bg_weaponDefs[altWeaponIndex]->iAltWeaponIndex;
    }

    if (pPS->weapon == iWeaponIndex)
        pPS->weapon = 0;

    return 1;
}




WeaponDef *BG_LoadWeaponDef(const char *folder, const char *name)
{
    WeaponDef *weapDef = BG_LoadWeaponDefInternal(folder, name);

    if (weapDef)
        return weapDef;

    if (I_stricmp(name, "defaultweapon_mp") == 0)
        Com_Error(1, "\x15"
                     "BG_LoadWeaponDef: Could not find default weapon");

    weapDef = BG_LoadWeaponDefInternal(folder, "defaultweapon_mp");
    if (!weapDef)
        Com_Error(1, "\x15"
                     "BG_LoadWeaponDef: Could not find default weapon");

    return weapDef;
}

int BG_SetupWeaponDef(WeaponDef *weapDef, void (*regWeap)(int))
{
    int weapIndex;

    if (!weapDef)
        return 0;

    weapIndex = BG_FindWeaponIndexForName(weapDef->szInternalName);
    if (weapIndex)
        return weapIndex;

    if (bg_iNumWeapons >= 127)
        Com_Error(1, "\x15"
                     "Exceeded maximum weapon count");

    weapIndex = ++bg_iNumWeapons;
    bg_weaponDefs[weapIndex] = weapDef;

    BG_SetupAmmoIndexes(weapIndex);
    BG_SetupSharedAmmoIndexes(weapIndex);
    BG_SetupClipIndexes(weapIndex);
    BG_SetupWeaponItem(weapIndex, weapDef);

    if (weapDef->offhandClass != OFFHAND_CLASS_NONE && weapDef->weapType != WEAPTYPE_GRENADE) {
        Com_Printf("[%s] Only grenades are currently supported for off hand use\n", weapDef->szInternalName);
    } else if (weapDef->weapType == WEAPTYPE_GRENADE && weapDef->offhandClass == OFFHAND_CLASS_NONE) {
        Com_Printf("[%s] No offhand class set\n", weapDef->szInternalName);
    }

    if (weapDef->szAltWeaponName && weapDef->szAltWeaponName[0]) {
        int altWeaponIndex = BG_GetWeaponIndexForName(weapDef->szAltWeaponName, (BG_RegisterWeapon)regWeap);
        WeaponDef *altWeaponDef;

        if (!altWeaponIndex)
            Com_Error(1, "\x15"
                         "could not find altWeapon '%s' for weapon '%s'",
                      weapDef->szAltWeaponName, weapDef->szInternalName);

        weapDef->iAltWeaponIndex = altWeaponIndex;
        altWeaponDef = bg_weaponDefs[altWeaponIndex];

        if (weapDef->weapSlot != altWeaponDef->weapSlot) {
            Com_Error(1, "\x15"
                         "weapon '%s' does not have same weaponSlot setting as its alt weapon '%s'",
                      weapDef->szInternalName, altWeaponDef->szInternalName);
        }

        if (weapDef->bSlotStackable != altWeaponDef->bSlotStackable) {
            Com_Error(1, "\x15"
                         "weapon '%s' does not have same slotStackable setting as its alt weapon '%s'",
                      weapDef->szInternalName, altWeaponDef->szInternalName);
        }
    } else {
        weapDef->iAltWeaponIndex = 0;
    }

    if (regWeap)
        regWeap(weapIndex);

    return weapIndex;
}

int BG_GetWeaponIndexForName(const char *name, BG_RegisterWeapon regWeap)
{
    WeaponDef *weapDef;
    int weapIndex;

    if (!name[0] || I_stricmp(name, "none") == 0)
        return 0;

    weapIndex = BG_FindWeaponIndexForName(name);
    if (weapIndex)
        return weapIndex;

    weapDef = BG_LoadWeaponDef("mp", name);
    if (weapDef)
        return BG_SetupWeaponDef(weapDef, (void (*)(int))regWeap);

    Com_Printf("Couldn't find weapon \"%s\"\n", name);
    return 0;
}

void BG_FillInAmmoItems(BG_RegisterWeapon regWeap)
{
    int *numItems = (int *)imp_bg_numItems;
    gitem_t *itemList = (gitem_t *)imp_bg_itemlist;
    int i;

    for (i = 0x81; i < *numItems; i++) {
        gitem_t *item = &itemList[i];
        char name[64];
        char *space;
        int weaponIndex;
        WeaponDef *weapDef;

        if (item->giType != IT_AMMO)
            continue;

        I_strncpyz(name, item->pickup_name, sizeof(name));
        for (space = name; *space; space++) {
            if (*space == ' ') {
                *space = '\0';
                break;
            }
        }

        weaponIndex = BG_GetWeaponIndexForName(name, regWeap);
        if (weaponIndex) {
            weapDef = bg_weaponDefs[weaponIndex];
            item->giTag = weaponIndex;
            item->giAmmoIndex = weapDef->iAmmoIndex;
            item->giClipIndex = weapDef->iClipIndex;
            return;
        }

        Com_Printf("^3WARNING^7: Could not find weapon for ammo item %s\n", item->pickup_name);
        weaponIndex = BG_GetWeaponIndexForName("defaultweapon_mp", regWeap);
        if (!weaponIndex) {
            Com_Error(1, "\x15"
                         "BG_FillInAmmoItems: weapon def for default weapon could not be found!");
        }

        weapDef = bg_weaponDefs[weaponIndex];
        item->giTag = 1;
        item->giAmmoIndex = weapDef->iAmmoIndex;
        item->giClipIndex = weapDef->iClipIndex;
    }
}

void CG_SetWeaponDefToDefaultWeapon(int iWeapon)
{
    WeaponDef *weapDef = bg_weaponDefs[iWeapon];
    int defaultWeaponNum = BG_GetWeaponIndexForName("defaultweapon_mp", 0);
    WeaponDef *defaultWeaponDef;
    int i;

    if (!defaultWeaponNum) {
        Com_Error(1, "\x15"
                     "CG_SetWeaponDefToDefaultWeapon: weapon def for default weapon could not be found!");
    }

    defaultWeaponDef = bg_weaponDefs[defaultWeaponNum];
    weapDef->szGunXModel = defaultWeaponDef->szGunXModel;
    weapDef->szHandXModel = defaultWeaponDef->szHandXModel;

    for (i = 0; i < 23; i++)
        weapDef->szXAnims[i] = defaultWeaponDef->szXAnims[i];
}

static void BG_TrackWeaponAngle(vec_t *value, float target, float rate, float frametime)
{
    float cur = *value;
    float delta = (target - cur) * frametime * rate;
    float minStep = frametime * 0.10000000149011612f;

    if (target > cur) {
        if (delta < minStep)
            delta = minStep;
        cur += delta;
        if (cur > target)
            cur = target;
    } else if (target < cur) {
        if (delta > -minStep)
            delta = -minStep;
        cur += delta;
        if (cur < target)
            cur = target;
    }

    *value = cur;
}

static void BG_AddWeaponMovementAngles(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    const vec_t *move;
    float minSpeed;
    float target[3];
    float frac;
    float rate;
    int i;

    if (ps->eFlags & 8) {
        move = weapDef->vProneMove;
        minSpeed = weapDef->fProneRotMinSpeed;
    } else if (ps->eFlags & 4) {
        move = weapDef->vDuckedMove;
        minSpeed = weapDef->fDuckedRotMinSpeed;
    } else {
        move = weapDef->vStandMove;
        minSpeed = weapDef->fStandRotMinSpeed;
    }

    if (ws->xyspeed <= minSpeed || ps->weaponstate == 5) {
        target[0] = 0.0f;
        target[1] = 0.0f;
        target[2] = 0.0f;
    } else {
        float topSpeed = (float)ps->speed - minSpeed;

        if (topSpeed == 0.0f)
            frac = 0.0f;
        else
            frac = (ws->xyspeed - minSpeed) / topSpeed;

        if (frac < 0.0f)
            frac = 0.0f;
        if (frac > 1.0f)
            frac = 1.0f;

        target[0] = move[0] * frac;
        target[1] = move[1] * frac;
        target[2] = move[2] * frac;
    }

    rate = (ps->viewHeightCurrent == 11.0f) ? weapDef->fPosProneRotRate : weapDef->fPosRotRate;
    for (i = 0; i < 3; i++)
        BG_TrackWeaponAngle(&ws->vLastMoveAng[i], target[i], rate, ws->frametime);

    if (ps->fWeaponPosFrac < 0.5f) {
        float scale = 1.0f;

        if (ps->fWeaponPosFrac != 0.0f)
            scale = 1.0f - ps->fWeaponPosFrac * 2.0f;

        angles[0] += ws->vLastMoveAng[0] * scale;
        angles[1] += ws->vLastMoveAng[1] * scale;
        angles[2] += ws->vLastMoveAng[2] * scale;
    }
}

static void BG_TrackWeaponIdleFactor(weaponState_t *ws, float target)
{
    float step = ws->frametime * 0.5f;

    if (ws->fLastIdleFactor < target) {
        ws->fLastIdleFactor += step;
        if (ws->fLastIdleFactor > target)
            ws->fLastIdleFactor = target;
    } else if (ws->fLastIdleFactor > target) {
        ws->fLastIdleFactor -= step;
        if (ws->fLastIdleFactor < target)
            ws->fLastIdleFactor = target;
    }
}

static void BG_AddWeaponIdleAngles(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float idleAmount;
    float idleSpeed;
    float idleFactor;
    float amount;

    if (weapDef->bADSPositionInfo) {
        idleAmount = weapDef->fHipIdleAmount + (weapDef->fAdsIdleAmount - weapDef->fHipIdleAmount) * f;
        idleSpeed = weapDef->hipIdleSpeed + (weapDef->adsIdleSpeed - weapDef->hipIdleSpeed) * f;
    } else {
        idleAmount = weapDef->fHipIdleAmount;
        if (idleAmount == 0.0f) {
            idleAmount = 80.0f;
            idleSpeed = 1.0f;
        } else {
            idleSpeed = weapDef->hipIdleSpeed;
        }
    }

    if (ps->eFlags & 8)
        idleFactor = weapDef->fIdleProneFactor;
    else if (ps->eFlags & 4)
        idleFactor = weapDef->fIdleCrouchFactor;
    else
        idleFactor = 1.0f;

    BG_TrackWeaponIdleFactor(ws, idleFactor);

    amount = idleAmount * ws->fLastIdleFactor;
    if (weapDef->overlayReticle)
        amount *= 1.0f - f;

    *ws->weapIdleTime += (int)(idleSpeed * ws->frametime * 1000.0f);

    angles[2] += sinf((float)*ws->weapIdleTime * 0.0005000000237487257f) * amount * 0.009999999776482582f;
    angles[1] += sinf((float)*ws->weapIdleTime * 0.000699999975040555f) * amount * 0.009999999776482582f;
    angles[0] += sinf((float)*ws->weapIdleTime * 0.0010000000474974513f) * amount * 0.009999999776482582f;
}

static void BG_AddWeaponBobAngles(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float cycle = BG_GetBobCycle(ps) + 0.7853981633974483f;
    float speed = ws->xyspeed * 0.1599999964237213f;
    float pitch = -BG_GetVerticalBobFactor(ps, cycle, speed, 10.0f);
    float yaw = -BG_GetHorizontalBobFactor(ps, cycle, speed, 10.0f);
    float roll = BG_GetHorizontalBobFactor(ps, cycle - 0.4712389167638204f, speed * 1.5f, 10.0f);

    if (roll > 0.0f)
        roll = 0.0f;

    if (f != 0.0f) {
        float factor = 1.0f - (1.0f - weapDef->fAdsBobFactor) * f;

        pitch *= factor;
        yaw *= factor;
        roll *= factor;
    }

    if (weapDef->overlayReticle) {
        float scale = 1.0f - f;

        pitch *= scale;
        yaw *= scale;
        roll *= scale;
    }

    angles[0] += pitch;
    angles[1] += yaw;
    angles[2] += roll;
}

static void BG_AddWeaponDamageKick(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float factor = f * 0.5f + 0.5f;
    float attack;
    float decay;
    float frac;
    int elapsed;

    if (!ws->damageTime)
        return;

    if (f != 0.0f && weapDef->overlayReticle)
        factor *= 1.0f - f * 0.75f;

    attack = 100.0f * factor;
    decay = 400.0f * factor;
    elapsed = ws->time - ws->damageTime;

    if ((float)elapsed < attack) {
        frac = GetLeanFraction((float)elapsed / attack) * factor;
    } else {
        float fade = 1.0f - ((float)elapsed - attack) / decay;

        if (fade <= 0.0f)
            return;

        frac = (1.0f - GetLeanFraction(1.0f - fade)) * factor;
    }

    angles[0] += frac * ws->v_dmg_pitch * 0.5f;
    angles[1] -= frac * ws->v_dmg_roll;
    angles[2] += frac * ws->v_dmg_roll * 0.5f;
}

static void BG_AddWeaponGunKick(weaponState_t *ws, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = ws->ps;
    float f = ps->fWeaponPosFrac;
    float fGunKickAccel;
    float fGunKickSpeedMax;
    float fGunKickSpeedDecay;
    float fGunKickStaticDecay;
    float timeLeft;

    if (!weapDef->bADSPositionInfo)
        goto add_offset;

    fGunKickAccel = weapDef->fHipGunKickAccel + (weapDef->fAdsGunKickAccel - weapDef->fHipGunKickAccel) * f;
    fGunKickSpeedMax = weapDef->fHipGunKickSpeedMax + (weapDef->fAdsGunKickSpeedMax - weapDef->fHipGunKickSpeedMax) * f;
    fGunKickSpeedDecay = weapDef->fHipGunKickSpeedDecay + (weapDef->fAdsGunKickSpeedDecay - weapDef->fHipGunKickSpeedDecay) * f;
    fGunKickStaticDecay = weapDef->fHipGunKickStaticDecay + (weapDef->fAdsGunKickStaticDecay - weapDef->fHipGunKickStaticDecay) * f;

    timeLeft = ws->frametime;
    while (timeLeft > 0.0f) {
        float step = timeLeft;
        qboolean pitchDone;
        qboolean yawDone;

        if (step > 0.004999999888241291f)
            step = 0.004999999888241291f;

        pitchDone = BG_CalculateWeaponPosition_GunRecoil_SingleAngle(&ws->vGunOffset[0], &ws->vGunSpeed[0],
                                                                     step, weapDef->fGunMaxPitch, fGunKickAccel, fGunKickSpeedMax, fGunKickSpeedDecay, fGunKickStaticDecay);
        yawDone = BG_CalculateWeaponPosition_GunRecoil_SingleAngle(&ws->vGunOffset[1], &ws->vGunSpeed[1],
                                                                   step, weapDef->fGunMaxYaw, fGunKickAccel, fGunKickSpeedMax, fGunKickSpeedDecay, fGunKickStaticDecay);

        if (pitchDone && yawDone)
            break;

        timeLeft -= step;
    }

add_offset:
    angles[0] += ws->vGunOffset[0];
    angles[1] += ws->vGunOffset[1];
    angles[2] += ws->vGunOffset[2];
}

void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles)
{
    playerState_t *ps = ws->ps;
    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];

    angles[0] = 0.0f;
    angles[1] = 0.0f;
    angles[2] = 0.0f;

    if (ps->leanf != 0.0f)
        angles[2] += GetLeanFraction(ps->leanf) * -2.0f;

    if (weapDef->bADSPositionInfo)
        angles[0] += ps->fWeaponPosFrac * weapDef->fAdsAimPitch;

    BG_AddWeaponMovementAngles(ws, angles, weapDef);
    BG_AddWeaponIdleAngles(ws, angles, weapDef);
    BG_AddWeaponBobAngles(ws, angles, weapDef);
    BG_AddWeaponDamageKick(ws, angles, weapDef);
    BG_AddWeaponGunKick(ws, angles, weapDef);

    angles[0] = AngleSubtract(angles[0], ws->swayAngles[0]);
    angles[1] = AngleSubtract(angles[1], ws->swayAngles[1]);
}

static void BG_TrackViewIdleFactor(viewState_t *vs, float target)
{
    float step = vs->frametime * 0.5f;

    if (vs->fLastIdleFactor < target) {
        vs->fLastIdleFactor += step;
        if (vs->fLastIdleFactor > target)
            vs->fLastIdleFactor = target;
    } else if (vs->fLastIdleFactor > target) {
        vs->fLastIdleFactor -= step;
        if (vs->fLastIdleFactor < target)
            vs->fLastIdleFactor = target;
    }
}

static void BG_AddViewDamageKick(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float factor;
    float frac;
    int elapsed;

    if (!vs->damageTime)
        return;

    factor = 1.0f - f * 0.5f;
    if (f != 0.0f && weapDef->overlayReticle)
        factor *= f * 0.5f + 1.0f;

    elapsed = vs->time - vs->damageTime;
    if ((float)elapsed < 100.0f) {
        frac = GetLeanFraction((float)elapsed / 100.0f);
    } else {
        float fade = 1.0f - ((float)elapsed - 100.0f) / 400.0f;

        if (fade <= 0.0f)
            return;

        frac = 1.0f - GetLeanFraction(1.0f - fade);
    }

    frac *= factor;
    angles[0] += frac * vs->v_dmg_pitch;
    angles[2] += frac * vs->v_dmg_roll;
}

static void BG_AddViewIdle(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float idleAmount;
    float idleSpeed;
    float idleFactor;
    float amount;

    if (!weapDef->overlayReticle)
        return;

    if (weapDef->bADSPositionInfo) {
        idleAmount = weapDef->fHipIdleAmount +
                     (weapDef->fAdsIdleAmount - weapDef->fHipIdleAmount) * f;
        idleSpeed = weapDef->hipIdleSpeed +
                    (weapDef->adsIdleSpeed - weapDef->hipIdleSpeed) * f;
    } else {
        idleAmount = weapDef->fHipIdleAmount;
        if (idleAmount == 0.0f) {
            idleAmount = 80.0f;
            idleSpeed = 1.0f;
        } else {
            idleSpeed = weapDef->hipIdleSpeed;
        }
    }

    if (ps->eFlags & 8)
        idleFactor = weapDef->fIdleProneFactor;
    else if (ps->eFlags & 4)
        idleFactor = weapDef->fIdleCrouchFactor;
    else
        idleFactor = 1.0f;

    if (f != 0.0f)
        BG_TrackViewIdleFactor(vs, idleFactor);

    amount = idleAmount * vs->fLastIdleFactor * f * ps->holdBreathScale;
    *vs->weapIdleTime += (int)(idleSpeed * vs->frametime * 1000.0f * ps->holdBreathScale);

    angles[1] += sinf((float)*vs->weapIdleTime * 0.000699999975040555f) * amount * 0.009999999776482582f;
    angles[0] += sinf((float)*vs->weapIdleTime * 0.0010000000474974513f) * amount * 0.009999999776482582f;
}

static void BG_AddScopedViewBob(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float cycle;
    float speed;
    float pitch;
    float yaw;
    float roll;

    if (!weapDef->overlayReticle)
        return;

    cycle = BG_GetBobCycle(ps) + 0.7853981633974483f;
    speed = vs->xyspeed * 0.1599999964237213f;

    pitch = -BG_GetVerticalBobFactor(ps, cycle, speed, 10.0f);
    yaw = -BG_GetHorizontalBobFactor(ps, cycle, speed, 10.0f);
    roll = BG_GetHorizontalBobFactor(ps, cycle - 0.4712389167638204f, speed * 1.5f, 10.0f);
    if (roll > 0.0f)
        roll = 0.0f;

    if (f != 0.0f) {
        float factor = 1.0f - (1.0f - weapDef->fAdsBobFactor) * f;

        pitch *= factor;
        yaw *= factor;
        roll *= factor;
    }

    angles[0] += pitch * f;
    angles[1] += yaw * f;
    angles[2] += roll * f;
}

static void BG_AddAdsViewBob(viewState_t *vs, vec_t *angles, const WeaponDef *weapDef)
{
    playerState_t *ps = vs->ps;
    float f = ps->fWeaponPosFrac;
    float scale;
    float cycle;

    if (ps->eFlags & 0x300)
        return;
    if (f == 0.0f || weapDef->fAdsViewBobMult == 0.0f)
        return;

    scale = f * weapDef->fAdsViewBobMult;
    cycle = BG_GetBobCycle(ps);
    angles[0] -= BG_GetVerticalBobFactor(ps, cycle, vs->xyspeed, 45.0f) * scale;
    angles[1] -= BG_GetHorizontalBobFactor(ps, cycle, vs->xyspeed, 45.0f) * scale;
}

void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles)
{
    playerState_t *ps = vs->ps;
    WeaponDef *weapDef;

    angles[0] = 0.0f;
    angles[1] = 0.0f;
    angles[2] = 0.0f;

    weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];

    BG_AddViewDamageKick(vs, angles, weapDef);
    BG_AddViewIdle(vs, angles, weapDef);
    BG_AddScopedViewBob(vs, angles, weapDef);
    BG_AddAdsViewBob(vs, angles, weapDef);
}

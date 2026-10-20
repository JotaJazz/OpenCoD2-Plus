#include "common_types.h"
#include "imports.h"
#include "bg_plus_compat.h"

/* The weapon's part of the player's movement (PM_Weapon, aim down the sight,
 * the aim spread, reloads, offhands, binoculars), as CoD2rev-Plus's
 * bg_weapons.cpp (the server's), which this follows line by line: the client
 * predicts its weapon with it. The rest of bg_weapons.cpp (weapon defs, ammo,
 * slots, the view's bob and kick) stays in bg_weapons.c. */

extern int bg_iNumWeapons;
extern WeaponDef *bg_weaponDefs[128];
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

extern void Com_Error(int code, const char *fmt, ...);
extern int BG_AnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force);
extern void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion);
extern void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
extern void PM_SetProneMovementOverride(playerState_t *ps);
extern void PM_AddEvent(playerState_t *ps, int newEvent);
extern int PM_GetEffectiveStance(playerState_t *ps);
extern Bool Mantle_IsWeaponInactive(playerState_t *ps);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetNumWeapons(void);
extern int BG_AmmoForWeapon(int weapon);
extern int BG_ClipForWeapon(int weapon);
extern int BG_GetAmmoClipSize(int iClipIndex);
extern qboolean BG_WeaponIsClipOnly(int weapon);
extern int BG_WeaponAmmo(const playerState_t *ps, int weapon);
extern int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass);
extern int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
extern int BG_IsPlayerWeaponInSlot(const playerState_t *pPS, int iWeaponIndex, qboolean bAnyMode);
extern Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex);
extern qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);

/* the functions below, as bg_public.h declares them */
bool PM_IsBinocularsADS( const playerState_t *ps );
void PM_StartWeaponAnim( playerState_t *ps, int anim );
qboolean BG_SetPlayerWeaponForSlot( playerState_t *ps, unsigned int slot, int iWeaponIndex );
bool BG_ValidateWeaponNumber( int weaponIndex );
int PM_WeaponAmmoAvailable( playerState_t *ps );
void PM_ExitAimDownSight( playerState_t *ps );
void PM_AdjustAimSpreadScale( pmove_t *pm, pml_t *pml );
qboolean PM_InteruptWeaponWithProneMove( playerState_t *ps );
void PM_UpdateAimDownSightLerp( pmove_t *pm, pml_t *pml );
void PM_UpdateAimDownSightFlag( pmove_t *pm, pml_t *pml );
void PM_ResetWeaponState( playerState_t *ps );
void PM_Weapon( pmove_t *pm, pml_t *pml );
void Binocular_State_Hold( playerState_t *ps );
void PM_EndHoldBreath( playerState_t *ps );
void PM_StartHoldBreath( playerState_t *ps );
bool PM_IsBinocularADSAllowed( int iWeaponState );
int BG_GetBinocularWeaponIndex();
void Binocular_State_Init( playerState_t *ps );
void PM_Weapon_OffHandEnd( playerState_t *ps );
void PM_Weapon_OffHandHold( playerState_t *ps );
void PM_Weapon_OffHandPrepare( playerState_t *ps );
void PM_Weapon_AddFiringAimSpreadScale( playerState_t *ps );
void PM_Weapon_FinishReloadEnd( playerState_t *ps );
void PM_Weapon_FinishWeaponRaise( playerState_t *ps );
void PM_ContinueWeaponAnim( playerState_t *ps, int anim );
void Binocular_State_Drop( playerState_t *ps );
void PM_ExitBinoculars( playerState_t *ps );
void PM_EnterBinoculars( playerState_t *ps );
void PM_SendEmtpyOffhandEvent( playerState_t *ps );
void PM_Weapon_FinishMelee( playerState_t *ps );
void PM_Weapon_FireMelee( playerState_t *ps );
qboolean PM_Weapon_FinishFiring( pmove_t *pm, qboolean delayedAction );
void PM_SwitchIfEmpty( playerState_t *ps );
qboolean PM_Weapon_AllowReload( playerState_t *ps );
void PM_SetWeaponReloadAddAmmoDelay( playerState_t *ps );
void PM_Weapon_FinishRechamber( playerState_t *ps );
qboolean PM_WeaponClipEmpty( playerState_t *ps );
void PM_WeaponUseAmmo( playerState_t *ps, int weaponIndex, int amount );
void PM_ReloadClip( playerState_t *ps );
void PM_HoldBreathFire( playerState_t *ps );
bool PM_IsAdsAllowed( playerState_t *ps, pml_t *pml );
void Binocular_State_End( pmove_t *pm );
void Binocular_State_Raise( playerState_t *ps );
void Binocular_State_Exit_ADS( playerState_t *ps );
void PM_ExitBinocularsQuick( playerState_t *ps );
bool PM_UpdateGrenadeThrow( playerState_t *ps, pml_t *pml );
void PM_Weapon_OffHand( pmove_t *pm );
void PM_Weapon_OffHandInit( playerState_t *ps );
void PM_Weapon_Idle( playerState_t *ps );
void PM_Weapon_CheckForMelee( pmove_t *pm, qboolean delayedAction );
void PM_Weapon_SetFPSFireAnim( playerState_t *ps );
void PM_Weapon_StartFiring( playerState_t *ps, qboolean delayedAction );
qboolean PM_Weapon_WeaponTimeAdjust( pmove_t *pm, pml_t *pml );
void PM_Weapon_ReloadDelayedAction( playerState_t *ps );
void PM_Weapon_FinishWeaponChange( pmove_t *pm );
void PM_BeginWeaponChange( playerState_t *ps, int newweapon );
void PM_SetReloadingState( playerState_t *ps );
qboolean PM_Weapon_CheckForRechamber( playerState_t *ps, qboolean delayedAction );
void PM_UpdateHoldBreath( pmove_t *pm, pml_t *pml );
void PM_Weapon_CheckForBinoculars( pmove_t *pm );
void PM_Weapon_CheckForChangeWeapon( pmove_t *pm );
void PM_Weapon_FinishReload( playerState_t *ps, qboolean delayedAction );
void PM_Weapon_FinishReloadStart( playerState_t *ps, qboolean delayedAction );
void PM_BeginWeaponReload( playerState_t *ps );
qboolean PM_Weapon_CheckFiringAmmo( playerState_t *ps );
void PM_Weapon_CheckForReload( pmove_t *pm );
void PM_Weapon_FireWeapon( playerState_t *ps, qboolean delayedAction );
void PM_Weapon_CheckForOffHand( pmove_t *pm );

/*
===============
PM_IsBinocularsADS
===============
*/
bool PM_IsBinocularsADS( const playerState_t *ps )
{
	assert(ps);

	if ( ps->weaponstate == WEAPON_BINOCULARS_HOLD )
	{
		return true;
	}

	if ( ps->weaponstate == WEAPON_BINOCULARS_EXIT_ADS )
	{
		return true;
	}

	return false;
}

/*
===============
PM_StartWeaponAnim
===============
*/
void PM_StartWeaponAnim( playerState_t *ps, int anim )
{
	assert(ps);

	if ( ps->pm_type >= PM_DEAD )
	{
		return;
	}

	ps->weapAnim = ( anim | ( ps->weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT );
}

/*
===============
BG_SetPlayerWeaponForSlot
===============
*/
qboolean BG_SetPlayerWeaponForSlot( playerState_t *ps, unsigned int slot, int iWeaponIndex )
{
	assert(ps);

	if ( !Com_BitCheck(ps->weapons, iWeaponIndex) )
	{
		return qfalse;
	}

	if ( BG_GetWeaponDef(iWeaponIndex)->weapSlot - 1 > 1 )
	{
		return qfalse;
	}

	if ( slot != SLOT_PRIMARY && slot != SLOT_PRIMARYB )
	{
		return qfalse;
	}

	if ( iWeaponIndex != ps->weaponslots[slot] && iWeaponIndex != ps->weapon )
	{
		ps->weapon = WP_NONE;
	}

	ps->weaponslots[slot] = iWeaponIndex;
	return qtrue;
}

/*
===============
BG_ValidateWeaponNumber
===============
*/
bool BG_ValidateWeaponNumber( int weaponIndex )
{
	return weaponIndex >= WP_NONE && weaponIndex <= BG_GetNumWeapons();
}

/*
===============
PM_WeaponAmmoAvailable
===============
*/
int PM_WeaponAmmoAvailable( playerState_t *ps )
{
	assert(ps);
	return ps->ammoclip[BG_ClipForWeapon(ps->weapon)];
}

/*
===============
PM_ExitAimDownSight
===============
*/
void PM_ExitAimDownSight( playerState_t *ps )
{
	assert(ps);
	PM_AddEvent(ps, EV_RESET_ADS);
	ps->pm_flags &= ~PMF_ADS;
}

/*
===============
PM_AdjustAimSpreadScale
===============
*/
void PM_AdjustAimSpreadScale( pmove_t *pm, pml_t *pml )
{
	playerState_t *ps;
	float wpnScale;
	float viewchange;
	float decrease;
	float increase;
	int i;
	WeaponDef *weapDef;

	ps = pm->ps;
	assert(ps);

	weapDef = BG_GetWeaponDef(pm->ps->weapon);
	wpnScale = weapDef->fHipSpreadDecayRate;

	if ( wpnScale == 0 )
	{
		increase = 0;
		decrease = 1;
	}
	else
	{
		if ( ps->groundEntityNum == ENTITYNUM_NONE && ps->pm_type != PM_NORMAL_LINKED )
		{
			wpnScale *= 0.5;
		}
		else if ( ps->eFlags & EF_PRONE )
		{
			wpnScale *= weapDef->fHipSpreadProneDecay;
		}
		else if ( ps->eFlags & EF_CROUCH )
		{
			wpnScale *= weapDef->fHipSpreadDuckedDecay;
		}

		decrease = wpnScale * pml->frametime;

		if ( ps->fWeaponPosFrac == 1 )
		{
			increase = 0;
		}
		else
		{
			viewchange = 0;

			if ( weapDef->fHipSpreadTurnAdd != 0 )
			{
				for ( i = 0; i < 2; i++ )
				{
					viewchange = I_fabs(AngleSubtract(SHORT2ANGLE( pm->cmd.angles[i] ), SHORT2ANGLE( pm->oldcmd.angles[i] )))
					             * 0.0099999998 * weapDef->fHipSpreadTurnAdd / pml->frametime + viewchange;
				}
			}

			if ( weapDef->fHipSpreadMoveAdd != 0 && (pm->cmd.forwardmove || pm->cmd.rightmove) )
			{
				if ( Vec2LengthSq(ps->velocity) > Square(bg_aimSpreadMoveSpeedThreshold->current.value) )
				{
					viewchange = viewchange + weapDef->fHipSpreadMoveAdd;
				}
			}

			if ( ps->groundEntityNum == ENTITYNUM_NONE && ps->pm_type != PM_NORMAL_LINKED )
			{
				for ( i = 0; i < 2; i++ )
				{
					viewchange = viewchange + 1.28;
				}
			}

			increase = viewchange * pml->frametime;
		}
	}

	ps->aimSpreadScale = (increase - decrease) * 255 + ps->aimSpreadScale;

	if ( ps->aimSpreadScale < 0 )
	{
		ps->aimSpreadScale = 0;
	}
	else if ( ps->aimSpreadScale > 255 )
	{
		ps->aimSpreadScale = 255;
	}
}

/*
===============
PM_InteruptWeaponWithProneMove
===============
*/
qboolean PM_InteruptWeaponWithProneMove( playerState_t *ps )
{
	assert(ps);

	if ( ps->weaponstate < WEAPON_FIRING )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_RELOADING )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_RELOAD_START )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_RELOAD_END )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_RELOADING_INTERUPT )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_RECHAMBERING )
	{
		return qtrue;
	}

	if ( ps->weaponstate == WEAPON_FIRING )
	{
		return qfalse;
	}

	if ( ps->weaponstate == WEAPON_MELEE_FIRE )
	{
		return qfalse;
	}

	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND_END )
	{
		return qfalse;
	}

	PM_Weapon_Idle(ps);
	return qtrue;
}

/*
===============
PM_UpdateAimDownSightLerp
===============
*/
void PM_UpdateAimDownSightLerp( pmove_t *pm, pml_t *pml )
{
	playerState_t *ps;
	bool adsRequested;
	int weapIndex;
	WeaponDef *weapDef;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	weapIndex = BG_GetViewmodelWeaponIndex(pm->ps);
	weapDef = BG_GetWeaponDef(weapIndex);

	if ( player_scopeExitOnDamage->current.enabled && ps->damageCount && weapDef->overlayReticle )
	{
		PM_ExitAimDownSight(ps);
		ps->fWeaponPosFrac = 0;
		ps->adsDelayTime = 0;
		return;
	}

	if ( !weapDef->bADSPositionInfo )
	{
		ps->fWeaponPosFrac = 0;
		ps->adsDelayTime = 0;
		return;
	}

	adsRequested = false;

	if ( !weapDef->bSegmentedReload
	        && ps->weaponstate == WEAPON_RELOADING
	        && ps->weaponTime - weapDef->iPositionReloadTransTime > 0
	        || weapDef->bSegmentedReload
	        && (ps->weaponstate == WEAPON_RELOADING
	            || ps->weaponstate == WEAPON_RELOADING_INTERUPT
	            || ps->weaponstate == WEAPON_RELOAD_START
	            || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT
	            || ps->weaponstate == WEAPON_RELOAD_END && ps->weaponTime - weapDef->iPositionReloadTransTime > 0)
	        || !weapDef->bRechamberWhileAds && ps->weaponstate == WEAPON_RECHAMBERING )
	{
		adsRequested = false;
	}
	else if ( ps->pm_flags & PMF_ADS )
	{
		adsRequested = true;
	}

	if ( weapDef->bADSFire && ps->weaponDelay && ps->weaponstate == WEAPON_FIRING )
	{
		adsRequested = true;
	}

	if ( ps->fWeaponPosFrac == 1 && !adsRequested && player_adsExitDelay->current.integer > 0 )
	{
		if ( !ps->adsDelayTime )
		{
			ps->adsDelayTime = pm->cmd.serverTime + player_adsExitDelay->current.integer;
		}

		if ( ps->adsDelayTime > pm->cmd.serverTime )
			adsRequested = true;
		else
			ps->adsDelayTime = 0;
	}
	else
	{
		ps->adsDelayTime = 0;
	}

	if ( adsRequested && ps->fWeaponPosFrac != 1 || !adsRequested && ps->fWeaponPosFrac != 0 )
	{
		if ( adsRequested )
			ps->fWeaponPosFrac += pml->msec * weapDef->fOOPosAnimLength[0];
		else
			ps->fWeaponPosFrac -= pml->msec * weapDef->fOOPosAnimLength[1];

		ps->fWeaponPosFrac = I_fclamp(ps->fWeaponPosFrac, 0, 1);
	}
}

/*
===============
PM_UpdateAimDownSightFlag
===============
*/
void PM_UpdateAimDownSightFlag( pmove_t *pm, pml_t *pml )
{
	bool adsRequested;
	bool adsAllowed;
	playerState_t *ps;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	ps->pm_flags &= ~PMF_ADS;

	adsAllowed = PM_IsAdsAllowed(ps, pml);
	adsRequested = false;

	if ( pm->cmd.buttons & BUTTON_ADS || PM_IsBinocularsADS(ps) )
	{
		adsRequested = true;
	}

	if ( adsRequested && adsAllowed )
	{
		if ( ps->pm_flags & PMF_PRONE )
		{
			if ( !(pm->oldcmd.buttons & BUTTON_ADS) || !pm->cmd.forwardmove && !pm->cmd.rightmove )
			{
				ps->pm_flags |= PMF_ADS;
				ps->pm_flags |= PMF_ADS_OVERRIDE;
			}
		}
		else
		{
			ps->pm_flags |= PMF_ADS;
		}
	}

	if ( ps->pm_flags & PMF_ADS )
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON_POSITION, qtrue, qtrue);
	else
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON_POSITION, qfalse, qtrue);
}

/*
===============
PM_ResetWeaponState
===============
*/
void PM_ResetWeaponState( playerState_t *ps )
{
	assert(ps);
	PM_Weapon_Idle(ps);
}

/*
===============
PM_Weapon
===============
*/
void PM_Weapon( pmove_t *pm, pml_t *pml )
{
	playerState_t *ps;
	qboolean delayedAction;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return;
	}

	if ( ps->pm_type >= PM_DEAD )
	{
		ps->weapon = WP_NONE;
		return;
	}

	if ( ps->eFlags & EF_TURRET_ACTIVE )
	{
		return;
	}

	PM_UpdateAimDownSightLerp(pm, pml);
	PM_UpdateHoldBreath(pm, pml);

	if ( PM_UpdateGrenadeThrow(ps, pml) )
	{
		return;
	}

	delayedAction = PM_Weapon_WeaponTimeAdjust(pm, pml);

	PM_Weapon_CheckForBinoculars(pm);
	PM_Weapon_CheckForOffHand(pm);
	PM_Weapon_CheckForChangeWeapon(pm);
	PM_Weapon_CheckForReload(pm);
	PM_Weapon_CheckForMelee(pm, delayedAction);

	if ( PM_Weapon_CheckForRechamber(ps, delayedAction) )
	{
		return;
	}

	if ( ps->pm_flags & PMF_PRONE && (pm->cmd.forwardmove || pm->cmd.rightmove)
	        || ps->weaponstate == WEAPON_MELEE_INIT
	        || ps->weaponstate == WEAPON_MELEE_FIRE )
	{
		ps->aimSpreadScale = 255;
	}

	assert((ps->weaponTime >= 0) && (ps->weaponDelay >= 0));

	if ( !delayedAction && ps->weaponTime || ps->weaponDelay )
	{
		return;
	}

	switch ( ps->weaponstate )
	{
	case WEAPON_RAISING:
		PM_Weapon_FinishWeaponRaise(ps);
		return;

	case WEAPON_DROPPING:
		PM_Weapon_FinishWeaponChange(pm);
		return;

	case WEAPON_RELOADING:
	case WEAPON_RELOADING_INTERUPT:
		PM_Weapon_FinishReload(ps, delayedAction);
		return;

	case WEAPON_RELOAD_START:
	case WEAPON_RELOAD_START_INTERUPT:
		PM_Weapon_FinishReloadStart(ps, delayedAction);
		return;

	case WEAPON_RELOAD_END:
		PM_Weapon_FinishReloadEnd(ps);
		return;

	case WEAPON_MELEE_INIT:
		PM_Weapon_FireMelee(ps);
		return;

	case WEAPON_MELEE_FIRE:
		PM_Weapon_FinishMelee(ps);
		return;

	case WEAPON_OFFHAND_INIT:
		PM_Weapon_OffHandPrepare(ps);
		return;

	case WEAPON_OFFHAND_PREPARE:
		PM_Weapon_OffHandHold(ps);
		return;

	case WEAPON_OFFHAND_HOLD:
		PM_Weapon_OffHand(pm);
		return;

	case WEAPON_OFFHAND:
		PM_Weapon_OffHandEnd(ps);
		return;

	case WEAPON_OFFHAND_END:
	case WEAPON_BINOCULARS_END:
		PM_Weapon_Idle(ps);
		return;

	case WEAPON_BINOCULARS_INIT:
		Binocular_State_Raise(ps);
		return;

	case WEAPON_BINOCULARS_RAISE:
		Binocular_State_Hold(ps);
		return;

	case WEAPON_BINOCULARS_HOLD:
		Binocular_State_Exit_ADS(ps);
		return;

	case WEAPON_BINOCULARS_EXIT_ADS:
		return;

	case WEAPON_BINOCULARS_DROP:
		Binocular_State_End(pm);
		return;
	}

	if ( !ps->weapon )
	{
		return;
	}

	if ( PM_Weapon_FinishFiring(pm, delayedAction) )
	{
		return;
	}

	PM_Weapon_FireWeapon(ps, delayedAction);
	assert((ps->weaponTime >= 0) && (ps->weaponDelay >= 0));
}

/*
===============
Binocular_State_Hold
===============
*/
void Binocular_State_Hold( playerState_t *ps )
{
	int weapIndex;
	WeaponDef *weapDef;

	assert(ps);

	weapIndex = BG_GetBinocularWeaponIndex();
	weapDef = BG_GetWeaponDef(weapIndex);

	ps->weaponstate = WEAPON_BINOCULARS_HOLD;
	ps->weaponTime = weapDef->iAdsTransInTime;
	ps->weaponDelay = 0;

	PM_EnterBinoculars(ps);
	PM_StartWeaponAnim(ps, WEAP_IDLE);
}

/*
===============
PM_EndHoldBreath
===============
*/
void PM_EndHoldBreath( playerState_t *ps )
{
	assert(ps);
	ps->pm_flags &= ~PMF_BREATH;
}

/*
===============
PM_StartHoldBreath
===============
*/
void PM_StartHoldBreath( playerState_t *ps )
{
	assert(ps);
	ps->pm_flags |= PMF_BREATH;
}

/*
===============
PM_IsBinocularADSAllowed
===============
*/
bool PM_IsBinocularADSAllowed( int iWeaponState )
{
	return iWeaponState == WEAPON_BINOCULARS_HOLD || iWeaponState == WEAPON_BINOCULARS_EXIT_ADS;
}

/*
===============
BG_GetBinocularWeaponIndex
===============
*/
int BG_GetBinocularWeaponIndex()
{
	int curWeaponIndex;

	for ( curWeaponIndex = 1; curWeaponIndex <= bg_iNumWeapons; curWeaponIndex++ )
	{
		if ( bg_weaponDefs[curWeaponIndex]->weapType == WEAPTYPE_BINOCULARS )
		{
			return curWeaponIndex;
		}
	}

	return WP_NONE;
}

/*
===============
Binocular_State_Init
===============
*/
void Binocular_State_Init( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	PM_ExitAimDownSight(ps);

	ps->weaponTime = 0;
	ps->weaponDelay = weapDef->iAdsTransOutTime + player_adsExitDelay->current.integer;
	ps->weaponstate = WEAPON_BINOCULARS_INIT;
}

/*
===============
PM_Weapon_OffHandEnd
===============
*/
void PM_Weapon_OffHandEnd( playerState_t *ps )
{
	assert(ps);

	if ( ps->weapon )
	{
		ps->weaponTime = BG_GetWeaponDef(ps->weapon)->quickRaiseTime;
		ps->weaponDelay = 0;

		PM_StartWeaponAnim(ps, WEAP_EMPTY_RAISE);
	}
	else
	{
		ps->weaponTime = 0;
		ps->weaponDelay = 1;
	}

	ps->weaponstate = WEAPON_OFFHAND_END;
	ps->pm_flags &= ~(PMF_FRAG | PMF_ADS_OVERRIDE);
}

/*
===============
PM_Weapon_OffHandHold
===============
*/
void PM_Weapon_OffHandHold( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	assert(ps->offHandIndex != WP_NONE);

	weapDef = BG_GetWeaponDef(ps->offHandIndex);

	ps->weaponstate = WEAPON_OFFHAND_HOLD;
	ps->weaponTime = 0;
	ps->weaponDelay = 0;
	ps->pm_flags |= PMF_FRAG;
	ps->grenadeTimeLeft = weapDef->iFuseTime;
}

/*
===============
PM_Weapon_OffHandPrepare
===============
*/
void PM_Weapon_OffHandPrepare( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	assert(ps->offHandIndex != WP_NONE);

	weapDef = BG_GetWeaponDef(ps->offHandIndex);

	ps->weaponstate = WEAPON_OFFHAND_PREPARE;
	ps->weaponTime = weapDef->iHoldFireTime;
	ps->weaponDelay = 0;
	ps->pm_flags |= PMF_FRAG;

	BG_AddPredictableEventToPlayerstate(EV_PREP_OFFHAND, ps->offHandIndex, ps);
	PM_StartWeaponAnim(ps, WEAP_HOLD_FIRE);

	PM_SetProneMovementOverride(ps);
}

/*
===============
PM_Weapon_AddFiringAimSpreadScale
===============
*/
void PM_Weapon_AddFiringAimSpreadScale( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->fWeaponPosFrac == 1.0 )
	{
		return;
	}

	ps->aimSpreadScale = weapDef->fHipSpreadFireAdd * 255 + ps->aimSpreadScale;

	if ( ps->aimSpreadScale > 255 )
	{
		ps->aimSpreadScale = 255;
	}
}

/*
===============
PM_Weapon_FinishReloadEnd
===============
*/
void PM_Weapon_FinishReloadEnd( playerState_t *ps )
{
	assert(ps);
	assert(ps->weaponstate == WEAPON_RELOAD_END);

	ps->weaponstate = WEAPON_READY;
	PM_StartWeaponAnim(ps, WEAP_IDLE);
}

/*
===============
PM_Weapon_FinishWeaponRaise
===============
*/
void PM_Weapon_FinishWeaponRaise( playerState_t *ps )
{
	assert(ps);
	assert(ps->weaponstate == WEAPON_RAISING);

	ps->weaponstate = WEAPON_READY;
	PM_StartWeaponAnim(ps, WEAP_IDLE);
}

/*
===============
PM_ContinueWeaponAnim
===============
*/
void PM_ContinueWeaponAnim( playerState_t *ps, int anim )
{
	assert(ps);

	if ( (ps->weapAnim & ~ANIM_TOGGLEBIT) != anim )
	{
		PM_StartWeaponAnim(ps, anim);
	}
}

/*
===============
Binocular_State_Drop
===============
*/
void Binocular_State_Drop( playerState_t *ps )
{
	int weapIndex;
	WeaponDef *weapDef;

	assert(ps);

	weapIndex = BG_GetBinocularWeaponIndex();
	weapDef = BG_GetWeaponDef(weapIndex);

	ps->weaponstate = WEAPON_BINOCULARS_DROP;
	ps->weaponTime = weapDef->iAdsTransOutTime + player_adsExitDelay->current.integer;
	ps->weaponDelay = 0;

	PM_AddEvent(ps, EV_BINOCULAR_DROP);
	PM_StartWeaponAnim(ps, WEAP_IDLE);
}

/*
===============
PM_ExitBinoculars
===============
*/
void PM_ExitBinoculars( playerState_t *ps )
{
	assert(ps);
	ps->pm_flags &= ~PMF_BINOCULARS;
	PM_AddEvent(ps, EV_BINOCULAR_EXIT);
}

/*
===============
PM_EnterBinoculars
===============
*/
void PM_EnterBinoculars( playerState_t *ps )
{
	assert(ps);
	ps->pm_flags |= PMF_BINOCULARS;
	PM_AddEvent(ps, EV_BINOCULAR_ENTER);
}

/*
===============
PM_SendEmtpyOffhandEvent
===============
*/
void PM_SendEmtpyOffhandEvent( playerState_t *ps )
{
	int i;

	assert(ps);

	// holding the grenade button without grenades sent this every frame,
	// pushing the player's shots and footsteps out of the event queue
	for ( i = 0; i < MAX_EVENTS; i++ )
	{
		if ( ps->events[i] == EV_EMPTY_OFFHAND )
		{
			return;
		}
	}

	PM_AddEvent(ps, EV_EMPTY_OFFHAND);
}

/*
===============
PM_Weapon_FinishMelee
===============
*/
void PM_Weapon_FinishMelee( playerState_t *ps )
{
	assert(ps);
	PM_ContinueWeaponAnim(ps, WEAP_IDLE);
	ps->weaponstate = WEAPON_READY;
}

/*
===============
PM_Weapon_FireMelee
===============
*/
void PM_Weapon_FireMelee( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->weaponTime < weapDef->iMeleeTime - weapDef->iMeleeDelay )
	{
		ps->weaponTime = weapDef->iMeleeTime - weapDef->iMeleeDelay;
	}

	PM_AddEvent(ps, EV_FIRE_MELEE);
	ps->weaponstate = WEAPON_MELEE_FIRE;

	PM_SetProneMovementOverride(ps);
}

/*
===============
PM_Weapon_FinishFiring
===============
*/
qboolean PM_Weapon_FinishFiring( pmove_t *pm, qboolean delayedAction )
{
	playerState_t *ps;

	assert(pm);

	if ( pm->cmd.buttons & BUTTON_ATTACK || delayedAction )
	{
		return qfalse;
	}

	ps = pm->ps;
	assert(ps);

	if ( pm->ps->weaponstate == WEAPON_FIRING )
	{
		PM_ContinueWeaponAnim(ps, WEAP_IDLE);
	}

	ps->weaponstate = WEAPON_READY;
	return qtrue;
}

/*
===============
PM_SwitchIfEmpty
===============
*/
void PM_SwitchIfEmpty( playerState_t *ps )
{
	assert(ps);

	if ( !BG_WeaponIsClipOnly(ps->weapon) )
	{
		return;
	}

	if ( ps->ammoclip[BG_ClipForWeapon(ps->weapon)] )
	{
		return;
	}

	if ( ps->ammo[BG_AmmoForWeapon(ps->weapon)] )
	{
		return;
	}

	BG_TakePlayerWeapon(ps, ps->weapon);
	PM_AddEvent(ps, EV_NOAMMO);
}

/*
===============
PM_Weapon_AllowReload
===============
*/
qboolean PM_Weapon_AllowReload( playerState_t *ps )
{
	int weapIndex;
	WeaponDef *weapDef;

	assert(ps);

	weapDef = BG_GetWeaponDef(ps->weapon);
	weapIndex = BG_ClipForWeapon(ps->weapon);

	if ( !ps->ammo[BG_AmmoForWeapon(ps->weapon)] || ps->ammoclip[weapIndex] >= BG_GetAmmoClipSize(weapIndex) )
	{
		return qfalse;
	}

	if ( !weapDef->bNoPartialReload )
	{
		return qtrue;
	}

	if ( weapDef->iReloadAmmoAdd && weapDef->iReloadAmmoAdd < BG_GetAmmoClipSize(weapIndex) )
	{
		return BG_GetAmmoClipSize(weapIndex) - ps->ammoclip[weapIndex] >= weapDef->iReloadAmmoAdd;
	}

	return !ps->ammoclip[weapIndex];
}

/*
===============
PM_SetWeaponReloadAddAmmoDelay
===============
*/
void PM_SetWeaponReloadAddAmmoDelay( playerState_t *ps )
{
	int iReloadAddTime;
	WeaponDef *weapDef;

	assert(ps);

	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->weaponstate == WEAPON_RELOAD_START || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
	{
		if ( weapDef->iReloadStartAddTime )
		{
			if ( weapDef->iReloadStartAddTime < weapDef->iReloadStartTime )
				iReloadAddTime = weapDef->iReloadStartAddTime;
			else
				iReloadAddTime = weapDef->iReloadStartTime;
		}
		else
		{
			iReloadAddTime = 0;
		}
	}
	else
	{
		if ( ps->ammoclip[BG_ClipForWeapon(ps->weapon)] || weapDef->weapType )
			iReloadAddTime = weapDef->iReloadTime;
		else
			iReloadAddTime = weapDef->iReloadEmptyTime;

		if ( weapDef->iReloadAddTime && weapDef->iReloadAddTime < iReloadAddTime )
			iReloadAddTime = weapDef->iReloadAddTime;
	}

	if ( weapDef->bBoltAction && Com_BitCheck(ps->weaponrechamber, ps->weapon) )
	{
		if ( !iReloadAddTime )
			iReloadAddTime = ps->weaponTime;

		if ( weapDef->iRechamberBoltTime < iReloadAddTime )
			iReloadAddTime = weapDef->iRechamberBoltTime;

		if ( !iReloadAddTime )
			iReloadAddTime = 1;

		ps->weaponDelay = iReloadAddTime;
	}
	else if ( iReloadAddTime )
	{
		ps->weaponDelay = iReloadAddTime;
	}
}

/*
===============
PM_Weapon_FinishRechamber
===============
*/
void PM_Weapon_FinishRechamber( playerState_t *ps )
{
	assert(ps);
	PM_ContinueWeaponAnim(ps, WEAP_IDLE);
	ps->weaponstate = WEAPON_READY;
}

/*
===============
PM_WeaponClipEmpty
===============
*/
qboolean PM_WeaponClipEmpty( playerState_t *ps )
{
	assert(ps);
	return ps->ammoclip[BG_ClipForWeapon(ps->weapon)] == 0;
}

/*
===============
PM_WeaponUseAmmo
===============
*/
void PM_WeaponUseAmmo( playerState_t *ps, int weaponIndex, int amount )
{
	assert(ps);
	ps->ammoclip[BG_ClipForWeapon(weaponIndex)] -= amount;
}

/*
===============
PM_ReloadClip
===============
*/
void PM_ReloadClip( playerState_t *ps )
{
	int ammoAdd;
	int ammoInClip;
	int ammoReserve;
	int clipIndex;
	int ammoIndex;
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->weaponstate == WEAPON_RELOAD_START || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
	{
		if ( !weapDef->iReloadStartAdd )
		{
			return;
		}
	}

	ammoIndex = BG_AmmoForWeapon(ps->weapon);
	clipIndex = BG_ClipForWeapon(ps->weapon);

	ammoReserve = ps->ammo[ammoIndex];
	ammoInClip = ps->ammoclip[clipIndex];

	ammoAdd = BG_GetAmmoClipSize(clipIndex) - ammoInClip;

	if ( ammoAdd > ammoReserve )
	{
		ammoAdd = ammoReserve;
	}

	if ( ps->weaponstate == WEAPON_RELOAD_START || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
	{
		if ( weapDef->iReloadStartAdd < BG_GetAmmoClipSize(clipIndex) && ammoAdd > weapDef->iReloadStartAdd )
		{
			ammoAdd = weapDef->iReloadStartAdd;
		}
	}
	else
	{
		if ( weapDef->iReloadAmmoAdd && weapDef->iReloadAmmoAdd < BG_GetAmmoClipSize(clipIndex) && ammoAdd > weapDef->iReloadAmmoAdd )
		{
			ammoAdd = weapDef->iReloadAmmoAdd;
		}
	}

	if ( ammoAdd )
	{
		ps->ammo[ammoIndex] -= ammoAdd;
		ps->ammoclip[clipIndex] += ammoAdd;
	}
}

/*
===============
PM_HoldBreathFire
===============
*/
void PM_HoldBreathFire( playerState_t *ps )
{
	int weapIndex;
	WeaponDef *weapDef;
	int breathHoldTime;

	assert(ps);

	weapIndex = BG_GetViewmodelWeaponIndex(ps);
	weapDef = BG_GetWeaponDef(weapIndex);

	if ( ps->fWeaponPosFrac != 1.0 )
	{
		return;
	}

	if ( !weapDef->overlayReticle )
	{
		return;
	}

	if ( weapDef->weapClass == WEAPCLASS_ITEM )
	{
		return;
	}

	breathHoldTime = player_breath_hold_time->current.value * 1000;

	if ( ps->holdBreathTimer < breathHoldTime )
	{
		ps->holdBreathTimer += player_breath_fire_delay->current.value * 1000;

		if ( ps->holdBreathTimer > breathHoldTime )
		{
			ps->holdBreathTimer = breathHoldTime;
		}
	}

	PM_EndHoldBreath(ps);
}

/*
===============
PM_IsAdsAllowed
===============
*/
bool PM_IsAdsAllowed( playerState_t *ps, pml_t *pml )
{
	int weapIndex;

	assert(ps);
	assert(pml);

	if ( ps->pm_type == PM_NORMAL_LINKED )
	{
		if ( pml->almostGroundPlane )
		{
			return false;
		}
	}

	if ( ps->pm_type >= PM_NORMAL_LINKED && ps->pm_type <= PM_DEAD_LINKED )
	{
		return false;
	}

	weapIndex = BG_GetViewmodelWeaponIndex(ps);

	if ( !BG_GetWeaponDef(weapIndex)->bADSPositionInfo )
	{
		return false;
	}

	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND_END )
	{
		return false;
	}

	if ( ps->weaponstate == WEAPON_MELEE_INIT || ps->weaponstate == WEAPON_MELEE_FIRE )
	{
		return false;
	}

	if ( ps->weaponstate == WEAPON_RAISING || ps->weaponstate == WEAPON_DROPPING )
	{
		return false;
	}

	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		return PM_IsBinocularADSAllowed(ps->weaponstate);
	}

	return true;
}

/*
===============
Binocular_State_End
===============
*/
void Binocular_State_End( pmove_t *pm )
{
	playerState_t *ps;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	ps->weaponstate = WEAPON_BINOCULARS_END;
	PM_ExitBinoculars(ps);

	if ( pm->cmd.weapon )
	{
		ps->weaponTime = BG_GetWeaponDef(ps->weapon)->quickRaiseTime;
		ps->weaponDelay = 0;
		PM_StartWeaponAnim(ps, WEAP_EMPTY_RAISE);
	}
	else
	{
		ps->weaponTime = 0;
		ps->weaponDelay = 1;
	}
}

/*
===============
Binocular_State_Raise
===============
*/
void Binocular_State_Raise( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	ps->weaponstate = WEAPON_BINOCULARS_RAISE;
	ps->weaponTime = weapDef->quickDropTime;
	ps->weaponDelay = 0;

	PM_StartWeaponAnim(ps, WEAP_EMPTY_DROP);
}

/*
===============
Binocular_State_Exit_ADS
===============
*/
void Binocular_State_Exit_ADS( playerState_t *ps )
{
	assert(ps);

	ps->weaponstate = WEAPON_BINOCULARS_EXIT_ADS;
	ps->weaponTime = 0;
	ps->weaponDelay = 0;
}

/*
===============
PM_ExitBinocularsQuick
===============
*/
void PM_ExitBinocularsQuick( playerState_t *ps )
{
	assert(ps);

	ps->fWeaponPosFrac = 0;

	PM_ExitAimDownSight(ps);
	PM_ExitBinoculars(ps);
}

/*
===============
PM_UpdateGrenadeThrow
===============
*/
bool PM_UpdateGrenadeThrow( playerState_t *ps, pml_t *pml )
{
	int weapIndex;
	WeaponDef *weapDef;

	assert(ps);
	assert(pml);

	if ( !(ps->pm_flags & PMF_FRAG) )
	{
		return false;
	}

	weapIndex = ps->offHandIndex;
	weapDef = BG_GetWeaponDef(weapIndex);

	if ( weapDef->weapType != WEAPTYPE_GRENADE )
	{
		return false;
	}

	if ( ps->grenadeTimeLeft <= 0 )
	{
		return false;
	}

	if ( weapDef->bCookOffHold )
	{
		ps->grenadeTimeLeft -= pml->msec;
	}

	if ( ps->grenadeTimeLeft > 0 )
	{
		return false;
	}

	PM_WeaponUseAmmo(ps, weapIndex, 1);
	PM_AddEvent(ps, EV_GRENADE_SUICIDE);

	return true;
}

/*
===============
PM_Weapon_OffHand
===============
*/
void PM_Weapon_OffHand( pmove_t *pm )
{
	playerState_t *ps;
	WeaponDef *weapDef;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	assert(ps->offHandIndex != WP_NONE);

	if ( pm->oldcmd.buttons & (BUTTON_FRAG | BUTTON_SMOKE) && pm->cmd.buttons & (BUTTON_FRAG | BUTTON_SMOKE) )
	{
		ps->weaponDelay = 1;
		return;
	}

	weapDef = BG_GetWeaponDef(ps->offHandIndex);

	ps->weaponstate = WEAPON_OFFHAND;
	ps->weaponTime = weapDef->iFireTime;
	ps->weaponDelay = 0;

	ps->pm_flags |= PMF_FRAG;

	BG_AddPredictableEventToPlayerstate(EV_USE_OFFHAND, ps->offHandIndex, ps);
	PM_StartWeaponAnim(ps, WEAP_ATTACK);

	PM_WeaponUseAmmo(ps, ps->offHandIndex, 1);
	BG_AnimScriptEvent(ps, ANIM_ET_FIREWEAPON, qfalse, qtrue);

	if ( !BG_WeaponAmmo(ps, ps->offHandIndex) )
	{
		PM_AddEvent(ps, EV_EMPTY_OFFHAND);
	}
}

/*
===============
PM_Weapon_OffHandInit
===============
*/
void PM_Weapon_OffHandInit( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	assert(ps->offHandIndex != WP_NONE);

	weapDef = BG_GetWeaponDef(ps->weapon);

	ps->weaponstate = WEAPON_OFFHAND_INIT;
	ps->weaponTime = weapDef->quickDropTime;
	ps->weaponDelay = 0;

	ps->pm_flags &= ~PMF_FRAG;

	PM_ExitAimDownSight(ps);
	PM_StartWeaponAnim(ps, WEAP_EMPTY_DROP);
}

/*
===============
PM_Weapon_Idle
===============
*/
void PM_Weapon_Idle( playerState_t *ps )
{
	assert(ps);

	ps->pm_flags &= ~(PMF_FRAG | PMF_ADS_OVERRIDE);

	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		PM_ExitBinocularsQuick(ps);
	}

	ps->weaponTime = 0;
	ps->weaponDelay = 0;
	ps->weaponstate = WEAPON_READY;

	PM_StartWeaponAnim(ps, WEAP_IDLE);
}

/*
===============
PM_Weapon_CheckForMelee
===============
*/
void PM_Weapon_CheckForMelee( pmove_t *pm, qboolean delayedAction )
{
	playerState_t *ps;
	WeaponDef *weapDef;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	weapDef = BG_GetWeaponDef(pm->ps->weapon);

	// Using binoculars
	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		return;
	}

	// Using grenade
	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND_END )
	{
		return;
	}

	if ( !weapDef->iMeleeDamage )
	{
		return;
	}

	if ( delayedAction )
	{
		return;
	}

	// Reloading
	if ( ps->weaponDelay )
	{
		if ( ps->weaponstate < WEAPON_RELOADING && ps->weaponstate > WEAPON_RELOAD_END )
		{
			return;
		}
	}

	if ( !(pm->cmd.buttons & BUTTON_MELEE) )
	{
		ps->pm_flags &= ~PMF_MELEE;
		return;
	}

	if ( ps->pm_flags & PMF_MELEE )
	{
		return;
	}

	ps->pm_flags |= PMF_MELEE;

	if ( ps->fWeaponPosFrac > 0 && weapDef->overlayReticle )
	{
		return;
	}

	if ( ps->weaponstate >= WEAPON_RAISING && ps->weaponstate <= WEAPON_DROPPING )
	{
		return;
	}

	// Already bashing
	if ( ps->weaponstate >= WEAPON_MELEE_INIT )
	{
		return;
	}

	BG_AnimScriptEvent(ps, ANIM_ET_MELEEATTACK, qfalse, qtrue);
	PM_StartWeaponAnim(ps, WEAP_MELEE_ATTACK);

	PM_AddEvent(ps, EV_MELEE_SWIPE);

	if ( !weapDef->iMeleeDelay )
	{
		PM_Weapon_FireMelee(ps);
		return;
	}

	ps->weaponTime = weapDef->iMeleeTime;
	ps->weaponDelay = weapDef->iMeleeDelay;
	ps->weaponstate = WEAPON_MELEE_INIT;

	PM_SetProneMovementOverride(ps);
}

/*
===============
PM_Weapon_SetFPSFireAnim
===============
*/
void PM_Weapon_SetFPSFireAnim( playerState_t *ps )
{
	assert(ps);

	if ( ps->fWeaponPosFrac > 0.75 )
	{
		if ( PM_WeaponClipEmpty(ps) )
			PM_StartWeaponAnim(ps, WEAP_ADS_ATTACK_LASTSHOT);
		else
			PM_StartWeaponAnim(ps, WEAP_ADS_ATTACK);
	}
	else
	{
		if ( PM_WeaponClipEmpty(ps) )
			PM_StartWeaponAnim(ps, WEAP_ATTACK_LASTSHOT);
		else
			PM_StartWeaponAnim(ps, WEAP_ATTACK);
	}
}

/*
===============
PM_Weapon_StartFiring
===============
*/
void PM_Weapon_StartFiring( playerState_t *ps, qboolean delayedAction )
{
	WeaponDef *weapDef;

	assert(ps);

	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( weapDef->weapType == WEAPTYPE_GRENADE )
	{
		if ( delayedAction )
		{
			return;
		}

		if ( PM_WeaponAmmoAvailable(ps) )
		{
			ps->grenadeTimeLeft = weapDef->iFuseTime;

			PM_StartWeaponAnim(ps, WEAP_HOLD_FIRE);
			PM_AddEvent(ps, EV_PULLBACK_WEAPON);
		}

		ps->weaponDelay = weapDef->iHoldFireTime;
		ps->weaponTime = 0;
		return;
	}

	ps->weaponDelay = weapDef->iFireDelay;
	ps->weaponTime = weapDef->iFireTime;

	if ( weapDef->bADSFire )
	{
		ps->weaponDelay = (int)((1.0 - ps->fWeaponPosFrac) * (1.0 / weapDef->fOOPosAnimLength[0]));
	}

	BG_AnimScriptEvent(ps, ANIM_ET_FIREWEAPON, qfalse, qtrue);

	if ( weapDef->bBoltAction )
	{
		Com_BitSet(ps->weaponrechamber, ps->weapon);
	}

	if ( ps->weaponstate != WEAPON_FIRING )
	{
		if ( ps->fWeaponPosFrac >= 1.0 )
			ps->weaponRestrictKickTime = weapDef->iFireDelay + weapDef->adsGunKickReducedKickBullets * weapDef->iFireTime;
		else
			ps->weaponRestrictKickTime = weapDef->iFireDelay + weapDef->hipGunKickReducedKickBullets * weapDef->iFireTime;
	}

	ps->weaponstate = WEAPON_FIRING;

	PM_SetProneMovementOverride(ps);
}

/*
===============
PM_Weapon_WeaponTimeAdjust
===============
*/
qboolean PM_Weapon_WeaponTimeAdjust( pmove_t *pm, pml_t *pml )
{
	playerState_t *ps;
	WeaponDef *weapDef;

	ps = pm->ps;
	assert(ps);

	weapDef = BG_GetWeaponDef(pm->ps->weapon);

	if ( ps->weaponRestrictKickTime > 0 )
	{
		ps->weaponRestrictKickTime -= pml->msec;
		ps->weaponRestrictKickTime = I_max(ps->weaponRestrictKickTime, 0);
	}

	if ( ps->weaponTime )
	{
		ps->weaponTime -= pml->msec;

		if ( ps->weaponTime <= 0 )
		{
			if ( ( ps->weaponstate < WEAPON_OFFHAND_INIT || ps->weaponstate > WEAPON_OFFHAND_END )
			        && weapDef->bSemiAuto
			        && pm->cmd.buttons & BUTTON_ATTACK
			        && ps->weapon == pm->cmd.weapon
			        && PM_WeaponAmmoAvailable(ps) )
			{
				ps->weaponTime = 1;

				if ( ps->weaponstate == WEAPON_RELOADING
				        || ps->weaponstate == WEAPON_RELOAD_START
				        || ps->weaponstate == WEAPON_RELOAD_END
				        || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT
				        || ps->weaponstate == WEAPON_RELOADING_INTERUPT )
				{
					ps->weaponTime = 0;
				}

				if ( (ps->weaponstate == WEAPON_RELOADING
				        || ps->weaponstate == WEAPON_RELOAD_START
				        || ps->weaponstate == WEAPON_RELOAD_END
				        || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT
				        || ps->weaponstate == WEAPON_RELOADING_INTERUPT)
				        && ps->weaponstate != WEAPON_RELOAD_END
				        && weapDef->bSegmentedReload
				        && weapDef->iReloadStartTime )
				{
					ps->weaponTime = 0;
				}
				else if ( ps->weaponstate == WEAPON_RECHAMBERING )
				{
					PM_Weapon_FinishRechamber(ps);
				}
				else if ( ps->weaponstate == WEAPON_FIRING
				          || ps->weaponstate == WEAPON_RECHAMBERING
				          || ps->weaponstate == WEAPON_MELEE_INIT
				          || ps->weaponstate == WEAPON_MELEE_FIRE )
				{
					PM_ContinueWeaponAnim(ps, WEAP_IDLE);
					ps->weaponstate = WEAPON_READY;
				}
			}
			else
			{
				ps->weaponTime = 0;
			}
		}
	}

	if ( !ps->weaponDelay )
	{
		return qfalse;
	}

	ps->weaponDelay -= pml->msec;

	if ( ps->weaponDelay > 0 )
	{
		return qfalse;
	}

	ps->weaponDelay = 0;

	return qtrue;
}

/*
===============
PM_Weapon_ReloadDelayedAction
===============
*/
void PM_Weapon_ReloadDelayedAction( playerState_t *ps )
{
	int reloadTime;
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( !weapDef->bBoltAction || !Com_BitCheck(ps->weaponrechamber, ps->weapon) )
	{
		PM_ReloadClip(ps);
		return;
	}

	Com_BitClear(ps->weaponrechamber, ps->weapon);
	PM_AddEvent(ps, EV_EJECT_BRASS);

	if ( ps->weaponstate == WEAPON_RELOAD_START || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
	{
		if ( !weapDef->iReloadStartAddTime )
		{
			return;
		}
	}

	if ( !ps->weaponTime )
	{
		PM_ReloadClip(ps);
		return;
	}

	if ( ps->weaponstate == WEAPON_RELOAD_START || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
	{
		reloadTime = weapDef->iReloadStartAddTime >= weapDef->iReloadStartTime ? weapDef->iReloadStartTime : weapDef->iReloadStartAddTime;
	}
	else
	{
		reloadTime = ps->ammoclip[BG_ClipForWeapon(ps->weapon)] || weapDef->weapType ? weapDef->iReloadTime : weapDef->iReloadEmptyTime;

		if ( weapDef->iReloadAddTime && weapDef->iReloadAddTime < reloadTime )
		{
			reloadTime = weapDef->iReloadAddTime;
		}
	}

	reloadTime -= weapDef->iRechamberBoltTime >= reloadTime ? qtrue : weapDef->iRechamberBoltTime;

	if ( reloadTime > 0 )
	{
		ps->weaponDelay = reloadTime;
		return;
	}

	PM_ReloadClip(ps);
}

/*
===============
PM_Weapon_FinishWeaponChange
===============
*/
void PM_Weapon_FinishWeaponChange( pmove_t *pm )
{
	bool bHasWeapon;
	qboolean bAltRaise;
	WeaponDef *weapDef;
	playerState_t *ps;
	int newweapon;
	int oldweapon;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	assert( ps->weaponstate == WEAPON_DROPPING );

	//BG_GetWeaponDef(pm->ps->weapon);

	if ( Mantle_IsWeaponInactive(ps) || ps->pm_flags & PMF_LADDER || ps->pm_flags & PMF_DISABLEWEAPON || !Com_BitCheck(ps->weapons, pm->cmd.weapon) )
	{
		newweapon = WP_NONE;
		bHasWeapon = Com_BitCheck(ps->weapons, newweapon);
	}
	else
	{
		newweapon = pm->cmd.weapon;

		if ( newweapon > BG_GetNumWeapons() )
		{
			newweapon = WP_NONE;
		}

		bHasWeapon = Com_BitCheck(ps->weapons, newweapon);
	}

	if ( !bHasWeapon )
	{
		newweapon = WP_NONE;
	}

	oldweapon = ps->weapon;
	ps->weapon = (byte)newweapon;
	assert((byte)ps->weapon == newweapon);

	//BG_GetWeaponDef(ps->weapon);

	if ( oldweapon == newweapon )
	{
		ps->weaponstate = WEAPON_READY;
		PM_StartWeaponAnim(ps, WEAP_IDLE);
	}
	else if ( oldweapon )
	{
		ps->weaponstate = WEAPON_RAISING;
		PM_SetProneMovementOverride(ps);

		bAltRaise = qfalse;

		if ( newweapon )
		{
			bAltRaise = newweapon == BG_GetWeaponDef(oldweapon)->iAltWeaponIndex;
		}

		if ( bAltRaise )
		{
			ps->weaponTime = BG_GetWeaponDef(newweapon)->iAltRaiseTime;
		}
		else
		{
			PM_AddEvent(ps, EV_RAISE_WEAPON);
			ps->weaponTime = BG_GetWeaponDef(newweapon)->iRaiseTime;
		}

		weapDef = BG_GetWeaponDef(newweapon);
		assert(weapDef);

		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_PLAYERANIMTYPE, weapDef->playerAnimType, qtrue);
		BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPONCLASS, weapDef->weapClass, qtrue);

		if ( newweapon && newweapon == BG_GetWeaponDef(oldweapon)->iAltWeaponIndex )
		{
			if ( ps->aimSpreadScale < 128 )
			{
				ps->aimSpreadScale = 128;
			}

			PM_StartWeaponAnim(ps, WEAP_ALTSWITCHTO);
		}
		else
		{
			BG_AnimScriptEvent(ps, ANIM_ET_RAISEWEAPON, qfalse, qfalse);
			ps->aimSpreadScale = 255;

			PM_StartWeaponAnim(ps, WEAP_RAISE);
		}
	}
	else
	{
		ps->weaponstate = WEAPON_RAISING;
		ps->weaponTime = BG_GetWeaponDef(newweapon)->iRaiseTime;
		ps->aimSpreadScale = 255;

		PM_StartWeaponAnim(ps, WEAP_RAISE);
		PM_SetProneMovementOverride(ps);
	}
}

/*
===============
PM_BeginWeaponChange
===============
*/
void PM_BeginWeaponChange( playerState_t *ps, int newweapon )
{
	qboolean altswitch;
	int slot;
	int oldweapon;
	WeaponDef *weapDef;
	qboolean oldWeaponNoAmmo;

	assert(ps);

	if ( newweapon && !Com_BitCheck(ps->weapons, newweapon))
	{
		return;
	}

	if ( ps->weaponstate == WEAPON_DROPPING )
	{
		return;
	}

#ifdef LIBCOD
	// CodeCallback_WeaponChange runs after this usercmd, outside of pmove
	extern int libcod_weaponChange[];
	if ( newweapon != ps->weapon )
		libcod_weaponChange[ps->clientNum] = newweapon + 1;
#endif

	ps->weaponDelay = 0;
	oldweapon = ps->weapon;

	if ( !oldweapon || !Com_BitCheck(ps->weapons, oldweapon) || ps->grenadeTimeLeft > 0 )
	{
		ps->weaponTime = 0;
		ps->weaponstate = WEAPON_DROPPING;
		ps->grenadeTimeLeft = 0;
		PM_SetProneMovementOverride(ps);
		return;
	}

	weapDef = BG_GetWeaponDef(oldweapon);
	altswitch = qfalse;

	if ( newweapon )
	{
		altswitch = newweapon == weapDef->iAltWeaponIndex;
	}

	oldWeaponNoAmmo = qtrue;

	if ( BG_WeaponIsClipOnly(oldweapon) )
	{
		oldWeaponNoAmmo = ps->ammoclip[BG_ClipForWeapon(oldweapon)] != 0;
	}

	ps->grenadeTimeLeft = 0;

	if ( altswitch )
	{
		PM_AddEvent(ps, EV_WEAPON_ALT);
		PM_StartWeaponAnim(ps, WEAP_ALTSWITCHFROM);
	}
	else if ( oldWeaponNoAmmo )
	{
		PM_AddEvent(ps, EV_PUTAWAY_WEAPON);
		PM_StartWeaponAnim(ps, WEAP_DROP);
	}

	if ( !altswitch && !(ps->pm_flags & PMF_MANTLE) )
	{
		BG_AnimScriptEvent(ps, ANIM_ET_DROPWEAPON, qfalse, qfalse);
	}

	ps->weaponstate = WEAPON_DROPPING;
	PM_SetProneMovementOverride(ps);

	if ( altswitch )
		ps->weaponTime = weapDef->iAltDropTime;
	else
		ps->weaponTime = weapDef->iDropTime;

	if ( altswitch )
	{
		slot = BG_IsPlayerWeaponInSlot(ps, oldweapon, qtrue);

		if ( slot )
		{
			BG_SetPlayerWeaponForSlot(ps, slot, newweapon);
		}
	}

}

/*
===============
PM_SetReloadingState
===============
*/
void PM_SetReloadingState( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->ammoclip[BG_ClipForWeapon(ps->weapon)] || weapDef->weapType )
	{
		PM_StartWeaponAnim(ps, WEAP_RELOAD);
		ps->weaponTime = weapDef->iReloadTime;
		PM_AddEvent(ps, EV_RELOAD);
	}
	else
	{
		PM_StartWeaponAnim(ps, WEAP_RELOAD_EMPTY);
		ps->weaponTime = weapDef->iReloadEmptyTime;
		PM_AddEvent(ps, EV_RELOAD_FROM_EMPTY);
	}

	if ( ps->weaponstate == WEAPON_RELOAD_START_INTERUPT )
		ps->weaponstate = WEAPON_RELOADING_INTERUPT;
	else
		ps->weaponstate = WEAPON_RELOADING;

	PM_SetWeaponReloadAddAmmoDelay(ps);
}

/*
===============
PM_Weapon_CheckForRechamber
===============
*/
qboolean PM_Weapon_CheckForRechamber( playerState_t *ps, qboolean delayedAction )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		return qfalse;
	}

	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND_END )
	{
		return qfalse;
	}

	if ( !weapDef->bBoltAction || !Com_BitCheck(ps->weaponrechamber, ps->weapon) )
	{
		return qfalse;
	}

	if ( ps->weaponstate == WEAPON_RECHAMBERING )
	{
		if ( delayedAction )
		{
			Com_BitClear(ps->weaponrechamber, ps->weapon);
			PM_AddEvent(ps, EV_EJECT_BRASS);

			if ( ps->weaponTime )
			{
				return qtrue;
			}
		}
	}

	if ( ps->weaponTime )
	{
		if ( ps->weaponstate == WEAPON_FIRING || ps->weaponstate == WEAPON_RECHAMBERING
		        || ps->weaponstate == WEAPON_MELEE_INIT ||
		        ps->weaponstate == WEAPON_MELEE_FIRE || ps->weaponDelay )
		{
			return qfalse;
		}
	}

	if ( ps->weaponstate == WEAPON_RECHAMBERING )
	{
		PM_Weapon_FinishRechamber(ps);
		return qfalse;
	}

	if ( ps->weaponstate != WEAPON_READY )
	{
		return qfalse;
	}

	if ( ps->fWeaponPosFrac > 0.75 )
		PM_StartWeaponAnim(ps, WEAP_ADS_RECHAMBER);
	else
		PM_StartWeaponAnim(ps, WEAP_RECHAMBER);

	ps->weaponstate = WEAPON_RECHAMBERING;
	ps->weaponTime = weapDef->iRechamberTime;

	if ( weapDef->iRechamberBoltTime && weapDef->iRechamberBoltTime < weapDef->iRechamberTime )
		ps->weaponDelay = weapDef->iRechamberBoltTime;
	else
		ps->weaponDelay = 1;

	PM_AddEvent(ps, EV_RECHAMBER_WEAPON);

	return qfalse;
}

/*
===============
PM_UpdateHoldBreath
===============
*/
void PM_UpdateHoldBreath( pmove_t *pm, pml_t *pml )
{
	playerState_t *ps;
	int breathGaspTime;
	int breathHoldTime;
	float lerp;
	float targetScale;
	int weapIndex;
	WeaponDef *weapDef;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	weapIndex = BG_GetViewmodelWeaponIndex(pm->ps);
	weapDef = BG_GetWeaponDef(weapIndex);

	breathHoldTime = player_breath_hold_time->current.value * 1000;
	breathGaspTime = player_breath_gasp_time->current.value * 1000;

	if ( breathHoldTime <= 0 )
	{
		PM_EndHoldBreath(ps);
		ps->holdBreathScale = 1.0;
		ps->holdBreathTimer = 0;
		return;
	}

	if ( ps->fWeaponPosFrac == 1.0 && weapDef->overlayReticle && weapDef->weapClass != WEAPCLASS_ITEM && pm->cmd.buttons & BUTTON_HOLDBREATH )
	{
		if ( !ps->holdBreathTimer )
		{
			PM_StartHoldBreath(ps);
		}
	}
	else
	{
		PM_EndHoldBreath(ps);
	}

	if ( ps->pm_flags & PMF_BREATH )
		ps->holdBreathTimer += pml->msec;
	else
		ps->holdBreathTimer -= pml->msec;

	if ( ps->holdBreathTimer < 0 )
	{
		ps->holdBreathTimer = 0;
	}

	if ( ps->pm_flags & PMF_BREATH )
	{
		if ( ps->holdBreathTimer > breathHoldTime )
		{
			ps->holdBreathTimer = breathHoldTime + breathGaspTime;
			PM_EndHoldBreath(ps);
		}
	}

	if ( ps->pm_flags & PMF_BREATH )
	{
		targetScale = 0;
		lerp = player_breath_hold_lerp->current.value;
	}
	else
	{
		targetScale = (player_breath_gasp_scale->current.value - 1.0) * (ps->holdBreathTimer / (float)(breathHoldTime + breathGaspTime)) + 1.0;
		lerp = player_breath_gasp_lerp->current.value;
	}

	ps->holdBreathScale = DiffTrack((targetScale - 1.0) * ps->fWeaponPosFrac + 1.0, ps->holdBreathScale, lerp, pml->frametime);
}

/*
===============
PM_Weapon_CheckForBinoculars
===============
*/
void PM_Weapon_CheckForBinoculars( pmove_t *pm )
{
	playerState_t *ps;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	// Disable binoculars while using scope
	if ( !( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END
	        || (ps->fWeaponPosFrac <= 0 || !BG_GetWeaponDef(ps->weapon)->overlayReticle)
	        && !(pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_MELEE | BUTTON_RELOAD | BUTTON_FRAG | BUTTON_SMOKE ) ) ) )
	{
		return;
	}

	// Optimization: no states except READY and binoculars are allowed
	if ( ps->weaponstate != WEAPON_READY )
	{
		if ( !( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END ) )
		{
			return;
		}
	}

	// Switching weapon ?
	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		if ( ( ps->damageCount || pm->oldcmd.weapon != pm->cmd.weapon
		        || pm->oldcmd.offHandIndex != pm->cmd.offHandIndex
		        || ( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_MELEE | BUTTON_RELOAD | BUTTON_FRAG | BUTTON_SMOKE ) ) ) )
		{
			PM_Weapon_Idle(ps);

			if ( pm->cmd.buttons & BUTTON_ATTACK )
			{
				ps->weaponTime = 1;
			}

			return;
		}
	}

	if ( pm->cmd.buttons & BUTTON_ATTACK )
	{
		return;
	}

	// Fire or release, used by Scr_Notify system
	if ( ps->weaponstate == WEAPON_BINOCULARS_EXIT_ADS )
	{
		if ( !( pm->oldcmd.buttons & ( BUTTON_USE | BUTTON_USERELOAD) ) && ( pm->cmd.buttons & ( BUTTON_USE | BUTTON_USERELOAD) ) )
		{
			PM_AddEvent(ps, EV_BINOCULAR_FIRE);
		}
		else if ( ( pm->oldcmd.buttons & ( BUTTON_USE | BUTTON_USERELOAD) ) && !( pm->cmd.buttons & ( BUTTON_USE | BUTTON_USERELOAD) ) )
		{
			PM_AddEvent(ps, EV_BINOCULAR_RELEASE);
		}
	}

	// default mode is toggle
	if ( player_toggleBinoculars->current.enabled )
	{
		if ( ( !( pm->oldcmd.buttons & BUTTON_BINOCULARS ) && pm->cmd.buttons & BUTTON_BINOCULARS ) && ( ps->weaponstate < WEAPON_BINOCULARS_INIT || ps->weaponstate > WEAPON_BINOCULARS_END ) )
		{
			// Pressed binoculars button - switch to binoculars
			if ( !BG_GetBinocularWeaponIndex() )
			{
				return; // No binoculars weapon file loaded
			}

			if ( pm->cmd.weapon )
			{
				if ( pm->cmd.buttons & BUTTON_ADS )
				{
					Binocular_State_Init(ps);
				}
				else
				{
					Binocular_State_Raise(ps);
				}
			}
			else
			{
				Binocular_State_Hold(ps);
			}
		}
		else if ( ( !( pm->oldcmd.buttons & BUTTON_BINOCULARS ) && pm->cmd.buttons & BUTTON_BINOCULARS ) && ( ps->weaponstate > WEAPON_BINOCULARS_INIT && ps->weaponstate < WEAPON_BINOCULARS_DROP ) )
		{
			// Pressed binoculars button while using binoculars - drop them
			if ( ps->weaponstate == WEAPON_BINOCULARS_RAISE )
			{
				Binocular_State_End(pm);
			}
			else
			{
				Binocular_State_Drop(ps);
			}
		}
	}
	else
	{
		if ( pm->cmd.buttons & BUTTON_BINOCULARS && ( ps->weaponstate < WEAPON_BINOCULARS_INIT || ps->weaponstate > WEAPON_BINOCULARS_END ) )
		{
			// Player is holding binoculars button
			if ( !BG_GetBinocularWeaponIndex() )
			{
				return; // No binoculars weapon file loaded
			}

			if ( pm->cmd.weapon )
			{
				if ( pm->cmd.buttons & BUTTON_ADS )
				{
					Binocular_State_Init(ps);
				}
				else
				{
					Binocular_State_Raise(ps);
				}
			}
			else
			{
				Binocular_State_Hold(ps);
			}
		}
		else if ( !( pm->cmd.buttons & BUTTON_BINOCULARS ) && ( ps->weaponstate > WEAPON_BINOCULARS_INIT && ps->weaponstate < WEAPON_BINOCULARS_DROP ) )
		{
			// Player released binoculars button
			if ( ps->weaponstate == WEAPON_BINOCULARS_RAISE )
			{
				Binocular_State_End(pm);
			}
			else
			{
				Binocular_State_Drop(ps);
			}
		}
	}
}

/*
===============
PM_Weapon_CheckForChangeWeapon
===============
*/
void PM_Weapon_CheckForChangeWeapon( pmove_t *pm )
{
	playerState_t *ps;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	// Using binoculars
	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		return;
	}

	// Using offhand
	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND_END )
	{
		return;
	}

	if ( !( !ps->weaponTime
	        || ps->weaponstate == WEAPON_RELOADING
	        || ps->weaponstate == WEAPON_RELOAD_START
	        || ps->weaponstate == WEAPON_RELOAD_END
	        || ps->weaponstate == WEAPON_RELOAD_START_INTERUPT
	        || ps->weaponstate == WEAPON_RELOADING_INTERUPT
	        || ps->weaponstate == WEAPON_RECHAMBERING
	        || ps->weaponstate != WEAPON_FIRING
	        && ps->weaponstate != WEAPON_RECHAMBERING
	        && ps->weaponstate != WEAPON_MELEE_INIT
	        && ps->weaponstate != WEAPON_MELEE_FIRE
	        && !ps->weaponDelay ) )
	{
		return;
	}

	if ( Mantle_IsWeaponInactive(ps) )
	{
		if ( !ps->weapon )
		{
			return;
		}

		PM_BeginWeaponChange(ps, WP_NONE);
		return;
	}

	if ( ps->pm_flags & PMF_LADDER )
	{
		if ( !ps->weapon )
		{
			return;
		}

		PM_BeginWeaponChange(ps, WP_NONE);
		return;
	}

	if ( ps->pm_flags & PMF_DISABLEWEAPON )
	{
		if ( !ps->weapon )
		{
			return;
		}

		PM_BeginWeaponChange(ps, WP_NONE);
		return;
	}

	if ( ps->weapon != pm->cmd.weapon )
	{
		if ( ( !(ps->pm_flags & PMF_FROZEN) || !ps->weapon ) )
		{
			if ( !pm->cmd.weapon || BG_IsWeaponValid(ps, pm->cmd.weapon) )
			{
				PM_BeginWeaponChange(ps, pm->cmd.weapon);
				return;
			}
		}
	}

	if ( ps->weapon )
	{
		if ( !Com_BitCheck(ps->weapons, ps->weapon) )
		{
			PM_BeginWeaponChange(ps, WP_NONE);
		}
	}
}

/*
===============
PM_Weapon_FinishReload
===============
*/
void PM_Weapon_FinishReload( playerState_t *ps, qboolean delayedAction )
{
	WeaponDef *weapDef;

	assert(ps);
	assert(ps->weaponstate == WEAPON_RELOADING || ps->weaponstate == WEAPON_RELOADING_INTERUPT);

	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( delayedAction )
	{
		PM_Weapon_ReloadDelayedAction(ps);

		if ( ps->weaponTime )
		{
			return;
		}
	}

	if ( ps->weaponTime )
	{
		return;
	}

	Com_BitClear(ps->weaponrechamber, ps->weapon);

	if ( !weapDef->bSegmentedReload )
	{
		ps->weaponstate = WEAPON_READY;
		PM_StartWeaponAnim(ps, WEAP_IDLE);
		return;
	}

	if ( ps->weaponstate != WEAPON_RELOADING_INTERUPT )
	{
		if ( PM_Weapon_AllowReload(ps) )
		{
			PM_SetReloadingState(ps);
			return;
		}
	}

	if ( weapDef->iReloadEndTime )
	{
		ps->weaponstate = WEAPON_RELOAD_END;
		PM_StartWeaponAnim(ps, WEAP_RELOAD_END);

		ps->weaponTime = weapDef->iReloadEndTime;
		PM_AddEvent(ps, EV_RELOAD_END);
	}
	else
	{
		ps->weaponstate = WEAPON_READY;
		PM_StartWeaponAnim(ps, WEAP_IDLE);
	}
}

/*
===============
PM_Weapon_FinishReloadStart
===============
*/
void PM_Weapon_FinishReloadStart( playerState_t *ps, qboolean delayedAction )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);
	assert((ps->weaponstate == WEAPON_RELOAD_START) || (ps->weaponstate == WEAPON_RELOAD_START_INTERUPT));

	if ( delayedAction )
	{
		PM_Weapon_ReloadDelayedAction(ps);
	}

	if ( ps->weaponTime )
	{
		return;
	}

	if ( ps->weaponstate != WEAPON_RELOAD_START_INTERUPT || !ps->ammoclip[BG_ClipForWeapon(ps->weapon)] )
	{
		if ( PM_Weapon_AllowReload(ps) )
		{
			PM_SetReloadingState(ps);
			return;
		}
	}

	Com_BitClear(ps->weaponrechamber, ps->weapon);

	if ( weapDef->iReloadEndTime )
	{
		ps->weaponstate = WEAPON_RELOAD_END;
		PM_StartWeaponAnim(ps, WEAP_RELOAD_END);

		ps->weaponTime = weapDef->iReloadEndTime;
		PM_AddEvent(ps, EV_RELOAD_END);
	}
	else
	{
		ps->weaponstate = WEAPON_READY;
		PM_StartWeaponAnim(ps, WEAP_IDLE);
	}
}

/*
===============
PM_BeginWeaponReload
===============
*/
void PM_BeginWeaponReload( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( ps->weaponstate != WEAPON_READY && ps->weaponstate != WEAPON_FIRING && ps->weaponstate != WEAPON_RECHAMBERING )
	{
		return;
	}

	if ( !BG_ValidateWeaponNumber(ps->weapon) )
	{
		return;
	}

	if ( !BG_WeaponIsClipOnly(ps->weapon) )
	{
		BG_AnimScriptEvent(ps, ANIM_ET_RELOAD, qfalse, qtrue);
	}

	PM_AddEvent(ps, EV_RESET_ADS);

	if ( !weapDef->bSegmentedReload || !weapDef->iReloadStartTime )
	{
		PM_SetReloadingState(ps);
		return;
	}

	PM_StartWeaponAnim(ps, WEAP_RELOAD_START);

	ps->weaponTime = weapDef->iReloadStartTime;
	ps->weaponstate = WEAPON_RELOAD_START;

	PM_AddEvent(ps, EV_RELOAD_START);
	PM_SetWeaponReloadAddAmmoDelay(ps);
}

/*
===============
PM_Weapon_CheckFiringAmmo
===============
*/
qboolean PM_Weapon_CheckFiringAmmo( playerState_t *ps )
{
	WeaponDef *weapDef;

	assert(ps);
	assert(ps->weapon != WP_NONE);

	weapDef = BG_GetWeaponDef(ps->weapon);

	if ( PM_WeaponAmmoAvailable(ps) > 0 )
	{
		return qtrue;
	}

	if ( weapDef->weapType != WEAPTYPE_GRENADE )
	{
		if ( ps->ammo[BG_AmmoForWeapon(ps->weapon)] < 1 )
		{
			PM_AddEvent(ps, EV_NOAMMO);
		}
	}

	if ( ps->ammo[BG_AmmoForWeapon(ps->weapon)] > 0 )
	{
		PM_BeginWeaponReload(ps);
	}
	else
	{
		Com_BitClear(ps->weaponrechamber, ps->weapon);
		PM_ContinueWeaponAnim(ps, WEAP_IDLE);
		ps->weaponTime += 500;
	}

	return qfalse;
}

/*
===============
PM_Weapon_CheckForReload
===============
*/
void PM_Weapon_CheckForReload( pmove_t *pm )
{
	playerState_t *ps;
	qboolean doReload;
	qboolean reloadRequested;
	WeaponDef *weapDef;

	doReload = qfalse;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	weapDef = BG_GetWeaponDef(pm->ps->weapon);

	// Using binoculars
	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		return;
	}

	// Using offhand
	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND_END )
	{
		return;
	}

	// Using melee
	if ( ps->weaponstate >= WEAPON_MELEE_INIT && ps->weaponstate <= WEAPON_MELEE_FIRE )
	{
		return;
	}

	reloadRequested = pm->cmd.buttons & BUTTON_RELOAD;

	// Deprecated flag, not used
	if ( ps->pm_flags & PMF_RELOAD )
	{
		ps->pm_flags &= ~PMF_RELOAD;
		reloadRequested = qtrue;
	}

	if ( weapDef->bSegmentedReload && ( ps->weaponstate == WEAPON_RELOAD_START || ps->weaponstate == WEAPON_RELOADING ) )
	{
		if ( pm->cmd.buttons & BUTTON_ATTACK && !( pm->oldcmd.buttons & BUTTON_ATTACK ) )
		{
			if ( ps->weaponstate == WEAPON_RELOAD_START )
			{
				ps->weaponstate = WEAPON_RELOAD_START_INTERUPT;
			}
			else if ( ps->weaponstate == WEAPON_RELOADING )
			{
				ps->weaponstate = WEAPON_RELOADING_INTERUPT;
			}
		}
	}

	switch ( ps->weaponstate )
	{
	case WEAPON_RAISING:
	case WEAPON_DROPPING:
	case WEAPON_MELEE_INIT:
	case WEAPON_MELEE_FIRE:
		return;

	case WEAPON_RELOADING:
	case WEAPON_RELOADING_INTERUPT:
	case WEAPON_RELOAD_START:
	case WEAPON_RELOAD_START_INTERUPT:
	case WEAPON_RELOAD_END:
		if ( pm->proneChange )
		{
			if ( !BG_WeaponIsClipOnly(ps->weapon) )
			{
				BG_AnimScriptEvent(ps, ANIM_ET_RELOAD, qfalse, qtrue);
			}
		}
		return;
	}

	if ( reloadRequested )
	{
		if ( PM_Weapon_AllowReload(ps) )
		{
			doReload = qtrue;
		}
	}

	if ( !ps->ammoclip[BG_ClipForWeapon(ps->weapon)] && ps->ammo[BG_AmmoForWeapon(ps->weapon)] )
	{
		if ( ps->weaponstate != WEAPON_FIRING )
		{
			// No ammo and player is proning and moving, reload not allowed
			if ( !(ps->pm_flags & PMF_PRONE) || !pm->cmd.forwardmove && !pm->cmd.rightmove )
			{
				doReload = qtrue;
			}
		}
	}

	if ( doReload )
	{
		PM_BeginWeaponReload(ps);
	}
}

/*
===============
PM_Weapon_FireWeapon
===============
*/
void PM_Weapon_FireWeapon( playerState_t *ps, qboolean delayedAction )
{
	WeaponDef *weapDef;

	assert(ps);
	weapDef = BG_GetWeaponDef(ps->weapon);

	PM_Weapon_StartFiring(ps, delayedAction);

	if ( !PM_Weapon_CheckFiringAmmo(ps) )
	{
		return;
	}

	if ( ps->weaponDelay )
	{
		return;
	}

	if ( PM_WeaponAmmoAvailable(ps) != -1 && !(ps->eFlags & EF_TURRET_ACTIVE) )
	{
		PM_WeaponUseAmmo(ps, ps->weapon, 1);
	}

	if ( weapDef->weapType == WEAPTYPE_GRENADE )
	{
		ps->weaponTime = weapDef->iFireTime;
	}

	PM_Weapon_SetFPSFireAnim(ps);

	if ( PM_WeaponClipEmpty(ps) )
		PM_AddEvent(ps, EV_FIRE_WEAPON_LASTSHOT);
	else
		PM_AddEvent(ps, EV_FIRE_WEAPON);

	PM_HoldBreathFire(ps);
	PM_Weapon_AddFiringAimSpreadScale(ps);
	PM_SwitchIfEmpty(ps);
}

/*
===============
PM_Weapon_CheckForOffHand
===============
*/
void PM_Weapon_CheckForOffHand( pmove_t *pm )
{
	WeaponDef *weapDef;
	int offhandSlot;
	int offHandIndex;
	playerState_t *ps;

	assert(pm);
	ps = pm->ps;
	assert(ps);

	if ( pm->ps->eFlags & EF_TURRET_ACTIVE )
	{
		return;
	}

	if ( ps->pm_flags & PMF_DISABLEWEAPON )
	{
		return;
	}

	// Using binoculars
	if ( ps->weaponstate >= WEAPON_BINOCULARS_INIT && ps->weaponstate <= WEAPON_BINOCULARS_END )
	{
		return;
	}

	// Using offhand
	if ( ps->weaponstate >= WEAPON_OFFHAND_INIT && ps->weaponstate <= WEAPON_OFFHAND ) // No OFFHAND_END ? Edit: yes, this is correct
	{
		return;
	}

	if ( Com_BitCheck(ps->weapons, pm->cmd.offHandIndex) )
	{
		ps->offHandIndex = pm->cmd.offHandIndex;
	}

	assert( ps->offHandIndex == WP_NONE || BG_GetWeaponDef( ps->offHandIndex )->offhandClass != OFFHAND_CLASS_NONE );

	if ( pm->cmd.buttons & BUTTON_FRAG )
	{
		offhandSlot = OFFHAND_SLOT_FRAG_GRENADE;
	}
	else
	{
		if ( !( pm->cmd.buttons & BUTTON_SMOKE ) )
		{
			return;
		}

		offhandSlot = OFFHAND_SLOT_SMOKE_GRENADE;
	}

	offHandIndex = BG_GetFirstAvailableOffhand(ps, offhandSlot);
	assert( offHandIndex == WP_NONE || BG_GetWeaponDef( offHandIndex )->offhandClass != OFFHAND_CLASS_NONE );

	if ( !offHandIndex )
	{
		PM_SendEmtpyOffhandEvent(ps);
		return;
	}

	BG_AddPredictableEventToPlayerstate(EV_SWITCH_OFFHAND, offHandIndex, ps);

	ps->offHandIndex = offHandIndex;
	weapDef = BG_GetWeaponDef(ps->offHandIndex);

	if ( weapDef->weapType != WEAPTYPE_GRENADE )
	{
		Com_Error(ERR_DROP, "[%s] Only grenades are currently supported for off hand use\n", weapDef->szInternalName);
	}

	if ( !weapDef->offhandClass )
	{
		Com_Error(ERR_DROP, "[%s] No offhand class set\n", weapDef->szInternalName);
	}

	if ( !ps->weapon || ps->weaponstate == WEAPON_OFFHAND_END )
		PM_Weapon_OffHandPrepare(ps);
	else
		PM_Weapon_OffHandInit(ps);
}

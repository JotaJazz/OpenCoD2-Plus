// COD2_CHECK_ANIMSCRIPT: in a client hosting a local game both bgames have
// loaded the same animation script, the engine's for cgame's prediction and
// CoD2rev-Plus's for the local server. This runs their playerstate functions
// (conditions, locomotion, events) on the same inputs and prints how many
// cases give other animations, timers or durations; scripts/anim_script_test.py
// reads it. The engine's functions are its C ones (src/PC/bgame).
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "../server/server.h"
#include "../game/g_shared.h"
#include "script_bridge.h"

extern "C" {
void cod2e_AnimUpdatePlayerStateConditions(pmove_t *pm) __asm__(COD2_SYM("BG_AnimUpdatePlayerStateConditions"));
int cod2e_AnimScriptAnimation(playerState_t *ps, int state, int movetype, int isContinue) __asm__(COD2_SYM("BG_AnimScriptAnimation"));
int cod2e_AnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event, int isContinue, int force) __asm__(COD2_SYM("BG_AnimScriptEvent"));
int cod2e_GetNumWeapons(void) __asm__(COD2_SYM("BG_GetNumWeapons"));
int cod2e_AnimInfo(int index, int *flags, long long *movetype, int *duration, float *moveSpeed) __asm__(COD2_SYM("BG_CheckAnimInfo"));
}

namespace {

struct Case
{
	int client;
	int weapon;
	int eFlags;
	float pitch;
	int buttons;
	int legsAnim;
	int legsTimer;
};

void Setup(playerState_t *ps, pmove_t *pm, const Case &c)
{
	memset(ps, 0, sizeof(*ps));
	memset(pm, 0, sizeof(*pm));
	ps->clientNum = c.client;
	ps->weapon = c.weapon;
	ps->eFlags = c.eFlags;
	ps->viewangles[PITCH] = c.pitch;
	ps->legsAnim = c.legsAnim;
	ps->torsoAnim = c.legsAnim;
	ps->legsTimer = c.legsTimer;
	ps->torsoTimer = c.legsTimer;
	pm->ps = ps;
	pm->cmd.buttons = c.buttons;
}

void NoSound(int, snd_alias_list_t *)
{
}

bool Same(const playerState_t &a, const playerState_t &b, int ra, int rb)
{
	return ra == rb && a.legsAnim == b.legsAnim && a.torsoAnim == b.torsoAnim && a.legsTimer == b.legsTimer
	       && a.torsoTimer == b.torsoTimer && a.legsAnimDuration == b.legsAnimDuration
	       && a.torsoAnimDuration == b.torsoAnimDuration;
}

}

// the engine's BG_LoadAnim calls it once cgame's script is loaded
extern "C" void BG_CheckAnimScript(void)
{
	static const int eFlags[] = { 0, EF_AIMDOWNSIGHT, EF_TURRET_ACTIVE, EF_CROUCH, EF_PRONE };
	static const float pitches[] = { -30, 30 };
	static const int legsAnims[] = { 0, 3, 138 | ANIM_TOGGLEBIT };
	int numWeapons;
	int cases = 0;
	int differ = 0;

	if (!getenv("COD2_CHECK_ANIMSCRIPT"))
		return;
	if (!level_bgs.animData.animScriptData.numAnimations)
	{
		Com_Printf("animscript check: no local server\n");
		return;
	}

	// the parsed animations (noteType aside: the server leaves it unset)
	const animScriptData_t *data = &level_bgs.animData.animScriptData;
	int numAnims = cod2e_AnimInfo(-1, NULL, NULL, NULL, NULL);
	int animsDiffer = numAnims != data->numAnimations;

	for (int i = 0; i < numAnims && i < data->numAnimations; ++i)
	{
		int flags, duration;
		long long movetype;
		float moveSpeed;

		cod2e_AnimInfo(i, &flags, &movetype, &duration, &moveSpeed);
		if (flags != data->animations[i].flags || movetype != data->animations[i].movetype
		        || duration != data->animations[i].duration || moveSpeed != data->animations[i].moveSpeed)
		{
			if (animsDiffer < 10)
				Com_Printf("animscript check: animation %d %s: engine flags %x movetype %llx, Plus flags %x movetype %llx\n",
				           i, data->animations[i].name, flags, (unsigned long long)movetype, data->animations[i].flags,
				           (unsigned long long)data->animations[i].movetype);
			++animsDiffer;
		}
	}
	Com_Printf("animscript check: %d animations, %d differ\n", numAnims, animsDiffer);

	void (*playSoundAlias)(int, snd_alias_list_t *) = level_bgs.animData.animScriptData.playSoundAlias;
	level_bgs.animData.animScriptData.playSoundAlias = NoSound;

	numWeapons = BG_GetNumWeapons();
	if (cod2e_GetNumWeapons() < numWeapons)
		numWeapons = cod2e_GetNumWeapons();

	for (int client = 0; client < 3; ++client)
	for (int weapon = 0; weapon < numWeapons; ++weapon)
	for (int f = 0; f < (int)ARRAY_COUNT(eFlags); ++f)
	for (int p = 0; p < (int)ARRAY_COUNT(pitches); ++p)
	for (int buttons = 0; buttons < 2; ++buttons)
	for (int l = 0; l < (int)ARRAY_COUNT(legsAnims); ++l)
	for (int timer = 0; timer < 2; ++timer)
	{
		const Case c = { client, weapon, eFlags[f], pitches[p], buttons ? BUTTON_ATTACK : 0, legsAnims[l], timer ? 100 : 0 };
		playerState_t pe, pp;
		pmove_t me, mp;

		for (int movetype = 0; movetype < NUM_ANIM_MOVETYPES; ++movetype)
		for (int state = 0; state < MAX_AISTATES; ++state)
		for (int isContinue = 0; isContinue < 2; ++isContinue)
		{
			Setup(&pe, &me, c);
			Setup(&pp, &mp, c);
			cod2e_AnimUpdatePlayerStateConditions(&me);
			BG_AnimUpdatePlayerStateConditions(&mp);
			int re = cod2e_AnimScriptAnimation(&pe, state, movetype, isContinue);
			int rp = BG_AnimScriptAnimation(&pp, state, movetype, isContinue);
			++cases;
			if (!Same(pe, pp, re, rp))
			{
				if (differ < 10)
					Com_Printf("animscript check: anim client %d weapon %d eFlags %x pitch %g state %d movetype %d: engine %d legs %d torso %d, Plus %d legs %d torso %d\n",
					           client, weapon, c.eFlags, c.pitch, state, movetype, re, pe.legsAnim, pe.torsoAnim, rp, pp.legsAnim, pp.torsoAnim);
				++differ;
			}
		}

		for (int event = 0; event < NUM_ANIM_EVENTTYPES; ++event)
		for (int force = 0; force < 2; ++force)
		{
			Setup(&pe, &me, c);
			Setup(&pp, &mp, c);
			cod2e_AnimUpdatePlayerStateConditions(&me);
			BG_AnimUpdatePlayerStateConditions(&mp);
			// each side picks among an item's commands with rand()
			srand(cases);
			int re = cod2e_AnimScriptEvent(&pe, (scriptAnimEventTypes_t)event, qfalse, force);
			srand(cases);
			int rp = BG_AnimScriptEvent(&pp, (scriptAnimEventTypes_t)event, qfalse, force);
			++cases;
			if (!Same(pe, pp, re, rp))
			{
				if (differ < 10)
					Com_Printf("animscript check: event client %d weapon %d eFlags %x pitch %g event %d: engine %d legs %d torso %d, Plus %d legs %d torso %d\n",
					           client, weapon, c.eFlags, c.pitch, event, re, pe.legsAnim, pe.torsoAnim, rp, pp.legsAnim, pp.torsoAnim);
				++differ;
			}
		}
	}

	level_bgs.animData.animScriptData.playSoundAlias = playSoundAlias;
	Com_Printf("animscript check: %d cases, %d differ\n", cases, differ);
}

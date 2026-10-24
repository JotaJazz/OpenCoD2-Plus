#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <stdlib.h>
#include <stdarg.h>
#include "bg_plus_compat.h"
#include <string.h>
extern scr_const_t scr_const;

/* File-scope alias: bound where no local can shadow `scr_const`, so uses below
   always reach the global even inside functions that declare their own `scr_const`. */
static scr_const_t * const scr_const_g = &scr_const;
extern animStringItem_t animBodyPartsStr[8];
extern animStringItem_t animConditionMountedStr[5];
extern animStringItem_t animConditionsStr[12];
extern animConditionTable_t animConditionsTable[12];
extern animStringItem_t animEventTypesStr[20];
extern animStringItem_t animMoveTypesStr[44];
extern animStringItem_t animParseModesStr[6];
extern animStringItem_t animStateStr[8];
extern animStringItem_t animStrafeStateStr[4];
extern animStringItem_t animWeaponClassStr[12];
extern animStringItem_t animWeaponPositionStr[3];

extern int I_strncmp(const char *s0, const char *s1, int n);
extern void I_strncat(char *dest, int size, const char *src);
extern int FS_Read(void *buffer, int len, fileHandle_t h);
extern int FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode);
extern void FS_FCloseFile(fileHandle_t h);
extern void Com_UngetToken(void);
extern const char *Com_Parse(const char **data_p);
extern const char *Com_ParseOnLine(const char **data_p);
extern void Com_EndParseSession(void);
extern void Com_BeginParseSession(const char *filename);

extern bgs_t *bgs;
static animScriptData_t *globalScriptData;
extern char *globalFilename;
static int numDefines[9];
static char defineStrings[10000];
static int defineStringsOffset;
static animStringItem_t defineStr[9][16];
static int defineBits[9][16][2];
static scriptAnimMoveTypes_t parseMovetype;
static int parseEvent;
static loadAnim_t *g_pLoadAnims;
static int *g_piNumLoadAnims;
static char input[100000];
static qboolean bScriptFileLoaded;
extern animStringItem_t weaponStrings[];
extern const dvar_t *bg_swingSpeed;
extern void *(*controller_names[6])();

extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern int Com_GetCurrentParseLine(void);
extern int ___tolower(int c);
#include <stdio.h>
extern int I_stricmp(const char *s0, const char *s1);
extern void Scr_FindAnim(const char *filename, const char *animName, scr_anim_t *anim, int user);
extern void Scr_PrecacheAnimTrees(Alloc_t Alloc, int user);
extern void Scr_EndLoadAnimTrees(void);
extern struct scr_animtree_t Scr_FindAnimTree(const char *filename);
extern unsigned int XAnimGetAnimTreeSize(const XAnim *anims);
extern void LargeLocal_LargeLocal(const LargeLocal *_this, int size);
extern void *LargeLocal_GetBuf(const LargeLocal *_this);
extern void ZN10LargeLocalD1Ev(LargeLocal *_this);
extern unsigned char scrMemTreeGlob[];
extern void XAnimClearTree(XAnimTree *tree);
extern int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
extern WeaponDef *BG_GetWeaponDef(int weaponIndex);
extern const float AngleSubtract(const float a1, const float a2);
extern const float AngleMod(const float a);
extern float GetLeanFraction(const float fFrac);
extern void AnglesSubtract(const vec_t *v1, const vec_t *v2, vec_t *v3);
extern qboolean DObjSetControlTagAngles(const struct DObj_s *obj, int *partBits, unsigned int tagName, vec_t *angles);
extern qboolean DObjSetLocalTag(const struct DObj_s *obj, int *partBits, unsigned int tagName, const vec_t *trans, const vec_t *angles);
extern float sinf(float);
extern float cosf(float);
extern void XAnimClearGoalWeight(struct XAnimTree_s *tree, unsigned int animIndex, float blendTime);
extern void XAnimSetCompleteGoalWeight(struct XAnimTree_s *tree, unsigned int animIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, unsigned int notifyType, int bRestart);
extern int XAnimSetCompleteGoalWeightKnobAll(struct XAnimTree_s *tree, unsigned int animIndex, unsigned int rootIndex, float goalWeight, float goalTime, float rate, unsigned int notifyName, int bRestart);
extern void XAnimSetAnimRate(struct XAnimTree_s *tree, unsigned int animIndex, float rate);
extern float XAnimGetWeight(const XAnimTree_s *tree, unsigned int animIndex);
extern float XAnimGetTime(const struct XAnimTree_s *tree, unsigned int animIndex);
extern void XAnimSetTime(XAnimTree *tree, unsigned int animIndex, float time);
extern Bool XAnimIsPrimitive(struct XAnim_s *anims, unsigned int animIndex);
extern Bool XAnimIsLooped(const struct XAnim_s *anims, unsigned int animIndex);
extern int XAnimGetLengthMsec(const struct XAnim_s *anims, unsigned int animIndex);
extern const vec_t Vec3Distance(const vec_t *v1, const vec_t *v2);
extern const char *XAnimGetAnimName(const XAnim *anims, unsigned int animIndex);
extern float XAnimGetLength(const XAnim *anims, unsigned int animIndex);
extern void XAnimGetRelDelta(const XAnim *anims, unsigned int animIndex, vec_t *rot, vec_t *trans, float time1, float time2);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern float sqrtf(float x);
void BG_AnimParseError(const char *msg, ...);
static inline __attribute__((always_inline)) int BG_StringHash(const char *str);
void BG_InitWeaponStrings(void);

void BG_LoadWeaponStrings(void);

void BG_InitWeaponStrings(void)
{
    memset(weaponStrings, 0, 0x400);
    BG_LoadWeaponStrings();
}

/* 0x4f8f20 */
int BG_PlayAnim(playerState_t *ps, int animNum, animBodyPart_t bodyPart, int forceDuration, qboolean setTimer, qboolean isContinue, qboolean force)
{
    int duration;
    qboolean wasSet = qfalse;

    if (forceDuration) {
        duration = forceDuration;
    } else {
        duration = globalScriptData->animations[animNum].duration + 50;   // account for lerping between anims
    }

    switch (bodyPart) {
    case ANIM_BP_BOTH:
    case ANIM_BP_LEGS:

        if ((ps->legsTimer < 50) || force) {
            if (!isContinue || !((ps->legsAnim & ~ANIM_TOGGLEBIT) == animNum)) {
                wasSet = qtrue;
                ps->legsAnimDuration = duration;
                ps->legsAnim = ((ps->legsAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animNum;
                if (setTimer) {
                    ps->legsTimer = duration;
                }
            } else if (setTimer && globalScriptData->animations[animNum].flags & 0x80) {
                ps->legsTimer = duration;
            }
        }

        if (bodyPart == ANIM_BP_LEGS) {
            break;
        }

        if (bodyPart == ANIM_BP_BOTH) {
            animNum = 0;
        }

    case ANIM_BP_TORSO:

        /* the torso's own anim does not count as set (1.3 too) */
        if ((ps->torsoTimer < 50) || force) {
            if (!isContinue || !((ps->torsoAnim & ~ANIM_TOGGLEBIT) == animNum)) {
                ps->torsoAnim = ((ps->torsoAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | animNum;
                if (setTimer) {
                    ps->torsoTimer = duration;
                }
                ps->torsoAnimDuration = duration;
            } else if (setTimer && globalScriptData->animations[animNum].flags & 0x80) {
                ps->torsoTimer = duration;
            }
        }

        break;

    default:
        break;
    }

    if (!wasSet) {
        return -1;
    }

    return duration;
}

static __attribute_regparm__(2) __attribute_sseregparm__
    void BG_SwingAngles(float *angle, int *swinging, float swingTolerance, float destination, float clampTolerance, float speed)
{
    float swing;
    float scale;
    float move;

    if (!*swinging) {
        swing = AngleSubtract(*angle, destination);
        if (swing > swingTolerance || swing < -swingTolerance) {
            *swinging = 1;
        }
    }

    if (!*swinging) {
        return;
    }

    swing = AngleSubtract(destination, *angle);
    scale = swing < 0.0f ? -swing : swing;
    scale *= 0.05000000074505806f;
    if (scale < 0.5f) {
        scale = 0.5f;
    }

    if (swing >= 0.0f) {
        move = (float)bgs->frametime * scale * speed;
        if (move >= swing) {
            move = swing;
            *swinging = 0;
        } else {
            *swinging = 1;
        }
    } else {
        move = (float)bgs->frametime * scale * -speed;
        if (move <= swing) {
            move = swing;
            *swinging = 0;
        } else {
            *swinging = 1;
        }
    }

    *angle = AngleMod(*angle + move);

    swing = AngleSubtract(destination, *angle);
    if (swing > clampTolerance) {
        *angle = AngleMod(destination - clampTolerance);
    } else if (swing < -clampTolerance) {
        *angle = AngleMod(destination + clampTolerance);
    }
}

void BG_UpdatePlayerDObj(struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci, int attachIgnoreCollision)
{
    DObjModel_s dobjModels[7];
    int iClientWeapon;
    struct XAnimTree_s *pAnimTree;
    int iNumModels;
    int i;

    iClientWeapon = (es->eFlags & 0x300) ? 0 : es->weapon;
    pAnimTree = ci->pXAnimTree;

    if (!ci->infoValid || !ci->model[0]) {
        XAnimClearTree(pAnimTree);
        bgs->SafeDObjFree(es->number);
        return;
    }

    if (pDObj && iClientWeapon == ci->iDObjWeapon) {
        if (!ci->dobjDirty)
            return;
    }

    bgs->SafeDObjFree(es->number);

    dobjModels[0].model = bgs->GetXModel(ci->model);
    dobjModels[0].boneName = NULL;
    dobjModels[0].ignoreCollision = 0;

    iNumModels = 1;
    for (i = 0; i < 6; ++i) {
        if (!ci->attachModelNames[i][0])
            continue;

        dobjModels[iNumModels].model = bgs->GetXModel(ci->attachModelNames[i]);
        dobjModels[iNumModels].boneName = ci->attachTagNames[i];
        dobjModels[iNumModels].ignoreCollision = (attachIgnoreCollision >> i) & 1;
        ++iNumModels;
    }

    ci->iDObjWeapon = iClientWeapon;
    bgs->CreateDObj(dobjModels, (unsigned short)iNumModels, pAnimTree, es->number, ci);
    ci->dobjDirty = 0;
}

void BG_LerpOffset(vec_t *offset_goal, float maxOffsetChange, vec_t *offset)
{
    float delta[3];
    float lenSq;
    float invLen;
    float scale;
    union {
        float f;
        int i;
    } y;

    delta[0] = offset_goal[0] - offset[0];
    delta[1] = offset_goal[1] - offset[1];
    delta[2] = offset_goal[2] - offset[2];
    lenSq = delta[0] * delta[0] + delta[1] * delta[1];
    lenSq += delta[2] * delta[2];

    if (lenSq == 0.0f) {
        return;
    }

    y.f = lenSq;
    y.i = 0x5f3759df - (y.i >> 1);
    invLen = y.f;
    invLen = invLen * (1.5f - (lenSq * 0.5f) * invLen * invLen);
    scale = invLen * maxOffsetChange;

    if (scale < 1.0f) {
        offset[0] += delta[0] * scale;
        offset[1] += delta[1] * scale;
        offset[2] += delta[2] * scale;
        return;
    }

    offset[0] = offset_goal[0];
    offset[1] = offset_goal[1];
    offset[2] = offset_goal[2];
}

void BG_UpdateConditionValue(int client, int condition, int value, qboolean checkConversion)
{
    if (checkConversion) {
        if (animConditionsTable[condition].type == ANIM_CONDTYPE_BITFLAGS) {
            bgs->clientinfo[client].clientConditions[condition][0] = 0;
            bgs->clientinfo[client].clientConditions[condition][1] = 0;

            Com_BitSet(bgs->clientinfo[client].clientConditions[condition], value);
            return;
        }
    }
    bgs->clientinfo[client].clientConditions[condition][0] = value;
}

/* 0x4f94b0 */
void BG_AnimUpdatePlayerStateConditions(pmove_t *pmove)
{
    WeaponDef *weaponDef;
    playerState_t *ps;

    ps = pmove->ps;

    weaponDef = BG_GetWeaponDef(BG_GetViewmodelWeaponIndex(ps));

    // WEAPON
    BG_UpdateConditionValue(ps->clientNum, ANIM_COND_PLAYERANIMTYPE, weaponDef->playerAnimType, qtrue);
    BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPONCLASS, weaponDef->weapClass, qtrue);

    if (ps->eFlags & EF_AIMDOWNSIGHT)
        BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON_POSITION, qtrue, qtrue);
    else
        BG_UpdateConditionValue(ps->clientNum, ANIM_COND_WEAPON_POSITION, qfalse, qtrue);

    // MOUNTED
    if (ps->eFlags & EF_TURRET_ACTIVE)
        BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue);
    else
        BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue);

    // UNDERHAND: the view's pitch (0x4f9668), not the weapon's position
    BG_UpdateConditionValue(ps->clientNum, ANIM_COND_UNDERHAND, ps->viewangles[PITCH] > 0, qtrue);

    if (pmove->cmd.buttons & BUTTON_ATTACK)
        BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FIRING, qtrue, qtrue);
    else
        BG_UpdateConditionValue(ps->clientNum, ANIM_COND_FIRING, qfalse, qtrue);
}

static inline __attribute__((always_inline)) void BG_SmoothControllerAngles(vec3_t current, const vec3_t goal, float maxChange)
{
    int axis;

    for (axis = 0; axis < 3; ++axis) {
        float delta = goal[axis] - current[axis];

        if (delta > maxChange) {
            current[axis] += maxChange;
        } else if (delta < -maxChange) {
            current[axis] -= maxChange;
        } else {
            current[axis] = goal[axis];
        }
    }
}

static inline __attribute__((always_inline)) unsigned int BG_ControllerTagName(int controllerIndex)
{
    return *(const unsigned short *)controller_names[controllerIndex];
}

void BG_Player_DoControllers(const struct DObj_s *pDObj, const entityState_t *es, int *partBits, clientInfo_t *ci, int frametime)
{
    vec3_t goals[8];
    vec3_t tagOriginAngles;
    vec3_t torsoAngles;
    vec3_t headAngles;
    float leanFrac;
    float leanDegrees;
    float waistDelta;
    float maxAngleChange;
    int i;

    if ((es->eFlags & 0x3) != 0) {
        memset(goals, 0, sizeof(goals));
    } else {
        tagOriginAngles[0] = 0.0f;
        tagOriginAngles[1] = ci->legs.yawAngle;
        tagOriginAngles[2] = 0.0f;

        torsoAngles[0] = 0.0f;
        torsoAngles[1] = ci->torso.yawAngle;
        torsoAngles[2] = 0.0f;

        headAngles[0] = ci->playerAngles[0];
        headAngles[1] = ci->playerAngles[1];
        headAngles[2] = ci->playerAngles[2];

        if ((ci->clientConditions[3][0] & 0xC0000) == 0) {
            torsoAngles[0] = ci->torso.pitchAngle;
            if ((es->eFlags & 8) != 0) {
                torsoAngles[0] = AngleNormalize180(torsoAngles[0]);
                torsoAngles[0] *= (torsoAngles[0] > 0.0f) ? 0.5f : 0.25f;
            }
        }

        AnglesSubtract(headAngles, torsoAngles, headAngles);
        AnglesSubtract(torsoAngles, tagOriginAngles, torsoAngles);

        goals[7][2] = es->fTorsoHeight;

        leanFrac = GetLeanFraction(ci->lerpLean);
        leanDegrees = leanFrac * 50.0f;
        torsoAngles[2] = leanDegrees * 0.925000011920929f;
        headAngles[2] = leanDegrees * 0.925000011920929f;

        if (leanFrac != 0.0f) {
            if ((es->eFlags & 4) != 0) {
                goals[7][1] = (leanFrac > 0.0f) ? (leanFrac * -2.5f) : (leanFrac * -12.5f);
            } else {
                goals[7][1] = (leanFrac > 0.0f) ? (leanFrac * -2.5f) : (leanFrac * -5.0f);
            }
        } else {
            goals[7][1] = 0.0f;
        }

        if ((es->eFlags & 0x20000) == 0) {
            tagOriginAngles[1] = AngleSubtract(tagOriginAngles[1], ci->playerAngles[1]);
        }

        if ((es->eFlags & 8) != 0) {
            float radians;
            float sinYaw;
            float oneMinusCos;

            if (leanFrac != 0.0f) {
                headAngles[2] *= 0.5f;
            }

            tagOriginAngles[0] += es->fTorsoPitch;
            radians = torsoAngles[1] * 0.017453292519943295f;
            sinYaw = sinf(radians);
            oneMinusCos = 1.0f - cosf(radians);

            goals[7][0] = oneMinusCos * -24.0f;
            goals[7][1] += sinYaw * -12.0f;
            if ((sinYaw * leanFrac) > 0.0f) {
                goals[7][1] += -leanFrac * oneMinusCos * 16.0f;
            }

            goals[0][0] = 0.0f;
            goals[0][1] = torsoAngles[2] * -1.2000000476837158f;
            goals[0][2] = torsoAngles[2] * 0.30000001192092896f;

            if (es->fTorsoPitch != 0.0f || es->fWaistPitch != 0.0f) {
                goals[0][0] += AngleSubtract(es->fWaistPitch, es->fTorsoPitch);
            }

            goals[1][0] = 0.0f;
            goals[1][1] = torsoAngles[1] * 0.10000000149011612f + torsoAngles[2] * -0.20000000298023224f;
            goals[1][2] = torsoAngles[2] * 0.20000000298023224f;

            goals[2][0] = torsoAngles[0];
            goals[2][1] = torsoAngles[1] * 0.800000011920929f + torsoAngles[2];
            goals[2][2] = torsoAngles[2] * -0.20000000298023224f;
        } else {
            if (leanFrac != 0.0f) {
                if ((es->eFlags & 4) == 0 || !(leanFrac > 0.0f)) {
                    torsoAngles[2] *= 1.25f;
                    headAngles[2] *= 1.25f;
                }
            }

            tagOriginAngles[2] += leanDegrees * 0.07500000298023224f;

            goals[0][0] = torsoAngles[0] * 0.20000000298023224f;
            goals[0][1] = torsoAngles[1] * 0.4000000059604645f;
            goals[0][2] = torsoAngles[2] * 0.5f;

            if (es->fTorsoPitch != 0.0f || es->fWaistPitch != 0.0f) {
                goals[0][0] += AngleSubtract(es->fWaistPitch, es->fTorsoPitch);
            }

            goals[1][0] = torsoAngles[0] * 0.30000001192092896f;
            goals[1][1] = torsoAngles[1] * 0.4000000059604645f;
            goals[1][2] = torsoAngles[2] * 0.5f;

            goals[2][0] = torsoAngles[0] * 0.5f;
            goals[2][1] = torsoAngles[1] * 0.20000000298023224f;
            goals[2][2] = torsoAngles[2] * -0.6000000238418579f;

            goals[7][0] = 0.0f;
        }

        goals[3][0] = headAngles[0] * 0.30000001192092896f;
        goals[3][1] = headAngles[1] * 0.30000001192092896f;
        goals[3][2] = 0.0f;

        goals[4][0] = headAngles[0] * 0.699999988079071f;
        goals[4][1] = headAngles[1] * 0.699999988079071f;
        goals[4][2] = headAngles[2] * -0.30000001192092896f;

        waistDelta = 0.0f;
        if (es->fTorsoPitch != 0.0f || es->fWaistPitch != 0.0f) {
            waistDelta = AngleSubtract(es->fWaistPitch, es->fTorsoPitch);
        }
        goals[5][0] = waistDelta;
        goals[5][1] = 0.0f;
        goals[5][2] = 0.0f;

        goals[6][0] = tagOriginAngles[0];
        goals[6][1] = tagOriginAngles[1];
        goals[6][2] = tagOriginAngles[2];
    }

    maxAngleChange = (float)frametime * 0.36000001430511475f;

    for (i = 0; i < 6; ++i) {
        BG_SmoothControllerAngles(ci->angles[i], goals[i], maxAngleChange);
        DObjSetControlTagAngles(pDObj, partBits, BG_ControllerTagName(i), ci->angles[i]);
    }

    BG_SmoothControllerAngles(ci->tag_origin_angles, goals[6], maxAngleChange);
    BG_LerpOffset(goals[7], (float)frametime * 0.10000000149011612f, ci->tag_origin_offset);
    DObjSetLocalTag(pDObj, partBits, ((const scr_const_t *)scr_const_g)->tag_origin,
                    ci->tag_origin_offset, ci->tag_origin_angles);
}

static int BG_AnimIndexNoToggle(int animIndex)
{
    return animIndex & ~0x200;
}

static animation_t *BG_GetAnimationForIndexChecked(int animIndex)
{
    if ((unsigned int)animIndex >= (unsigned int)globalScriptData->numAnimations) {
        Com_Error(1, "\025BG_GetAnimationForIndex: index out of bounds");
    }

    return &globalScriptData->animations[animIndex];
}

static qboolean BG_AnimationHasMoveClass(int animIndex)
{
    animation_t *anim = BG_GetAnimationForIndexChecked(animIndex);
    return (anim->movetype & ((((long long)0x180) << 32) | 0xc4LL)) != 0;
}

static qboolean BG_AnimationHasTurnClass(int animIndex)
{
    animation_t *anim = BG_GetAnimationForIndexChecked(animIndex);
    return (anim->movetype & 0x308LL) != 0;
}

static float BG_LerpAnimBlendSeconds(const lerpFrame_t *lf)
{
    return (float)lf->animationTime * 0.0010000000474974513f;
}

static void BG_CopyLerpFramePosition(lerpFrame_t *lf, const entityState_t *es)
{
    lf->oldFramePos[0] = es->pos.trBase[0];
    lf->oldFramePos[1] = es->pos.trBase[1];
    lf->oldFramePos[2] = es->pos.trBase[2];
}

static float BG_Fabsf(float value)
{
    return value < 0.0f ? -value : value;
}

static void __attribute_regparm__(3) BG_RunLerpFrameRate(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, entityState_t *es)
{
    XAnimTree_s *pAnimTree;
    struct XAnim_s *pXAnims;
    animation_t *oldAnimation;
    animation_t *anim;
    int oldAnimationNumber;
    int oldAnimIndex;
    int animNum;
    int bLegsAnim;
    int hadNoAnimation;
    qboolean oldAnimationUsesZOnly;
    float startTime;

    oldAnimation = lf->animation;
    oldAnimationUsesZOnly = (oldAnimation != NULL && (oldAnimation->flags & 2) != 0);
    pAnimTree = ci->pXAnimTree;
    oldAnimationNumber = lf->animationNumber;

    if (oldAnimationNumber == newAnimation) {
        if (oldAnimation != NULL) {
            animNum = BG_AnimIndexNoToggle(oldAnimationNumber);
        } else {
            if (BG_AnimIndexNoToggle(oldAnimationNumber) == 0) {
                return;
            }
            animNum = BG_AnimIndexNoToggle(oldAnimationNumber);
        }
    } else {
        unsigned int maxAnimations;
        int oldMoveClass;
        int newMoveClass;
        int oldTurnClass;
        int newTurnClass;

        bLegsAnim = (lf == &ci->legs);
        hadNoAnimation = (oldAnimation == NULL);
        lf->animationNumber = newAnimation;
        animNum = BG_AnimIndexNoToggle(newAnimation);

        maxAnimations = (unsigned int)bgs->animScriptData.numAnimations;
        if ((unsigned int)animNum >= maxAnimations) {
            Com_Error(1, "\025Player animation index out of range (%i): %i", maxAnimations, animNum);
        }

        pXAnims = bgs->animScriptData.animTree.anims;
        if (animNum != 0) {
            anim = &bgs->animScriptData.animations[animNum];
            lf->animation = anim;
            lf->animationTime = anim->initialLerp;
        } else {
            anim = NULL;
            lf->animation = NULL;
            lf->animationTime = 200;
        }

        if (animNum != 0) {
            newMoveClass = BG_AnimationHasMoveClass(animNum);
            oldAnimIndex = BG_AnimIndexNoToggle(oldAnimationNumber);
            oldMoveClass = BG_AnimationHasMoveClass(oldAnimIndex);
            newTurnClass = BG_AnimationHasTurnClass(animNum);
            oldTurnClass = BG_AnimationHasTurnClass(oldAnimIndex);

            if (bLegsAnim && (newMoveClass != oldMoveClass || newTurnClass != oldTurnClass)) {
                ci->stanceTransitionTime = bgs->time + 400;
            }
        }

        if (hadNoAnimation && bLegsAnim) {
            lf->animationTime = 0;
        } else {
            int maxBlendMsec;
            float zero = 0.0f;

            if (anim == NULL) {
                maxBlendMsec = (hadNoAnimation || oldAnimation == NULL || oldAnimation->moveSpeed == zero) ? 170 : 250;
            } else if (lf->animationTime <= 0) {
                maxBlendMsec = (anim->moveSpeed == zero) ? ((hadNoAnimation || oldAnimation == NULL || oldAnimation->moveSpeed == zero) ? 170 : 250) : 120;
            } else {
                maxBlendMsec = -1;
            }

            if (maxBlendMsec >= 0) {
                int elapsed = bgs->time - ci->stanceTransitionTime;
                if (elapsed > maxBlendMsec) {
                    elapsed = maxBlendMsec;
                }
                if (elapsed > lf->animationTime) {
                    lf->animationTime = elapsed;
                }
            }
        }

        startTime = 0.0f;
        if (anim != NULL && anim->moveSpeed != 0.0f) {
            if (XAnimIsLooped(pXAnims, animNum)) {
                oldAnimIndex = BG_AnimIndexNoToggle(oldAnimationNumber);
                if (hadNoAnimation || oldAnimation == NULL || oldAnimation->moveSpeed != 0.0f || !XAnimIsLooped(pXAnims, oldAnimIndex)) {
                    if (XAnimIsPrimitive(pXAnims, oldAnimIndex)) {
                        int lengthMsec = XAnimGetLengthMsec(pXAnims, oldAnimIndex) + 200;
                        float length = (float)lengthMsec;
                        int msec = bgs->time % lengthMsec;
                        startTime = ((float)msec / length) + ((float)ci->clientNum * 0.36000001430511475f);
                        startTime -= (float)(int)startTime;
                    } else {
                        int msec = bgs->time % 1000;
                        startTime = ((float)msec / 1000.0f) + ((float)ci->clientNum * 0.36000001430511475f);
                        startTime -= (float)(int)startTime;
                    }
                } else {
                    startTime = XAnimGetTime(pAnimTree, oldAnimIndex);
                }
            }
        }

        if (oldAnimation != NULL) {
            oldAnimIndex = BG_AnimIndexNoToggle(oldAnimationNumber);
            XAnimClearGoalWeight(pAnimTree, oldAnimIndex, BG_LerpAnimBlendSeconds(lf));
        }

        if (animNum == 0) {
            if (!bLegsAnim) {
                XAnimSetCompleteGoalWeight(pAnimTree, bgs->animScriptData.torsoAnim, 0.0f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, 0, 0);
                XAnimSetCompleteGoalWeight(pAnimTree, bgs->animScriptData.legsAnim, 1.0f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, 0, 0);
            }
            return;
        }

        if (!bLegsAnim) {
            ci->leftHandGun = 0;
            ci->dobjDirty = 1;
        }

        if (bLegsAnim && (anim->flags & 0x40) != 0) {
            if (XAnimIsLooped(pXAnims, animNum)) {
                Com_Error(1, "\025death animation '%s' is looping", anim->name);
            }

            if ((es->eFlags & 0x80000) != 0) {
                XAnimSetCompleteGoalWeight(pAnimTree, animNum, 1.0f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, anim->noteType, 0);
                goto after_set_goal;
            }

            XAnimSetCompleteGoalWeightKnobAll(pAnimTree, animNum, bgs->generic_human.root.index, 1.0f, 0.0f, 1.0f, 0, 0);
            XAnimSetTime(pAnimTree, animNum, 1.0f);
            goto after_set_goal;
        }

        if (anim->moveSpeed != 0.0f && XAnimGetWeight(pAnimTree, animNum) == 0.0f) {
            XAnimSetCompleteGoalWeight(pAnimTree, animNum, 1.0f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, anim->noteType, !bLegsAnim);
            XAnimSetTime(pAnimTree, animNum, startTime);
        } else {
            XAnimSetCompleteGoalWeight(pAnimTree, animNum, 1.0f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, anim->noteType, !bLegsAnim);
        }

        if (!bLegsAnim) {
            XAnimSetCompleteGoalWeight(pAnimTree, bgs->animScriptData.torsoAnim, 1.0f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, anim->noteType, 0);
            XAnimSetCompleteGoalWeight(pAnimTree, bgs->animScriptData.legsAnim, 0.009999999776482582f, BG_LerpAnimBlendSeconds(lf), 1.0f, 0, anim->noteType, 0);
        }

    after_set_goal:;
    }

    if (animNum == 0) {
        return;
    }

    anim = lf->animation;
    if (anim == NULL || anim->moveSpeed == 0.0f || lf->oldFrameSnapshotTime == 0 || lf->oldFrameSnapshotTime == bgs->latestSnapshotTime) {
        lf->animSpeedScale = 1.0f;
        lf->oldFrameSnapshotTime = bgs->latestSnapshotTime;
        BG_CopyLerpFramePosition(lf, es);
    } else {
        int deltaMsec;
        float dist;
        float seconds;

        if (oldAnimationUsesZOnly) {
            dist = BG_Fabsf(lf->oldFramePos[2] - es->pos.trBase[2]);
        } else {
            dist = Vec3Distance(lf->oldFramePos, es->pos.trBase);
        }

        deltaMsec = bgs->latestSnapshotTime - lf->oldFrameSnapshotTime;
        seconds = (float)deltaMsec * 0.0010000000474974513f;
        lf->animSpeedScale = (dist / seconds) / anim->moveSpeed;
        lf->oldFrameSnapshotTime = bgs->latestSnapshotTime;
        BG_CopyLerpFramePosition(lf, es);

        if (lf->animSpeedScale < 0.10000000149011612f) {
            if (lf->animSpeedScale < 0.009999999776482582f && oldAnimationUsesZOnly) {
                lf->animSpeedScale = 0.0f;
            } else {
                lf->animSpeedScale = 0.10000000149011612f;
            }
        } else if (lf->animSpeedScale > 2.0f) {
            if ((anim->flags & 2) != 0) {
                if (lf->animSpeedScale > 4.0f) {
                    lf->animSpeedScale = 4.0f;
                }
            } else if (anim->moveSpeed > 150.0f) {
                lf->animSpeedScale = 2.0f;
            } else if (anim->moveSpeed < 20.0f) {
                if (lf->animSpeedScale > 3.0f) {
                    lf->animSpeedScale = 3.0f;
                }
            } else {
                float fScaleMax = ((anim->moveSpeed - 20.0f) / -130.0f) + 3.0f;
                if (lf->animSpeedScale > fScaleMax) {
                    lf->animSpeedScale = fScaleMax;
                }
            }
        }
    }

    if (lf->animationNumber != 0) {
        XAnimSetAnimRate(pAnimTree, BG_AnimIndexNoToggle(lf->animationNumber), lf->animSpeedScale);
    }
}

static void BG_ClearFinishedLerpFrame(const XAnimTree_s *pAnimTree, lerpFrame_t *lf)
{
    if (lf->animationNumber == 0) {
        return;
    }

    if (XAnimGetWeight(pAnimTree, BG_AnimIndexNoToggle(lf->animationNumber)) != 0.0f) {
        return;
    }

    lf->animationNumber = 0;
    lf->animation = NULL;
    lf->animationTime = 150;
}

static qboolean BG_ConditionValueIsActive(const clientInfo_t *ci, int condition)
{
    int bit;

    if (animConditionsTable[condition].type != ANIM_CONDTYPE_BITFLAGS) {
        return ci->clientConditions[condition][0] != 0;
    }

    for (bit = 1; bit < 64; ++bit) {
        if ((ci->clientConditions[condition][bit >> 5] & (1 << (bit & 31))) != 0) {
            return 1;
        }
    }

    return 0;
}

static void BG_UpdateEntityPlayerAnimConditions(const entityState_t *es, const clientInfo_t *ci)
{
    int client;
    WeaponDef *weaponDef;
    int legsAnim;
    long long movetype;

    weaponDef = BG_GetWeaponDef(es->weapon);
    client = es->clientNum;

    BG_UpdateConditionValue(client, 0, weaponDef->playerAnimType, 1);
    BG_UpdateConditionValue(client, 1, weaponDef->weapClass, 1);
    BG_UpdateConditionValue(client, 7, (es->eFlags & 0x40000) != 0, 1);
    BG_UpdateConditionValue(client, 2, (es->eFlags & 0x300) != 0, 1);
    BG_UpdateConditionValue(client, 4, ci->playerAngles[0] > 0.0f, 1);
    BG_UpdateConditionValue(client, 5, (es->eFlags & 4) != 0, 1);
    BG_UpdateConditionValue(client, 6, (es->eFlags & 0x40) != 0, 1);

    legsAnim = BG_AnimIndexNoToggle(es->legsAnim);
    movetype = bgs->animScriptData.animations[legsAnim].movetype;
    if (movetype != 0 && movetype != (long long)(int)bgs->clientinfo[client].clientConditions[3][0]) {
        bgs->clientinfo[client].clientConditions[3][0] = (int)movetype;
    }

    if ((bgs->animScriptData.animations[legsAnim].flags & 0x10) != 0) {
        BG_UpdateConditionValue(client, 8, 1, 1);
    } else if ((bgs->animScriptData.animations[legsAnim].flags & 0x20) != 0) {
        BG_UpdateConditionValue(client, 8, 2, 1);
    } else {
        BG_UpdateConditionValue(client, 8, 0, 1);
    }
}

void BG_PlayerAnimation(const struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci)
{
    float moveDir;
    float playerPitch;
    float playerYaw;
    float moveYaw;
    float torsoYawDest;
    float torsoYawClamp;
    float legsYawDest;
    float legsSwingTolerance;
    float torsoPitchDest;
    float swingSpeed;
    XAnimTree_s *pAnimTree;
    int eFlags;

    (void)pDObj;
    (void)GetLeanFraction(ci->lerpLean);

    moveDir = ci->lerpMoveDir;
    playerPitch = ci->playerAngles[0];
    playerYaw = AngleMod(ci->playerAngles[1]);
    eFlags = es->eFlags;

    if ((eFlags & 0x3) != 0) {
        ci->torso.yawing = 1;
        ci->torso.pitching = 1;
        ci->legs.yawing = 1;
    } else if ((ci->clientConditions[3][0] & 0xc0000) != 0) {
        ci->torso.yawing = 1;
        ci->torso.pitching = 1;
        ci->legs.yawing = 1;
    } else if ((eFlags & 0x40) != 0) {
        ci->torso.yawing = 1;
        ci->torso.pitching = 1;
        ci->legs.yawing = 1;
    } else if ((ci->clientConditions[3][0] & 6) != 0 && BG_ConditionValueIsActive(ci, 6)) {
        ci->torso.yawing = 1;
        ci->torso.pitching = 1;
    } else {
        ci->torso.yawing = 1;
        ci->torso.pitching = 1;
        ci->legs.yawing = 1;
    }

    moveYaw = moveDir + playerYaw;
    swingSpeed = (bg_swingSpeed)->current.value;

    if ((eFlags & 0x20000) != 0) {
        torsoYawDest = playerYaw;
        torsoYawClamp = 90.0f;
        legsYawDest = playerYaw;
        legsSwingTolerance = 0.0f;
    } else if ((ci->clientConditions[3][0] & 0xc0000) != 0) {
        torsoYawDest = moveYaw;
        torsoYawClamp = 0.0f;
        legsYawDest = moveYaw;
        legsSwingTolerance = 0.0f;
    } else if ((eFlags & 0x4000) != 0) {
        torsoYawDest = playerYaw;
        torsoYawClamp = 90.0f;
        legsYawDest = playerYaw;
        legsSwingTolerance = 0.0f;
    } else if ((eFlags & 8) != 0 || (eFlags & 0x40000) != 0) {
        torsoYawDest = playerYaw;
        torsoYawClamp = 90.0f;
        legsYawDest = moveYaw;
        legsSwingTolerance = 0.0f;
    } else if ((eFlags & 0x40) != 0) {
        torsoYawDest = playerYaw;
        torsoYawClamp = 45.0f;
        legsYawDest = moveYaw;
        legsSwingTolerance = 0.0f;
    } else {
        torsoYawDest = playerYaw + moveDir * 0.30000001192092896f;
        torsoYawClamp = 90.0f;
        legsYawDest = moveYaw;
        legsSwingTolerance = 40.0f;
    }

    BG_SwingAngles(&ci->torso.yawAngle, &ci->torso.yawing, 0.0f, torsoYawDest, torsoYawClamp, swingSpeed);

    if ((eFlags & 0x20000) != 0) {
        BG_SwingAngles(&ci->legs.yawAngle, &ci->legs.yawing, 0.0f, legsYawDest, 150.0f, swingSpeed);
    } else if ((eFlags & 8) != 0) {
        ci->legs.yawing = 0;
        ci->legs.yawAngle = moveYaw;
    } else if ((bgs->animScriptData.animations[BG_AnimIndexNoToggle(es->legsAnim)].flags & 0x30) != 0) {
        ci->legs.yawing = 0;
        BG_SwingAngles(&ci->legs.yawAngle, &ci->legs.yawing, 0.0f, playerYaw, 150.0f, swingSpeed);
    } else {
        BG_SwingAngles(&ci->legs.yawAngle, &ci->legs.yawing, legsSwingTolerance, legsYawDest, 150.0f, swingSpeed);
    }

    if ((eFlags & 0x3) != 0) {
        ci->torso.yawAngle = playerYaw;
        ci->legs.yawAngle = playerYaw;
    }

    if ((ci->clientConditions[3][0] & 0xc0000) != 0) {
        ci->torso.yawAngle = moveYaw;
        ci->legs.yawAngle = moveYaw;
    }

    if ((eFlags & 0x20000) != 0 || (eFlags & 0x3) != 0 || (ci->clientConditions[3][0] & 0xc0000) != 0 || eFlags == 0x4000) {
        torsoPitchDest = 0.0f;
    } else {
        torsoPitchDest = playerPitch;
        if (torsoPitchDest > 180.0f) {
            torsoPitchDest -= 360.0f;
        }
        torsoPitchDest *= 0.6000000238418579f;
    }

    BG_SwingAngles(&ci->torso.pitchAngle, &ci->torso.pitching, 0.0f, torsoPitchDest, 45.0f, 0.15000000596046448f);

    BG_UpdateEntityPlayerAnimConditions(es, ci);

    pAnimTree = ci->pXAnimTree;
    BG_ClearFinishedLerpFrame(pAnimTree, &ci->legs);
    BG_ClearFinishedLerpFrame(pAnimTree, &ci->torso);

    if (ci->leftHandGun && BG_AnimIndexNoToggle(ci->torso.animationNumber) == 0) {
        ci->leftHandGun = 0;
        ci->dobjDirty = 1;
    }

    BG_RunLerpFrameRate(ci, &ci->legs, es->legsAnim, es);
    BG_RunLerpFrameRate(ci, &ci->torso, es->torsoAnim, es);
}

int BG_AnimationIndexForString(const char *string, const char *string_1)
{
    int hash;
    int i;

    if (g_pLoadAnims) {
        loadAnim_t *pAnim;

        hash = BG_StringHash(string);
        pAnim = g_pLoadAnims;
        for (i = 0; i < *g_piNumLoadAnims; ++i, ++pAnim) {
            if (pAnim->iNameHash == hash && !I_stricmp(string, pAnim->szAnimName)) {
                return i;
            }
        }

        pAnim = &g_pLoadAnims[*g_piNumLoadAnims];
        Scr_FindAnim("multiplayer", string, &pAnim->anim, bgs->anim_user);
        strcpy(pAnim->szAnimName, string);
        pAnim->iNameHash = hash;
        i = *g_piNumLoadAnims;
        *g_piNumLoadAnims = i + 1;
        return i;
    }

    hash = BG_StringHash(string);
    for (i = 0; i < globalScriptData->numAnimations; ++i) {
        if (globalScriptData->animations[i].nameHash == hash &&
            !I_stricmp(string, globalScriptData->animations[i].name)) {
            return i;
        }
    }

    BG_AnimParseError("BG_AnimationIndexForString: unknown player animation '%s'", string);
    return -1;
}

static inline __attribute__((always_inline)) int BG_StringHash(const char *str)
{
    int hash = 0;
    int i;

    for (i = 0; str[i] != '\0'; ++i)
        hash += (signed char)___tolower((signed char)str[i]) * (i + 0x77);

    if (hash == -1) {
        hash = 0;
    }

    return hash;
}

void BG_AnimParseError(const char *msg, ...)
{
    char text[1024];
    va_list argptr;

    va_start(argptr, msg);
    vsnprintf(text, sizeof(text), msg, argptr);
    va_end(argptr);

    if (globalFilename)
        Com_Error(1, "\x15%s: (%s, line %i)", text, globalFilename,
                  Com_GetCurrentParseLine() + 1);
    else
        Com_Error(1, "\x15%s", text);
}

int BG_IndexForString(const char *token, animStringItem_t *strings, qboolean allowFail)
{
    int i;
    int hash = BG_StringHash(token);

    for (i = 0; strings[i].string; ++i) {
        if (strings[i].hash == -1) {
            strings[i].hash = BG_StringHash(strings[i].string);
        }

        if (strings[i].hash == hash && !I_stricmp(token, strings[i].string)) {
            return i;
        }
    }

    if (!allowFail) {
        BG_AnimParseError("BG_IndexForString: unknown token '%s'", token);
    }

    return -1;
}

static void __attribute_regparm__(3)
    BG_ParseConditionBits(const char **text_pp, animStringItem_t *stringTable, int condIndex, int *result)
{
    char currentString[64];
    int tempBits[2];
    int minus;

    currentString[0] = '\0';
    result[0] = 0;
    tempBits[0] = 0;
    tempBits[1] = 0;
    minus = 0;

    for (;;) {
        const char *token;
        int endFlag;

        token = Com_ParseOnLine(text_pp);
        if (!token || !*token) {
            Com_UngetToken();
            if (!currentString[0]) {
                return;
            }
            endFlag = 1;
        } else {
            endFlag = (I_stricmp(token, ",") == 0);
        }

        if (!I_stricmp(token, "none")) {
            result[0] |= 1;
            if (endFlag) {
                return;
            }
            continue;
        }

        if (!I_stricmp(token, "none,")) {
            result[0] |= 1;
            return;
        }

        if (!I_stricmp(token, "NOT")) {
            token = "MINUS";
        }

        if (!endFlag) {
            if (!I_stricmp(token, "AND") || !I_stricmp(token, "MINUS")) {
                endFlag = 0;
            } else {
                char *mutableToken = (char *)token;
                int len = strlen(mutableToken);

                if (len > 0 && mutableToken[len - 1] == ',') {
                    mutableToken[len - 1] = '\0';
                    endFlag = 1;
                }

                if (currentString[0]) {
                    I_strncat(currentString, 64, " ");
                }
                I_strncat(currentString, 64, mutableToken);

                if (!I_stricmp(token, "AND") || !I_stricmp(token, "MINUS") || endFlag) {
                } else {
                    continue;
                }
            }
        }

        if (!currentString[0]) {
            if (endFlag) {
                BG_AnimParseError("BG_ParseConditionBits: unexpected end of condition");
            }

            if (!I_stricmp(token, "MINUS")) {
                minus = 1;
                continue;
            }

            BG_AnimParseError("BG_ParseConditionBits: unexpected '%s'", token);
        } else {
            if (!I_stricmp(currentString, "all")) {
                tempBits[0] = -1;
                tempBits[1] = -1;
            } else {
                int defineIndex = BG_IndexForString(currentString, defineStr[condIndex], 1);   /* was hardcoded condIndex<<7 (x86 128-byte row) -> wrong row on x64 (256-byte rows) */

                if (defineIndex >= 0) {
                    tempBits[0] = defineBits[condIndex][defineIndex][0];
                    tempBits[1] = defineBits[condIndex][defineIndex][1];
                } else {
                    int bitNum = BG_IndexForString(currentString, stringTable, 0);
                    tempBits[bitNum >> 5] |= 1 << (bitNum & 31);
                }
            }

            if (minus) {
                result[0] &= ~tempBits[0];
                result[1] &= ~tempBits[1];
            } else {
                result[0] |= tempBits[0];
                result[1] |= tempBits[1];
            }

            currentString[0] = '\0';
            if (!I_stricmp(token, "MINUS")) {
                minus = 1;
            }
        }

        if (endFlag) {
            return;
        }
    }
}

static qboolean __attribute_regparm__(2)
    BG_ParseConditions(const char **text_pp, animScriptItem_t *scriptItem)
{
    int conditionValue[2];

    conditionValue[0] = 0;
    conditionValue[1] = 0;

    for (;;) {
        const char *token = Com_ParseOnLine(text_pp);
        int conditionIndex;

        if (!token || !*token) {
            if (!scriptItem->numConditions) {
                BG_AnimParseError("BG_ParseConditions: no conditions found");
            }
            return 1;
        }

        if (!I_stricmp(token, "default")) {
            return 1;
        }

        conditionIndex = BG_IndexForString(token, animConditionsStr, 0);

        if (animConditionsTable[conditionIndex].type == ANIM_CONDTYPE_BITFLAGS) {
            BG_ParseConditionBits(text_pp, animConditionsTable[conditionIndex].values, conditionIndex, conditionValue);
        } else if (animConditionsTable[conditionIndex].type == ANIM_CONDTYPE_VALUE) {
            char *valueToken;
            int len;

            if (!animConditionsTable[conditionIndex].values) {
                conditionValue[0] = 1;
            } else {
                valueToken = (char *)Com_ParseOnLine(text_pp);
                if (!valueToken || !*valueToken) {
                    BG_AnimParseError("BG_ParseConditions: expected condition value, found end of line");
                }

                len = strlen(valueToken);
                if (len > 0 && valueToken[len - 1] == ',') {
                    valueToken[len - 1] = '\0';
                }

                conditionValue[0] = BG_IndexForString(valueToken, animConditionsTable[conditionIndex].values, 0);
            }
        } else {
            BG_AnimParseError("BG_ParseConditions: expected condition value, found end of line");
        }

        scriptItem->conditions[scriptItem->numConditions].index = conditionIndex;
        scriptItem->conditions[scriptItem->numConditions].value[0] = conditionValue[0];
        scriptItem->conditions[scriptItem->numConditions].value[1] = conditionValue[1];
        ++scriptItem->numConditions;
    }
}

void BG_ParseCommands(const char **input, animScriptItem_t *scriptItem, animScriptData_t *scriptData)
{
    animScriptCommand_t *command = NULL;
    int commandPart = 0;

    for (;;) {
        const char *token = Com_Parse(input);

        if (!token || !*token) {
            return;
        }

        if (!I_stricmp(token, "}")) {
            *input -= strlen(token);
            return;
        }

        if (!commandPart) {
            if (scriptItem->numCommands > 7) {
                BG_AnimParseError("BG_ParseCommands: exceeded maximum number of animations (%i)", 8);
            }

            command = &scriptItem->commands[scriptItem->numCommands++];
            command->soundAlias = NULL;
        }

        command->bodyPart[commandPart] = BG_IndexForString(token, animBodyPartsStr, 1);

        if (command->bodyPart[commandPart] > 0) {
            int animIndex;
            animation_t *anim;

            token = Com_ParseOnLine(input);
            if (!token || !*token) {
                BG_AnimParseError("BG_ParseCommands: expected animation");
            }

            animIndex = BG_AnimationIndexForString(token, NULL);
            command->animIndex[commandPart] = animIndex;
            command->animDuration[commandPart] = scriptData->animations[animIndex].duration;

            if (!g_pLoadAnims) {
                anim = &scriptData->animations[animIndex];

                if (parseMovetype && command->bodyPart[commandPart] != 2) {
                    anim->movetype |= 1LL << parseMovetype;

                    if ((unsigned int)(parseMovetype - 18) <= 1 && anim->moveSpeed != 0.0f) {
                        anim->flags |= 2;
                    }

                    if (scriptItem->numConditions > 0) {
                        int conditionIndex;

                        for (conditionIndex = 0; conditionIndex < scriptItem->numConditions; ++conditionIndex) {
                            if (scriptItem->conditions[conditionIndex].index == 8) {
                                if (scriptItem->conditions[conditionIndex].value[0] == 1) {
                                    anim->flags |= 0x10;
                                } else if (scriptItem->conditions[conditionIndex].value[0] == 2) {
                                    anim->flags |= 0x20;
                                }
                                break;
                            }
                        }
                    }
                }

                if (parseEvent == 2) {
                    anim->flags |= 8;
                    anim->initialLerp = 30;
                } else if (parseEvent == 1) {
                    anim->moveSpeed = 0.0f;
                    anim->flags |= 0x40;
                } else if (parseEvent == 10 || (unsigned int)(parseMovetype - 21) <= 9) {
                    anim->moveSpeed = 0.0f;
                }
            }

            for (;;) {
                token = Com_ParseOnLine(input);
                if (!token || !*token) {
                    break;
                }

                if (!I_stricmp(token, "duration")) {
                    token = Com_ParseOnLine(input);
                    if (!token || !*token) {
                        BG_AnimParseError("BG_ParseCommands: expected duration value");
                    }
                    command->animDuration[commandPart] = atoi(token);
                    continue;
                }

                if (!I_stricmp(token, "turretanim")) {
                    if (!g_pLoadAnims) {
                        scriptData->animations[command->animIndex[commandPart]].flags |= 4;
                    }

                    if (command->bodyPart[commandPart] != 3) {
                        BG_AnimParseError("BG_ParseCommands: Turret animations can only be played on the 'both' body part");
                    }
                    continue;
                }

                if (!I_stricmp(token, "blendtime")) {
                    token = Com_ParseOnLine(input);
                    if (!token || !*token) {
                        BG_AnimParseError("BG_ParseCommands: expected blendtime value");
                    }

                    if (!g_pLoadAnims) {
                        scriptData->animations[command->animIndex[commandPart]].initialLerp = atoi(token);
                    }
                    continue;
                }

                Com_UngetToken();
                break;
            }
        } else {
            *input -= strlen(token);
        }

        if (command->bodyPart[commandPart] != 3) {
            ++commandPart;
            if (commandPart <= 1) {
                if (commandPart > 0) {
                    token = Com_ParseOnLine(input);
                    if (!token || !*token) {
                        return;
                    }

                    *input -= strlen(token);
                }
                continue;
            }
        }

        for (;;) {
            token = Com_ParseOnLine(input);
            if (!token || !*token) {
                break;
            }

            if (I_stricmp(token, "sound")) {
                BG_AnimParseError("BG_ParseCommands: unknown parameter '%s'", token);
                continue;
            }

            token = Com_ParseOnLine(input);
            if (!token || !*token) {
                BG_AnimParseError("BG_ParseCommands: expected sound");
            }

            if (strstr(token, ".wav")) {
                BG_AnimParseError("BG_ParseCommands: wav files not supported, only sound scripts");
            }

            command->soundAlias = globalScriptData->soundAlias(token);
        }

        commandPart = 0;
    }
}

static void __attribute_regparm__(3)
    BG_AnimParseAnimScript(animScriptData_t *scriptData, loadAnim_t *pLoadAnims, int *piNumAnims)
{
    const char *text_p;
    const char *token;
    int indexes[4];
    animScriptItem_t *currentScriptItem;
    animScript_t *currentScript;
    int parseMode;
    int indentLevel;
    int i;

    if (!bScriptFileLoaded) {
        fileHandle_t f;
        int iLen = FS_FOpenFileByMode(globalFilename, &f, (fsMode_t)(0));

        if (iLen < 0) {
            Com_Error(1, "\x15"
                         "Couldn't load player animation script %s\n",
                      globalFilename);
        }

        if ((unsigned int)iLen > 0x1869e) {
            Com_Error(1, "\x15"
                         "Couldn't load player animation script %s\n",
                      globalFilename);
        }

        FS_Read(input, iLen, f);
        input[iLen] = '\0';
        FS_FCloseFile(f);
        bScriptFileLoaded = 1;
    }

    globalScriptData = scriptData;
    g_pLoadAnims = pLoadAnims;
    g_piNumLoadAnims = piNumAnims;

    memset(weaponStrings, 0, 0x400);
    BG_LoadWeaponStrings();

    memset(defineStr, 0, sizeof(defineStr));   /* was 0x480 (x86 size); x64 rows are wider */
    memset(defineStrings, 0, sizeof(defineStrings));
    memset(numDefines, 0, sizeof(numDefines));
    defineStringsOffset = 0;

    for (i = 0; i < 4; ++i) {
        indexes[i] = -1;
    }

    text_p = input;
    Com_BeginParseSession("BG_AnimParseAnimScript");

    currentScriptItem = NULL;
    currentScript = NULL;
    parseMode = 0;
    indentLevel = 0;

    for (;;) {
        token = Com_Parse(&text_p);
        if (!token || !*token) {
            break;
        }

        i = BG_IndexForString(token, animParseModesStr, 1);
        if (i >= 0) {
            if (indentLevel) {
                BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
            }
            parseMode = i;
            parseMovetype = (scriptAnimMoveTypes_t)(0);
            parseEvent = -1;
            continue;
        }

        if ((unsigned int)parseMode > 4) {
            continue;
        }

        if (parseMode == 0) {
            int defineType;
            int defineIndex;
            char *defineName;

            if (I_stricmp(token, "set")) {
                continue;
            }

            token = Com_ParseOnLine(&text_p);
            if (!token || !*token) {
                BG_AnimParseError("BG_AnimParseAnimScript: expected condition type string");
            }

            defineType = BG_IndexForString(token, animConditionsStr, 0);
            if (animConditionsTable[defineType].type) {
                BG_AnimParseError("BG_AnimParseAnimScript: can not make a define of type '%s'", token);
            }

            token = Com_ParseOnLine(&text_p);
            if (!token || !*token) {
                BG_AnimParseError("BG_AnimParseAnimScript: expected condition define string");
            }

            defineIndex = numDefines[defineType];
            if (defineStringsOffset + (int)strlen(token) + 1 > 0x270f) {
                BG_AnimParseError("BG_CopyStringIntoBuffer: out of buffer space");
            }

            defineName = &defineStrings[defineStringsOffset];
            strcpy(defineName, token);
            defineStringsOffset += (int)strlen(token) + 1;
            defineStr[defineType][defineIndex].string = defineName;
            defineStr[defineType][defineIndex].hash = BG_StringHash(defineName);

            token = Com_ParseOnLine(&text_p);
            if (!token || !*token) {
                BG_AnimParseError("BG_AnimParseAnimScript: expected '=', found end of line");
            }
            if (I_stricmp(token, "=")) {
                BG_AnimParseError("BG_AnimParseAnimScript: expected '=', found '%s'", token);
            }

            BG_ParseConditionBits(&text_p, animConditionsTable[defineType].values, defineType,
                                  defineBits[defineType][defineIndex]);
            ++numDefines[defineType];
            continue;
        }

        if (parseMode == 1 || parseMode == 2) {
            if (!I_stricmp(token, "{")) {
                if (indentLevel > 2) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                }
                if (indexes[indentLevel] < 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                }
                ++indentLevel;
                continue;
            }

            if (!I_stricmp(token, "}")) {
                --indentLevel;
                if (indentLevel < 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                    indentLevel = 0;
                }
                indexes[indentLevel] = -1;
                continue;
            }

            if (!indentLevel) {
                if (indexes[0] >= 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                    continue;
                }

                if (I_stricmp(token, "state")) {
                    BG_AnimParseError("BG_AnimParseAnimScript: expected 'state'");
                }

                token = Com_ParseOnLine(&text_p);
                if (!token || !*token) {
                    BG_AnimParseError("BG_AnimParseAnimScript: expected state type");
                }

                indexes[0] = BG_IndexForString(token, animStateStr, 0);
                token = Com_Parse(&text_p);
                if (!token || I_stricmp(token, "{")) {
                    BG_AnimParseError("BG_AnimParseAnimScript: expected '{'");
                }
                indentLevel = 1;
                continue;
            }

            if (indentLevel == 1) {
                if (indexes[1] >= 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                    continue;
                }

                indexes[1] = BG_IndexForString(token, animMoveTypesStr, 0);
                if (parseMode == 1) {
                    currentScript = &scriptData->scriptAnims[indexes[0]][indexes[1]];
                    parseMovetype = (scriptAnimMoveTypes_t)(indexes[1]);
                } else {
                    currentScript = &scriptData->scriptCannedAnims[indexes[0]][indexes[1]];
                }
                memset(currentScript, 0, sizeof(*currentScript));
                continue;
            }

            if (indentLevel == 2) {
                animScriptItem_t tempScriptItem;
                const char *start = text_p - strlen(token);

                text_p = start;
                if (I_strncmp(text_p, token, strlen(token))) {
                    BG_AnimParseError("BG_AnimParseAnimScript: internal error");
                }

                memset(&tempScriptItem, 0, sizeof(tempScriptItem));
                indexes[2] = BG_ParseConditions(&text_p, &tempScriptItem);

                if (currentScript->numItems > 0x7f) {
                    BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum items per script (%i)", 0x80);
                }
                if (scriptData->numScriptItems > 0x7ff) {
                    BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum global items (%i)", 0x800);
                }

                currentScriptItem = &scriptData->scriptItems[scriptData->numScriptItems++];
                currentScript->items[currentScript->numItems++] = currentScriptItem;
                memcpy(currentScriptItem, &tempScriptItem, sizeof(tempScriptItem));
                continue;
            }

            if (indentLevel == 3) {
                const char *start = text_p - strlen(token);

                text_p = start;
                if (I_strncmp(text_p, token, strlen(token))) {
                    BG_AnimParseError("BG_AnimParseAnimScript: internal error");
                }
                BG_ParseCommands(&text_p, currentScriptItem, scriptData);
                continue;
            }

            BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
            continue;
        }

        if (parseMode == 3 || parseMode == 4) {
            if (!I_stricmp(token, "{")) {
                if (indentLevel > 2) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                }
                if (indexes[indentLevel] < 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                }
                ++indentLevel;
                continue;
            }

            if (!I_stricmp(token, "}")) {
                --indentLevel;
                if (indentLevel < 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                    indentLevel = 0;
                }
                indexes[indentLevel] = -1;
                continue;
            }

            if (!indentLevel) {
                if (indexes[0] >= 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                    continue;
                }

                if (parseMode == 3) {
                    int fromState;
                    int toState;

                    if (I_stricmp(token, "statechange")) {
                        BG_AnimParseError("BG_AnimParseAnimScript: expected 'statechange', got '%s'", token);
                    }

                    token = Com_ParseOnLine(&text_p);
                    if (!token || !*token) {
                        BG_AnimParseError("BG_AnimParseAnimScript: expected <state type>");
                    }
                    fromState = BG_IndexForString(token, animStateStr, 0);

                    token = Com_ParseOnLine(&text_p);
                    if (!token || !*token) {
                        BG_AnimParseError("BG_AnimParseAnimScript: expected <state type>");
                    }
                    toState = BG_IndexForString(token, animStateStr, 0);
                    indexes[0] = toState;

                    currentScript = &scriptData->scriptStateChange[fromState][toState];
                    token = Com_Parse(&text_p);
                    if (!token || I_stricmp(token, "{")) {
                        BG_AnimParseError("BG_AnimParseAnimScript: expected '{'");
                    }
                    indentLevel = 1;
                    memset(currentScript, 0, sizeof(*currentScript));
                } else {
                    indexes[0] = BG_IndexForString(token, animEventTypesStr, 0);
                    currentScript = &scriptData->scriptEvents[indexes[0]];
                    parseEvent = indexes[0];
                    memset(currentScript, 0, sizeof(*currentScript));
                }
                continue;
            }

            if (indentLevel == 1) {
                animScriptItem_t tempScriptItem;
                const char *start;

                if (indexes[1] >= 0) {
                    BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
                    continue;
                }

                start = text_p - strlen(token);
                text_p = start;
                if (I_strncmp(text_p, token, strlen(token))) {
                    BG_AnimParseError("BG_AnimParseAnimScript: internal error");
                }

                memset(&tempScriptItem, 0, sizeof(tempScriptItem));
                indexes[1] = BG_ParseConditions(&text_p, &tempScriptItem);

                if (currentScript->numItems > 0x7f) {
                    BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum items per script (%i)", 0x80);
                }
                if (scriptData->numScriptItems > 0x7ff) {
                    BG_AnimParseError("BG_AnimParseAnimScript: exceeded maximum global items (%i)", 0x800);
                }

                currentScriptItem = &scriptData->scriptItems[scriptData->numScriptItems++];
                currentScript->items[currentScript->numItems++] = currentScriptItem;
                memcpy(currentScriptItem, &tempScriptItem, sizeof(tempScriptItem));
                continue;
            }

            if (indentLevel == 2) {
                const char *start = text_p - strlen(token);

                text_p = start;
                if (I_strncmp(text_p, token, strlen(token))) {
                    BG_AnimParseError("BG_AnimParseAnimScript: internal error");
                }
                BG_ParseCommands(&text_p, currentScriptItem, scriptData);
                continue;
            }

            BG_AnimParseError("BG_AnimParseAnimScript: unexpected '%s'", token);
        }
    }

    if (indentLevel) {
        BG_AnimParseError("BG_AnimParseAnimScript: unexpected end of file: %s");
    }

    globalFilename = NULL;
    Com_EndParseSession();
}

void BG_InitWeaponString(int index, const char *name)
{
    int hash = 0;
    int i;

    weaponStrings[index].string = name;

    for (i = 0; name[i] != '\0'; i++) {
        hash += (signed char)___tolower((signed char)name[i]) * (i + 0x77);
    }

    if (hash == -1) {
        hash = 0;
    }

    weaponStrings[index].hash = hash;   /* was hardcoded index*8+4 (x86 stride) -> wrong slot on x64 */
}

#if COD2_PLUS_CLIENT_SERVER
/* src/compat/client_server_anim_check.cpp: compares this script's functions
 * with CoD2rev-Plus's (COD2_CHECK_ANIMSCRIPT) */
extern void BG_CheckAnimScript(void);

static int BG_CheckNoSound(int client, snd_alias_list_t *aliasList)
{
    (void)client;
    (void)aliasList;
    return 0;
}
#endif

void BG_LoadAnim(void)
{
    LargeLocal playerAnims_large_local;
    loadAnim_t *playerAnims;
    int iNumPlayerAnims = 0;
    XAnim *anims;
    int numTreeAnims;
    int i;
    int j;

    LargeLocal_LargeLocal(&playerAnims_large_local, 0x9000);
    playerAnims = (loadAnim_t *)LargeLocal_GetBuf(&playerAnims_large_local);

    Scr_FindAnim("multiplayer", "root", &bgs->generic_human.root, bgs->anim_user);
    Scr_FindAnim("multiplayer", "torso", &bgs->generic_human.torso, bgs->anim_user);
    Scr_FindAnim("multiplayer", "legs", &bgs->generic_human.legs, bgs->anim_user);
    Scr_FindAnim("multiplayer", "turning", &bgs->generic_human.turning, bgs->anim_user);

    BG_AnimParseAnimScript(&bgs->animScriptData, playerAnims, &iNumPlayerAnims);

    Scr_PrecacheAnimTrees( (Alloc_t)(bgs->AllocXAnim), bgs->anim_user);

    anims = Scr_FindAnimTree("multiplayer").anims;
    if (!anims) {
        Com_Error(1, "\x15"
                     "Could not find animation tree '%s'",
                  "multiplayer");
    }
    bgs->generic_human.tree.anims = anims;

    bgs->animScriptData.animTree.anims = bgs->generic_human.tree.anims;
    bgs->animScriptData.torsoAnim = bgs->generic_human.torso.index;
    bgs->animScriptData.legsAnim = bgs->generic_human.legs.index;
    bgs->animScriptData.turningAnim = bgs->generic_human.turning.index;

    Scr_EndLoadAnimTrees();

    anims = globalScriptData->animTree.anims;
    numTreeAnims = XAnimGetAnimTreeSize(anims);
    globalScriptData->numAnimations = numTreeAnims;

    globalScriptData->animations[0].flags |= 0x101;
    I_strncpyz(globalScriptData->animations[0].name, "root", 0x40);
    globalScriptData->animations[0].nameHash = 0;

    for (i = 1; i < numTreeAnims; ++i) {
        animation_t *anim;
        loadAnim_t *pAnim;
        int k;

        if (i >= globalScriptData->numAnimations) {
            Com_Error(1, "\x15Player animation index %i out of 0 to %i range", i, globalScriptData->numAnimations);
        }

        anim = &globalScriptData->animations[i];

        pAnim = NULL;
        {
            loadAnim_t *p = g_pLoadAnims;
            int n = *g_piNumLoadAnims;

            for (k = 0; k < n; ++k, ++p) {
                if (p->anim.index == i) {
                    pAnim = p;
                    break;
                }
            }
        }

        if (!pAnim) {
            anim->flags |= 0x100;
            I_strncpyz(anim->name, "unused", 0x40);
            anim->nameHash = 0;
            continue;
        }

        if (!XAnimIsPrimitive(anims, i)) {
            anim->flags |= 1;
            I_strncpyz(anim->name, pAnim->szAnimName, 0x40);
            anim->nameHash = pAnim->iNameHash;
            if (anim->initialLerp == 0) {
                anim->initialLerp = -1;
            }
            anim->duration = 0;
            anim->moveSpeed = 0;
        } else {
            float length;

            I_strncpyz(anim->name, XAnimGetAnimName(anims, i), 0x40);
            if (!anim->name[0]) {
                anim->nameHash = 0;
            } else {
                anim->nameHash = BG_StringHash(anim->name);
            }
            if (anim->initialLerp == 0) {
                anim->initialLerp = -1;
            }

            length = XAnimGetLength(anims, i);
            if (length == 0.0f) {
                anim->duration = 0x1f4;
                anim->moveSpeed = 0;
            } else {
                vec3_t angleDelta;
                vec3_t posDelta;

                anim->duration = (int)(length * 1000.0f);
                XAnimGetRelDelta(anims, i, angleDelta, posDelta, 0.0f, 1.0f);
                anim->moveSpeed = sqrtf(posDelta[0] * posDelta[0] +
                                        posDelta[1] * posDelta[1] +
                                        posDelta[2] * posDelta[2]) /
                                  length;
                if (anim->duration <= 0x1f3) {
                    anim->duration = 0x1f4;
                }
            }

            if (XAnimIsLooped(anims, i)) {
                anim->flags |= 0x80;
            }
        }
    }

    BG_AnimParseAnimScript(globalScriptData, NULL, NULL);

    for (j = 0; j < globalScriptData->numAnimations; ++j) {
        globalScriptData->animations[j].noteType = 0;
    }

    if (bgs->anim_user == 0) {
        animScript_t *script = &globalScriptData->scriptEvents[10];

        for (i = 0; i < script->numItems; ++i) {
            animScriptItem_t *item = script->items[i];
            int c;

            for (c = 0; c < item->numCommands; ++c) {
                animScriptCommand_t *cmd = &item->commands[c];

                if (cmd->bodyPart[0]) {
                    globalScriptData->animations[cmd->animIndex[0]].noteType = 1;
                }
                if (cmd->bodyPart[1]) {
                    globalScriptData->animations[cmd->animIndex[1]].noteType = 1;
                }
            }
        }
    }

    ZN10LargeLocalD1Ev(&playerAnims_large_local);

#if COD2_PLUS_CLIENT_SERVER
    if (getenv("COD2_CHECK_ANIMSCRIPT")) {
        int (*playSoundAlias)(int, snd_alias_list_t *) = globalScriptData->playSoundAlias;

        globalScriptData->playSoundAlias = BG_CheckNoSound;
        BG_CheckAnimScript();
        globalScriptData->playSoundAlias = playSoundAlias;
    }
#endif
}

/* returns the duration of the animation, -1 if no anim was set (0x4f9060) */
int BG_ExecuteCommand(playerState_t *ps, animScriptCommand_t *scriptCommand, qboolean setTimer, qboolean isContinue, qboolean force)
{
    int duration = -1;
    qboolean playedLegsAnim = qfalse;

    if (scriptCommand->bodyPart[0]) {
        duration = scriptCommand->animDuration[0] + 50;
        if (scriptCommand->bodyPart[0] == ANIM_BP_LEGS || scriptCommand->bodyPart[0] == ANIM_BP_BOTH) {
            playedLegsAnim = (BG_PlayAnim(ps, scriptCommand->animIndex[0], (animBodyPart_t)scriptCommand->bodyPart[0], duration, setTimer, isContinue, force) > -1);
        } else {
            BG_PlayAnim(ps, scriptCommand->animIndex[0], (animBodyPart_t)scriptCommand->bodyPart[0], duration, setTimer, isContinue, force);
        }
    }

    if (scriptCommand->bodyPart[1]) {
        duration = scriptCommand->animDuration[0] + 50;
        // just play the animation for the torso
        if (scriptCommand->bodyPart[0] == ANIM_BP_LEGS || scriptCommand->bodyPart[0] == ANIM_BP_BOTH) {
            playedLegsAnim = (BG_PlayAnim(ps, scriptCommand->animIndex[1], (animBodyPart_t)scriptCommand->bodyPart[1], duration, setTimer, isContinue, force) > -1);
        } else {
            BG_PlayAnim(ps, scriptCommand->animIndex[1], (animBodyPart_t)scriptCommand->bodyPart[1], duration, setTimer, isContinue, force);
        }
    }

    if (scriptCommand->soundAlias) {
        globalScriptData->playSoundAlias(ps->clientNum, scriptCommand->soundAlias);
    }

    if (!playedLegsAnim) {
        return -1;
    }

    return duration;
}

static qboolean BG_EvaluateConditions(clientInfo_t *ci, animScriptItem_t *scriptItem)
{
    int i;
    animScriptCondition_t *cond;

    for (i = 0, cond = scriptItem->conditions; i < scriptItem->numConditions; i++, cond++) {
        switch (animConditionsTable[cond->index].type) {
        case ANIM_CONDTYPE_BITFLAGS:
            if (!(ci->clientConditions[cond->index][0] & cond->value[0]) &&
                !(ci->clientConditions[cond->index][1] & cond->value[1])) {
                return qfalse;
            }
            break;
        case ANIM_CONDTYPE_VALUE:
            if (!(ci->clientConditions[cond->index][0] == cond->value[0])) {
                return qfalse;
            }
            break;
        default:
            break;
        }
    }

    // all conditions must have passed
    return qtrue;
}

/* the first script item that passes all its conditions, NULL if none */
static animScriptItem_t *BG_FirstValidItem(int client, animScript_t *script)
{
    animScriptItem_t **ppScriptItem;

    int i;

    for (i = 0, ppScriptItem = script->items; i < script->numItems; i++, ppScriptItem++) {
        if (BG_EvaluateConditions(&bgs->clientinfo[client], *ppScriptItem)) {
            return *ppScriptItem;
        }
    }

    return NULL;
}

/* the duration in milliseconds that this model should be paused, -1 if no
 * event found */
int BG_AnimScriptEvent(playerState_t *ps, scriptAnimEventTypes_t event, qboolean isContinue, qboolean force)
{
    animScript_t *script;
    animScriptItem_t *scriptItem;
    animScriptCommand_t *scriptCommand;

    if (event != ANIM_ET_DEATH && ps->pm_type >= PM_DEAD) {
        return -1;
    }

    script = &globalScriptData->scriptEvents[event];
    if (!script->numItems) {
        return -1;
    }
    // find the first script item, that passes all the conditions for this event
    scriptItem = BG_FirstValidItem(ps->clientNum, script);
    if (!scriptItem) {
        return -1;
    }
    if (!scriptItem->numCommands) {
        return -1;
    }
    // pick a random command
    scriptCommand = &scriptItem->commands[rand() % scriptItem->numCommands];

    // run it
    return BG_ExecuteCommand(ps, scriptCommand, qtrue, isContinue, force);
}

/* runs the normal locomotive animations; 1 if an animation was set, -1 if
 * no animation was found, 0 otherwise (0x4f9190) */
int BG_AnimScriptAnimation(playerState_t *ps, aistateEnum_t state, scriptAnimMoveTypes_t movetype, qboolean isContinue)
{
    animScript_t *script = NULL;
    animScriptItem_t *scriptItem = NULL;
    animScriptCommand_t *scriptCommand = NULL;

    // Allow fallen movetype while dead
    if (ps->pm_type >= PM_DEAD) {
        return -1;
    }

    // try finding a match in all states ABOVE the given state
    while (!scriptItem && state >= AISTATE_RELAXED) {
        script = &globalScriptData->scriptAnims[state][movetype];
        if (!script->numItems) {
            state = (aistateEnum_t)(state - 1);   /* VC7.1 C++: no -- on enum */
            continue;
        }
        // find the first script item, that passes all the conditions for this event
        scriptItem = BG_FirstValidItem(ps->clientNum, script);
        if (!scriptItem) {
            state = (aistateEnum_t)(state - 1);
            continue;
        }
    }

    if (!scriptItem) {
        return -1;
    }
    if (!scriptItem->numCommands) {
        return -1;
    }
    // save this as our current movetype
    BG_UpdateConditionValue(ps->clientNum, ANIM_COND_MOVETYPE, movetype, qtrue);
    // the same command for a client each time, not a random one (0x4f9212)
    scriptCommand = &scriptItem->commands[ps->clientNum % scriptItem->numCommands];

    // run it
    return (BG_ExecuteCommand(ps, scriptCommand, qfalse, isContinue, qfalse) != -1);
}

animStringItem_t animBodyPartsStr[8] = {
    { (const char *)&str_002aeae4, (int)0xffffffff },
    { (const char *)&str_002aee70, (int)0xffffffff },
    { (const char *)&str_002aee78, (int)0xffffffff },
    { (const char *)&str_002aee80, (int)0xffffffff },
    { 0, (int)0xffffffff },
    { 0, 0x0 },
    { 0, 0x0 },
    { 0, 0x0 }
};
animStringItem_t animConditionMountedStr[5] = {
    { (const char *)&str_002aeae4, (int)0xffffffff },
    { (const char *)&str_002abcf4, (int)0xffffffff },
    { 0, (int)0xffffffff },
    { 0, 0x0 },
    { 0, 0x0 }
};
animStringItem_t animConditionsStr[12] = {
    { (const char *)&str_002aedac, (int)0xffffffff },
    { (const char *)&str_002aedbc, (int)0xffffffff },
    { (const char *)&str_002aedc8, (int)0xffffffff },
    { (const char *)&str_002aedd0, (int)0xffffffff },
    { (const char *)&str_002aeddc, (int)0xffffffff },
    { (const char *)&str_002aede8, (int)0xffffffff },
    { (const char *)&str_002aedf4, (int)0xffffffff },
    { (const char *)&str_002aedfc, (int)0xffffffff },
    { (const char *)&str_002aee0c, (int)0xffffffff },
    { 0, (int)0xffffffff },
    { 0, 0x0 },
    { 0, 0x0 }
};
animConditionTable_t animConditionsTable[12] = {
    { (animScriptConditionTypes_t)0x0, (animStringItem_t *)&weaponStrings },
    { (animScriptConditionTypes_t)0x0, (animStringItem_t *)&animWeaponClassStr },
    { (animScriptConditionTypes_t)0x1, (animStringItem_t *)&animConditionMountedStr },
    { (animScriptConditionTypes_t)0x0, (animStringItem_t *)&animMoveTypesStr },
    { (animScriptConditionTypes_t)0x1, 0 },
    { (animScriptConditionTypes_t)0x1, 0 },
    { (animScriptConditionTypes_t)0x1, 0 },
    { (animScriptConditionTypes_t)0x1, (animStringItem_t *)&animWeaponPositionStr },
    { (animScriptConditionTypes_t)0x1, (animStringItem_t *)&animStrafeStateStr },
    { (animScriptConditionTypes_t)0x0, 0 },
    { (animScriptConditionTypes_t)0x0, 0 },
    { (animScriptConditionTypes_t)0x0, 0 }
};
animStringItem_t animEventTypesStr[20] = {
    { (const char *)&str_002ae9fc, (int)0xffffffff },
    { (const char *)&str_002aea04, (int)0xffffffff },
    { (const char *)&str_002aea0c, (int)0xffffffff },
    { (const char *)&str_002aea18, (int)0xffffffff },
    { (const char *)&str_002aea20, (int)0xffffffff },
    { (const char *)&str_002aea28, (int)0xffffffff },
    { (const char *)&str_002aea30, (int)0xffffffff },
    { (const char *)&str_002aea3c, (int)0xffffffff },
    { (const char *)&str_002aea48, (int)0xffffffff },
    { (const char *)&str_002aea54, (int)0xffffffff },
    { (const char *)&str_002aea64, (int)0xffffffff },
    { (const char *)&str_002aea6c, (int)0xffffffff },
    { (const char *)&str_002aea7c, (int)0xffffffff },
    { (const char *)&str_002aea8c, (int)0xffffffff },
    { (const char *)&str_002aea9c, (int)0xffffffff },
    { (const char *)&str_002aeaac, (int)0xffffffff },
    { (const char *)&str_002aeabc, (int)0xffffffff },
    { (const char *)&str_002aeacc, (int)0xffffffff },
    { (const char *)&str_002aead8, (int)0xffffffff },
    { 0, (int)0xffffffff }
};
animStringItem_t animMoveTypesStr[44] = {
    { (const char *)&str_002aeae4, (int)0xffffffff },
    { (const char *)&str_002aeaf4, (int)0xffffffff },
    { (const char *)&str_002aeafc, (int)0xffffffff },
    { (const char *)&str_002aeb04, (int)0xffffffff },
    { (const char *)&str_002aeb10, (int)0xffffffff },
    { (const char *)&str_002aeb18, (int)0xffffffff },
    { (const char *)&str_002aeb20, (int)0xffffffff },
    { (const char *)&str_002aeb28, (int)0xffffffff },
    { (const char *)&str_002aeb34, (int)0xffffffff },
    { (const char *)&str_002aeb40, (int)0xffffffff },
    { (const char *)&str_002aeb4c, (int)0xffffffff },
    { (const char *)&str_002aeb50, (int)0xffffffff },
    { (const char *)&str_002aeb58, (int)0xffffffff },
    { (const char *)&str_002aeb60, (int)0xffffffff },
    { (const char *)&str_002aeb68, (int)0xffffffff },
    { (const char *)&str_002aeb74, (int)0xffffffff },
    { (const char *)&str_002aeb80, (int)0xffffffff },
    { (const char *)&str_002aeb8c, (int)0xffffffff },
    { (const char *)&str_002aeb98, (int)0xffffffff },
    { (const char *)&str_002aeba0, (int)0xffffffff },
    { (const char *)&str_002aebac, (int)0xffffffff },
    { (const char *)&str_002aebb8, (int)0xffffffff },
    { (const char *)&str_002aebc8, (int)0xffffffff },
    { (const char *)&str_002aebd8, (int)0xffffffff },
    { (const char *)&str_002aebe8, (int)0xffffffff },
    { (const char *)&str_002aebf8, (int)0xffffffff },
    { (const char *)&str_002aec08, (int)0xffffffff },
    { (const char *)&str_002aec18, (int)0xffffffff },
    { (const char *)&str_002aec28, (int)0xffffffff },
    { (const char *)&str_002aec3c, (int)0xffffffff },
    { (const char *)&str_002aec4c, (int)0xffffffff },
    { (const char *)&str_002aec5c, (int)0xffffffff },
    { (const char *)&str_002aec6c, (int)0xffffffff },
    { (const char *)&str_002aec7c, (int)0xffffffff },
    { (const char *)&str_002aec88, (int)0xffffffff },
    { (const char *)&str_002aec98, (int)0xffffffff },
    { (const char *)&str_002aeca8, (int)0xffffffff },
    { (const char *)&str_002aecbc, (int)0xffffffff },
    { (const char *)&str_002aecd4, (int)0xffffffff },
    { (const char *)&str_002aecec, (int)0xffffffff },
    { (const char *)&str_002aed04, (int)0xffffffff },
    { 0, (int)0xffffffff },
    { 0, 0x0 },
    { 0, 0x0 }
};
animStringItem_t animParseModesStr[6] = {
    { (const char *)&str_002aee18, (int)0xffffffff },
    { (const char *)&str_002aee20, (int)0xffffffff },
    { (const char *)&str_002aee2c, (int)0xffffffff },
    { (const char *)&str_002aee40, (int)0xffffffff },
    { (const char *)&str_002aee50, (int)0xffffffff },
    { 0, (int)0xffffffff }
};
animStringItem_t animStateStr[8] = {
    { (const char *)&str_002aed1c, (int)0xffffffff },
    { (const char *)&str_002aed24, (int)0xffffffff },
    { (const char *)&str_002aed2c, (int)0xffffffff },
    { (const char *)&str_002aed34, (int)0xffffffff },
    { 0, (int)0xffffffff },
    { 0, 0x0 },
    { 0, 0x0 },
    { 0, 0x0 }
};
animStringItem_t animStrafeStateStr[4] = {
    { (const char *)&str_002aed3c, (int)0xffffffff },
    { (const char *)&str_002aed40, (int)0xffffffff },
    { (const char *)&str_002aed48, (int)0xffffffff },
    { 0, (int)0xffffffff }
};
animStringItem_t animWeaponClassStr[12] = {
    { (const char *)&str_002aed58, (int)0xffffffff },
    { (const char *)&str_002aed60, (int)0xffffffff },
    { (const char *)&str_002aed64, (int)0xffffffff },
    { (const char *)&str_002aed68, (int)0xffffffff },
    { (const char *)&str_002aed70, (int)0xffffffff },
    { (const char *)&str_002aed78, (int)0xffffffff },
    { (const char *)&str_002aed80, (int)0xffffffff },
    { (const char *)&str_002aed90, (int)0xffffffff },
    { (const char *)&str_002aed98, (int)0xffffffff },
    { (const char *)&str_002aeda4, (int)0xffffffff },
    { 0, (int)0xffffffff },
    { 0, 0x0 }
};
animStringItem_t animWeaponPositionStr[3] = {
    { (const char *)&str_002aed50, (int)0xffffffff },
    { (const char *)&str_002aed54, (int)0xffffffff },
    { 0, (int)0xffffffff }
};

const char str_002abcf4[] = "MG42";
const char str_002ae9fc[] = "PAIN";
const char str_002aea04[] = "DEATH";
const char str_002aea0c[] = "FIREWEAPON";
const char str_002aea18[] = "JUMP";
const char str_002aea20[] = "JUMPBK";
const char str_002aea28[] = "LAND";
const char str_002aea30[] = "DROPWEAPON";
const char str_002aea3c[] = "RAISEWEAPON";
const char str_002aea48[] = "CLIMBMOUNT";
const char str_002aea54[] = "CLIMBDISMOUNT";
const char str_002aea64[] = "RELOAD";
const char str_002aea6c[] = "CROUCH_TO_PRONE";
const char str_002aea7c[] = "PRONE_TO_CROUCH";
const char str_002aea8c[] = "STAND_TO_CROUCH";
const char str_002aea9c[] = "CROUCH_TO_STAND";
const char str_002aeaac[] = "STAND_TO_PRONE";
const char str_002aeabc[] = "PRONE_TO_STAND";
const char str_002aeacc[] = "MELEEATTACK";
const char str_002aead8[] = "SHELLSHOCK";
const char str_002aeae4[] = "** UNUSED **";
const char str_002aeaf4[] = "IDLE";
const char str_002aeafc[] = "IDLECR";
const char str_002aeb04[] = "IDLEPRONE";
const char str_002aeb10[] = "WALK";
const char str_002aeb18[] = "WALKBK";
const char str_002aeb20[] = "WALKCR";
const char str_002aeb28[] = "WALKCRBK";
const char str_002aeb34[] = "WALKPRONE";
const char str_002aeb40[] = "WALKPRONEBK";
const char str_002aeb4c[] = "RUN";
const char str_002aeb50[] = "RUNBK";
const char str_002aeb58[] = "RUNCR";
const char str_002aeb60[] = "RUNCRBK";
const char str_002aeb68[] = "TURNRIGHT";
const char str_002aeb74[] = "TURNLEFT";
const char str_002aeb80[] = "TURNRIGHTCR";
const char str_002aeb8c[] = "TURNLEFTCR";
const char str_002aeb98[] = "CLIMBUP";
const char str_002aeba0[] = "CLIMBDOWN";
const char str_002aebac[] = "MANTLE_ROOT";
const char str_002aebb8[] = "MANTLE_UP_57";
const char str_002aebc8[] = "MANTLE_UP_51";
const char str_002aebd8[] = "MANTLE_UP_45";
const char str_002aebe8[] = "MANTLE_UP_39";
const char str_002aebf8[] = "MANTLE_UP_33";
const char str_002aec08[] = "MANTLE_UP_27";
const char str_002aec18[] = "MANTLE_UP_21";
const char str_002aec28[] = "MANTLE_OVER_HIGH";
const char str_002aec3c[] = "MANTLE_OVER_MID";
const char str_002aec4c[] = "MANTLE_OVER_LOW";
const char str_002aec5c[] = "FLINCH_FORWARD";
const char str_002aec6c[] = "FLINCH_BACKWARD";
const char str_002aec7c[] = "FLINCH_LEFT";
const char str_002aec88[] = "FLINCH_RIGHT";
const char str_002aec98[] = "STUMBLE_FORWARD";
const char str_002aeca8[] = "STUMBLE_BACKWARD";
const char str_002aecbc[] = "STUMBLE_WALK_FORWARD";
const char str_002aecd4[] = "STUMBLE_WALK_BACKWARD";
const char str_002aecec[] = "STUMBLE_CROUCH_FORWARD";
const char str_002aed04[] = "STUMBLE_CROUCH_BACKWARD";
const char str_002aed1c[] = "RELAXED";
const char str_002aed24[] = "QUERY";
const char str_002aed2c[] = "ALERT";
const char str_002aed34[] = "COMBAT";
const char str_002aed3c[] = "NOT";
const char str_002aed40[] = "LEFT";
const char str_002aed48[] = "RIGHT";
const char str_002aed50[] = "HIP";
const char str_002aed54[] = "ADS";
const char str_002aed58[] = "RIFLE";
const char str_002aed60[] = "MG";
const char str_002aed64[] = "SMG";
const char str_002aed68[] = "SPREAD";
const char str_002aed70[] = "PISTOL";
const char str_002aed78[] = "GRENADE";
const char str_002aed80[] = "ROCKETLAUNCHER";
const char str_002aed90[] = "TURRET";
const char str_002aed98[] = "NON-PLAYER";
const char str_002aeda4[] = "ITEM";
const char str_002aedac[] = "PLAYERANIMTYPE";
const char str_002aedbc[] = "WEAPONCLASS";
const char str_002aedc8[] = "MOUNTED";
const char str_002aedd0[] = "MOVETYPE";
const char str_002aeddc[] = "UNDERHAND";
const char str_002aede8[] = "CROUCHING";
const char str_002aedf4[] = "FIRING";
const char str_002aedfc[] = "WEAPON_POSITION";
const char str_002aee0c[] = "STRAFING";
const char str_002aee18[] = "defines";
const char str_002aee20[] = "animations";
const char str_002aee2c[] = "canned_animations";
const char str_002aee40[] = "statechanges";
const char str_002aee50[] = "events";
const char str_002aee70[] = "LEGS";
const char str_002aee78[] = "TORSO";
const char str_002aee80[] = "BOTH";

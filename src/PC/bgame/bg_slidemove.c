#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

/* The client's slide and step moves, as CoD2rev-Plus's bg_slidemove.cpp (the
 * server's): the client predicts its moves with them, and any difference from
 * the server's shows as a prediction miss. */

extern float floorf(float x);
extern float fabsf(float x);

extern void PM_ClipVelocity(const vec_t *in, const vec_t *normal, vec_t *out);
extern void PM_playerTrace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);
extern void PM_AddTouchEnt(pmove_t *pm, int entityNum);
extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern const vec_t Vec3Normalize(vec_t *v);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern qboolean PM_ShouldMakeFootsteps(pmove_t *pm);
extern void PM_FootstepEvent(pmove_t *pm, pml_t *pml, int iOldBobCycle, int iNewBobCycle, qboolean bFootStep);
extern void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
extern void Jump_ClearState(playerState_t *ps);
extern qboolean Jump_IsPlayerAboveMax(playerState_t *ps);
extern Bool Jump_GetStepHeight(playerState_t *ps, const vec_t *origin, float *stepSize);
extern void Jump_ClampVelocity(playerState_t *ps, const vec_t *origin);
extern qboolean BG_CheckProne(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);

#define MAX_CLIP_PLANES 8
#define PM_SLIDEMOVE_ABI COD2_REGPARM(3) BM_NOINLINE

#define PMF_PRONE 0x1
#define PMF_LADDER 0x20
#define PMF_TIME_LAND 0x80000
#define STEPSIZE 18.0f
#define STEPSIZE_PRONE 10.0f
#define ENTITYNUM_NONE 0x3ff
#define MAX_CLIENTS 64
#define PM_DEAD 6
#define EV_STEP_VIEW 0x8f

static qboolean PM_SLIDEMOVE_ABI PM_SlideMove(pmove_t *pm, pml_t *pml, qboolean gravity);
static qboolean PM_VerifyPronePosition(pmove_t *pm, const vec_t *vFallbackOrg, const vec_t *vFallbackVel);
void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity);

static void PM_VectorCopy(const vec_t *in, vec_t *out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

static void PM_VectorLerp(const vec_t *from, const vec_t *to, float frac, vec_t *out)
{
    out[0] = from[0] + (to[0] - from[0]) * frac;
    out[1] = from[1] + (to[1] - from[1]) * frac;
    out[2] = from[2] + (to[2] - from[2]) * frac;
}

static float PM_DotProduct(const vec_t *a, const vec_t *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity)
{
    int old;
    int iDelta;
    playerState_t *ps;
    qboolean jumping;
    vec3_t endpos;
    int bHadGround;
    float fStepAmount;
    int iBumps;
    float fStepSize;
    qboolean bCloser;
    vec2_t vel;
    vec2_t flatDelta;
    vec3_t down;
    vec3_t up;
    trace_t trace;
    vec3_t down_v;
    vec3_t down_o;
    vec3_t start_v;
    vec3_t start_o;

    fStepAmount = 0.0f;
    ps = pm->ps;
    jumping = 0;

    if (ps->pm_flags & PMF_LADDER) {
        bHadGround = 0;
        Jump_ClearState(ps);
    } else if (pml->groundPlane) {
        bHadGround = 1;
    } else {
        bHadGround = 0;
        if ((ps->pm_flags & PMF_TIME_LAND) && ps->pm_time) {
            Jump_ClearState(ps);
        }
    }

    /* try slidemove from this position */
    PM_VectorCopy(ps->origin, start_o);
    PM_VectorCopy(ps->velocity, start_v);

    iBumps = PM_SlideMove(pm, pml, gravity);

    fStepSize = (ps->pm_flags & PMF_PRONE) ? STEPSIZE_PRONE : STEPSIZE;

    if (ps->groundEntityNum == ENTITYNUM_NONE) {
        if ((ps->pm_flags & PMF_TIME_LAND) && ps->pm_time) {
            Jump_ClearState(ps);
        }

        if (iBumps && (ps->pm_flags & PMF_TIME_LAND)) {
            if (Jump_GetStepHeight(ps, start_o, &fStepSize)) {
                if (fStepSize < 1.0f) {
                    return;
                }
                jumping = 1;
            }
        }

        /* never step up when you still have up velocity */
        if (!jumping && !((ps->pm_flags & PMF_LADDER) && ps->velocity[2] > 0.0f)) {
            return;
        }
    }

    /* push down the final amount */
    PM_VectorCopy(ps->origin, down_o);
    PM_VectorCopy(ps->velocity, down_v);

    flatDelta[0] = down_o[0] - start_o[0];
    flatDelta[1] = down_o[1] - start_o[1];

    if (iBumps) {
        PM_VectorCopy(start_o, up);
        up[2] += fStepSize + 1.0f;

        PM_playerTrace(pm, &trace, start_o, pm->mins, pm->maxs, up, ps->clientNum, pm->tracemask);

        fStepAmount = (fStepSize + 1.0f) * trace.fraction - 1.0f;

        if (fStepAmount < 1.0f) {
            fStepAmount = 0.0f;
        } else {
            ps->origin[0] = up[0];
            ps->origin[1] = up[1];
            ps->origin[2] = start_o[2] + fStepAmount;
            PM_VectorCopy(start_v, ps->velocity);

            PM_SlideMove(pm, pml, gravity);
        }
    }

    if (bHadGround || fStepAmount != 0.0f) {
        PM_VectorCopy(ps->origin, down);
        down[2] -= fStepAmount;

        if (bHadGround) {
            down[2] -= 9.0f;
        }

        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, down, ps->clientNum, pm->tracemask);

        /* no step onto a player */
        if (trace.entityNum < MAX_CLIENTS) {
            PM_VectorCopy(down_o, ps->origin);
            PM_VectorCopy(down_v, ps->velocity);
            return;
        }

        if (trace.fraction < 1.0f) {
            if (trace.normal[2] < 0.30000001f) {
                PM_VectorCopy(down_o, ps->origin);
                PM_VectorCopy(down_v, ps->velocity);
                return;
            }

            PM_VectorLerp(ps->origin, down, trace.fraction, ps->origin);
            PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity);
        } else if (fStepAmount != 0.0f) {
            ps->origin[2] -= fStepAmount;
        }
    }

    vel[0] = ps->origin[0] - start_o[0];
    vel[1] = ps->origin[1] - start_o[1];
    bCloser = flatDelta[0] * ps->velocity[0] + flatDelta[1] * ps->velocity[1] + 0.001f >=
              vel[0] * ps->velocity[0] + vel[1] * ps->velocity[1];

    if (bCloser || (jumping && Jump_IsPlayerAboveMax(ps))) {
        PM_VectorCopy(down_o, ps->origin);
        PM_VectorCopy(down_v, ps->velocity);

        fStepAmount = 0.0f;

        if (bHadGround) {
            PM_VectorCopy(ps->origin, down);
            down[2] -= 9.0f;

            /* if the down trace can trace back to the original position directly, don't step */
            PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, down, ps->clientNum, pm->tracemask);

            if (trace.fraction < 1.0f) {
                PM_VectorLerp(ps->origin, down, trace.fraction, endpos);
                fStepAmount = endpos[2] - ps->origin[2];
                PM_VectorCopy(endpos, ps->origin);
                PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity);
            }
        }
    }

    if (jumping) {
        Jump_ClampVelocity(ps, down_o);
    }

    if (!bHadGround) {
        return;
    }
    if (ps->pm_type >= PM_DEAD) {
        return;
    }
    if (!PM_VerifyPronePosition(pm, start_o, start_v)) {
        return;
    }
    if (fabsf(ps->origin[2] - down_o[2]) <= 0.5f) {
        return;
    }

    /* use the step move */
    iDelta = (int)floorf(ps->origin[2] - down_o[2] + 0.5f);
    if (!iDelta) {
        return;
    }
    if (iDelta < -16) {
        iDelta = -16;
    } else if (iDelta > 24) {
        iDelta = 24;
    }

    iDelta += 128;
    BG_AddPredictableEventToPlayerstate(EV_STEP_VIEW, iDelta, ps);
    {
        float scale = (1.0f - fabsf(ps->origin[2] - start_o[2]) / fStepSize) * 0.80000001f + 0.19999999f;

        ps->velocity[0] *= scale;
        ps->velocity[1] *= scale;
        ps->velocity[2] *= scale;
    }
    iDelta -= 128;

    if ((iDelta < 0 ? -iDelta : iDelta) < 4) {
        return;
    }
    if (ps->groundEntityNum == ENTITYNUM_NONE) {
        return;
    }
    if (!PM_ShouldMakeFootsteps(pm)) {
        return;
    }

    iDelta = (iDelta < 0 ? -iDelta : iDelta) / 2;
    if (iDelta > 4) {
        iDelta = 4;
    }

    old = ps->bobCycle;
    ps->bobCycle = (int)((float)old + ((float)iDelta * 1.25f + 7.0f)) & 255;
    PM_FootstepEvent(pm, pml, old, ps->bobCycle, 1);
}

static float PM_PermuteRestrictiveClipPlanes(const vec_t *velocity, int planeCount, vec3_t *planes, int *permutation)
{
    int permutedIndex;
    float parallel[MAX_CLIP_PLANES];
    int planeIndex;

    for (planeIndex = 0; planeIndex < planeCount; planeIndex++) {
        parallel[planeIndex] = PM_DotProduct(velocity, planes[planeIndex]);

        for (permutedIndex = planeIndex;
             permutedIndex && parallel[planeIndex] <= parallel[permutation[permutedIndex - 1]];
             permutedIndex--) {
            permutation[permutedIndex] = permutation[permutedIndex - 1];
        }

        permutation[permutedIndex] = planeIndex;
    }

    return parallel[permutation[0]];
}

static qboolean PM_SLIDEMOVE_ABI PM_SlideMove(pmove_t *pm, pml_t *pml, qboolean gravity)
{
    int bumpcount, numbumps;
    vec3_t dir;
    float d;
    int numplanes;
    vec3_t planes[MAX_CLIP_PLANES];
    vec3_t primal_velocity;
    vec3_t clipVelocity;
    int i, j, k;
    trace_t trace;
    vec3_t end;
    float time_left;
    float into;
    vec3_t endVelocity;
    vec3_t endClipVelocity;
    playerState_t *ps;
    int permutation[MAX_CLIP_PLANES];

    ps = pm->ps;
    numbumps = 4;

    PM_VectorCopy(ps->velocity, primal_velocity);
    PM_VectorCopy(ps->velocity, endVelocity);

    if (gravity) {
        endVelocity[2] = endVelocity[2] - (float)ps->gravity * pml->frametime;
        ps->velocity[2] = (ps->velocity[2] + endVelocity[2]) * 0.5f;
        primal_velocity[2] = endVelocity[2];

        if (pml->groundPlane) {
            /* slide along the ground plane */
            PM_ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity);
        }
    }

    time_left = pml->frametime;

    /* never turn against the ground plane */
    if (pml->groundPlane) {
        PM_VectorCopy(pml->groundTrace.normal, planes[0]);
        numplanes = 1;
    } else {
        numplanes = 0;
    }

    /* never turn against original velocity */
    Vec3NormalizeTo(ps->velocity, planes[numplanes]);
    numplanes++;

    for (bumpcount = 0; bumpcount < numbumps; bumpcount++) {
        /* calculate position we are trying to move to */
        end[0] = ps->origin[0] + time_left * ps->velocity[0];
        end[1] = ps->origin[1] + time_left * ps->velocity[1];
        end[2] = ps->origin[2] + time_left * ps->velocity[2];

        /* see if we can make it there */
        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

        if (trace.allsolid) {
            /* entity is completely trapped in another solid */
            ps->velocity[2] = 0.0f;
            return 1;
        }

        if (trace.fraction > 0.0f) {
            /* actually covered some distance */
            PM_VectorLerp(ps->origin, end, trace.fraction, ps->origin);
        }

        if (trace.fraction == 1.0f) {
            break; /* moved the entire distance */
        }

        /* save entity for contact */
        PM_AddTouchEnt(pm, trace.entityNum);

        time_left -= time_left * trace.fraction;

        if (numplanes >= MAX_CLIP_PLANES) {
            /* this shouldn't really happen */
            ps->velocity[0] = 0.0f;
            ps->velocity[1] = 0.0f;
            ps->velocity[2] = 0.0f;
            return 1;
        }

        /* if this is the same plane we hit before, nudge velocity out along
         * it, which fixes some epsilon issues with non-axial planes */
        for (i = 0; i < numplanes; i++) {
            if (PM_DotProduct(trace.normal, planes[i]) > 0.99900001f) {
                PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity);
                ps->velocity[0] += trace.normal[0];
                ps->velocity[1] += trace.normal[1];
                ps->velocity[2] += trace.normal[2];
                break;
            }
        }

        if (i < numplanes) {
            continue;
        }

        PM_VectorCopy(trace.normal, planes[numplanes]);
        numplanes++;

        /* modify velocity so it parallels all of the clip planes: find a
         * plane that it enters */
        into = PM_PermuteRestrictiveClipPlanes(ps->velocity, numplanes, planes, permutation);

        if (into >= 0.1f) {
            continue; /* move doesn't interact with the plane */
        }

        /* see how hard we are hitting things */
        if (-into > pml->impactSpeed) {
            pml->impactSpeed = -into;
        }

        /* slide along the plane */
        PM_ClipVelocity(ps->velocity, planes[permutation[0]], clipVelocity);
        PM_ClipVelocity(endVelocity, planes[permutation[0]], endClipVelocity);

        for (j = 1; j < numplanes; j++) {
            if (PM_DotProduct(clipVelocity, planes[permutation[j]]) >= 0.1f) {
                continue; /* move doesn't interact with the plane */
            }

            /* try clipping the move to the plane */
            PM_ClipVelocity(clipVelocity, planes[permutation[j]], clipVelocity);
            PM_ClipVelocity(endClipVelocity, planes[permutation[j]], endClipVelocity);

            /* see if it goes back into the first clip plane */
            if (PM_DotProduct(clipVelocity, planes[permutation[0]]) >= 0.0f) {
                continue;
            }

            /* slide the original velocity along the crease */
            Vec3Cross(planes[permutation[0]], planes[permutation[j]], dir);
            Vec3Normalize(dir);
            d = PM_DotProduct(dir, ps->velocity);
            clipVelocity[0] = dir[0] * d;
            clipVelocity[1] = dir[1] * d;
            clipVelocity[2] = dir[2] * d;

            d = PM_DotProduct(dir, endVelocity);
            endClipVelocity[0] = dir[0] * d;
            endClipVelocity[1] = dir[1] * d;
            endClipVelocity[2] = dir[2] * d;

            /* see if there is a third plane the the new move enters */
            for (k = 1; k < numplanes; k++) {
                if (k == j) {
                    continue;
                }
                if (PM_DotProduct(clipVelocity, planes[permutation[k]]) >= 0.1f) {
                    continue; /* move doesn't interact with the plane */
                }

                /* stop dead at a tripple plane interaction */
                ps->velocity[0] = 0.0f;
                ps->velocity[1] = 0.0f;
                ps->velocity[2] = 0.0f;
                return 1;
            }
        }

        /* if we have fixed all interactions, try another move */
        PM_VectorCopy(clipVelocity, ps->velocity);
        PM_VectorCopy(endClipVelocity, endVelocity);
    }

    if (gravity) {
        PM_VectorCopy(endVelocity, ps->velocity);
    }

    /* don't change velocity if in a timer */
    if (ps->pm_time) {
        PM_VectorCopy(primal_velocity, ps->velocity);
    }

    return bumpcount != 0;
}

static qboolean PM_VerifyPronePosition(pmove_t *pm, const vec_t *vFallbackOrg, const vec_t *vFallbackVel)
{
    qboolean bProneOK;
    playerState_t *ps;

    ps = pm->ps;

    if (!(ps->pm_flags & PMF_PRONE)) {
        return 1;
    }

    bProneOK = BG_CheckProne(ps->clientNum, ps->origin, ps->maxs[0], 30.0f, ps->proneDirection,
                             &ps->fTorsoHeight, &ps->fTorsoPitch, &ps->fWaistPitch,
                             1, 1, NULL, pm->handler, PCT_CLIENT, 66.0f);

    if (!bProneOK) {
        PM_VectorCopy(vFallbackOrg, ps->origin);
        PM_VectorCopy(vFallbackVel, ps->velocity);
    }

    return bProneOK;
}

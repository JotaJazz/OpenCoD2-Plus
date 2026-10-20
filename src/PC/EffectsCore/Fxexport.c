#include "common_types.h"
#include "imports.h"

extern volatile qboolean fx_camera_valid;

extern struct DObj_s * Com_GetClientDObj(int handle, int localClientNum);
extern int DObjGetBoneIndex(const DObj *obj, unsigned int boneName);
extern void FxScheduler_PlayEffect(void *scheduler, EffectTemplate *fx, const vec_t *org, ...);
extern void FxScheduler_PlayEffectWithDir(void *scheduler, EffectTemplate *fx, const vec_t *org, const vec_t *forward);
extern int FX_Init(int rendererExists);
extern void FX_Free(int freeAll);
extern void FxHelper_AdjustCamera(const FxHelper *_this, refdef_t *refdef, float zfar);
extern void FxHelper_AdjustTime(const FxHelper *_this, int intime);
extern void FxHelper_WarpTime(const FxHelper *_this, int intime);
extern float FxScheduler_GetEffectLength(const FxScheduler *_this, EffectTemplate *fx);

extern byte *fx_scheduler_ptr;
extern byte *fx_helper_ptr;

extern int effectActiveCountBolt;
extern int privateEffectActiveCountBolt;
extern int effectActiveCountNonBolt;
extern int privateEffectActiveCountNonBolt;

int FX_GetBoneIndex(const int entNum, unsigned int bone);
void FX_PlaySimpleEffect(EffectTemplate *fx, const vec_t *org);
void FX_PlayEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd);
/* along a direction with its up: forward, left (up x forward) and up, the
 * axis convention of the scheduler */
void FX_PlayOrientedEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd, const vec_t *up)
{
    extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
    vec3_t axis[3];

    axis[0][0] = fwd[0];
    axis[0][1] = fwd[1];
    axis[0][2] = fwd[2];
    Vec3Cross(up, fwd, axis[1]);
    axis[2][0] = up[0];
    axis[2][1] = up[1];
    axis[2][2] = up[2];
    FxScheduler_PlayEffect(*(void **)*(void **)&fx_scheduler_ptr, fx, org, (MediaHandles *(*)[4])axis, (const FxBoltInfo *)0);
}

void FX_PlayEntityEffect(EffectTemplate *fx, const vec_t *org, vec3_t *axis, const FxBoltInfo *bolt);
int FX_InitSystem(int rendererExists);
void FX_FreeSystem(void);
void FX_FreeActive(void);
void FX_AdjustCamera(PrimType (*refdef)[256], float zfar);
void FX_AdjustTime(int time);
void FX_WarpTime(int time);
float FX_GetEffectLength(EffectTemplate *fx);
void Server_SwitchToValidFxScheduler(void);

int FX_GetBoneIndex(const int entNum, unsigned int bone)
{
    void *pObj = Com_GetClientDObj(entNum, 0);
    if (pObj == NULL)
        return -1;
    return DObjGetBoneIndex( (const DObj *)(pObj), bone);
}

void FX_PlaySimpleEffect(EffectTemplate *fx, const vec_t *org)
{
    /* an effect's forward is up (its x axis is the normal it plays along), as
     * CoD2MP_s.exe 1.3 (0x499500): forward (0, 0, 1), then (1, 0, 0) and
     * (0, 1, 0); no bolt */
    vec3_t axis[3] = { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };

    FxScheduler_PlayEffect(*(void **)*(void **)&fx_scheduler_ptr, fx, org, (MediaHandles *(*)[4])axis, (const FxBoltInfo *)0);
}

void FX_PlayEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd)
{
    FxScheduler_PlayEffectWithDir(*(void **)*(void **)&fx_scheduler_ptr, fx, org, fwd);
}

void FX_PlayEntityEffect(EffectTemplate *fx, const vec_t *org, vec3_t *axis, const FxBoltInfo *bolt)
{
    FxScheduler_PlayEffect(*(void **)*(void **)&fx_scheduler_ptr, fx, org, axis, bolt);
}

int FX_InitSystem(int rendererExists)
{
    return FX_Init((unsigned char)rendererExists);
}

void FX_FreeSystem(void)
{
    FX_Free(1);
}

void FX_FreeActive(void)
{
    FX_Free(0);
}

void FX_AdjustCamera(PrimType (*refdef)[256], float zfar)
{
    FxHelper_AdjustCamera( (const FxHelper *)(*(void **)*(void **)&fx_helper_ptr), (refdef_t *)(refdef), zfar);
    fx_camera_valid = 1;
}

extern void *imp_effectActiveCountBolt;
extern void *imp_effectActiveCountNonBolt;
extern void *imp_privateEffectActiveCountBolt;
extern void *imp_privateEffectActiveCountNonBolt;
void FX_AdjustTime(int time)
{
    fx_camera_valid = 0;
    FxHelper_AdjustTime( (const FxHelper *)(*(void **)*(void **)&fx_helper_ptr), time);
    *(int *)imp_privateEffectActiveCountBolt = *(int *)imp_effectActiveCountBolt;
    *(int *)imp_privateEffectActiveCountNonBolt = *(int *)imp_effectActiveCountNonBolt;
}

void FX_WarpTime(int time)
{
    FxHelper_WarpTime( (const FxHelper *)(*(void **)*(void **)&fx_helper_ptr), time);
}

float FX_GetEffectLength(EffectTemplate *fx)
{
    return FxScheduler_GetEffectLength( (const FxScheduler *)(*(void **)*(void **)&fx_scheduler_ptr), fx);
}

void Server_SwitchToValidFxScheduler(void)
{

}

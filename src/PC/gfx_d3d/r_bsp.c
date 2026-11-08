#include "common_types.h"
extern dvar_t *r_rendererInUse;
#include "imports.h"

/* 1.3's offsets, read by field so x64 lays them out its own way */
COD2_ASSERT_FIELD(GfxWorld, vd.worldVb, 0x30);
COD2_ASSERT_FIELD(GfxWorld, sunLight, 0xb4);
COD2_ASSERT_FIELD(GfxWorld, mins, 0x13c);
COD2_ASSERT_FIELD(GfxWorld, maxs, 0x148);
extern refimport_t ri;
#include "bytematch.h"
#include <string.h>

extern void R_FreeStaticVertexBuffer(IDirect3DVertexBuffer9 *vb);
extern void *R_AllocStaticVertexBuffer(IDirect3DVertexBuffer9 **vb_out, int size);
extern void R_FinishStaticVertexBuffer(IDirect3DVertexBuffer9 *vb);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void R_InterpretSunLightParseParamsIntoLights(SunLightParseParams *sunParse, GfxLight *lights);
extern float ColorNormalize(float *in, float *out);
extern GfxWorld *R_LoadWorldInternal(const char *name);
extern void RB_InitLightVisHistory(const char *name);
extern void R_FlushSun(void);
extern void R_ResetShadowCookies(void);
extern void R_InitStaticModelIndexCache(void);
extern void *Hunk_AllocInternal(int size);
extern void R_InitStaticModelDynamicData(int index);
extern GfxImage * Image_Register(const char *imageName, int semantic, int imageTrack);

extern r_global_permanent_t rgp;

extern byte r_frontEndData_ptr[];

extern vec3_t vec3_colorintensity;

static inline __attribute__((always_inline)) dvar_t *R_DvarFromImport(void *imp)
{
    return *(dvar_t **)imp;
}

void R_ResetSunLightOverride(void);
void R_ReleaseWorld(void);
void R_GetWorldBounds(vec_t *min, vec_t *max);
void R_InterpretSunLightParseParams(SunLightParseParams *sunParse);
void R_SetSunLightOverride(const vec_t *sunColor);
IDirect3DVertexBuffer9 *R_CreateWorldVertexBuffer(GfxWorldVertex *vertices, int vertexCount);
void R_ReloadWorld(void);
void R_ShutdownWorld(void);
void R_UpdateLightsFromDvars(void);
void R_LoadWorld(const char *name, int *checksum);
void R_ResetSunLightParseParams(void);

void R_ResetSunLightOverride(void)
{
    byte *world = (*(byte **)&rgp.world);

    vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunLight.color[0];
    const vec_t *src = (const vec_t *)&((GfxWorld *)world)->sunColorFromBsp[0];
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

void R_ReleaseWorld(void)
{

    if (((GfxWorld *)*(byte **)&rgp.world)->vd.worldVb != NULL) {
        R_FreeStaticVertexBuffer((IDirect3DVertexBuffer9 *)((GfxWorld *)*(byte **)&rgp.world)->vd.worldVb);
        *(void **)&((GfxWorld *)*(byte **)&rgp.world)->vd.worldVb = NULL;
    }
}

void R_GetWorldBounds(vec_t *min, vec_t *max)
{

    const vec_t *bmin = ((GfxWorld *)*(byte **)&rgp.world)->mins;
    min[0] = bmin[0];
    min[1] = bmin[1];
    min[2] = bmin[2];

    const vec_t *bmax = ((GfxWorld *)*(byte **)&rgp.world)->maxs;
    max[0] = bmax[0];
    max[1] = bmax[1];
    max[2] = bmax[2];
}

void R_InterpretSunLightParseParams(SunLightParseParams *sunParse)
{

    R_InterpretSunLightParseParamsIntoLights(sunParse, &((GfxWorld *)*(byte **)&rgp.world)->sunLight);

    {
        byte *world = (*(byte **)&rgp.world);
        vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunColorFromBsp[0];
        const vec_t *src = (const vec_t *)&((GfxWorld *)world)->sunLight.color[0];
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
}

void R_SetSunLightOverride(const vec_t *sunColor)
{
    const dvar_t *device = r_rendererInUse;

    if (device->current.integer == 2)
        return;

    {
        byte *world = (*(byte **)&rgp.world);

        vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunLight.color[0];
        dst[0] = sunColor[0];
        dst[1] = sunColor[1];
        dst[2] = sunColor[2];
    }
}

IDirect3DVertexBuffer9 *R_CreateWorldVertexBuffer(GfxWorldVertex *vertices, int vertexCount)
{
    IDirect3DVertexBuffer9 *worldVb;
    byte *dataPtr;
    int sizeVerts;
    int vertIndex;

    sizeVerts = (r_rendererInUse->current.integer == 2) ? 0x20 : 0x44;
    sizeVerts *= vertexCount;

    dataPtr = (byte *)R_AllocStaticVertexBuffer(&worldVb, sizeVerts);

    if (r_rendererInUse->current.integer != 2) {

        Com_Memcpy(dataPtr, vertices, sizeVerts);
    } else {

        for (vertIndex = 0; vertIndex < vertexCount; vertIndex++) {
            byte *src = (byte *)vertices + vertIndex * 0x44;
            byte *dst = dataPtr + vertIndex * 0x20;

            *(int *)(dst + 0x00) = *(int *)(src + 0x00);
            *(int *)(dst + 0x04) = *(int *)(src + 0x04);
            *(int *)(dst + 0x08) = *(int *)(src + 0x08);

            *(int *)(dst + 0x0c) = *(int *)(src + 0x18);
            *(int *)(dst + 0x10) = *(int *)(src + 0x1c);
            *(int *)(dst + 0x14) = *(int *)(src + 0x20);
            *(int *)(dst + 0x18) = *(int *)(src + 0x24);
            *(int *)(dst + 0x1c) = *(int *)(src + 0x28);
        }
    }

    R_FinishStaticVertexBuffer(worldVb);
    return (IDirect3DVertexBuffer9 *)worldVb;
}

void R_ReloadWorld(void)
{
    byte *world = (*(byte **)&rgp.world);

    (*(void **)&((GfxWorld *)world)->vd.worldVb) = R_CreateWorldVertexBuffer(
        ((GfxWorld *)world)->vd.vertices,
        ((GfxWorld *)world)->vertexCount);
}

void R_ShutdownWorld(void)
{
    byte *world = (*(byte **)&rgp.world);

    if (world == NULL)
        return;

    {
        void *vb = (*(void **)&((GfxWorld *)world)->vd.worldVb);
        if (vb != NULL) {
            R_FreeStaticVertexBuffer((IDirect3DVertexBuffer9 *)vb);
            *(void **)&((GfxWorld *)*(byte **)&rgp.world)->vd.worldVb = NULL;
        }
    }

    (*(void **)&rgp.world) = NULL;
}

void R_UpdateLightsFromDvars(void)
{
    byte sunParse[128];
    dvar_t *dvar;
    int channelIter;
    byte *world;

    dvar = R_DvarFromImport(imp_r_lightTweakAmbient);
    *(float *)(sunParse + 0x40) = dvar->current.value;

    dvar = R_DvarFromImport(imp_r_lightTweakDiffuseFraction);
    *(float *)(sunParse + 0x50) = dvar->current.value;

    dvar = R_DvarFromImport(imp_r_lightTweakSunLight);
    *(float *)(sunParse + 0x54) = dvar->current.value;

    dvar = R_DvarFromImport(imp_r_lightTweakAmbientColor);
    for (channelIter = 0; channelIter < 3; channelIter++) {
        *(float *)(sunParse + 0x44 + channelIter * 4) = (float)dvar->current.color[channelIter];
    }
    ColorNormalize((float *)(sunParse + 0x44), (float *)(sunParse + 0x44));

    dvar = R_DvarFromImport(imp_r_lightTweakSunColor);
    for (channelIter = 0; channelIter < 3; channelIter++) {
        *(float *)(sunParse + 0x58 + channelIter * 4) = (float)dvar->current.color[channelIter];
    }
    ColorNormalize((float *)(sunParse + 0x58), (float *)(sunParse + 0x58));

    dvar = R_DvarFromImport(imp_r_lightTweakSunDiffuseColor);
    for (channelIter = 0; channelIter < 3; channelIter++) {
        *(float *)(sunParse + 0x64 + channelIter * 4) = (float)dvar->current.color[channelIter];
    }
    ColorNormalize((float *)(sunParse + 0x64), (float *)(sunParse + 0x64));

    sunParse[0x70] = 1;

    dvar = R_DvarFromImport(imp_r_lightTweakSunDirection);
    {
        vec_t *src = dvar->current.vector;
        *(int *)(sunParse + 0x74) = *(int *)(src + 0);
        *(int *)(sunParse + 0x78) = *(int *)(src + 1);
        *(int *)(sunParse + 0x7c) = *(int *)(src + 2);
    }

    world = (*(byte **)&rgp.world);
    if (world) {   /* no world (e.g. main menu, no map loaded) -> nothing to update */
        R_InterpretSunLightParseParamsIntoLights((SunLightParseParams *)sunParse, &((GfxWorld *)world)->sunLight);

        {
            vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunColorFromBsp[0];
            const vec_t *src = (const vec_t *)&((GfxWorld *)world)->sunLight.color[0];
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
        }
    }
}

void R_LoadWorld(const char *name, int *checksum)
{
    byte *world;
    SunLightParseParams *sp;
    refimport_t *refimport;
    dvar_t *dvar;
    byte *frontEnd;
    int i;

    RB_InitLightVisHistory(name);
    world = (byte *)R_LoadWorldInternal(name);
    (*(void **)&rgp.world) = world;

    if (checksum != NULL) {
        *checksum = (*(int *)&((GfxWorld *)world)->checksum);
    }

    world = *(byte **)&rgp.world;
    sp = &((GfxWorld *)world)->sunParse;

    refimport = (refimport_t *)&ri;

    refimport->Dvar_SetFloat(R_DvarFromImport(imp_r_lightTweakAmbient), sp->ambientScale);
    refimport->Dvar_SetFloat(R_DvarFromImport(imp_r_lightTweakDiffuseFraction), sp->diffuseFraction);
    refimport->Dvar_SetFloat(R_DvarFromImport(imp_r_lightTweakSunLight), sp->sunLight);

    refimport->Dvar_SetColor(R_DvarFromImport(imp_r_lightTweakAmbientColor),
                             sp->ambientColor[0], sp->ambientColor[1],
                             sp->ambientColor[2], 1.0f);
    refimport->Dvar_SetColor(R_DvarFromImport(imp_r_lightTweakSunColor),
                             sp->sunColor[0], sp->sunColor[1],
                             sp->sunColor[2], 1.0f);
    refimport->Dvar_SetColor(R_DvarFromImport(imp_r_lightTweakSunDiffuseColor),
                             sp->diffuseColor[0], sp->diffuseColor[1],
                             sp->diffuseColor[2], 1.0f);

    refimport->Dvar_SetVec3(R_DvarFromImport(imp_r_lightTweakSunDirection),
                            sp->angles[0], sp->angles[1],
                            sp->angles[2]);

    dvar = R_DvarFromImport(imp_r_lightTweakAmbient);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakDiffuseFraction);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunLight);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakAmbientColor);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunColor);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunDiffuseColor);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunDirection);
    refimport->Dvar_SetModified(dvar);

    R_UpdateLightsFromDvars();
    R_FlushSun();
    R_ResetShadowCookies();
    R_InitStaticModelIndexCache();

    frontEnd = (byte *)imp_rg;
    world = (*(byte **)&rgp.world);

    ((r_globals_t *)frontEnd)->smodelDyncs =
        (GfxStaticModelDynamic *)Hunk_AllocInternal(((GfxWorld *)world)->smodelCount * (int)sizeof(GfxStaticModelDynamic));
    world = (*(byte **)&rgp.world);
    ((r_globals_t *)frontEnd)->surfaces =
        (GfxSurfaceDynamic *)Hunk_AllocInternal(((GfxWorld *)world)->surfaceCount * (int)sizeof(GfxSurfaceDynamic));
    world = (*(byte **)&rgp.world);
    ((r_globals_t *)frontEnd)->cullGroups =
        (GfxCullGroupDynamic *)Hunk_AllocInternal(((GfxWorld *)world)->cullGroupCount * (int)sizeof(GfxCullGroupDynamic));

    world = (*(byte **)&rgp.world);
    if (((GfxWorld *)world)->smodelCount > 0) {
        for (i = 0; i < ((GfxWorld *)world)->smodelCount; i++) {
            R_InitStaticModelDynamicData(i);
        }
    }

    {
        const dvar_t *device = r_rendererInUse;
        if (device->current.integer == 2) {
            (*(void **)&rgp.sunHalfAngleImage) =
                Image_Register("$sunhalfangle", 1, 0);
        }
    }
}

void R_ResetSunLightParseParams(void)
{
    R_UpdateLightsFromDvars();
}

vec3_t vec3_colorintensity = { 0.299f, 0.587f, 0.114f }; /* truncated 8->3 (blob over-capture) */

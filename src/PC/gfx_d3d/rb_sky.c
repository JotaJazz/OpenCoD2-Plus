#include "common_types.h"
#include <math.h>
static GfxColor bm_color_from_bits(unsigned int b) { GfxColor c; *(unsigned int *)&c = b; return c; }
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern refimport_t ri;
extern int alwaysfails;

extern SunFlareDynamic sunFlareArray[4];

extern IDirect3DQuery9 *r_occlusionQuery;

#define dx_device_ptr imp_dx
#define dx_fence ((volatile int *)&alwaysfails)
extern r_global_permanent_t rgp;
#define r_frontEndDataOut ((void *)imp_backEnd)
#define gfx_buf ((void *)imp_tess)
#define r_dvar_sunEnable imp_r_drawSun
#define r_defaultColor (*(unsigned int *)imp_colorWhite)
#define r_videoConfig ((void *)imp_vidConfig)
#define r_phys ((void *)&ri)
#define r_contentmask ((unsigned int)(uintptr_t)imp_vec3_origin)

extern void RB_ClearScreen(int whichToClear, const vec_t *color, float depth, int stencil);
extern void RB_DrawStretchPic(const Material *material, float x, float y, float w, float h, float s0, float t0, float s1, float t1, D3DCOLOR color, GfxPrimStatsTarget statsTarget);
extern void RB_EndSurface(void);
extern void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex);
extern void RB_PushMatrixStack(void);
extern void RB_PopMatrixStack(void);
extern void RB_DrawFullScreenColoredQuad(const Material *material, float s0, float t0, float s1, float t1, D3DCOLOR color);
extern void MatrixIdentity44(float (*out)[4]);
extern void RB_SetProjectionMatrix(const D3DMATRIX *matrix);
extern void RB_SetViewMatrix(const D3DMATRIX *matrix);
extern void RB_Set3D(void);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern const vec_t Vec3Normalize(vec_t *v);
extern void RB_SetViewMatrixForWDx7(float w);
extern void WinSleep(DWORD dwMilliseconds);

int RB_CalcSunSpriteSamples(void);
static GfxVertex *RB_SetTessQuad(union GfxColor color);
static GfxVertexDx7 *RB_SetTessQuadDx7(GfxColor color);
static void RB_TessSunBillboard(float widthInClipSpace, float heightInClipSpace);
void RB_DrawSunPostEffects(int viewIndex);
void RB_DrawSun(int viewIndex);

#define FIELD(base, offset, type) (*(type *)((char *)(base) + (offset)))
#define FIELDP(base, offset, type) ((type *)((char *)(base) + (offset)))

typedef int(D3DVTCC *QueryIssueFn)(void *query, int flags);
typedef int(D3DVTCC *QueryGetDataFn)(void *query, void *pData, int dataSize, int flags);
typedef int(D3DVTCC *DeviceBeginSceneFn)(void *device);
typedef int(D3DVTCC *DeviceEndSceneFn)(void *device);
typedef void *(__attribute__((cdecl)) * TraceCallFn)(int hitNum, void *start, vec_t *end, float *frac1, float *frac2, int contentmask, int flags);

int RB_CalcSunSpriteSamples(void)
{
    IDirect3DQuery9 *occlusionQuery;
    void **vtable;
    void *device;
    int sampleCount;
    int result;

    occlusionQuery = r_occlusionQuery;
    if (!occlusionQuery) {
        return 0;
    }

    do {
        device = FIELD(&dx_device_ptr, 0, void *);
        device = FIELD(device, 8, void *);
        vtable = FIELD(device, 0, void **);
        ((DeviceBeginSceneFn)vtable[0xa4 / 4])(device);
    } while (FIELD(dx_fence, 0, int) != 0);

    RB_ClearScreen(2, (const vec_t *)&r_defaultColor, 1.0f, 0);

    do {
        vtable = FIELD(occlusionQuery, 0, void **);
        ((QueryIssueFn)vtable[0x18 / 4])(occlusionQuery, 2);
    } while (FIELD(dx_fence, 0, int) != 0);

    {
        Material *mat;
        mat = rgp.whiteMaterial;
        RB_DrawStretchPic(mat, 0.0f, 0.0f, 16.0f, 16.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0xffffffff, (GfxPrimStatsTarget)(10));
    }
    RB_EndSurface();

    do {
        vtable = FIELD(occlusionQuery, 0, void **);
        ((QueryIssueFn)vtable[0x18 / 4])(occlusionQuery, 1);
    } while (FIELD(dx_fence, 0, int) != 0);

    do {
        device = FIELD(&dx_device_ptr, 0, void *);
        device = FIELD(device, 8, void *);
        vtable = FIELD(device, 0, void **);
        ((DeviceEndSceneFn)vtable[0xa8 / 4])(device);
    } while (FIELD(dx_fence, 0, int) != 0);

    do {
        vtable = FIELD(occlusionQuery, 0, void **);
        result = ((QueryGetDataFn)vtable[0x1c / 4])(occlusionQuery, &sampleCount, 4, 1);
        if (result == 1) {
            WinSleep(0);
        }
    } while (result == 1);

    if (result != 0) {
        sampleCount = 0x100;
    }

    return sampleCount;
}

static GfxVertex *RB_SetTessQuad(union GfxColor color)
{
    char *tess;
    int vertCount;
    GfxVertex *v;

    tess = (char *)gfx_buf;
    vertCount = FIELD(tess, 0x5a7d4, int);

    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 0] = vertCount + 3;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 1] = vertCount;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 2] = vertCount + 2;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 3] = vertCount + 2;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 4] = vertCount;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 5] = vertCount + 1;

    v = (GfxVertex *)((char *)tess + (unsigned short)vertCount * 64);

    v[0].normal[0] = 0.0f;
    v[0].normal[1] = 0.0f;
    v[0].normal[2] = 1.0f;
    v[0].texCoord[0] = 0.0f;
    v[0].texCoord[1] = 0.0f;
    v[0].color = color;
    v[0].tangent[0] = 1.0f;
    v[0].tangent[1] = 0.0f;
    v[0].tangent[2] = 0.0f;
    v[0].binormal[0] = 0.0f;
    v[0].binormal[1] = 1.0f;
    v[0].binormal[2] = 0.0f;

    v[1].normal[0] = 0.0f;
    v[1].normal[1] = 0.0f;
    v[1].normal[2] = 1.0f;
    v[1].texCoord[0] = 1.0f;
    v[1].texCoord[1] = 0.0f;
    v[1].color = color;
    v[1].tangent[0] = 1.0f;
    v[1].tangent[1] = 0.0f;
    v[1].tangent[2] = 0.0f;
    v[1].binormal[0] = 0.0f;
    v[1].binormal[1] = 1.0f;
    v[1].binormal[2] = 0.0f;

    v[2].normal[0] = 0.0f;
    v[2].normal[1] = 0.0f;
    v[2].normal[2] = 1.0f;
    v[2].texCoord[0] = 1.0f;
    v[2].texCoord[1] = 1.0f;
    v[2].color = color;
    v[2].tangent[0] = 1.0f;
    v[2].tangent[1] = 0.0f;
    v[2].tangent[2] = 0.0f;
    v[2].binormal[0] = 0.0f;
    v[2].binormal[1] = 1.0f;
    v[2].binormal[2] = 0.0f;

    v[3].normal[0] = 0.0f;
    v[3].normal[1] = 0.0f;
    v[3].normal[2] = 1.0f;
    v[3].texCoord[0] = 0.0f;
    v[3].texCoord[1] = 1.0f;
    v[3].color = color;
    v[3].tangent[0] = 1.0f;
    v[3].tangent[1] = 0.0f;
    v[3].tangent[2] = 0.0f;
    v[3].binormal[0] = 0.0f;
    v[3].binormal[1] = 1.0f;
    v[3].binormal[2] = 0.0f;

    FIELD(tess, 0x5a7d4, unsigned int) += 4;
    FIELD(tess, 0x5a7d0, unsigned int) += 6;

    return v;
}

static GfxVertexDx7 *RB_SetTessQuadDx7(GfxColor color)
{
    char *tess;
    int vertCount;
    GfxVertexDx7 *v;

    tess = (char *)gfx_buf;
    vertCount = FIELD(tess, 0x5a7d4, int);

    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 0] = vertCount + 3;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 1] = vertCount;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 2] = vertCount + 2;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 3] = vertCount + 2;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 4] = vertCount;
    FIELD(tess, 0x5a7b0, unsigned short *)
    [FIELD(tess, 0x5a7d0, unsigned int) + 5] = vertCount + 1;

    v = (GfxVertexDx7 *)((char *)tess + (unsigned short)vertCount * 36);

    v[0].normal[0] = 0.0f;
    v[0].normal[1] = 0.0f;
    v[0].normal[2] = 1.0f;
    v[0].texCoord[0] = 0.0f;
    v[0].texCoord[1] = 0.0f;
    v[0].color = color;

    v[1].normal[0] = 0.0f;
    v[1].normal[1] = 0.0f;
    v[1].normal[2] = 1.0f;
    v[1].texCoord[0] = 1.0f;
    v[1].texCoord[1] = 0.0f;
    v[1].color = color;

    v[2].normal[0] = 0.0f;
    v[2].normal[1] = 0.0f;
    v[2].normal[2] = 1.0f;
    v[2].texCoord[0] = 1.0f;
    v[2].texCoord[1] = 1.0f;
    v[2].color = color;

    v[3].normal[0] = 0.0f;
    v[3].normal[1] = 0.0f;
    v[3].normal[2] = 1.0f;
    v[3].texCoord[0] = 0.0f;
    v[3].texCoord[1] = 1.0f;
    v[3].color = color;

    FIELD(tess, 0x5a7d4, unsigned int) += 4;
    FIELD(tess, 0x5a7d0, unsigned int) += 6;

    return v;
}

static float DotColumn(const float *a, const float *matrix, int col)
{
    return a[0] * matrix[col] + a[1] * matrix[col + 4] + a[2] * matrix[col + 8];
}

static inline __attribute__((always_inline)) float FadeToGoal(float goal, float current, int fadeInTime, int fadeOutTime, int frameTime)
{
    if (goal > current) {
        if (fadeInTime > 0) {
            float step = (float)frameTime / (float)fadeInTime;
            float newVal = current + step;
            if (newVal > goal) {
                return goal;
            }
            return newVal;
        }
        return goal;
    } else {

        if (goal < current) {
            if (fadeOutTime > 0) {
                float step = (float)frameTime / (float)fadeOutTime;
                float newVal = current - step;
                if (newVal < goal) {
                    return goal;
                }
                return newVal;
            }
        }
        return current;
    }
}

static void RB_TessSunBillboard(float widthInClipSpace, float heightInClipSpace)
{
    D3DCOLOR colorVal;
    void *rendererGlob;
    float *viewProjectionMatrix;
    float identity[16];
    float sunDir[3];
    float clipX, clipY, clipZ, clipW;
    float nearClip;
    float scale;
    GfxVertex *verts;
    GfxVertexDx7 *vertsDx7;

    colorVal = (D3DCOLOR)(unsigned int)widthInClipSpace;

    rendererGlob = (void *)r_rendererInUse;
    if (FIELD(rendererGlob, 8, int) == 2) {

        void *frontEnd = r_frontEndDataOut;
        float *viewMat;
        void *viewData = FIELD(frontEnd, 0x3c8, void *);
        float *matCol0 = FIELDP(viewData, 0xc8, float);
        float *matCol1 = FIELDP(viewData, 0xd8, float);
        float *matCol2 = FIELDP(viewData, 0xe8, float);

        float *sunDirPtr = FIELDP(rgp.world, 0x1b4, float);
        float sx = sunDirPtr[0], sy = sunDirPtr[1], sz = sunDirPtr[2];

        clipX = sx * matCol0[0] + sy * matCol1[0] + sz * matCol2[0];
        clipY = sx * matCol0[1] + sy * matCol1[1] + sz * matCol2[1];
        clipZ = sx * matCol0[2] + sy * matCol1[2] + sz * matCol2[2];
        clipW = sx * matCol0[3] + sy * matCol1[3] + sz * matCol2[3];

        scale = 1.0f / clipW;

        clipX *= scale;
        clipY *= scale;
        clipZ *= scale;

        MatrixIdentity44( (float (*)[4])(identity));
        RB_SetProjectionMatrix( (const D3DMATRIX *)(identity));
        RB_SetViewMatrix( (const D3DMATRIX *)(identity));

        vertsDx7 = RB_SetTessQuadDx7(*(GfxColor *)&colorVal);

        vertsDx7[0].xyz[0] = clipX;
        vertsDx7[0].xyz[1] = clipY;
        vertsDx7[0].xyz[2] = clipZ;

        vertsDx7[1].xyz[0] = clipX;
        vertsDx7[1].xyz[1] = clipY;
        vertsDx7[1].xyz[2] = clipZ;

        vertsDx7[2].xyz[0] = clipX;
        vertsDx7[2].xyz[1] = clipY;
        vertsDx7[2].xyz[2] = clipZ;

        vertsDx7[3].xyz[0] = clipX;
        vertsDx7[3].xyz[1] = clipY;
        vertsDx7[3].xyz[2] = clipZ;

        vertsDx7[0].xyz[0] += widthInClipSpace;
        vertsDx7[0].xyz[1] += heightInClipSpace;
        vertsDx7[0].xyz[2] -= 0.001f;

        vertsDx7[1].xyz[0] += widthInClipSpace;
        vertsDx7[1].xyz[1] -= heightInClipSpace;
        vertsDx7[1].xyz[2] -= 0.001f;

        vertsDx7[2].xyz[0] -= widthInClipSpace;
        vertsDx7[2].xyz[1] -= heightInClipSpace;
        vertsDx7[2].xyz[2] -= 0.001f;

        vertsDx7[3].xyz[0] -= widthInClipSpace;
        vertsDx7[3].xyz[1] += heightInClipSpace;
        vertsDx7[3].xyz[2] -= 0.001f;
    } else {

        void *scene = rgp.world;
        float *sunDirPtr = FIELDP(scene, 0x1b4, float);
        float sx = sunDirPtr[0], sy = sunDirPtr[1], sz = sunDirPtr[2];

        viewProjectionMatrix = FIELDP(r_frontEndDataOut, 0x3f0, float);

        clipX = sx * viewProjectionMatrix[0] + sy * viewProjectionMatrix[0x10 / 4] + sz * viewProjectionMatrix[0x20 / 4];
        clipY = sx * viewProjectionMatrix[1] + sy * viewProjectionMatrix[0x14 / 4] + sz * viewProjectionMatrix[0x24 / 4];
        clipZ = sx * viewProjectionMatrix[2] + sy * viewProjectionMatrix[0x18 / 4] + sz * viewProjectionMatrix[0x28 / 4];
        clipW = sx * viewProjectionMatrix[3] + sy * viewProjectionMatrix[0x1c / 4] + sz * viewProjectionMatrix[0x2c / 4];

        MatrixIdentity44( (float (*)[4])(identity));
        RB_SetProjectionMatrix( (const D3DMATRIX *)(identity));
        RB_SetViewMatrix( (const D3DMATRIX *)(identity));

        verts = RB_SetTessQuad(*(GfxColor *)&colorVal);
        nearClip = clipW * -0.001f;

        verts[0].xyzw[0] = clipX;
        verts[0].xyzw[1] = clipY;
        verts[0].xyzw[2] = clipZ;
        verts[0].xyzw[3] = clipW;

        verts[1].xyzw[0] = clipX;
        verts[1].xyzw[1] = clipY;
        verts[1].xyzw[2] = clipZ;
        verts[1].xyzw[3] = clipW;

        verts[2].xyzw[0] = clipX;
        verts[2].xyzw[1] = clipY;
        verts[2].xyzw[2] = clipZ;
        verts[2].xyzw[3] = clipW;

        verts[3].xyzw[0] = clipX;
        verts[3].xyzw[1] = clipY;
        verts[3].xyzw[2] = clipZ;
        verts[3].xyzw[3] = clipW;

        heightInClipSpace *= clipW;
        widthInClipSpace *= clipW;

        verts[0].xyzw[0] += widthInClipSpace;
        verts[0].xyzw[1] += heightInClipSpace;
        verts[0].xyzw[2] += nearClip;

        verts[1].xyzw[0] += widthInClipSpace;
        verts[1].xyzw[1] -= heightInClipSpace;
        verts[1].xyzw[2] += nearClip;

        verts[2].xyzw[0] -= widthInClipSpace;
        verts[2].xyzw[1] -= heightInClipSpace;
        verts[2].xyzw[2] += nearClip;

        verts[3].xyzw[0] -= widthInClipSpace;
        verts[3].xyzw[1] += heightInClipSpace;
        verts[3].xyzw[2] += nearClip;
    }
}

void RB_DrawSunPostEffects(int viewIndex)
{
    D3DCOLOR color;
    SunFlareDynamic *sunFlare;
    int frameTime;
    void *frontEnd;
    void *scene;
    float sizeIn640x480;
    float alpha;

    color = 0;

    sunFlare = &sunFlareArray[viewIndex];

    frontEnd = r_frontEndDataOut;

    if (sunFlare->lastTime == 0) {
        frameTime = 10;
    } else {
        int backendTime = FIELD(frontEnd, 0x3b8, int);
        if (backendTime <= sunFlare->lastTime) {
            frameTime = 10;
        } else {
            frameTime = backendTime - sunFlare->lastTime;
        }
    }

    sunFlare->lastTime = FIELD(frontEnd, 0x3b8, int);

    {
        void *dvar = *(void **)r_dvar_sunEnable;
        if (FIELD(dvar, 8, unsigned char) == 0)
            return;
    }

    scene = rgp.world;
    if (FIELD(scene, 0x160, unsigned char) == 0)
        return;

    {
        Material *sunMaterial = (Material *)(FIELD(scene, 0x168, void *));
        float sunFlareCosBegin, sunFlareCosEnd;
        float lastDot;
        float cosAngle;

        if (sunMaterial == 0)
            goto after_sunflare_material;

        lastDot = sunFlare->lastDot;
        sunFlareCosBegin = FIELD(scene, 0x174, float);

        if (sunFlareCosBegin >= lastDot)
            goto after_sunflare_material;

        sunFlareCosEnd = FIELD(scene, 0x17c, float);
        if (lastDot < sunFlareCosEnd) {

            alpha = (lastDot - sunFlareCosBegin) / (sunFlareCosEnd - sunFlareCosBegin);
        } else {
            alpha = 1.0f;
        }

        {
            float spriteScale = FIELD(scene, 0x180, float);
            float spriteSize = FIELD(scene, 0x178, float);
            int iFadeInTime = FIELD(scene, 0x184, int);
            int iFadeOutTime = FIELD(scene, 0x188, int);
            float fGoalAlpha;

            fGoalAlpha = alpha * spriteScale;
            sizeIn640x480 = alpha * spriteSize;
            sizeIn640x480 += FIELD(scene, 0x170, float);

            sunFlare->flareIntensity = FadeToGoal(sunFlare->lastVisibility, sunFlare->flareIntensity, iFadeInTime, iFadeOutTime, frameTime);
            alpha = sunFlare->flareIntensity * fGoalAlpha;
        }

        {
            char *tessBuf = (char *)gfx_buf;
            if (FIELD(tessBuf, 0x5a7d0, int) != 0 || FIELD(tessBuf, 0x5a7e0, int) != 0) {
                RB_EndSurface();
            }

            {
                Material *mat = FIELD(rgp.world, 0x168, Material *);
                char *tb = (char *)gfx_buf;
                if (mat == FIELD(tb, 0x5a7bc, Material *) && FIELD(tb, 0x5a7c0, int) == 3) {

                } else {
                    if (FIELD(tb, 0x5a7d0, int) != 0 || FIELD(tb, 0x5a7e0, int) != 0) {
                        RB_EndSurface();
                    }
                    RB_BeginSurface(mat, (MaterialTechniqueType)(3), 0x1f);
                }
            }
        }

        RB_PushMatrixStack();

        {
            int alphaByte;
            alpha *= 255.0f;
            alphaByte = (int)floorf(alpha + 0.5f);
            ((unsigned char *)&color)[0] = 0xFF;
            ((unsigned char *)&color)[1] = (unsigned char)alphaByte;
            ((unsigned char *)&color)[2] = (unsigned char)alphaByte;
            ((unsigned char *)&color)[3] = (unsigned char)alphaByte;
        }

        RB_TessSunBillboard(sizeIn640x480 / 640.0f, sizeIn640x480 / 480.0f);
        RB_EndSurface();
        RB_PopMatrixStack();

        frontEnd = r_frontEndDataOut;
    }

after_sunflare_material: {

    float *viewAxis;
    float *sunDirPtr;
    float dot;
    float glareGoal, blindGoal;
    float glareValue, blindValue;
    float glareAlpha, blindAlpha;
    int iFadeInTime, iFadeOutTime;
    int alphaByte, alphaByte2;
    void *viewData;

    frontEnd = r_frontEndDataOut;
    viewData = FIELD(frontEnd, 0x3c8, void *);

    scene = rgp.world;
    sunDirPtr = FIELDP(scene, 0x1b4, float);
    viewAxis = FIELDP(viewData, 0xc, float);

    dot = sunDirPtr[0] * viewAxis[0] + sunDirPtr[1] * viewAxis[1] + sunDirPtr[2] * viewAxis[2];

    if (FIELD(scene, 0x194, float) <= 0.0f) {
        glareAlpha = 0.0f;
    } else {
        float blindCosBegin = FIELD(scene, 0x18c, float);
        if (dot > blindCosBegin) {
            float blindCosEnd = FIELD(scene, 0x190, float);
            if (dot >= blindCosEnd) {
                blindGoal = 1.0f;
            } else {
                blindGoal = (dot - blindCosBegin) / (blindCosEnd - blindCosBegin);
            }
        } else {
            blindGoal = 0.0f;
        }

        blindGoal *= sunFlare->lastVisibility;
        iFadeOutTime = FIELD(scene, 0x19c, int);
        iFadeInTime = FIELD(scene, 0x198, int);

        sunFlare->currentBlind = FadeToGoal(blindGoal, sunFlare->currentBlind, iFadeInTime, iFadeOutTime, frameTime);
        glareAlpha = sunFlare->currentBlind * FIELD(scene, 0x194, float) * 255.0f;
    }

    if (FIELD(scene, 0x1a8, float) <= 0.0f) {
        blindAlpha = 0.0f;
    } else {
        float glareCosBegin = FIELD(scene, 0x1a0, float);
        if (dot > glareCosBegin) {
            float glareCosEnd = FIELD(scene, 0x1a4, float);
            if (dot >= glareCosEnd) {
                glareGoal = 1.0f;
            } else {
                glareGoal = (dot - glareCosBegin) / (glareCosEnd - glareCosBegin);
            }
        } else {
            glareGoal = 0.0f;
        }

        glareGoal *= sunFlare->lastVisibility;
        iFadeOutTime = FIELD(scene, 0x1b0, int);
        iFadeInTime = FIELD(scene, 0x1ac, int);

        sunFlare->currentGlare = FadeToGoal(glareGoal, sunFlare->currentGlare, iFadeInTime, iFadeOutTime, frameTime);
        blindAlpha = sunFlare->currentGlare * FIELD(scene, 0x1a8, float) * 255.0f;
    }

    {
        alphaByte = (int)glareAlpha;
        alphaByte2 = (int)blindAlpha;
        ((unsigned char *)&color)[0] = (unsigned char)alphaByte;
        ((unsigned char *)&color)[1] = (unsigned char)alphaByte2;
        ((unsigned char *)&color)[2] = (unsigned char)alphaByte2;
        ((unsigned char *)&color)[3] = (unsigned char)alphaByte2;

        {
            Material *screenEffectMat = FIELD(rgp.world, 0x1058, Material *);
            RB_DrawFullScreenColoredQuad(screenEffectMat, 0.0f, 0.0f, 1.0f, 1.0f, color);
        }
    }
}

    RB_EndSurface();
    return;
}

void RB_DrawSun(int viewIndex)
{
    SunFlareDynamic *sunFlare;
    float sunVisibility;
    void *frontEnd;
    void *scene;
    int queryIndex;
    float savedVisibility;

    sunVisibility = 0.0f;

    {
        void *dvar = *(void **)r_dvar_sunEnable;
        if (FIELD(dvar, 8, unsigned char) == 0)
            return;
    }

    scene = rgp.world;
    if (FIELD(scene, 0x160, unsigned char) == 0)
        return;

    frontEnd = r_frontEndDataOut;
    if (FIELD(frontEnd, 0x4bd, unsigned char) != 0) {
        RB_Set3D();
    }

    sunFlare = &sunFlareArray[viewIndex];

    {

        int numQueries = FIELD(frontEnd, 0x430, int);
        int queryBase = numQueries * 2;
        int frameCount = FIELD(frontEnd, 0x3b0, int);
        int parity = frameCount & 0x80000001;
        if (parity < 0) {
            parity = ((parity - 1) | (int)0xFFFFFFFE) + 1;
        }
        queryIndex = queryBase + parity;

        if (sunFlare->sunQuery[queryIndex] == 0) {

            float *viewMat;
            void *viewData = FIELD(frontEnd, 0x3c8, void *);
            float *transform0 = FIELDP(viewData, 0xc8, float);
            float *transform1 = FIELDP(viewData, 0xd8, float);
            float *transform2 = FIELDP(viewData, 0xe8, float);

            float *sDirPtr = FIELDP(rgp.world, 0x1b4, float);
            float sx = sDirPtr[0], sy = sDirPtr[1], sz = sDirPtr[2];

            float cW = sx * transform0[3] + sy * transform1[3] + sz * transform2[3];
            savedVisibility = 0.0f;

            if (cW > 0.0f) {
                int vidWidth, vidHeight;
                void *videoConfig = r_videoConfig;
                vidWidth = FIELD(videoConfig, 0, int);
                vidHeight = FIELD(videoConfig, 4, int);

                float cX = sx * transform0[0] + sy * transform1[0] + sz * transform2[0];
                cX = (cX / cW + 1.0f) * (float)vidWidth;
                int screenX = (int)floorf((cX - 16.0f) * 0.5f + 0.5f);

                float cY = sx * transform0[1] + sy * transform1[1] + sz * transform2[1];
                cY = (cY / cW + 1.0f) * (float)vidHeight;
                int screenY = (int)floorf((cY - 16.0f) * 0.5f + 0.5f);

                int right = screenX + 16;
                int bottom = screenY + 16;

                if (screenX < 0)
                    screenX = 0;
                if (right > vidWidth)
                    right = vidWidth;
                if (screenY < 0)
                    screenY = 0;
                if (bottom > vidHeight)
                    bottom = vidHeight;

                if (screenX < right && screenY < bottom) {
                    int area = (right - screenX) * (bottom - screenY);
                    savedVisibility = (float)area * 0.00390625f;
                }
            }

            sunFlare->lastVisibility = savedVisibility;

            if (savedVisibility == 0.0f) {
                goto after_sun_trace;
            }

            {
                float sunTraceEnd[3];
                float *sDirPtr2;
                void *viewData2;
                float *startPos;

                frontEnd = r_frontEndDataOut;
                viewData2 = FIELD(frontEnd, 0x3c8, void *);
                startPos = FIELDP(viewData2, 0, float);
                sDirPtr2 = FIELDP(rgp.world, 0x1b4, float);

                sunTraceEnd[0] = sDirPtr2[0] * 262144.0f + startPos[0];
                sunTraceEnd[1] = sDirPtr2[1] * 262144.0f + startPos[1];
                sunTraceEnd[2] = sDirPtr2[2] * 262144.0f + startPos[2];

                {

                    extern int CM_BoxSightTrace(int oldHitNum, const vec_t *start, const vec_t *end,
                                                const vec_t *mins, const vec_t *maxs, int model, int brushmask);
                    static const vec_t kZeroVec[3] = { 0.0f, 0.0f, 0.0f };
                    int hitNum = sunFlare->hitNum;
                    hitNum = CM_BoxSightTrace(hitNum, (const vec_t *)FIELDP(viewData2, 0, void *),
                                              sunTraceEnd, kZeroVec, kZeroVec, 0, 0x2003);
                    sunFlare->hitNum = hitNum;
                }

                if (sunFlare->hitNum != 0) {
                    sunFlare->lastVisibility = 0.0f;
                    savedVisibility = 0.0f;
                } else {
                    savedVisibility = 0.0f;
                }
            }

            goto after_sun_trace;
        }

        {
            char *tessBuf = (char *)gfx_buf;
            Material *occlusionMat;

            if (FIELD(tessBuf, 0x5a7d0, int) != 0 || FIELD(tessBuf, 0x5a7e0, int) != 0) {
                RB_EndSurface();
            }

            occlusionMat = FIELD(rgp.world, 0x103c, Material *);

            {
                char *tb = (char *)gfx_buf;
                if (occlusionMat == FIELD(tb, 0x5a7bc, Material *) && FIELD(tb, 0x5a7c0, int) == 3) {

                } else {
                    if (FIELD(tb, 0x5a7d0, int) != 0 || FIELD(tb, 0x5a7e0, int) != 0) {
                        RB_EndSurface();
                    }
                    RB_BeginSurface(occlusionMat, (MaterialTechniqueType)(3), 0x1f);
                }
            }

            RB_PushMatrixStack();

            {
                float screenW, screenH;
                frontEnd = r_frontEndDataOut;
                screenW = (float)FIELD(frontEnd, 0x43c, int);
                screenH = (float)FIELD(frontEnd, 0x438, int);
                RB_TessSunBillboard(16.0f / screenW, 16.0f / screenH);
            }

            if (sunFlare->sunQueryIssued[queryIndex]) {
                unsigned int drawnSampleCount = 0;
                int getDataResult;
                IDirect3DQuery9 *query;

                do {
                    query = sunFlare->sunQuery[queryIndex];
                    {
                        void **qvt = FIELD(query, 0, void **);
                        getDataResult = ((QueryGetDataFn)qvt[0x1c / 4])(query, &drawnSampleCount, 4, 1);
                    }
                    if (getDataResult == 1) {
                        WinSleep(0);
                    }
                } while (getDataResult == 1);

                if (getDataResult != 0) {
                    sunFlare->error = 1;
                }

                {
                    void *devPtr = FIELD(&dx_device_ptr, 0, void *);
                    unsigned int maxSamples = FIELD(devPtr, 0x2c2c, unsigned int);
                    if (maxSamples < drawnSampleCount) {
                        sunFlare->error = 1;
                    }

                    if ((int)drawnSampleCount < 0) {

                        unsigned int half = (drawnSampleCount >> 1) | (drawnSampleCount & 1);
                        sunVisibility = (float)(int)half * 2.0f / (float)(int)maxSamples;
                    } else {
                        sunVisibility = (float)(int)drawnSampleCount / (float)(int)maxSamples;
                    }
                }
            } else {
                sunVisibility = 0.0f;
            }

            {
                int numQueries2;
                frontEnd = r_frontEndDataOut;
                numQueries2 = FIELD(frontEnd, 0x430, int);

                if (numQueries2 == 0) {
                    sunFlare->cumulVisibility = 0.0f;
                }

                sunFlare->cumulVisibility += sunVisibility;

                {
                    int maxQueries = FIELD(frontEnd, 0x434, int) - 1;

                    if (numQueries2 == maxQueries) {

                        if (sunFlare->cumulVisibility > 1.0f) {
                            sunFlare->cumulVisibility = 1.0f;
                        }

                        if (!sunFlare->error) {
                            sunFlare->lastVisibility = sunFlare->cumulVisibility;
                        }
                        sunFlare->error = 0;
                    }
                }
            }

            {
                IDirect3DQuery9 *query = sunFlare->sunQuery[queryIndex];
                void **qvt = FIELD(query, 0, void **);
                ((QueryIssueFn)qvt[0x18 / 4])(query, 2);
            }
            RB_EndSurface();

            {
                IDirect3DQuery9 *query = sunFlare->sunQuery[queryIndex];
                void **qvt = FIELD(query, 0, void **);
                ((QueryIssueFn)qvt[0x18 / 4])(query, 1);
            }
            sunFlare->sunQueryIssued[queryIndex] = 1;
            RB_PopMatrixStack();
            savedVisibility = 0.0f;
        }
    }

after_sun_trace: {

    float *viewAxis;
    float *sunDirPtr;
    void *viewData;
    float dot;

    frontEnd = r_frontEndDataOut;
    viewData = FIELD(frontEnd, 0x3c8, void *);
    viewAxis = FIELDP(viewData, 0xc, float);

    scene = rgp.world;
    sunDirPtr = FIELDP(scene, 0x1b4, float);

    dot = sunDirPtr[0] * viewAxis[0] + sunDirPtr[1] * viewAxis[1] + sunDirPtr[2] * viewAxis[2];

    sunFlare->lastDot = dot;

    if (savedVisibility >= dot)
        return;

    {
        void *rendererGlob = (void *)r_rendererInUse;

        if (FIELD(rendererGlob, 8, int) == 2) {

            float *sunDir;
            Material *sunMat;

            scene = rgp.world;
            sunDir = FIELDP(scene, 0x1b4, float);
            sunMat = FIELD(scene, 0x164, Material *);

            {
                char *tb = (char *)gfx_buf;
                if (sunMat == FIELD(tb, 0x5a7bc, Material *) && FIELD(tb, 0x5a7c0, int) == 1) {

                } else {
                    if (FIELD(tb, 0x5a7d0, int) != 0 || FIELD(tb, 0x5a7e0, int) != 0) {
                        RB_EndSurface();
                    }
                    RB_BeginSurface(sunMat, (MaterialTechniqueType)(1), 0x1f);
                }
            }

            {
                GfxVertexDx7 *vd;
                float halfSize;
                float szZ;
                float cross1[3], cross2[3];
                vec3_t up;
                float dxp1[3], dxp2[3], dxm1[3], dxm2[3];

                vd = RB_SetTessQuadDx7(bm_color_from_bits(0xffffffff));

                halfSize = 0.0013110929867252707f;
                halfSize *= FIELD(rgp.world, 0x16c, float);

                szZ = sunDir[2];
                if (szZ * szZ > 0.99f) {
                    up[0] = 1.0f;
                    up[1] = 0.0f;
                    up[2] = 0.0f;
                } else {

                    up[0] = sunDir[1];
                    up[1] = -sunDir[0];
                    up[2] = 0.0f;
                }

                Vec3Cross(sunDir, up, cross1);
                Vec3Normalize(cross1);

                cross1[0] *= halfSize;
                cross1[1] *= halfSize;
                cross1[2] *= halfSize;

                Vec3Cross(cross1, sunDir, cross2);

                dxp1[0] = cross1[0] + cross2[0];
                dxp1[1] = cross1[1] + cross2[1];
                dxp1[2] = cross1[2] + cross2[2];

                dxm1[0] = cross1[0] - cross2[0];
                dxm1[1] = cross1[1] - cross2[1];
                dxm1[2] = cross1[2] - cross2[2];

                vd[0].xyz[0] = sunDir[0] + dxp1[0];
                vd[0].xyz[1] = sunDir[1] + dxp1[1];
                vd[0].xyz[2] = sunDir[2] + dxp1[2];

                vd[1].xyz[0] = sunDir[0] + dxm1[0];
                vd[1].xyz[1] = sunDir[1] + dxm1[1];
                vd[1].xyz[2] = sunDir[2] + dxm1[2];

                vd[2].xyz[0] = sunDir[0] - dxp1[0];
                vd[2].xyz[1] = sunDir[1] - dxp1[1];
                vd[2].xyz[2] = sunDir[2] - dxp1[2];

                vd[3].xyz[0] = sunDir[0] - dxm1[0];
                vd[3].xyz[1] = sunDir[1] - dxm1[1];
                vd[3].xyz[2] = sunDir[2] - dxm1[2];

                RB_SetViewMatrixForWDx7(0.0f);
                RB_EndSurface();
                RB_SetViewMatrixForWDx7(1.0f);
            }
        } else {

            float *sunDir;
            Material *sunMat;

            scene = rgp.world;
            sunDir = FIELDP(scene, 0x1b4, float);
            sunMat = FIELD(scene, 0x164, Material *);

            {
                char *tb = (char *)gfx_buf;
                if (sunMat == FIELD(tb, 0x5a7bc, Material *) && FIELD(tb, 0x5a7c0, int) == 1) {

                } else {
                    if (FIELD(tb, 0x5a7d0, int) != 0 || FIELD(tb, 0x5a7e0, int) != 0) {
                        RB_EndSurface();
                    }
                    RB_BeginSurface(sunMat, (MaterialTechniqueType)(1), 0x1f);
                }
            }

            {
                GfxVertex *v;
                float halfSize;
                float szZ;
                float cross1[3], cross2[3];
                vec3_t up;
                float dxp1[3], dxp2[3], dxm1[3], dxm2[3];

                v = RB_SetTessQuad(bm_color_from_bits(0xffffffff));

                halfSize = 0.0013110929867252707f;
                halfSize *= FIELD(rgp.world, 0x16c, float);

                szZ = sunDir[2];
                if (szZ * szZ > 0.99f) {
                    up[0] = 1.0f;
                    up[1] = 0.0f;
                    up[2] = 0.0f;
                } else {
                    up[0] = sunDir[1];
                    up[1] = -sunDir[0];
                    up[2] = 0.0f;
                }

                Vec3Cross(sunDir, up, cross1);
                Vec3Normalize(cross1);

                cross1[0] *= halfSize;
                cross1[1] *= halfSize;
                cross1[2] *= halfSize;

                Vec3Cross(cross1, sunDir, cross2);

                dxp1[0] = cross1[0] + cross2[0];
                dxp1[1] = cross1[1] + cross2[1];
                dxp1[2] = cross1[2] + cross2[2];

                dxm1[0] = cross1[0] - cross2[0];
                dxm1[1] = cross1[1] - cross2[1];
                dxm1[2] = cross1[2] - cross2[2];

                v[0].xyzw[0] = sunDir[0] + dxp1[0];
                v[0].xyzw[1] = sunDir[1] + dxp1[1];
                v[0].xyzw[2] = sunDir[2] + dxp1[2];

                v[1].xyzw[0] = sunDir[0] + dxm1[0];
                v[1].xyzw[1] = sunDir[1] + dxm1[1];
                v[1].xyzw[2] = sunDir[2] + dxm1[2];

                v[2].xyzw[0] = sunDir[0] - dxp1[0];
                v[2].xyzw[1] = sunDir[1] - dxp1[1];
                v[2].xyzw[2] = sunDir[2] - dxp1[2];

                v[3].xyzw[0] = sunDir[0] - dxm1[0];
                v[3].xyzw[1] = sunDir[1] - dxm1[1];
                v[3].xyzw[2] = sunDir[2] - dxm1[2];

                v[0].xyzw[3] = 0.0f;
                v[1].xyzw[3] = 0.0f;
                v[2].xyzw[3] = 0.0f;
                v[3].xyzw[3] = 0.0f;

                RB_EndSurface();
            }
        }
    }
}

    return;
}

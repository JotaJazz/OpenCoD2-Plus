#include "common_types.h"
#include <stdlib.h>
extern struct DxGlobals dx;
/* File-scope alias: bound where no local can shadow `dx`, so uses below
   always reach the global even inside functions that declare their own `dx`. */
static struct DxGlobals * const dx_g = &dx;
extern GfxBackEndData *frontEndDataOut;
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern int alwaysfails;
#include "bytematch.h"
extern DxGlobals dx;
extern const dvar_t *r_optimizeXModels;
extern const dvar_t *developer;
extern const dvar_t *r_skinCache;
extern const dvar_t *r_xdebug;
extern const dvar_t *r_smc_enable;
extern GfxScene scene;

COD2_ASSERT_FIELD(r_vb_state_t, buffer, 8);
COD2_ASSERT_FIELD(DxGlobals, gpuSync, 11296);
COD2_ASSERT_FIELD(DxGlobals, skinnedCacheLockAddr, 11712);

static int warnCount;
static int warnCount_00c85b04;
static int warnCount_00c85b08;
static int warnCount_00c85b0c;
static int warnCount_00c85b10;
extern void *Hunk_AllocInternal(int size);
extern void DB_EnumXAssets(int type, void (*func)(XAssetHeader, void *), void *data, qboolean overrides);
extern int XModelBad(union XAssetHeader header);
extern void XModelUnoptimize(union XAssetHeader header);
extern void XModelOptimize(XModel *xmodel);
extern float Vec3Distance(const void *a, const void *b);
extern int DObjGetLodForDist(const DObj *obj, int modelIndex, float dist);
extern void DObjSetModel(DObj *obj, const XModel *model);
extern int DObjGetNumModels(const struct DObj_s *obj);
extern int DObjGetSurfaces(const struct DObj_s *obj, DSurface_s *surfaces, int *partBits, char *lods);
extern struct XModel *DObjGetModel(const struct DObj_s *obj, int modelIndex);
extern const struct trXSkin_t *XModelGetSkins(const struct XModel *model);
extern struct XModel *XModelPrecache(const char *name, Alloc_t Alloc, Alloc_t AllocColl);
extern Bool R_ValidXModelName(const char *name);
extern refimport_t ri;

#ifdef GFX_REAL_D3D9
int g_smc_hit, g_smc_new;
long long g_smc_recache_ms;
#endif
#ifdef GFX_REAL_D3D9

extern const char *XModelGetName(const XModel *model);
static void SMT_Log(const char *tag, struct XModel *mdl, GfxEntity *ent)
{
    static const char *seen[512];
    static int nseen;
    const char *nm;
    int i;
    float ox, oy, oz, sc;
    int bad;
    if (!getenv("REALD3D9_SMTLOG") || !mdl || !ent)
        return;
    nm = XModelGetName(mdl);
    for (i = 0; i < nseen; i++)
        if (seen[i] == nm)
            return;
    if (nseen < 512)
        seen[nseen++] = nm;
    ox = ent->origin[0];
    oy = ent->origin[1];
    oz = ent->origin[2];
    sc = ent->scale;
    bad = (ox != ox || oy != oy || oz != oz ||
           ox > 1e6f || ox < -1e6f || oy > 1e6f || oy < -1e6f || oz > 1e6f || oz < -1e6f ||
           sc <= 0.0f || sc > 1e4f || sc != sc);
    fprintf(stderr, "[SMT] %-6s '%-34.34s' origin=(%.0f %.0f %.0f) scale=%.3f ax0=(%.2f %.2f %.2f)%s\n",
            tag, nm ? nm : "(null)", ox, oy, oz, sc,
            ent->axis[0][0], ent->axis[0][1], ent->axis[0][2], bad ? "   <<< SUSPICIOUS" : "");
    fflush(stderr);
}
#endif
extern r_global_permanent_t rgp;
__attribute__((used)) extern const int boxVerts[24][3] = {
    { 0, 0, 0 },
    { 1, 0, 0 },
    { 0, 0, 0 },
    { 0, 1, 0 },
    { 1, 1, 0 },
    { 1, 0, 0 },
    { 1, 1, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
    { 1, 0, 1 },
    { 0, 0, 1 },
    { 0, 1, 1 },
    { 1, 1, 1 },
    { 1, 0, 1 },
    { 1, 1, 1 },
    { 0, 1, 1 },
    { 0, 0, 0 },
    { 0, 0, 1 },
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 0, 1, 0 },
    { 0, 1, 1 },
    { 1, 1, 0 },
    { 1, 1, 1 },
};
extern const int s_streamSourceInfo[];
extern int DObjNumBones(const struct DObj_s *obj);
extern void DObjGetBoneInfo(const struct DObj_s *obj, struct XBoneInfo_s **boneInfoArray);
extern DObjAnimMat *DObjGetRotTransArray(const DObj *obj);
extern void CG_DObjCalcPose(const centity_t *cent, const struct DObj_s *obj, int *partBits);
extern void MatrixTransformVectorQuatTrans(const vec_t *in, const DObjAnimMat *mat, vec_t *out);
extern void MatrixTransformVector(const vec_t *in, const void *matrix, vec_t *out);
extern void R_AddDebugLine(DebugGlobals *debugGlobalsEntry, const vec_t *start, const vec_t *end, const vec_t *color);

#define VTABLE(obj) (*(void ***)((void *)(obj)))

typedef HRESULT(D3DVTCC *BufferUnlockFn)(void *buffer);

static void *Hunk_AllocXModelPrecache(int size);
static void *Hunk_AllocXModelPrecacheColl(int size);
struct XModel *R_RegisterModel(const char *name);
GfxBrushModel *R_RegisterInlineModel(int modelIndex);
void R_SetIgnorePrecacheErrors(qboolean ignore);
qboolean R_GetIgnorePrecacheErrors(void);
void R_UnlockSkinnedCache(void);
struct DObj_s *R_GetGfxEntityDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_DObjReplaceMaterial(struct DObj_s *obj, int lod, int surfaceIndex, MaterialHandle material);
static void R_ReleaseModel(union XAssetHeader header, void *data);
void R_OptimizeAllModels(void);
void R_ReleaseAllModels(void);
void *Model_Alloc(int size);
static void R_OptimizeModel(XAssetHeader header, void *data);
void R_GetRigidTransform(const DObjSkelMat *bone, const vec_t *origin, vec3_t *axis, float scale, vec3_t *boneAxis);
void R_ModelBounds(GfxBrushModel *bmodel, vec_t *mins, vec_t *maxs);
void R_LockSkinnedCache(GfxLockType lockType);
void R_ShutdownModels(void);
void R_FinishLoadingModels(void);
void R_DObjGetSurfMaterials(struct DObj_s *obj, int lod, MaterialHandle *matHandleArray);
static void R_XModelDebugBoxes_impl(const byte *sceneEnt, const byte *ent, const void *obj);
static void R_XModelDebugAxes_impl(const byte *sceneEnt, const byte *ent, const void *obj);
void R_UpdateXModelBounds(GfxSceneEntity *sceneEnt, GfxEntity *ent);
static int R_PreSkinXSurface(GfxSceneEntity *sceneEnt, const struct DObj_s *obj, const DSurface *surface, int surfaceIndex, char *lods, byte *surfPos);
void R_SkinSceneDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent);
static int R_PreSkinStaticSurface(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex, const struct XModel *model, XSurface *xsurf, int surfaceIndex, int lod, qboolean *needSkinningSurf, byte *surfPos);
void R_SkinXModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex);
void R_SkinStaticModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex);
void R_SkinSceneEnt(GfxSceneEntity *sceneEnt, GfxEntity *ent);
static void R_SkinXSurfaceSkinned(struct GfxModelSkinnedSurface *skinnedSurf,
                                  const DObjSkelMat *boneMatrix);
void R_SkinXModelCmd(SkinXModelCmd *skinCmd, int context);
void R_SkinRigidXModelCmd(SkinRigidXModelCmd *skinRigidCmd);

static void *Hunk_AllocXModelPrecache(int size)
{
    return Hunk_AllocInternal(size);
}

static void *Hunk_AllocXModelPrecacheColl(int size)
{
    return Hunk_AllocInternal(size);
}

struct XModel *R_RegisterModel(const char *name)
{
    if (!R_ValidXModelName(name)) {
        ((void (*)(int, const char *, ...))ri.Printf)(2, "R_RegisterModel: Invalid model name '%s'\n", name);
        return NULL;
    }
    return XModelPrecache(name + 7, (Alloc_t)Hunk_AllocXModelPrecache, (Alloc_t)Hunk_AllocXModelPrecacheColl);
}

GfxBrushModel *R_RegisterInlineModel(int modelIndex)
{
    GfxWorld *world = rgp.world;
    return &world->models[modelIndex];
}

void R_SetIgnorePrecacheErrors(qboolean ignore)
{
    r_globals_t *rgg = (r_globals_t *)imp_rg;
    rgg->ignorePrecacheErrors = (ignore != 0);
}

qboolean R_GetIgnorePrecacheErrors(void)
{
    r_globals_t *rgg = (r_globals_t *)imp_rg;
    return rgg->ignorePrecacheErrors != 0;
}

void R_UnlockSkinnedCache(void)
{
    DxGlobals *dx;
    GfxBackEndData *frontEndData;
    IDirect3DVertexBuffer9 *vb;

    dx = (DxGlobals *)imp_dx;
    if (!dx->skinnedCacheLockAddr) {
        return;
    }

    dx->skinnedCacheLockAddr = NULL;
    frontEndData = frontEndDataOut;
    vb = (IDirect3DVertexBuffer9 *)frontEndData->skinnedCacheVb->buffer;

    do {
        ((BufferUnlockFn)VTABLE(vb)[0x30 / 4])((void *)vb);
    } while (*(volatile int *)&alwaysfails != 0);
}

struct DObj_s *R_GetGfxEntityDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent)
{
    r_globals_t *rgg = (r_globals_t *)imp_rg;

    int reTypeVal = ent->reType;
    struct DObj_s *obj = (struct DObj_s *)(intptr_t)reTypeVal;
    if (!obj) {
        obj = (struct DObj_s *)sceneEnt->u.obj;
    } else {
        obj = rgg->modelDObj;
        DObjSetModel(obj, (const XModel *)((void *)sceneEnt->u.data));
    }
    return obj;
}

void R_DObjReplaceMaterial(struct DObj_s *obj, int lod, int surfaceIndex, MaterialHandle material)
{
    char lods[8];
    int partBits[4];
    DSurface_s surfaces[64];
    int numModels = DObjGetNumModels(obj);

    for (int i = 0; i < numModels; i++)
        lods[i] = (char)lod;

    DObjGetSurfaces(obj, surfaces, partBits, lods);

    short modelIndex = surfaces[surfaceIndex].modelIndex;
    const struct trXSkin_t *skins = XModelGetSkins(DObjGetModel(obj, modelIndex));
    if (!skins)
        return;

    char lodVal = lods[modelIndex];
    short subMatIndex = surfaces[surfaceIndex].subMatIndex;
    ((MaterialHandle **)skins)[lodVal][subMatIndex] = material;
}

static void R_ReleaseModel(union XAssetHeader header, void *data)
{
    if (XModelBad(header))
        return;
    XModelUnoptimize(header);
}

void R_OptimizeAllModels(void)
{
    int v = r_optimizeXModels->current.integer;
    if (!v)
        return;
    DB_EnumXAssets(1, R_OptimizeModel, 0, 1);
}

void R_ReleaseAllModels(void)
{
    DB_EnumXAssets(1, R_ReleaseModel, 0, 1);
}

void *Model_Alloc(int size)
{
    return Hunk_AllocInternal(size);
}

static void R_OptimizeModel(XAssetHeader header, void *data)
{
    if (XModelBad(header))
        return;
    XModelOptimize(header.model);
}

void R_GetRigidTransform(const DObjSkelMat *bone, const vec_t *origin, vec3_t *axis, float scale, vec3_t *boneAxis)
{
    float *ax = (float *)axis;
    float *b = (float *)bone;
    float *ba = (float *)boneAxis;
    float sa[9];
    int row, col;

    for (col = 0; col < 9; col++)
        sa[col] = scale * ax[col];

    for (row = 0; row < 4; row++) {
        float r0 = b[row * 4 + 0];
        float r1 = b[row * 4 + 1];
        float r2 = b[row * 4 + 2];

        ba[row * 3 + 0] = r0 * sa[0] + r1 * sa[3] + r2 * sa[6];
        ba[row * 3 + 1] = r0 * sa[1] + r1 * sa[4] + r2 * sa[7];
        ba[row * 3 + 2] = r0 * sa[2] + r1 * sa[5] + r2 * sa[8];

        if (row == 3) {
            ba[row * 3 + 0] += origin[0];
            ba[row * 3 + 1] += origin[1];
            ba[row * 3 + 2] += origin[2];
        }
    }
}

void R_ModelBounds(GfxBrushModel *bmodel, vec_t *mins, vec_t *maxs)
{
    if (bmodel) {
        mins[0] = bmodel->bounds[0][0];
        mins[1] = bmodel->bounds[0][1];
        mins[2] = bmodel->bounds[0][2];
        maxs[0] = bmodel->bounds[1][0];
        maxs[1] = bmodel->bounds[1][1];
        maxs[2] = bmodel->bounds[1][2];
    } else {
        mins[0] = mins[1] = mins[2] = 0;
        maxs[0] = maxs[1] = maxs[2] = 0;
    }
}

extern void R_FatalLockError(HRESULT hr);

void R_LockSkinnedCache(GfxLockType lockType)
{
    GfxBackEndData *frontEndData = frontEndDataOut;
    byte *vb = (byte *)frontEndData->skinnedCacheVb->buffer;
    DxGlobals *dx = (DxGlobals *)imp_dx;
    void **vtable;
    DWORD lockFlags;
    HRESULT hr;

    if (!vb)
        return;

    if (dx->gpuSync != 0 || lockType != 0)
        lockFlags = 0x1000;
    else
        lockFlags = 0x2000;

    vtable = *(void ***)vb;
    hr = ((HRESULT (D3DVTCC *)(void *, UINT, UINT, void **, DWORD))(vtable[0x2C / 4]))(
        vb, 0, 0, (void **)&dx->skinnedCacheLockAddr, lockFlags);

    if (hr < 0)
        R_FatalLockError(hr);

    if ((uintptr_t)((DxGlobals *)dx)->skinnedCacheLockAddr & 0xf) {
        vtable = *(void ***)vb;
        ((HRESULT (D3DVTCC *)(void *))(vtable[0x30 / 4]))(vb);
        ((DxGlobals *)dx)->skinnedCacheLockAddr = NULL;
    }
}

void R_ShutdownModels(void)
{
    DB_EnumXAssets(1, R_ReleaseModel, 0, 1);
}

void R_FinishLoadingModels(void)
{
    if (r_optimizeXModels->current.integer == 0)
        return;
    DB_EnumXAssets(1, R_OptimizeModel, 0, 1);
}

void R_DObjGetSurfMaterials(struct DObj_s *obj, int lod, MaterialHandle *matHandleArray)
{
    char lods[8];
    int partBits[4];
    DSurface_s surfaces[64];
    int numModels = DObjGetNumModels(obj);

    for (int i = 0; i < numModels; i++)
        lods[i] = (char)lod;

    int surfaceCount = DObjGetSurfaces(obj, surfaces, partBits, lods);

    for (int surfaceIndex = 0; surfaceIndex < surfaceCount; surfaceIndex++) {
        short modelIndex = surfaces[surfaceIndex].modelIndex;
        const struct trXSkin_t *skins = XModelGetSkins(DObjGetModel(obj, modelIndex));
        if (skins) {
            char lodVal = lods[modelIndex];
            short subMatIndex = surfaces[surfaceIndex].subMatIndex;
            matHandleArray[surfaceIndex] = ((MaterialHandle **)skins)[lodVal][subMatIndex];
        } else {
            matHandleArray[surfaceIndex] = 0;
        }
    }
}

static int R_GetSurfaceData_impl(const byte *ent, const void *obj, void *surfaces, int *partBits, char *lods)
{
    r_globals_t *rg = (r_globals_t *)imp_rg;
    int modelCount, i;
    float dist, scale;

    modelCount = DObjGetNumModels((const DObj_s *)obj);
    scale = ((GfxEntity *)ent)->scale;

    dist = Vec3Distance(((GfxEntity *)ent)->origin, rg->lodParms.origin);
    dist = dist * rg->lodParms.scale + rg->lodParms.bias;

    if (scale != 0.0f)
        dist /= scale;

    for (i = 0; i < modelCount; i++) {
        lods[i] = (char)DObjGetLodForDist( (const DObj *)(obj), i, dist);
    }

    return DObjGetSurfaces((const DObj_s *)obj, (DSurface_s *)surfaces, partBits, lods);
}

static int R_GetSurfaceData(const byte *ent, const void *obj, void *surfaces, int *partBits, char *lods)
{
    return R_GetSurfaceData_impl(ent, obj, surfaces, partBits, lods);
}

static void R_XModelDebugBoxes_impl(const byte *sceneEnt, const byte *ent, const void *obj)
{
    byte surfaces[0x100];
    int partBits[5];
    char lods[16];
    void *boneInfoArray[200];
    int boneIndex, boneCount;

    R_GetSurfaceData_impl(ent, obj, surfaces, partBits, lods);

    if (*(void **)(sceneEnt + 8))
        CG_DObjCalcPose( (const centity_t *)(*(void **)(sceneEnt + 8)), (const DObj_s *)(obj), partBits);

    boneCount = DObjNumBones((const struct DObj_s *)obj);
    DObjGetBoneInfo((const struct DObj_s *)obj, (struct XBoneInfo_s **)boneInfoArray);
    byte *rotTransArray = (byte *)DObjGetRotTransArray((const DObj *)obj);
    if (!rotTransArray)
        return;

    vec4_t color = { 1.0f, 1.0f, 1.0f, 0.0f };
    byte *entMatrix = (byte *)(ent + 0x14);
    float *entOrigin = (float *)(ent + 0x3c);
    DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;

    for (boneIndex = 0; boneIndex < boneCount; boneIndex++) {

        if (!((partBits[boneIndex >> 5] >> (boneIndex & 0x1f)) & 1))
            continue;

        float *bi = (float *)boneInfoArray[boneIndex];
        byte *quatTrans = rotTransArray + boneIndex * 0x20;

        int edge;
        for (edge = 0; edge < 12; edge++) {
            const int *sv = boxVerts[edge * 2];
            const int *ev = boxVerts[edge * 2 + 1];

            vec3_t org, vec, start;
            org[0] = bi[sv[0] * 3 + 0];
            org[1] = bi[sv[1] * 3 + 1];
            org[2] = bi[sv[2] * 3 + 2];
            MatrixTransformVectorQuatTrans(org, (const DObjAnimMat *)(quatTrans), vec);
            MatrixTransformVector(vec, entMatrix, start);
            start[0] += entOrigin[0];
            start[1] += entOrigin[1];
            start[2] += entOrigin[2];

            vec3_t end;
            org[0] = bi[ev[0] * 3 + 0];
            org[1] = bi[ev[1] * 3 + 1];
            org[2] = bi[ev[2] * 3 + 2];
            MatrixTransformVectorQuatTrans(org, (const DObjAnimMat *)(quatTrans), vec);
            MatrixTransformVector(vec, entMatrix, end);
            end[0] += entOrigin[0];
            end[1] += entOrigin[1];
            end[2] += entOrigin[2];

            R_AddDebugLine(debugGlobals, start, end, color);
        }
    }
}

static void R_XModelDebugBoxes(const byte *sceneEnt, const byte *ent, const void *obj)
{
    R_XModelDebugBoxes_impl(sceneEnt, ent, obj);
}

static void R_XModelDebugAxes_impl(const byte *sceneEnt, const byte *ent, const void *obj)
{
    byte surfaces[0x100];
    int partBits[5];
    char lods[16];
    int boneIndex, boneCount, axis;

    R_GetSurfaceData_impl(ent, obj, surfaces, partBits, lods);

    if (*(void **)(sceneEnt + 8))
        CG_DObjCalcPose( (const centity_t *)(*(void **)(sceneEnt + 8)), (const DObj_s *)(obj), partBits);

    vec3_t translation[3] = { { 6.0f, 0.0f, 0.0f }, { 0.0f, 6.0f, 0.0f }, { 0.0f, 0.0f, 6.0f } };

    boneCount = DObjNumBones((const struct DObj_s *)obj);
    byte *rotTransArray = (byte *)DObjGetRotTransArray((const DObj *)obj);
    if (!rotTransArray || boneCount <= 0)
        return;

    byte *entMatrix = (byte *)(ent + 0x14);
    float *entOrigin = (float *)(ent + 0x3c);
    DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;

    for (boneIndex = 0; boneIndex < boneCount; boneIndex++) {
        if (!((partBits[boneIndex >> 5] >> (boneIndex & 0x1f)) & 1))
            continue;

        byte *quatTrans = rotTransArray + boneIndex * 0x20;

        for (axis = 0; axis < 3; axis++) {

            vec4_t color = { 0.0f, 0.0f, 0.0f, 0.0f };
            color[axis] = 1.0f;

            vec3_t vec, start;
            MatrixTransformVectorQuatTrans((const vec_t *)imp_vec3_origin, (const DObjAnimMat *)(quatTrans), vec);
            MatrixTransformVector(vec, entMatrix, start);
            start[0] += entOrigin[0];
            start[1] += entOrigin[1];
            start[2] += entOrigin[2];

            vec3_t end;
            MatrixTransformVectorQuatTrans(translation[axis], (const DObjAnimMat *)(quatTrans), vec);
            MatrixTransformVector(vec, entMatrix, end);
            end[0] += entOrigin[0];
            end[1] += entOrigin[1];
            end[2] += entOrigin[2];

            R_AddDebugLine(debugGlobals, start, end, color);
        }
    }
}

static void R_XModelDebugAxes(const byte *sceneEnt, const byte *ent, const void *obj)
{
    R_XModelDebugAxes_impl(sceneEnt, ent, obj);
}

extern int DObjBad(const DObj *obj);
extern struct XSurface_s *DObjGetSurface(const DObj *obj, int modelIndex, int subMatIndex, int lod);
extern int XSurfaceGetNumVerts(const XSurface *surface);
extern int InterlockedExchange(volatile int *dest, int value);
extern int InterlockedExchangeAdd(volatile int *dest, int value);
extern int XSurfaceGetBoneOffset(const XSurface *surf);
extern void ClearBounds(float *mins, float *maxs);
extern void GetRotatedBounds(vec3_t *baseBounds, const vec_t *origin, vec3_t *axis, vec3_t *rotatedBounds);
extern int InterlockedCompareExchange(volatile int *dest, int exchange, int comparand);
void R_UpdateXModelBounds(GfxSceneEntity *sceneEnt, GfxEntity *ent)
{
    r_globals_t *rgg = (r_globals_t *)imp_rg;
    char *se = (char *)sceneEnt;
    char *e = (char *)ent;
    void *obj;
    int boneCount, i;
    int partBits[4];
    short surfaces[67];
    char lods[8];
    float bounds[6];
    /* DObjGetBoneInfo fills this with one XBoneInfo* per bone (8 bytes each on
     * x64). The old `int boneInfo[128]` (512 B) held only 64 pointers, so a model
     * with >64 bones overran the stack -- clobbering the return address, which on
     * return jumped into heap garbage (rip in a heap page, no exe frames). Match
     * the other callers' `void *[200]` (R_XModelDebugBoxes_impl, sv_game_mp.c) and
     * dereference each entry below. */
    void *boneInfo[200];

    if (((GfxSceneEntity *)se)->cullState > 3)
        return;

    if (InterlockedCompareExchange(&((GfxSceneEntity *)se)->cullState, 0, 1) != 0) {
    }

    if (((GfxEntity *)e)->reType != 0) {
        void *defaultModel = rgg->modelDObj;
        obj = (void *)((GfxSceneEntity *)se)->u.data;
        DObjSetModel((DObj_s *)defaultModel, (const XModel *)(obj));
    } else {
        obj = (void *)((GfxSceneEntity *)se)->u.data;
    }

    if (DObjBad( (const DObj *)(obj))) {
        if (developer->current.integer) {
            R_XModelDebugBoxes_impl((const byte *)sceneEnt, (const byte *)ent, obj);
            R_XModelDebugAxes_impl((const byte *)sceneEnt, (const byte *)ent, obj);
        }
        goto set_origin_bounds;
    }

    {
        int surfCount;
        surfCount = R_GetSurfaceData_impl((const byte *)ent, obj, surfaces, partBits, lods);
        if (surfCount == 0)
            goto set_origin_bounds;
    }

    {
        void *anim = (void *)((GfxSceneEntity *)se)->cent;
        if (anim)
            CG_DObjCalcPose( (const centity_t *)(anim), (const DObj_s *)(obj), partBits);
    }

    {
        const DObjAnimMat *boneMatrix = DObjGetRotTransArray((const DObj *)obj);
        if (!boneMatrix)
            goto set_origin_bounds;

        ClearBounds(&bounds[0], &bounds[3]);
        DObjGetBoneInfo((const struct DObj_s *)obj, (struct XBoneInfo_s **)boneInfo);
        boneCount = DObjNumBones((const struct DObj_s *)obj);

        for (i = 0; i < boneCount; i++) {
            if (!(partBits[i >> 5] & (1 << (i & 0x1f))))
                continue;

            {
                const float *q = boneMatrix[i].quat;
                float w2 = boneMatrix[i].transWeight;
                const float *trans = boneMatrix[i].trans;
                /* boneInfo[i] is a pointer to bone i's XBoneInfo floats. The
                 * decomp read it inline as `&boneInfo[i*4]` (pointer bytes as
                 * bounds); dereference like the debug-box path at R_XModelDebugBoxes_impl. */
                const int *bi = (const int *)boneInfo[i];

                float xx2 = w2 * q[0], yy2 = w2 * q[1], zz2 = w2 * q[2];
                float xx = xx2 * q[0], xy = xx2 * q[1], xz = xx2 * q[2], xw = xx2 * q[3];
                float yy = yy2 * q[1], yz = yy2 * q[2], yw = yy2 * q[3];
                float zz = zz2 * q[2], zw = zz2 * q[3];

                float m00 = 1.0f - (yy + zz), m01 = xy + zw, m02 = xz - yw;
                float m10 = xy - zw, m11 = 1.0f - (xx + zz), m12 = yz + xw;
                float m20 = xz + yw, m21 = yz - xw, m22 = 1.0f - (xx + yy);

                float bmin, bmax;
                const float *bif = (const float *)bi;

#define AXIS_CONTRIB(rot, biOfs)                                         \
    do {                                                                 \
        int sel = (*(int *)&(rot)) >> 31 & 3;  \
        bmin += (rot) * bif[sel];                                        \
        bmax += (rot) * bif[3 - sel];                                    \
    } while (0)

                bmin = trans[0];
                bmax = trans[0];
                AXIS_CONTRIB(m00, 0);
                AXIS_CONTRIB(m10, 1);
                AXIS_CONTRIB(m20, 2);
                if (bmin < bounds[0])
                    bounds[0] = bmin;
                if (bmax > bounds[3])
                    bounds[3] = bmax;

                bmin = trans[1];
                bmax = trans[1];
                AXIS_CONTRIB(m01, 0);
                AXIS_CONTRIB(m11, 1);
                AXIS_CONTRIB(m21, 2);
                if (bmin < bounds[1])
                    bounds[1] = bmin;
                if (bmax > bounds[4])
                    bounds[4] = bmax;

                bmin = trans[2];
                bmax = trans[2];
                AXIS_CONTRIB(m02, 0);
                AXIS_CONTRIB(m12, 1);
                AXIS_CONTRIB(m22, 2);
                if (bmin < bounds[2])
                    bounds[2] = bmin;
                if (bmax > bounds[5])
                    bounds[5] = bmax;

#undef AXIS_CONTRIB
            }
        }

        GetRotatedBounds( (vec3_t (*))(bounds), ((GfxEntity *)e)->origin, (vec3_t (*))((float *)((GfxEntity *)e)->axis), (vec3_t (*))(((GfxSceneEntity *)se)->curMins));
        ((GfxSceneEntity *)se)->cullState = 2;
        return;
    }

set_origin_bounds:

    ((GfxSceneEntity *)se)->curMins[0] = ((GfxEntity *)e)->origin[0];
    ((GfxSceneEntity *)se)->curMins[1] = ((GfxEntity *)e)->origin[1];
    ((GfxSceneEntity *)se)->curMins[2] = ((GfxEntity *)e)->origin[2];
    ((GfxSceneEntity *)se)->curMaxs[0] = ((GfxEntity *)e)->origin[0];
    ((GfxSceneEntity *)se)->curMaxs[1] = ((GfxEntity *)e)->origin[1];
    ((GfxSceneEntity *)se)->curMaxs[2] = ((GfxEntity *)e)->origin[2];
    ((GfxSceneEntity *)se)->cullState = 2;
}

/* GfxModel*Surface packing offsets/sizes for the surface WRITE path (R_PreSkinXSurface /
   R_PreSkinStaticSurface). The reads (R_AddXModelSurfaces, R_SkinXSurfaceSkinned) use the
   TYPED structs, so these must match: on x64 GfxModelSurface = {surfType@0; XSurface*
   xsurf@8} (16B); skinned adds {int skinnedCachedOffset@16; union skinnedVert@24} (0x20);
   rigid adds {vec3 boneAxis[4]@16} (0x40). Keep x86 byte-identical. MUST agree with
   s_XModelSurfaceSize[] in r_scene.c. */
#if defined(COD2_X64)
#define PSK_XSURF   8
#define PSK_F2      16
#define PSK_F3      24
#define PSK_SZ_SKIN 0x20
#define PSK_SZ_RIGID 0x40
#else
#define PSK_XSURF   4
#define PSK_F2      8
#define PSK_F3      0xc
#define PSK_SZ_SKIN 0x10
#define PSK_SZ_RIGID 0x38
#endif

static int R_PreSkinXSurface(GfxSceneEntity *sceneEnt, const struct DObj_s *obj, const DSurface *surface, int surfaceIndex, char *lods, byte *surfPos)
{
    int surfIdx = surface->modelIndex;
    int skinIndex;
    void *model, *skins, *xsurf;
    void *material;

    model = DObjGetModel(obj, surfIdx);
    skins = (void *)XModelGetSkins((const XModel *)model);
    if (!skins)
        return 0;

    skinIndex = (signed char)lods[surfIdx];
    {
        int xsurfOfs = surface->subMatIndex;
        /* skins[skinIndex][xsurfOfs] -- pointer-of-pointers, strides are pointer-sized (was *4 = x86) */
        material = *(void **)((char *)*(void **)((char *)skins + skinIndex * (int)sizeof(void *)) + xsurfOfs * (int)sizeof(void *));
    }
    xsurf = DObjGetSurface(obj, surfIdx, surface->subMatIndex, skinIndex);

    ((GfxSceneEntity *)sceneEnt)->materials[surfaceIndex] = (const Material *)material;

    if (((XSurface *)xsurf)->surfRigid.vb) {

        *(int *)surfPos = 4;
        *(void **)(surfPos + PSK_XSURF) = xsurf;
        return PSK_SZ_RIGID;
    }

    if (r_skinCache->current.enabled && ((XSurface *)xsurf)->indexBuffer) {
        *(void **)(surfPos + PSK_F3) = 0;
        int vertCount = XSurfaceGetNumVerts( (const XSurface *)(xsurf));
        int isDx7 = (r_rendererInUse->current.integer == 2);
        int stride = isDx7 ? 0x24 : 0x40;
        char *dx = (char *)dx_g;

        if (((DxGlobals *)dx)->skinnedCacheLockAddr) {

            GfxBackEndData *fed = frontEndDataOut;
            void *lockPtr = fed->skinnedCacheVb;
            int offset = InterlockedExchangeAdd((volatile int *)lockPtr, vertCount * stride);
            int capacity = *(int *)((char *)lockPtr + 4);
            if (*(int *)lockPtr > capacity) {
                if (offset <= capacity) {
                    InterlockedExchange((volatile int *)lockPtr, offset);
                }
                if (*(int *)fed != warnCount) {
                    warnCount = *(int *)fed;
                    (*(int (**)(int, const char *, ...))&ri)(2, "MAX_SKINNED_CACHE_VERTICES exceeded\n");
                }
                offset = -1;
            }
            *(int *)(surfPos + PSK_F2) = offset;
            if (offset >= 0) {
                void *basePtr = ((DxGlobals *)dx)->skinnedCacheLockAddr;
                if ((char *)basePtr + *(int *)lockPtr) {

                    *(int *)surfPos = 3;
                    *(void **)(surfPos + PSK_XSURF) = xsurf;
                    return PSK_SZ_SKIN;
                }
            }
        }
    }

    *(int *)(surfPos + PSK_F2) = -1;
    {
        int vertCount = XSurfaceGetNumVerts( (const XSurface *)(xsurf));
        int isDx7 = (r_rendererInUse->current.integer == 2);
        int stride = isDx7 ? 0x24 : 0x40;
        int needed = vertCount * stride;
        char *dx = (char *)dx_g;
        int current = ((DxGlobals *)dx)->tempSkinPos;
        if (current + needed > 0xa00000) {
            GfxBackEndData *fed = frontEndDataOut;
            if (*(int *)fed != warnCount) {
                warnCount = *(int *)fed;
                (*(int (**)(int, const char *, ...))&ri)(2, "Exceeded dynamic vertex buffer limit\n");
            }
            return 0;
        }
        *(void **)(surfPos + PSK_F3) = (char *)((DxGlobals *)dx)->tempSkinBuf + current; /* was (int)ptr -> truncated */
        ((DxGlobals *)dx)->tempSkinPos += needed;

        ((void (*)(void *, int))ri.Z_VirtualCommitInternal)(*(void **)(surfPos + PSK_F3), needed);
    }
    *(int *)surfPos = 3;
    *(void **)(surfPos + PSK_XSURF) = xsurf;
    return PSK_SZ_SKIN;
}

extern int InterlockedExchangeAdd(volatile int *dest, int value);
extern void R_AddFrontendCmd(int type, void *data);
extern int DObjGetMatOffset(const DObj *obj, int modelIndex);
void R_SkinSceneDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent)
{
    char *se = (char *)sceneEnt;
    void *obj = (void *)sceneEnt->u.data;
#ifdef GFX_REAL_D3D9
    SMT_Log("DOBJ", obj ? DObjGetModel((const struct DObj_s *)obj, 0) : NULL, ent);
#endif
    int surfaceCount, boneCount;
    const DObjAnimMat *boneMatrix;
    DSurface surfaces[64];
    int partBits[4];
    char lods[8];
#if defined(COD2_X64)
    byte surfBuf[7040]; /* x64: surfaces are ~2x (8-byte ptrs); double the x86 3520 */
#else
    byte surfBuf[3520];
#endif

    if (((GfxSceneEntity *)se)->cullState > 3)
        return;

    if (InterlockedCompareExchange(&((GfxSceneEntity *)se)->cullState, 2, 3) != 2) {
    }

    if (DObjBad( (const DObj *)(obj))) {
        if (developer->current.integer) {
            R_XModelDebugBoxes_impl((const byte *)sceneEnt, (const byte *)ent, obj);
            R_XModelDebugAxes_impl((const byte *)sceneEnt, (const byte *)ent, obj);
        }
        ((GfxSceneEntity *)se)->cullState = 4;
        return;
    }

    boneCount = DObjNumBones((const struct DObj_s *)obj);
    {
        int sc;
        sc = R_GetSurfaceData_impl((const byte *)ent, obj, surfaces, partBits, lods);
        surfaceCount = sc;
    }
    if (surfaceCount == 0) {
        ((GfxSceneEntity *)se)->cullState = 4;
        return;
    }

    boneMatrix = DObjGetRotTransArray((const DObj *)obj);
    if (!boneMatrix) {
        ((GfxSceneEntity *)se)->cullState = 4;
        return;
    }

    {
        int startIndex = InterlockedExchangeAdd((volatile int *)&scene.sceneEntMaterialCount, surfaceCount);
        extern int __mh_execute_header;
        if (startIndex + surfaceCount > 4096 /*MAX_SCENE_SURFS_PLUS_ENTITIES; was &__mh_execute_header magic (=0x1000 on Mac), garbage on x64*/) {
            scene.sceneEntMaterialCount = 4096 /*MAX_SCENE_SURFS_PLUS_ENTITIES; was &__mh_execute_header magic (=0x1000 on Mac), garbage on x64*/;
            {
                GfxBackEndData *fed = frontEndDataOut;
                if (*(int *)fed != warnCount) {
                    warnCount = *(int *)fed;
                    typedef void (*PrintFunc)(int, const char *, ...);
                    (*(PrintFunc *)&ri)(2, "MAX_SCENE_SURFS_PLUS_ENTITIES exceeded\n");
                }
            }
            ((GfxSceneEntity *)se)->cullState = 4;
            return;
        }

        ((GfxSceneEntity *)se)->materials = &scene.sceneMaterials[startIndex];
    }

    {
        byte *surfPtr = surfBuf;
        int i;

        for (i = 0; i < surfaceCount; i++) {
            int result = R_PreSkinXSurface(sceneEnt, (const struct DObj_s *)obj,
                                           &surfaces[i], i, lods, surfPtr);
            if (!result) {
                ((GfxSceneEntity *)se)->cullState = 4;
                return;
            }
            surfPtr += result;
        }

        {
            int size = (int)(surfPtr - surfBuf);
            GfxBackEndData *fed = frontEndDataOut;
            int offset = InterlockedExchangeAdd((volatile int *)&fed->surfPos, size);
            if (offset + size > 0x20000) {
                fed->surfPos = 0x20000;
                if (*(int *)fed != warnCount) {
                    warnCount = *(int *)fed;
                    typedef void (*PrintFunc)(int, const char *, ...);
                    (*(PrintFunc *)&ri)(2, "MAX_SKINNED_CACHE exceeded\n");
                }
                ((GfxSceneEntity *)se)->cullState = 4;
                return;
            }

            ((GfxSceneEntity *)se)->surfs = (surfaceType_t *)(fed->surfsBuffer + offset);
            memcpy(fed->surfsBuffer + offset, surfBuf, size);
        }
    }

    ((GfxSceneEntity *)se)->surfCount = surfaceCount;

    {
        const dvar_t *xdebugDvar = r_xdebug;
        int xdebug = xdebugDvar->current.integer;
        if (xdebug) {
            if (xdebug & 1) {
                R_XModelDebugBoxes_impl((const byte *)sceneEnt, (const byte *)ent, obj);
                xdebugDvar = r_xdebug;
            }
            if (xdebugDvar->current.integer & 2) {
                R_XModelDebugAxes_impl((const byte *)sceneEnt, (const byte *)ent, obj);
            }
        }
    }

    if (boneCount == 1) {

        SkinRigidXModelCmd rigidCmd;
        rigidCmd.surfs = ((GfxSceneEntity *)se)->surfs;
        rigidCmd.surfCount = surfaceCount;
        rigidCmd.e = (GfxEntity *)ent;

        memcpy(&rigidCmd.mat, boneMatrix, sizeof(DObjAnimMat));
        R_AddFrontendCmd(2, &rigidCmd);
    } else {

        SkinXModelCmd skinCmd;
        int i;
        memset(&skinCmd, 0, sizeof(skinCmd));
        skinCmd.surfs = ((GfxSceneEntity *)se)->surfs;
        skinCmd.surfCount = (byte)surfaceCount;
        skinCmd.e = (GfxEntity *)ent;
        skinCmd.mat = boneMatrix;
        skinCmd.boneCount = (byte)boneCount;
        memcpy(skinCmd.surfacePartBits, partBits, sizeof(skinCmd.surfacePartBits));

        for (i = 0; i < surfaceCount; i++) {
            skinCmd.matOffset[i] = (byte)DObjGetMatOffset( (const DObj *)(obj), surfaces[i].modelIndex);
        }

        R_AddFrontendCmd(surfaceCount > 10 ? 7 : 6, &skinCmd);
    }

    ((GfxSceneEntity *)se)->cullState = 4;
}

extern GfxStaticModelSurfaceCached *R_CacheStaticModelSurface(GfxStaticSurface *staticSurf, const XSurface *xsurf, int smodelIndex, const Material *material);
extern void R_UsedCachedStaticModelSurface(GfxStaticModelSurfaceCached *surf);
static int R_PreSkinStaticSurface(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex,
                                  const struct XModel *model, XSurface *xsurf, int surfaceIndex, int lod,
                                  qboolean *needSkinningSurf, byte *surfPos)
{
    r_globals_t *rgg = (r_globals_t *)imp_rg;
    void *skins = (void *)XModelGetSkins((const XModel *)model);
    if (!skins)
        return 0;

    {
        /* skins is XModel**[lod] of Material*[surf]; the strides are pointer-sized (was *4 = x86) */
        void *material = *(void **)((char *)*(void **)((char *)skins + lod * (int)sizeof(void *)) + surfaceIndex * (int)sizeof(void *));
        ((GfxSceneEntity *)sceneEnt)->materials[surfaceIndex] = (const Material *)material;
    }

    if (ent->reType == 2 && r_smc_enable->current.enabled) {
        void *xsurfMat = (void *)xsurf;
        if (XSurfaceGetBoneOffset((const XSurface *)xsurfMat) != -1) { /* was (int)(intptr_t) -> truncated */
            int isDx7 = (r_rendererInUse->current.integer == 2);
            if (!isDx7) {

                void *mat = (void *)((GfxSceneEntity *)sceneEnt)->materials[surfaceIndex];
                void *techSet = (void *)((Material *)mat)->techniqueSet;
                MaterialTechnique *tech34 = techSet ? ((MaterialTechniqueSet *)techSet)->techniques[12] : 0;
#ifdef GFX_REAL_D3D9
                if (getenv("REALD3D9_SMCLOG")) {
                    static int sl;
                    static const char *seen[256];
                    static int ns;
                    int k;
                    const char *nm = ((Material *)mat)->info.name;
                    for (k = 0; k < ns; k++)
                        if (seen[k] == nm)
                            goto smc_logged;
                    if (ns < 256)
                        seen[ns++] = nm;
                    if (sl++ < 200)
                        fprintf(stderr, "[SMC] mat='%.30s' techSet=%p tech12=%p passCnt=%d grid=%d -> %s\n",
                                nm ? nm : "?", techSet, (void *)tech34,
                                tech34 ? tech34->passCount : -1,
                                (tech34 && tech34->passCount) ? tech34->passArray.dx7[0].gridLighting : -1,
                                (tech34 && tech34->passCount && tech34->passArray.dx7[0].gridLighting) ? "CACHE" : "PER-FRAME-SKIN");
                    fflush(stderr);
                smc_logged:;
                }
#endif
                if (tech34 && tech34->passCount && tech34->passArray.dx7[0].gridLighting) {
                    goto try_smc;
                }
            } else {
            try_smc: {
                /* was: smcData + smodelIndex*8+4 (x86 GfxStaticModelDynamic stride/staticSurfs
                 * offset), surfaceIndex*16 (x86 GfxStaticSurface stride), lod*4 (x86 ptr). All
                 * grow on x64 -> use typed access. */
                GfxStaticModelDynamic *smcData = rgg->smodelDyncs;
                GfxStaticSurface *staticSurf = &smcData[smodelIndex].staticSurfs[surfaceIndex];
                GfxStaticModelSurfaceCached *cached = staticSurf->cachedLods[lod];
#ifdef GFX_REAL_D3D9
                if (getenv("REALD3D9_SMCHIT")) {
                    extern int g_smc_hit, g_smc_new;
                    if (cached)
                        g_smc_hit++;
                    else
                        g_smc_new++;
                }
#endif
                if (!cached) {
                    void *mat = (void *)((GfxSceneEntity *)sceneEnt)->materials[surfaceIndex];
#ifdef GFX_REAL_D3D9
                    extern long long g_smc_recache_ms;
                    extern long long QueryPerf(void);
                    long long _t0 = getenv("REALD3D9_SMCHIT") ? QueryPerf() : 0;
#endif
                    cached = (GfxStaticModelSurfaceCached *)R_CacheStaticModelSurface(staticSurf, xsurf, smodelIndex, (const Material *)(mat));
#ifdef GFX_REAL_D3D9
                    if (_t0)
                        g_smc_recache_ms += QueryPerf() - _t0;
#endif
                    staticSurf->cachedLods[lod] = cached;
                    if (!cached)
                        goto no_smc;
                }
                R_UsedCachedStaticModelSurface(cached);
                *(int *)surfPos = 5;
                *(void **)(surfPos + PSK_XSURF) = xsurf;
                *(void **)(surfPos + PSK_F2) = cached;
                *(void **)(surfPos + PSK_F3) = ent;
                return PSK_SZ_SKIN;
            }
            }
        }
    }
no_smc:

    if (((XSurface *)xsurf)->surfRigid.vb) {
        *(int *)surfPos = 4;
        *(void **)(surfPos + PSK_XSURF) = xsurf;
        *needSkinningSurf = 1;
        return PSK_SZ_RIGID;
    }

    if (r_skinCache->current.enabled && ((XSurface *)xsurf)->indexBuffer) {
        *(void **)(surfPos + PSK_F3) = 0;
        int vertCount = XSurfaceGetNumVerts(xsurf);
        int isDx7 = (r_rendererInUse->current.integer == 2);
        int stride = isDx7 ? 0x24 : 0x40;
        char *dx = (char *)dx_g;
        if (((DxGlobals *)dx)->skinnedCacheLockAddr) {
            GfxBackEndData *fed = frontEndDataOut;
            void *lockPtr = fed->skinnedCacheVb;
            int offset = InterlockedExchangeAdd((volatile int *)lockPtr, vertCount * stride);
            if (*(int *)lockPtr > *(int *)((char *)lockPtr + 4)) {
                if (offset <= *(int *)((char *)lockPtr + 4))
                    InterlockedExchange((volatile int *)lockPtr, offset);
                if (*(int *)fed != warnCount) {
                    warnCount = *(int *)fed;
                    (*(int (**)(int, const char *, ...))&ri)(2, "MAX_SKINNED_CACHE_VERTICES exceeded\n");
                }
                offset = -1;
            }
            *(int *)(surfPos + PSK_F2) = offset;
            if (offset >= 0 && (char *)((DxGlobals *)dx)->skinnedCacheLockAddr + *(int *)lockPtr) {
                *(int *)surfPos = 3;
                *(void **)(surfPos + PSK_XSURF) = xsurf;
                *needSkinningSurf = 1;
                return PSK_SZ_SKIN;
            }
        }
    }
    *(int *)(surfPos + PSK_F2) = -1;
    {
        int vertCount = XSurfaceGetNumVerts(xsurf);
        int isDx7 = (r_rendererInUse->current.integer == 2);
        int stride = isDx7 ? 0x24 : 0x40;
        int needed = vertCount * stride;
        char *dx = (char *)dx_g;
        int current = ((DxGlobals *)dx)->tempSkinPos;
        if (current + needed > 0xa00000) {
            GfxBackEndData *fed = frontEndDataOut;
            if (*(int *)fed != warnCount) {
                warnCount = *(int *)fed;
                (*(int (**)(int, const char *, ...))&ri)(2, "Exceeded dynamic vertex buffer limit\n");
            }
            return 0;
        }
        *(void **)(surfPos + PSK_F3) = (char *)((DxGlobals *)dx)->tempSkinBuf + current; /* was (int)ptr -> truncated */
        ((DxGlobals *)dx)->tempSkinPos += needed;
        ((void (*)(void *, int))ri.Z_VirtualCommitInternal)(*(void **)(surfPos + PSK_F3), needed);
    }
    *(int *)surfPos = 3;
    *(void **)(surfPos + PSK_XSURF) = xsurf;
    *needSkinningSurf = 1;
    return PSK_SZ_SKIN;
}

extern int XModelNumBones(const struct XModel *model);
extern int XModelGetSurfaces(const XModel *model, XSurface ***surfaces, int lod, int **partBits);
extern int XModelGetLodForDist(const XModel *model, float dist);
extern const DObjAnimMat *XModelGetBasePose(const struct XModel *model);
void R_SkinXModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex)
{
    r_globals_t *rgg = (r_globals_t *)imp_rg;
    char *se = (char *)sceneEnt;
    char *e = (char *)ent;
    XModel *model = (XModel *)((GfxSceneEntity *)se)->u.data;
#ifdef GFX_REAL_D3D9
    SMT_Log("STATIC", (struct XModel *)model, ent);
#endif
    int boneCount, surfaceCount, lod;
    XSurface **surfacesPtr;
    int *partBits;
    qboolean needSkinningSurf = 0;
#if defined(COD2_X64)
    byte surfBuf[7040]; /* x64: surfaces are ~2x (8-byte ptrs); double the x86 3520 */
#else
    byte surfBuf[3520];
#endif

    if (((GfxSceneEntity *)se)->cullState > 3)
        return;

    if (InterlockedCompareExchange(&((GfxSceneEntity *)se)->cullState, 2, 3) != 2) {
    }

    if (XModelBad(*(union XAssetHeader *)&model)) {
        if (developer->current.integer) {
            void *defaultObj = rgg->modelDObj;
            DObjSetModel((struct DObj_s *)defaultObj, model);
            R_XModelDebugBoxes_impl((const byte *)sceneEnt, (const byte *)ent, defaultObj);
            R_XModelDebugAxes_impl((const byte *)sceneEnt, (const byte *)ent, defaultObj);
        }
        ((GfxSceneEntity *)se)->cullState = 4;
        return;
    }

    boneCount = XModelNumBones(model);

    {
        r_globals_t *rg = (r_globals_t *)imp_rg;
        float dist = Vec3Distance(((GfxEntity *)e)->origin, rg->lodParms.origin);
        dist = dist * rg->lodParms.scale + rg->lodParms.bias;
        float scale = ((GfxEntity *)e)->scale;
        if (scale != 0.0f)
            dist /= scale;
        lod = XModelGetLodForDist(model, dist);
    }
    if (lod < 0) {
        ((GfxSceneEntity *)se)->cullState = 4;
        return;
    }

    surfaceCount = XModelGetSurfaces(model, &surfacesPtr, lod, &partBits);

    {
        int startIdx = InterlockedExchangeAdd((volatile int *)&scene.sceneEntMaterialCount, surfaceCount);
        extern int __mh_execute_header;
        if (startIdx + surfaceCount > 4096 /*MAX_SCENE_SURFS_PLUS_ENTITIES; was &__mh_execute_header magic (=0x1000 on Mac), garbage on x64*/) {
            scene.sceneEntMaterialCount = 4096 /*MAX_SCENE_SURFS_PLUS_ENTITIES; was &__mh_execute_header magic (=0x1000 on Mac), garbage on x64*/;
            GfxBackEndData *fed = frontEndDataOut;
            if (*(int *)fed != warnCount) {
                warnCount = *(int *)fed;
                (*(int (**)(int, const char *, ...))&ri)(2, "MAX_SCENE_SURFS_PLUS_ENTITIES exceeded\n");
            }
            ((GfxSceneEntity *)se)->cullState = 4;
            return;
        }
        ((GfxSceneEntity *)se)->materials = &scene.sceneMaterials[startIdx];
    }

    {
        byte *surfPtr = surfBuf;
        int i;
        XSurface **surfArray = surfacesPtr;
        for (i = 0; i < surfaceCount; i++) {
            int result = R_PreSkinStaticSurface(sceneEnt, ent, smodelIndex,
                                                model, surfArray[i], i, lod, &needSkinningSurf, surfPtr);
            if (!result) {
                ((GfxSceneEntity *)se)->cullState = 4;
                return;
            }
            surfPtr += result;
        }

        {
            int size = (int)(surfPtr - surfBuf);
            GfxBackEndData *fed = frontEndDataOut;
            int offset = InterlockedExchangeAdd((volatile int *)&fed->surfPos, size);
            if (offset + size > 0x20000) {
                fed->surfPos = 0x20000;
                if (*(int *)fed != warnCount) {
                    warnCount = *(int *)fed;
                    (*(int (**)(int, const char *, ...))&ri)(2, "MAX_SKINNED_CACHE exceeded\n");
                }
                ((GfxSceneEntity *)se)->cullState = 4;
                return;
            }
            ((GfxSceneEntity *)se)->surfs = (surfaceType_t *)(fed->surfsBuffer + offset);
            memcpy(fed->surfsBuffer + offset, surfBuf, size);
        }
    }

    ((GfxSceneEntity *)se)->surfCount = surfaceCount;

    {
        int xdebug = r_xdebug->current.integer;
        if (xdebug) {
            void *defaultObj = rgg->modelDObj;
            DObjSetModel((struct DObj_s *)defaultObj, model);
            if (xdebug & 1) {
                R_XModelDebugBoxes_impl((const byte *)sceneEnt, (const byte *)ent, defaultObj);
            }
            if (r_xdebug->current.integer & 2) {
                R_XModelDebugAxes_impl((const byte *)sceneEnt, (const byte *)ent, defaultObj);
            }
        }
    }

    if (!needSkinningSurf) {
        ((GfxSceneEntity *)se)->cullState = 4;
        return;
    }

    {
        const DObjAnimMat *basePose = XModelGetBasePose(model);
        if (boneCount == 1) {
            SkinRigidXModelCmd rigidCmd;
            rigidCmd.surfs = ((GfxSceneEntity *)se)->surfs;
            rigidCmd.surfCount = surfaceCount;
            rigidCmd.e = (GfxEntity *)ent;
            memcpy(&rigidCmd.mat, basePose, sizeof(DObjAnimMat));
            R_AddFrontendCmd(2, &rigidCmd);
        } else {
            SkinXModelCmd skinCmd;
            int i;
            memset(&skinCmd, 0, sizeof(skinCmd));
            skinCmd.surfs = ((GfxSceneEntity *)se)->surfs;
            skinCmd.surfCount = (byte)surfaceCount;
            skinCmd.e = (GfxEntity *)ent;
            skinCmd.mat = basePose;
            skinCmd.boneCount = (byte)boneCount;
            memcpy(skinCmd.surfacePartBits, partBits, sizeof(skinCmd.surfacePartBits));
            for (i = 0; i < surfaceCount; i++)
                skinCmd.matOffset[i] = 0;
            R_AddFrontendCmd(surfaceCount > 10 ? 7 : 6, &skinCmd);
        }
    }
    ((GfxSceneEntity *)se)->cullState = 4;
}

void R_SkinStaticModel(GfxSceneEntity *sceneEnt, GfxEntity *ent, int smodelIndex)
{
    R_SkinXModel(sceneEnt, ent, smodelIndex);
}

void R_SkinSceneEnt(GfxSceneEntity *sceneEnt, GfxEntity *ent)
{
    if (ent->reType == 1)
        R_SkinXModel(sceneEnt, ent, -1);
    else
        R_SkinSceneDObj(sceneEnt, ent);
}

extern XVertexInfo *XSurfaceGetVertexInfoArray(const XSurface *surf);

static void R_SkinXSurfaceSkinned(struct GfxModelSkinnedSurface *skinnedSurf,
                                  const DObjSkelMat *boneMatrix)
{
    const XSurface *xsurf = skinnedSurf->surf.xsurf;
    int cachedOffset = skinnedSurf->skinnedCachedOffset;
#ifdef W32_SKINTRACE
    {
        static int n;
        if ((n++ % 200) == 0)
            fprintf(stderr, "[skin] call#%d xsurf=%p cachedOff=%d bone=%p\n",
                    n, (void *)xsurf, cachedOffset, (void *)boneMatrix);
        fflush(stderr);
    }
#endif
    char *out;
    int boneOffset;
    int vertCount;
    int i;
    const char *vp;
    const XVertexInfo *vi;

    if (cachedOffset < 0)
        out = (char *)skinnedSurf->skinnedVert.variant;
    else
        out = (char *)dx.skinnedCacheLockAddr + cachedOffset; /* was (int)ptr -> truncated on x64 */

    boneOffset = XSurfaceGetBoneOffset(xsurf); /* was (int)(intptr_t)xsurf -> truncated the surface ptr */

    if (r_rendererInUse->current.integer == 2) {

        if (boneOffset == -1) {

            vi = XSurfaceGetVertexInfoArray(xsurf);
            vertCount = XSurfaceGetNumVerts( (const XSurface *)((void *)(intptr_t)xsurf));
            if (vertCount <= 0)
                return;
            vp = (const char *)vi;
            for (i = 0; i < vertCount; i++) {
                const XVertexInfo *v = (const XVertexInfo *)vp;
                const DObjSkelMat *mat =
                    (const DObjSkelMat *)((const char *)boneMatrix + (short)v->boneOffset);
                float pos[3], nrm[3];
                int w;

                pos[0] = (v->offset[0] * mat->axis[0][0] + v->offset[1] * mat->axis[1][0]) + (v->offset[2] * mat->axis[2][0] + mat->origin[0]);
                pos[1] = (v->offset[0] * mat->axis[0][1] + v->offset[1] * mat->axis[1][1]) + (v->offset[2] * mat->axis[2][1] + mat->origin[1]);
                pos[2] = (v->offset[0] * mat->axis[0][2] + v->offset[1] * mat->axis[1][2]) + (v->offset[2] * mat->axis[2][2] + mat->origin[2]);
                nrm[0] = v->normal[0] * mat->axis[0][0] + v->normal[1] * mat->axis[1][0] + v->normal[2] * mat->axis[2][0];
                nrm[1] = v->normal[0] * mat->axis[0][1] + v->normal[1] * mat->axis[1][1] + v->normal[2] * mat->axis[2][1];
                nrm[2] = v->normal[0] * mat->axis[0][2] + v->normal[1] * mat->axis[1][2] + v->normal[2] * mat->axis[2][2];

                if ((w = v->numWeights) != 0) {
                    const XBlendInfo *b = (const XBlendInfo *)(vp + sizeof(XVertexInfo));
                    float scale = (float)v->boneWeight * 0.00390625f;
                    int j;
                    pos[0] *= scale;
                    pos[1] *= scale;
                    pos[2] *= scale;
                    for (j = 0; j < w; j++, b++) {
                        const DObjSkelMat *m2 =
                            (const DObjSkelMat *)((const char *)boneMatrix + (short)b->boneOffset);
                        float s = (float)b->boneWeight * 1.52587890625e-05f;
                        pos[0] += (b->offset[0] * m2->axis[0][0] + b->offset[1] * m2->axis[1][0] + b->offset[2] * m2->axis[2][0] + m2->origin[0]) * s;
                        pos[1] += (b->offset[0] * m2->axis[0][1] + b->offset[1] * m2->axis[1][1] + b->offset[2] * m2->axis[2][1] + m2->origin[1]) * s;
                        pos[2] += (b->offset[0] * m2->axis[0][2] + b->offset[1] * m2->axis[1][2] + b->offset[2] * m2->axis[2][2] + m2->origin[2]) * s;
                    }
                    vp = (const char *)b;
                } else {
                    vp += sizeof(XVertexInfo);
                }

                ((float *)out)[0] = pos[0];
                ((float *)out)[1] = pos[1];
                ((float *)out)[2] = pos[2];
                ((float *)out)[3] = nrm[0];
                ((float *)out)[4] = nrm[1];
                ((float *)out)[5] = nrm[2];
                *(unsigned int *)(out + 0x18) = *(const unsigned int *)v->color;
                ((float *)out)[7] = v->texCoordX;
                ((float *)out)[8] = v->texCoordY;
                out += 0x24;
            }
            return;
        }

        vi = XSurfaceGetVertexInfoArray(xsurf);
        vertCount = XSurfaceGetNumVerts( (const XSurface *)((void *)(intptr_t)xsurf));
        {
            const DObjSkelMat *mat = (const DObjSkelMat *)((const char *)boneMatrix + boneOffset);
            if (vertCount <= 0)
                return;
            vp = (const char *)vi;
            for (i = 0; i < vertCount; i++) {
                const XVertexInfo *v = (const XVertexInfo *)vp;
                unsigned int col = *(const unsigned int *)v->color;
                float texX = v->texCoordX, texY = v->texCoordY;
                float nx = v->normal[0], n1 = v->normal[1], n2 = v->normal[2];

                ((float *)out)[3] = nx * mat->axis[0][0] + n1 * mat->axis[1][0] + n2 * mat->axis[2][0];
                ((float *)out)[4] = nx * mat->axis[0][1] + n1 * mat->axis[1][1] + n2 * mat->axis[2][1];
                ((float *)out)[5] = nx * mat->axis[0][2] + n1 * mat->axis[1][2] + n2 * mat->axis[2][2];
                ((float *)out)[0] = v->offset[0] * mat->axis[0][0] + v->offset[1] * mat->axis[1][0] + v->offset[2] * mat->axis[2][0] + mat->origin[0];
                ((float *)out)[1] = v->offset[0] * mat->axis[0][1] + v->offset[1] * mat->axis[1][1] + v->offset[2] * mat->axis[2][1] + mat->origin[1];
                ((float *)out)[2] = v->offset[0] * mat->axis[0][2] + v->offset[1] * mat->axis[1][2] + v->offset[2] * mat->axis[2][2] + mat->origin[2];
                *(unsigned int *)(out + 0x18) = col;
                ((float *)out)[7] = texX;
                ((float *)out)[8] = texY;
                vp += sizeof(XVertexInfo);
                out += 0x24;
            }
        }
        return;
    }

    if (boneOffset == -1) {

        vi = XSurfaceGetVertexInfoArray(xsurf);
        vertCount = XSurfaceGetNumVerts( (const XSurface *)((void *)(intptr_t)xsurf));
        if (vertCount <= 0)
            return;
        vp = (const char *)vi;
        for (i = 0; i < vertCount; i++) {
            const XVertexInfo *v = (const XVertexInfo *)vp;
            const DObjSkelMat *mat =
                (const DObjSkelMat *)((const char *)boneMatrix + (short)v->boneOffset);
            float pos[3], nrm[3], bin[3], tan[3];
            int w;

            pos[0] = (v->offset[0] * mat->axis[0][0] + v->offset[1] * mat->axis[1][0]) + (v->offset[2] * mat->axis[2][0] + mat->origin[0]);
            pos[1] = (v->offset[0] * mat->axis[0][1] + v->offset[1] * mat->axis[1][1]) + (v->offset[2] * mat->axis[2][1] + mat->origin[1]);
            pos[2] = (v->offset[0] * mat->axis[0][2] + v->offset[1] * mat->axis[1][2]) + (v->offset[2] * mat->axis[2][2] + mat->origin[2]);
            nrm[0] = v->normal[0] * mat->axis[0][0] + v->normal[1] * mat->axis[1][0] + v->normal[2] * mat->axis[2][0];
            nrm[1] = v->normal[0] * mat->axis[0][1] + v->normal[1] * mat->axis[1][1] + v->normal[2] * mat->axis[2][1];
            nrm[2] = v->normal[0] * mat->axis[0][2] + v->normal[1] * mat->axis[1][2] + v->normal[2] * mat->axis[2][2];
            bin[0] = v->binormal[0] * mat->axis[0][0] + v->binormal[1] * mat->axis[1][0] + v->binormal[2] * mat->axis[2][0];
            bin[1] = v->binormal[0] * mat->axis[0][1] + v->binormal[1] * mat->axis[1][1] + v->binormal[2] * mat->axis[2][1];
            bin[2] = v->binormal[0] * mat->axis[0][2] + v->binormal[1] * mat->axis[1][2] + v->binormal[2] * mat->axis[2][2];
            tan[0] = v->tangent[0] * mat->axis[0][0] + v->tangent[1] * mat->axis[1][0] + v->tangent[2] * mat->axis[2][0];
            tan[1] = v->tangent[0] * mat->axis[0][1] + v->tangent[1] * mat->axis[1][1] + v->tangent[2] * mat->axis[2][1];
            tan[2] = v->tangent[0] * mat->axis[0][2] + v->tangent[1] * mat->axis[1][2] + v->tangent[2] * mat->axis[2][2];

            if ((w = v->numWeights) != 0) {
                const XBlendInfo *b = (const XBlendInfo *)(vp + sizeof(XVertexInfo));
                float scale = (float)v->boneWeight * 0.00390625f;
                int j;
                pos[0] *= scale;
                pos[1] *= scale;
                pos[2] *= scale;
                for (j = 0; j < w; j++, b++) {
                    const DObjSkelMat *m2 =
                        (const DObjSkelMat *)((const char *)boneMatrix + (short)b->boneOffset);
                    float s = (float)b->boneWeight * 1.52587890625e-05f;
                    pos[0] += (b->offset[0] * m2->axis[0][0] + b->offset[1] * m2->axis[1][0] + b->offset[2] * m2->axis[2][0] + m2->origin[0]) * s;
                    pos[1] += (b->offset[0] * m2->axis[0][1] + b->offset[1] * m2->axis[1][1] + b->offset[2] * m2->axis[2][1] + m2->origin[1]) * s;
                    pos[2] += (b->offset[0] * m2->axis[0][2] + b->offset[1] * m2->axis[1][2] + b->offset[2] * m2->axis[2][2] + m2->origin[2]) * s;
                }
                vp = (const char *)b;
            } else {
                vp += sizeof(XVertexInfo);
            }

            ((float *)out)[0] = pos[0];
            ((float *)out)[1] = pos[1];
            ((float *)out)[2] = pos[2];
            ((float *)out)[3] = 1.0f;
            ((float *)out)[4] = nrm[0];
            ((float *)out)[5] = nrm[1];
            ((float *)out)[6] = nrm[2];
            *(unsigned int *)(out + 0x1c) = *(const unsigned int *)v->color;
            ((float *)out)[8] = v->texCoordX;
            ((float *)out)[9] = v->texCoordY;
            ((float *)out)[10] = bin[0];
            ((float *)out)[11] = bin[1];
            ((float *)out)[12] = bin[2];
            ((float *)out)[13] = tan[0];
            ((float *)out)[14] = tan[1];
            ((float *)out)[15] = tan[2];
            out += 0x40;
        }
        return;
    }

    vi = XSurfaceGetVertexInfoArray(xsurf);
    vertCount = XSurfaceGetNumVerts( (const XSurface *)((void *)(intptr_t)xsurf));
    {
        const DObjSkelMat *mat = (const DObjSkelMat *)((const char *)boneMatrix + boneOffset);
        if (vertCount <= 0)
            return;
        vp = (const char *)vi;
        for (i = 0; i < vertCount; i++) {
            const XVertexInfo *v = (const XVertexInfo *)vp;
            unsigned int col = *(const unsigned int *)v->color;
            float texX = v->texCoordX, texY = v->texCoordY;
            float nx = v->normal[0];

            ((float *)out)[4] = nx * mat->axis[0][0] + v->normal[1] * mat->axis[1][0] + v->normal[2] * mat->axis[2][0];
            ((float *)out)[5] = nx * mat->axis[0][1] + v->normal[1] * mat->axis[1][1] + v->normal[2] * mat->axis[2][1];
            ((float *)out)[6] = nx * mat->axis[0][2] + v->normal[1] * mat->axis[1][2] + v->normal[2] * mat->axis[2][2];
            ((float *)out)[10] = v->binormal[0] * mat->axis[0][0] + v->binormal[1] * mat->axis[1][0] + v->binormal[2] * mat->axis[2][0];
            ((float *)out)[11] = v->binormal[0] * mat->axis[0][1] + v->binormal[1] * mat->axis[1][1] + v->binormal[2] * mat->axis[2][1];
            ((float *)out)[12] = v->binormal[0] * mat->axis[0][2] + v->binormal[1] * mat->axis[1][2] + v->binormal[2] * mat->axis[2][2];
            ((float *)out)[13] = v->tangent[0] * mat->axis[0][0] + v->tangent[1] * mat->axis[1][0] + v->tangent[2] * mat->axis[2][0];
            ((float *)out)[14] = v->tangent[0] * mat->axis[0][1] + v->tangent[1] * mat->axis[1][1] + v->tangent[2] * mat->axis[2][1];
            ((float *)out)[15] = v->tangent[0] * mat->axis[0][2] + v->tangent[1] * mat->axis[1][2] + v->tangent[2] * mat->axis[2][2];
            ((float *)out)[0] = v->offset[0] * mat->axis[0][0] + v->offset[1] * mat->axis[1][0] + v->offset[2] * mat->axis[2][0] + mat->origin[0];
            ((float *)out)[1] = v->offset[0] * mat->axis[0][1] + v->offset[1] * mat->axis[1][1] + v->offset[2] * mat->axis[2][1] + mat->origin[1];
            ((float *)out)[2] = v->offset[0] * mat->axis[0][2] + v->offset[1] * mat->axis[1][2] + v->offset[2] * mat->axis[2][2] + mat->origin[2];
            ((float *)out)[3] = 1.0f;
            *(unsigned int *)(out + 0x1c) = col;
            ((float *)out)[8] = texX;
            ((float *)out)[9] = texY;
            vp += sizeof(XVertexInfo);
            out += 0x40;
        }
    }
}

void R_SkinXModelCmd(SkinXModelCmd *skinCmd, int context)
{
    char *matArray = (char *)imp_g_skinBuffers + context * 5 * 8192;
    const DObjAnimMat *bones = skinCmd->mat;
    int boneCount = skinCmd->boneCount;
    int boneIndex;
    const surfaceType_t *surfPos;
    int surfaceIndex;

    for (boneIndex = 0; boneIndex < boneCount; boneIndex++) {

        if (skinCmd->surfacePartBits[boneIndex >> 5] & (1 << (boneIndex & 0x1f))) {
            const float *q = bones[boneIndex].quat;
            const float *t = bones[boneIndex].trans;
            float w2 = bones[boneIndex].transWeight;
            float *m = (float *)(matArray + boneIndex * 64);

            float xx2 = w2 * q[0], yy2 = w2 * q[1], zz2 = w2 * q[2];
            float xx = xx2 * q[0], xy = xx2 * q[1], xz = xx2 * q[2], xw = xx2 * q[3];
            float yy = yy2 * q[1], yz = yy2 * q[2], yw = yy2 * q[3];
            float zz = zz2 * q[2], zw = zz2 * q[3];

            m[0] = 1.0f - (yy + zz);
            m[1] = zw + xy;
            m[2] = xz - yw;
            m[3] = 0.0f;
            m[4] = xy - zw;
            m[5] = 1.0f - (xx + zz);
            m[6] = xw + yz;
            m[7] = 0.0f;
            m[8] = xz + yw;
            m[9] = yz - xw;
            m[10] = 1.0f - (xx + yy);
            m[11] = 0.0f;
            m[12] = t[0];
            m[13] = t[1];
            m[14] = t[2];
            m[15] = 1.0f;
        }
    }

    surfPos = skinCmd->surfs;
    for (surfaceIndex = 0; surfaceIndex < skinCmd->surfCount; surfaceIndex++) {
        int surfType = *(const int *)surfPos;
        const DObjSkelMat *boneMatrix = (const DObjSkelMat *)(matArray + skinCmd->matOffset[surfaceIndex] * 64);
#ifdef GFX_REAL_D3D9

        if (getenv("REALD3D9_STALECHK") && (surfType == 3 || surfType == 4)) {
            int b = skinCmd->matOffset[surfaceIndex];
            int flagged = (skinCmd->surfacePartBits[b >> 5] >> (b & 0x1f)) & 1;
            if (!flagged) {
                static int sc;
                if (sc++ < 40)
                    fprintf(stderr, "[STALE] surf=%d surfType=%d(%s) matOffsetBone=%d NOT in partBits (boneCount=%d)%s\n",
                            surfaceIndex, surfType, surfType == 4 ? "RIGID" : "skinned", b, boneCount,
                            surfType == 4 ? "  <<< REAL stale-matrix bug" : "  (skinned: maybe ok)");
                fflush(stderr);
            }
        }
#endif

        if (surfType == 3) {

            R_SkinXSurfaceSkinned((struct GfxModelSkinnedSurface *)surfPos, boneMatrix);
            surfPos = (const surfaceType_t *)((const byte *)surfPos + PSK_SZ_SKIN);   /* was x86 16 */
        } else if (surfType == 5) {

            surfPos = (const surfaceType_t *)((const byte *)surfPos + PSK_SZ_SKIN);   /* was x86 16 */
        } else {

            const GfxModelRigidSurface *rigidSurf = (const GfxModelRigidSurface *)surfPos;
            surfPos = (const surfaceType_t *)((const byte *)surfPos + PSK_SZ_RIGID);   /* was x86 0x38 */
            {
                extern int XSurfaceGetBoneOffset(const XSurface *surf);
                /* was *(int*)(rigidSurf+4): x86 xsurf offset + truncated ptr; xsurf @8 on x64 */
                int boneOffset = XSurfaceGetBoneOffset(rigidSurf->surf.xsurf);
                GfxEntity *refEnt = skinCmd->e;
                R_GetRigidTransform(
                    (const DObjSkelMat *)((const char *)boneMatrix + boneOffset),
                    refEnt->origin,
                    refEnt->axis,
                    refEnt->scale,
                    (vec3_t *)rigidSurf->boneAxis);   /* was rigidSurf+8 (x86 boneAxis offset) */
            }
        }
    }
}

extern int XSurfaceGetBoneOffset(const XSurface *surf);

void R_SkinRigidXModelCmd(SkinRigidXModelCmd *skinRigidCmd)
{
    float *q = skinRigidCmd->mat.quat;
    float scale = skinRigidCmd->mat.transWeight;

    float sx = scale * q[0], sy = scale * q[1], sz = scale * q[2];
    float xx = sx * q[0], xy = sx * q[1], xz = sx * q[2], xw = sx * q[3];
    float yy = sy * q[1], yz = sy * q[2], yw = sy * q[3];
    float zz = sz * q[2], zw = sz * q[3];

    float mtx[16];
    mtx[0] = 1.0f - yy - zz;
    mtx[1] = xy + zw;
    mtx[2] = xz - yw;
    mtx[3] = 0.0f;
    mtx[4] = xy - zw;
    mtx[5] = 1.0f - xx - zz;
    mtx[6] = yz + xw;
    mtx[7] = 0.0f;
    mtx[8] = xz + yw;
    mtx[9] = yz - xw;
    mtx[10] = 1.0f - xx - yy;
    mtx[11] = 0.0f;

    mtx[12] = skinRigidCmd->mat.trans[0];
    mtx[13] = skinRigidCmd->mat.trans[1];
    mtx[14] = skinRigidCmd->mat.trans[2];
    mtx[15] = 1.0f;

    byte *surfPos = (byte *)skinRigidCmd->surfs;
    GfxEntity *refEnt = skinRigidCmd->e;
    int i;

    for (i = 0; i < skinRigidCmd->surfCount; i++) {
        int surfType = *(int *)surfPos;

        if (surfType == 5) {
            /* cached surface, same 0x20 packing as skinned (was x86 0x10) */
            surfPos += PSK_SZ_SKIN;
            continue;
        }

        if (surfType == 4) {

            GfxModelRigidSurface *rigidSurf = (GfxModelRigidSurface *)surfPos;
            surfPos += PSK_SZ_RIGID;   /* was x86 0x38 */
            float entScale = refEnt->scale;
            /* was *(int*)(rigidSurf+4): x86 xsurf offset + truncated ptr; xsurf is @8 on x64 */
            int boneOffset = XSurfaceGetBoneOffset(rigidSurf->surf.xsurf);
            R_GetRigidTransform(
                (const DObjSkelMat *)((byte *)mtx + boneOffset),
                refEnt->origin,
                refEnt->axis,
                entScale,
                (vec3_t *)rigidSurf->boneAxis);   /* was rigidSurf+8 (x86 boneAxis offset) */
        } else {

            byte *surf = surfPos;
            surfPos += PSK_SZ_SKIN;   /* was x86 0x10 */
            R_SkinXSurfaceSkinned((struct GfxModelSkinnedSurface *)surf, (const DObjSkelMat *)mtx);
        }
    }
}

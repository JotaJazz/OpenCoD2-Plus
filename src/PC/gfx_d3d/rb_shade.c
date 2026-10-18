#include "common_types.h"
extern struct DxGlobals dx;
/* File-scope alias: bound where no local can shadow `dx`, so uses below
   always reach the global even inside functions that declare their own `dx`. */
static struct DxGlobals * const dx_g = &dx;
extern dvar_t *r_rendererInUse;
extern const dvar_t *r_objectiveColorDx7Min;
extern const dvar_t *r_objectiveColorDx7Max;
extern const dvar_t *r_outdoorAwayBias;
extern const dvar_t *r_outdoorDownBias;
extern const dvar_t *r_debugShader;
extern const dvar_t *r_drawPrimFloor;
extern const dvar_t *r_drawPrimCap;
extern materialCommands_t tess;
extern r_global_permanent_t rgp;
extern struct DxState dxState;
#include <math.h>
#include "imports.h"
extern refimport_t ri;
extern int alwaysfails;
#include "bytematch.h"

extern void RB_ChangeAlphaStageState(int stageIndex, int texStageBits);
extern void RB_ChangeColorStageState(int stageIndex, int texStageBits);
extern void RB_ChangeState_0(int stateBits0);
extern void RB_ChangeState_1(int stateBits1);
extern void RB_SetSampler(int samplerIndex, int samplerState, GfxImage *image);
extern void RB_SetSamplerConstantDx7(unsigned int color);
extern void RB_SetViewMatrixForWDx7(float w);
extern void RB_UpdateFogColor(FogColorSrcEnum fogColorSrc);

extern DxGlobals dx;

extern const float lightGridLookupMatrix[4][4];
extern const vec4_t debugShaderConsts[];
extern const GfxStateOverride overrideEnableRenormalize;
extern const DWORD s_fvfForVertDeclType[];

extern void R_FatalLockError(HRESULT hr);
extern void R_Error(int level, const char *msg, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void RB_ChangeIndices(IDirect3DIndexBuffer9 *ib);
extern void RB_ChangeStreamSource(int streamIndex, IDirect3DVertexBuffer9 *vb, int vertexOffset, int vertexStride);
extern void *CColorConverter_GetColorConverter(int mode);
extern void MatrixInverse44(const float *mat, float *dst);
extern void MatrixTranspose44(const float *in, float *out);
extern void MatrixMultiply44(const float (*a)[4], const float (*b)[4], float (*out)[4]);
extern void MatrixTransformVector44(const float *vec, const float (*mat)[4], float *out);
extern Bool RB_GetViewport(GfxViewport *viewport);
extern void MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(void *proj, float width, float height);
extern void RB_UpdateViewport(void);
extern int RB_SetIteratorFog(void);
extern int RB_DeriveEntityLights(vec4_t *colorForDir, float sunVisibility, const Material *material, D3DLIGHT9 *lights, int maxLights);
extern void RB_SetupEntityLighting(const GfxEntity *ent, GfxEntityLighting *lighting);
extern void RB_SetCodeConstant(int constant, vec_t x, vec_t y, vec_t z, vec_t w);

static inline char *RB_TessBase(void)
{
    return (char *)imp_tess;
}

void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex);
int RB_SetIndexData(const r_index_t *indices, int indexCount);
void RB_DrawIndexedPrim(const GfxDrawPrimArgs *args, int primCount);
static void RB_GetTextureFromCode_impl(int codeTexture, void **image, byte *samplerState);
#ifndef __EMSCRIPTEN__
static void RB_GetTextureFromCode(int codeTexture, void **image, byte *samplerState);
#endif
static void RB_SetEntityHwLightsDx7_impl(vec4_t *colorForDir, float sunVisibility);
#ifndef __EMSCRIPTEN__
static void RB_SetEntityHwLightsDx7(vec4_t *colorForDir, float sunVisibility);
#endif
void RB_CreateDynamicBuffers(void);
static void RB_SetupLighting_impl(void);
static void RB_SetupLighting(void);
void RB_SetVertexData(unsigned int streamIndex, const void *data, int vertexCount, int stride);
static void RB_SetShaderAndDecl(const MaterialPassDx9 *pass, MaterialVertexDeclType vertDeclType, byte *dxState);
static const float *RB_GetCodeMatrix(int source, int firstRow);
static void __attribute_regparm__(3) RB_DrawSingleTechnique(MaterialTechniqueType techType, MaterialVertexDeclType vertDeclType, const GfxDrawPrimArgs *args, const GfxStateOverride *stateOverride);
void RB_DrawTechnique(MaterialVertexDeclType vertDeclType, const GfxDrawPrimArgs *args);
void RB_EndSurface(void);

extern int g_tess_since_begin;
int g_begin_surface_calls = 0;

static int RB_MaterialNameContains(const Material *material, const char *needle)
{
    const char *name;
    const char *hay;
    const char *n;

    if (!material || !needle || !*needle) {
        return 0;
    }

    name = material->info.name;
    if (!name) {
        return 0;
    }

    for (hay = name; *hay; ++hay) {
        n = needle;
        while (*n && hay[n - needle] == *n) {
            ++n;
        }
        if (!*n) {
            return 1;
        }
    }

    return 0;
}

static void RB_X64TraceEndSurface(const char *stage, const Material *material,
                                  const MaterialTechnique *technique, const materialCommands_t *mc)
{
#if defined(COD2_X64) || defined(_M_X64) || defined(__x86_64__)
    static int traceCount;
    FILE *f;

    if (traceCount >= 160 || !RB_MaterialNameContains(material, "fonts/gamefonts")) {
        return;
    }

    f = fopen("x64_surface_trace.txt", traceCount ? "a" : "w");
    if (!f) {
        return;
    }

    {
        const MaterialPassDx9 *pass = technique ? &technique->passArray.dx9[0] : NULL;
        const MaterialTextureDef *tex = material && material->textureCount > 0 ? &material->textures[0] : NULL;
        const GfxImage *image = tex && tex->semantic != 5 ? tex->u.image : NULL;

        fprintf(f,
                "surface[%d] %s mat=%p name=%s techType=%d tech=%p passCount=%u flags=0x%x idx=%d verts=%d optIdx=%d decl=%d viewportNull=%d texCount=%u tex0=%s texState=0x%x texSem=%d img=%p imgName=%s imgSem=%d imgSize=%ux%u imgTex=%p vArgs=%u pArgs=%u vs=%s ps=%s\n",
                traceCount,
                stage ? stage : "<null>",
                (const void *)material,
                material && material->info.name ? material->info.name : "<null>",
                mc ? mc->techType : -1,
                (const void *)technique,
                technique ? (unsigned)technique->passCount : 0,
                technique ? (unsigned)technique->flags : 0,
                mc ? mc->indexCount : -1,
                mc ? mc->vertexCount : -1,
                mc ? mc->optimizedIndexCount : -1,
                mc ? mc->declType : -1,
                dxState.viewportIsNull,
                material ? (unsigned)material->textureCount : 0,
                tex && tex->name ? tex->name : "<null>",
                tex ? (unsigned)tex->samplerState : 0,
                tex ? (int)tex->semantic : -1,
                (const void *)image,
                image && image->name ? image->name : "<null>",
                image ? (int)image->semantic : -1,
                image ? image->width : 0,
                image ? image->height : 0,
                image ? (void *)image->texture.basemap : NULL,
                pass ? (unsigned)pass->vertexArgCount : 0,
                pass ? (unsigned)pass->pixelArgCount : 0,
                pass && pass->vertexShader && pass->vertexShader->name ? pass->vertexShader->name : "<null>",
                pass && pass->pixelShader && pass->pixelShader->name ? pass->pixelShader->name : "<null>");
    }
    fclose(f);
    traceCount++;
#else
    (void)stage;
    (void)material;
    (void)technique;
    (void)mc;
#endif
}

static void RB_X64TraceSampler(const char *stage, const Material *material, int argType, int dest,
                               const char *sourceName, int codeTexture, GfxImage *image, int samplerState)
{
#if defined(COD2_X64) || defined(_M_X64) || defined(__x86_64__)
    static int traceCount;
    FILE *f;

    if (traceCount >= 80 || !RB_MaterialNameContains(material, "fonts/gamefonts")) {
        return;
    }

    f = fopen("x64_sampler_trace.txt", traceCount ? "a" : "w");
    if (!f) {
        return;
    }

    fprintf(f,
            "sampler[%d] %s mat=%s type=%d dest=%d source=%s code=%d state=0x%x image=%p imageName=%s mapType=%d semantic=%d size=%ux%u tex=%p\n",
            traceCount,
            stage ? stage : "<null>",
            material && material->info.name ? material->info.name : "<null>",
            argType,
            dest,
            sourceName ? sourceName : "<null>",
            codeTexture,
            samplerState,
            (void *)image,
            image && image->name ? image->name : "<null>",
            image ? image->mapType : -1,
            image ? image->semantic : -1,
            image ? image->width : 0,
            image ? image->height : 0,
            image ? (void *)image->texture.basemap : NULL);
    fclose(f);
    traceCount++;
#else
    (void)stage;
    (void)material;
    (void)argType;
    (void)dest;
    (void)sourceName;
    (void)codeTexture;
    (void)image;
    (void)samplerState;
#endif
}

void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex)
{
    char *tess = RB_TessBase();
    (*(int *)&((materialCommands_t *)tess)->declType) = 0;
    (*(int *)&((materialCommands_t *)tess)->optimizedVertexSource) = 0;
    ((materialCommands_t *)tess)->indexCount = 0;
    ((materialCommands_t *)tess)->vertexCount = 0;
    ((materialCommands_t *)tess)->firstVertex = 0;
    ((materialCommands_t *)tess)->lastVertex = 0;
    ((materialCommands_t *)tess)->material = material;
    (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType) = techType;
    ((materialCommands_t *)tess)->lmapIndex = lmapIndex;
}

int RB_SetIndexData(const r_index_t *indices, int indexCount)
{
    char *dx = (char *)dx_g;
    int indexDataSize = indexCount * 2;
    char *lockState = (*(char **)&((DxGlobals *)dx)->dynamicIndexBuffer);
    Bool overflow;
    unsigned int byteOffset;
    int baseIndex;
    IDirect3DIndexBuffer9 *ib;
    DWORD lockFlags;
    void *bufferData;
    HRESULT hr;

    overflow = (*(int *)lockState + indexDataSize) > *(int *)(lockState + 4);
    if (overflow) {
        *(int *)lockState = 0;
        lockState = (*(char **)&((DxGlobals *)dx)->dynamicIndexBuffer);
    }

    if (*(int *)lockState == 0) {
        lockState = dx + __builtin_offsetof(DxGlobals, dynamicIndexBufferPool);
        (*(char **)&((DxGlobals *)dx)->dynamicIndexBuffer) = lockState;
    }

    byteOffset = *(unsigned int *)lockState;
    baseIndex = byteOffset >> 1;
    ib = *(IDirect3DIndexBuffer9 **)(lockState + 8);

    if (!overflow && (((DxGlobals *)dx)->gpuSync != 0 || byteOffset != 0))
        lockFlags = 0x1000;
    else
        lockFlags = 0x2000;

    hr = ((HRESULT (*)(IDirect3DIndexBuffer9 *, UINT, UINT, void **, DWORD))(*(void ***)(ib))[11])(ib, (UINT)byteOffset, (UINT)indexDataSize, &bufferData, lockFlags);
    if (hr < 0)
        R_FatalLockError(hr);

    Com_Memcpy(bufferData, indices, indexDataSize);

    do {
        ((HRESULT (*)(IDirect3DIndexBuffer9 *))(*(void ***)(ib))[12])(ib);
    } while (*(int *)&alwaysfails);

    if (ib != dxState.indexBuffer)
        RB_ChangeIndices(ib);

    *(int *)(*(char **)&((DxGlobals *)dx)->dynamicIndexBuffer) += indexDataSize;

    return baseIndex;
}

void RB_DrawIndexedPrim(const GfxDrawPrimArgs *args, int primCount)
{
    IDirect3DDevice9 *device;
    void **vtable;

    if (!args || primCount <= 0)
        return;

    device = (IDirect3DDevice9 *)dx.device;
    if (!device)
        return;

    vtable = *(void ***)device;
    if (!vtable || !vtable[0x148 / 4])
        return;

#ifdef GFX_REAL_D3D9
    {
        extern int g_rb_dip_calls;
        g_rb_dip_calls++;
    }
    if (getenv("REALD3D9_NODRAW"))
        return;
    if (getenv("REALD3D9_SCENELOG")) {
        static int sl = 0;
        if ((sl++ % 200) == 0)
            fprintf(stderr, "[SCENELOG] draw: inScene=%d\n", dx.inScene);
    }

    if (getenv("REALD3D9_VPLOG") && args->vertexCount >= (getenv("REALD3D9_VPALL") ? 0u : 200u)) {
        static int budget = 40;
        if (budget > 0) {
            budget--;
            float vp[6];
            IDirect3DSurface9 *rt = NULL;
            unsigned char d[64];
            ((HRESULT(D3DVTCC *)(void *, void *))vtable[0xC0 / 4])(device, vp);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0x98 / 4])(device, 0, &rt);
            unsigned rtw = 0, rth = 0;
            if (rt) {
                memset(d, 0, sizeof d);
                ((HRESULT(D3DVTCC *)(void *, void *))((*(void ***)rt)[0x30 / 4]))(rt, d);
                rtw = *(unsigned *)(d + 24);
                rth = *(unsigned *)(d + 28);
                ((HRESULT(D3DVTCC *)(void *))((*(void ***)rt)[2]))(rt);
            }
            void *bb = dx.renderTargets[0].colorSurface;
            void *vs = NULL, *ps = NULL, *tex0 = NULL, *tex1 = NULL;
            DWORD cwm = 0, zenable = 0, ztest = 0;
            ((HRESULT(D3DVTCC *)(void *, void *))vtable[0x174 / 4])(device, &vs);
            ((HRESULT(D3DVTCC *)(void *, void *))vtable[0x1B0 / 4])(device, &ps);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0x100 / 4])(device, 0, &tex0);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0x100 / 4])(device, 1, &tex1);
            DWORD abe = 0, srcb = 0, dstb = 0, ate = 0, fvf = 99;
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 168 , &cwm);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 7 , &zenable);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 23 , &ztest);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 27 , &abe);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 19 , &srcb);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 20 , &dstb);
            ((HRESULT(D3DVTCC *)(void *, DWORD, void *))vtable[0xE8 / 4])(device, 15 , &ate);
            (void)fvf;
            if (vs)
                ((HRESULT(D3DVTCC *)(void *))((*(void ***)vs)[2]))(vs);
            if (ps)
                ((HRESULT(D3DVTCC *)(void *))((*(void ***)ps)[2]))(ps);
            if (tex0)
                ((HRESULT(D3DVTCC *)(void *))((*(void ***)tex0)[2]))(tex0);
            if (tex1)
                ((HRESULT(D3DVTCC *)(void *))((*(void ***)tex1)[2]))(tex1);

            if (getenv("REALD3D9_CONSTDUMP")) {
                static int cd2 = 0;
                if (cd2 < 3) {
                    cd2++;
                    float vsc[256 * 4], psc[64 * 4];
                    int r;
                    memset(vsc, 0, sizeof vsc);
                    memset(psc, 0, sizeof psc);
                    ((HRESULT(D3DVTCC *)(void *, UINT, void *, UINT))vtable[0x17C / 4])(device, 0, vsc, 96);
                    ((HRESULT(D3DVTCC *)(void *, UINT, void *, UINT))vtable[0x1B8 / 4])(device, 0, psc, 32);
                    fprintf(stderr, "[CONSTDUMP] nonzero VS regs:");
                    for (r = 0; r < 96; r++)
                        if (vsc[r * 4] || vsc[r * 4 + 1] || vsc[r * 4 + 2] || vsc[r * 4 + 3])
                            fprintf(stderr, " c%d=[%.2f %.2f %.2f %.2f]", r, vsc[r * 4], vsc[r * 4 + 1], vsc[r * 4 + 2], vsc[r * 4 + 3]);
                    fprintf(stderr, "\n[CONSTDUMP] nonzero PS regs:");
                    for (r = 0; r < 32; r++)
                        if (psc[r * 4] || psc[r * 4 + 1] || psc[r * 4 + 2] || psc[r * 4 + 3])
                            fprintf(stderr, " c%d=[%.2f %.2f %.2f %.2f]", r, psc[r * 4], psc[r * 4 + 1], psc[r * 4 + 2], psc[r * 4 + 3]);
                    fprintf(stderr, "\n");
                }
            }

            if (tex0 && getenv("REALD3D9_TEXDUMP")) {
                static int td = 0;
                if (td < 4) {
                    td++;

                    unsigned char ld[64];
                    memset(ld, 0, sizeof ld);
                    ((HRESULT(D3DVTCC *)(void *, UINT, void *))((*(void ***)tex0)[0x44 / 4]))(tex0, 0, ld);
                    unsigned tw = *(unsigned *)(ld + 24), th = *(unsigned *)(ld + 28);
                    unsigned fmt = *(unsigned *)ld, pool = *(unsigned *)(ld + 12);
                    struct {
                        int Pitch;
                        void *pBits;
                    } lr;
                    lr.pBits = 0;
                    lr.Pitch = 0;
                    HRESULT lhr = ((HRESULT(D3DVTCC *)(void *, UINT, void *, void *, DWORD))((*(void ***)tex0)[0x4C / 4]))(tex0, 0, &lr, NULL, 0x10 );
                    if (lhr >= 0 && lr.pBits && lr.Pitch > 0) {
                        unsigned *px = (unsigned *)lr.pBits;
                        unsigned acc = 0;
                        int i;
                        for (i = 0; i < 8; i++)
                            acc |= px[i];
                        fprintf(stderr, "[TEXDUMP] t0=%p %ux%u fmt=%u pool=%u pitch=%d px0=%08x px1=%08x OR8=%08x\n",
                                tex0, tw, th, fmt, pool, lr.Pitch, px[0], px[1], acc);
                        ((HRESULT(D3DVTCC *)(void *, UINT))((*(void ***)tex0)[0x50 / 4]))(tex0, 0);
                    } else {
                        fprintf(stderr, "[TEXDUMP] t0=%p %ux%u fmt=%u pool=%u LOCK FAILED hr=0x%08x\n",
                                tex0, tw, th, fmt, pool, (unsigned)lhr);
                    }
                }
            }
            fprintf(stderr, "[VPLOG] %s vp%ux%u | vs=%p ps=%p t0=%p t1=%p cwm=%lu | blend=%lu src=%lu dst=%lu atest=%lu | verts=%u\n",
                    (rt == bb ? "BB" : "OFF"), ((unsigned *)vp)[2], ((unsigned *)vp)[3],
                    vs, ps, tex0, tex1, (unsigned long)cwm,
                    (unsigned long)abe, (unsigned long)srcb, (unsigned long)dstb, (unsigned long)ate,
                    (unsigned)args->vertexCount);
            fflush(stderr);
        }
    }
#endif
    { extern volatile int g_lastdraw[6];
      g_lastdraw[0] = args->u.buf.baseVertex; g_lastdraw[1] = args->firstVertexFromBase;
      g_lastdraw[2] = args->vertexCount; g_lastdraw[3] = args->u.buf.baseIndex;
      g_lastdraw[4] = primCount; g_lastdraw[5] = g_lastdraw[5] + 1; }
    do {
        ((HRESULT(D3DVTCC *)(IDirect3DDevice9 *, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT))
             vtable[0x148 / 4])(device,
                                D3DPT_TRIANGLELIST,
                                args->u.buf.baseVertex,
                                (UINT)args->firstVertexFromBase,
                                (UINT)args->vertexCount,
                                (UINT)args->u.buf.baseIndex,
                                (UINT)primCount);
    } while (*(int *)&alwaysfails);
}

static void RB_GetTextureFromCode_impl(int codeTexture, void **image, byte *samplerState)
{
    char *rgp = (char *)imp_rgp;
    char *tess;
    r_backEndGlobals_t *backEnd = (r_backEndGlobals_t *)imp_backEnd;
    char *dx = (char *)dx_g;
    int lmapIdx;
    char *drawSurfs;
    int idx;

    switch (codeTexture) {
    default:
        *samplerState = 0;
        *image = NULL;
        return;

    case 0:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->blackImage);
        *samplerState = 1;
        return;

    case 1:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->whiteImage);
        *samplerState = 1;
        return;

    case 2:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->identityNormalMapImage);
        *samplerState = 1;
        return;

    case 3:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->specularityImage);
        *samplerState = 0x32;
        return;

    case 4: {
        char *surfs = (*(char **)&((r_global_permanent_t *)rgp)->world);
        if (!surfs || !(*(void **)&((GfxWorld *)surfs)->smodelLightingImage))
            Com_Error(1, "Sampler 'smodelLighting' is only valid in a map.\n");
        *image = *(void **)((char *)(*(char **)&((r_global_permanent_t *)rgp)->world) + 0x10c);
        *samplerState = 0x72;
        return;
    }

    case 5:
    case 6:
        *image = (&((r_global_permanent_t *)rgp)->whiteImage)[codeTexture];
        *samplerState = 0x32;
        return;

    case 7:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->lightmapWeightsImage);
        *samplerState = 0x32;
        return;

    case 8:
    case 9:
    case 10:
    case 11: {
        tess = (char *)imp_tess;
        lmapIdx = ((materialCommands_t *)tess)->lmapIndex;
        if (lmapIdx == 0x1f) {
            char *mat = (*(char **)&((materialCommands_t *)tess)->material);
            R_Error(0, "Material '%s' tried to use a lightmap but doesn't have one set.\n\nThis is usually because a model skin uses a world material type.\nSometimes it is caused by a material switching after a map was\ncompiled to a surface type that uses a lightmap.\n", *(char **)mat);
            tess = (char *)imp_tess;
            lmapIdx = ((materialCommands_t *)tess)->lmapIndex;
        }
        drawSurfs = (*(char **)&((r_global_permanent_t *)rgp)->world);
        *image = *(void **)((*(char **)&((GfxWorld *)drawSurfs)->lightmaps) + lmapIdx * 16 + codeTexture * 4 - 0x20);
        *samplerState = 0x32;

        {
            int mode = *(int *)((char *)*(void **)imp_r_lightMap + 8);
            if (mode == 1) {
                *image = (*(void **)&((r_global_permanent_t *)rgp)->whiteImage);
                *samplerState = 1;
            } else if (mode == 2) {
                *image = (*(void **)&((r_global_permanent_t *)rgp)->blackImage);
                *samplerState = 1;
            }
        }
        return;
    }

    case 12:
        *image = (*(void **)&((DxGlobals *)dx)->renderTargets[4].image);
        *samplerState = 0x32;
        return;

    case 13:
        *image = (void *)backEnd->currentFeedbackImage;
        *samplerState = 0x32;
        return;

    case 14:
        idx = backEnd->resolvedPostSunTarget;
        *image = (void *)((DxGlobals *)dx)->renderTargets[idx].image;
        *samplerState = 0x32;
        return;

    case 15:
        idx = backEnd->resolvedSceneTarget;
        *image = (void *)((DxGlobals *)dx)->renderTargets[idx].image;
        *samplerState = 0x32;
        return;

    case 16: {
        drawSurfs = (*(char **)&((r_global_permanent_t *)rgp)->world);
        if (!drawSurfs || !(*(void **)&((GfxWorld *)drawSurfs)->skyImage))
            R_Error(1, "Tried to use 'sampler.sky' when it isn't valid\n");
        drawSurfs = (*(char **)&((r_global_permanent_t *)rgp)->world);
        *image = (*(void **)&((GfxWorld *)drawSurfs)->skyImage);
        *samplerState = ((GfxWorld *)drawSurfs)->skySamplerState;
        return;
    }

    case 17: {
        char *entry = (char *)backEnd->light[0].def;
        *image = (*(void **)&((GfxLightDef *)entry)->attenuation.image);
        *samplerState = ((GfxLightDef *)entry)->attenuation.samplerState;
        return;
    }

    case 18: {
        char *entry = (char *)backEnd->light[1].def;
        *image = (*(void **)&((GfxLightDef *)entry)->attenuation.image);
        *samplerState = ((GfxLightDef *)entry)->attenuation.samplerState;
        return;
    }

    case 19: {
        int sc_on = *(byte *)((char *)*(void **)imp_sc_enable + 8);
        if (sc_on) {
            char *entity = (char *)backEnd->currentEntity;
            if (*(int *)entity > 2 || (*(byte *)(entity + 5) & 1)) {
                *image = (*(void **)&((DxGlobals *)dx)->renderTargets[3].image);
                *samplerState = 0x32;
                return;
            }
        }
        *image = (*(void **)&((r_global_permanent_t *)rgp)->whiteImage);
        *samplerState = 0x32;
        return;
    }

    case 20: {
        drawSurfs = (*(char **)&((r_global_permanent_t *)rgp)->world);
        if (!drawSurfs || !(*(void **)&((GfxWorld *)drawSurfs)->outdoorImage))
            R_Error(1, "Tried to use 'sampler.outdoor' when it isn't valid\n");
        *image = *(void **)((char *)(*(char **)&((r_global_permanent_t *)rgp)->world) + 0x200);
        *samplerState = 0x32;
        return;
    }

    case 21:
        *image = NULL;
        *samplerState = 0x31;
        return;

    case 22:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->sunHalfAngleImage);
        *samplerState = 0x32;
        return;

    case 23:
        *image = (*(void **)&((r_global_permanent_t *)rgp)->waterColorImage);
        *samplerState = 0x32;
        return;
    }
}

#ifdef __EMSCRIPTEN__

static void RB_GetTextureFromCode(int codeTexture, void **image, byte *samplerState)
{
    RB_GetTextureFromCode_impl(codeTexture, image, samplerState);
}
#else

static void RB_GetTextureFromCode(int codeTexture, void **image, byte *samplerState)
{
    RB_GetTextureFromCode_impl(codeTexture, image, samplerState);
}

#endif

static void RB_SetEntityHwLightsDx7_impl(vec4_t *colorForDir, float sunVisibility)
{
    D3DLIGHT9 lights[8];
    const Material *material;
    int lightCount;
    void *device;
    void **vtable;
    int i;

    material = tess.material;
    lightCount = RB_DeriveEntityLights(colorForDir, sunVisibility, material, lights, 8);

    for (i = 0; i < lightCount; i++) {

        do {
            device = ((DxGlobals *)imp_dx)->device;
            vtable = *(void ***)device;
            ((HRESULT(D3DVTCC *)(void *, DWORD, BOOL))(vtable[0xD4 / 4]))(device, (DWORD)i, 1);
        } while (*(volatile int *)&alwaysfails);

        do {
            device = ((DxGlobals *)imp_dx)->device;
            vtable = *(void ***)device;
            ((HRESULT(D3DVTCC *)(void *, DWORD, const D3DLIGHT9 *))(vtable[0xCC / 4]))(device, (DWORD)i, &lights[i]);
        } while (*(volatile int *)&alwaysfails);
    }

    for (i = (lightCount > 0) ? lightCount : 0; (unsigned)i <= 7; i++) {
        do {
            device = ((DxGlobals *)imp_dx)->device;
            vtable = *(void ***)device;
            ((HRESULT(D3DVTCC *)(void *, DWORD, BOOL))(vtable[0xD4 / 4]))(device, (DWORD)i, 0);
        } while (*(volatile int *)&alwaysfails);
    }
}

#ifdef __EMSCRIPTEN__

static void RB_SetEntityHwLightsDx7(vec4_t *colorForDir, float sunVisibility)
{
    RB_SetEntityHwLightsDx7_impl(colorForDir, sunVisibility);
}
#else

static void RB_SetEntityHwLightsDx7(vec4_t *colorForDir, float sunVisibility)
{
    RB_SetEntityHwLightsDx7_impl(colorForDir, sunVisibility);
}

#endif

void RB_CreateDynamicBuffers(void)
{
    byte *t;
    void *first = ((void *(*)(int))ri.Hunk_AllocInternal)(0x200000);
    t = (byte *)imp_tess;
    ((materialCommands_t *)t)->indices = (r_index_t *)first;
    ((materialCommands_t *)t)->optimizedIndices = (r_index_t *)((void *(*)(int))ri.Hunk_AllocInternal)(0x200000);
}

static void RB_SetupLighting_impl(void)
{
    r_backEndGlobals_t *backEnd = (r_backEndGlobals_t *)imp_backEnd;
    char *lighting;
    char *entity;
    int rendererType;

    rendererType = r_rendererInUse->current.integer;

    if (rendererType == 2) {

        int techType = tess.techType;

        if ((unsigned)(techType - 15) <= 2)
            return;

        lighting = (char *)backEnd->currentEntityLighting;
        if (lighting) {
            if (*(int *)lighting != backEnd->viewCount) {
                RB_SetupEntityLighting(
                    backEnd->currentEntity,
                    (GfxEntityLighting *)lighting);
                lighting = (char *)backEnd->currentEntityLighting;
            }

            RB_SetEntityHwLightsDx7_impl(
                (vec4_t *)(lighting + 8),
                *(float *)(lighting + 4));
        } else {
            entity = (char *)backEnd->currentEntity;
            if (*(int *)entity == 2) {

                RB_SetEntityHwLightsDx7_impl(
                    *(vec4_t **)(entity + 8),
                    ((GfxEntity *)entity)->lighting.dx7.sunVisibility);
            }
        }
        return;
    }

    lighting = (char *)backEnd->currentEntityLighting;
    if (lighting) {
        if (*(int *)lighting != backEnd->viewCount) {
            RB_SetupEntityLighting(
                backEnd->currentEntity,
                (GfxEntityLighting *)lighting);
            lighting = (char *)backEnd->currentEntityLighting;
        }

        RB_SetCodeConstant(0x85,
                           backEnd->light[0].color[0],
                           backEnd->light[0].color[1],
                           backEnd->light[0].color[2],
                           *(vec_t *)(lighting + 4));

        Com_Memcpy(&backEnd->codeConsts[18][0], ((GfxEntityLighting *)lighting)->colorForDir, 96);
    } else {

        backEnd->codeConsts[5][0] = backEnd->light[0].color[0];
        backEnd->codeConsts[5][1] = backEnd->light[0].color[1];
        backEnd->codeConsts[5][2] = backEnd->light[0].color[2];
        backEnd->codeConsts[5][3] = backEnd->light[0].color[3];

        entity = (char *)backEnd->currentEntity;
        if (*(int *)entity == 2) {

            GfxWorld *smodel = rgp.world;

            backEnd->codeConsts[25][0] = smodel->smodelLightingLookupScale[0];
            backEnd->codeConsts[25][1] = smodel->smodelLightingLookupScale[1];
            backEnd->codeConsts[25][2] = smodel->smodelLightingLookupScale[2];
            backEnd->codeConsts[25][3] = 0.0f;

            backEnd->codeConsts[24][0] = (*(float *)&((GfxEntity *)entity)->lighting.dx7.colorForDir);
            backEnd->codeConsts[24][1] = ((GfxEntity *)entity)->lighting.dx7.sunVisibility;
            backEnd->codeConsts[24][2] = ((GfxEntity *)entity)->lighting.baseCoords[2];
            backEnd->codeConsts[24][3] = 0.0f;
        }
    }
}

#ifdef __EMSCRIPTEN__

static void RB_SetupLighting(void)
{
    RB_SetupLighting_impl();
}
#else

static BM_NOINLINE void RB_SetupLighting(void)
{
    RB_SetupLighting_impl();
}

#endif

static void RB_CopyVerticesWithColorConvert(const byte *src, byte *dst,
                                                             int vertCount, int stride,
                                                             int colorOfs, void *converter)
{
    typedef void (*ConvertFunc)(void *conv, int *outColor, const byte *srcColor);
    ConvertFunc convert = *(ConvertFunc *)*(void ***)converter;
    int i;

    for (i = 0; i < vertCount; i++) {
        int j;
        int convertedColor;

        for (j = 0; j < colorOfs; j += 4)
            *(int *)(dst + j) = *(const int *)(src + j);

        convert(converter, &convertedColor, src + colorOfs);
        *(int *)(dst + colorOfs) = convertedColor;

        for (j = colorOfs + 4; j < stride; j += 4)
            *(int *)(dst + j) = *(const int *)(src + j);

        src += stride;
        dst += stride;
    }
}

void RB_SetVertexData(unsigned int streamIndex, const void *data, int vertexCount, int stride)
{
    char *dx = (char *)dx_g;
    int totalSize = stride * vertexCount;
    int *lockSlot = (*(int **)&((DxGlobals *)dx)->dynamicVertexBuffer);
    IDirect3DVertexBuffer9 *dxVb;
    int writeOffset;

    if (!lockSlot)
        return;

    dxVb = *(IDirect3DVertexBuffer9 **)(lockSlot + 2);
    writeOffset = lockSlot[0];
    DWORD lockFlags;
    byte *bufferData;
    HRESULT hr;

    if (writeOffset == 0 || ((DxGlobals *)dx)->gpuSync == 0)
        lockFlags = 0x2000;
    else
        lockFlags = 0;

    hr = ((HRESULT(__attribute__((stdcall)) *)(IDirect3DVertexBuffer9 *, UINT, UINT, void **, DWORD))(*(void ***)(dxVb))[0x2c / 4])(dxVb, (UINT)writeOffset, (UINT)totalSize, (void **)&bufferData, lockFlags);

    if (hr < 0)
        R_FatalLockError(hr);

    {
        int numVerts = totalSize / stride;
        void *converter = CColorConverter_GetColorConverter(0);

        switch (stride) {
        case 0x14:
            Com_Memcpy(bufferData, data, totalSize);
            break;

        case 0x18:
            RB_CopyVerticesWithColorConvert((const byte *)data, bufferData,
                                            numVerts, stride, 0x0c, converter);
            break;

        case 0x20:
            RB_CopyVerticesWithColorConvert((const byte *)data, bufferData,
                                            numVerts, stride, 0x0c, converter);
            break;

        case 0x24:
            RB_CopyVerticesWithColorConvert((const byte *)data, bufferData,
                                            numVerts, stride, 0x18, converter);
            break;

        case 0x40:
            RB_CopyVerticesWithColorConvert((const byte *)data, bufferData,
                                            numVerts, stride, 0x1c, converter);
            break;

        case 0x44:
            RB_CopyVerticesWithColorConvert((const byte *)data, bufferData,
                                            numVerts, stride, 0x18, converter);
            break;

        default:
            Com_Memcpy(bufferData, data, totalSize);
            break;
        }
    }

    do {
        ((HRESULT(__attribute__((stdcall)) *)(IDirect3DVertexBuffer9 *))(*(void ***)(dxVb))[0x30 / 4])(dxVb);
    } while (*(int *)&alwaysfails);

    {
        int vertexOffset = lockSlot[0];

        if (dxVb != dxState.streams[streamIndex].vb) {
            RB_ChangeStreamSource(streamIndex, dxVb, vertexOffset, stride);
        } else if (dxState.streams[streamIndex].offset != vertexOffset ||
                   dxState.streams[streamIndex].stride != stride) {
            RB_ChangeStreamSource(streamIndex, dxVb, vertexOffset, stride);
        }
    }

    lockSlot[0] += totalSize;
}

static void RB_SetShaderAndDecl(const MaterialPassDx9 *pass, MaterialVertexDeclType vertDeclType, byte *dxState)
{
    DxState *state = (DxState *)dxState;
    IDirect3DVertexDeclaration9 *vertexDecl = NULL;
    IDirect3DVertexShader9 *vertexShader = NULL;
    IDirect3DPixelShader9 *pixelShader = NULL;
    void *device = dx.device;
    void **vtable = *(void ***)device;

    if (pass->vertexDecl)
        vertexDecl = (IDirect3DVertexDeclaration9 *)pass->vertexDecl->decl[vertDeclType];
    if (pass->vertexShader)
        vertexShader = pass->vertexShader->u.vs;
    if (pass->pixelShader)
        pixelShader = pass->pixelShader->u.ps;

    if (state->vertexDecl != vertexDecl) {
        do {
            ((HRESULT(D3DVTCC *)(void *, IDirect3DVertexDeclaration9 *))vtable[0x15c / 4])(device, vertexDecl);
        } while (*(int *)&alwaysfails);
        state->vertexDecl = vertexDecl;
        state->fvf = 0;
    }

    if (state->vertexShader != vertexShader) {
        do {
            ((HRESULT(D3DVTCC *)(void *, IDirect3DVertexShader9 *))vtable[0x170 / 4])(device, vertexShader);
        } while (*(int *)&alwaysfails);
        state->vertexShader = vertexShader;
    }

    if (state->pixelShader != pixelShader) {
        do {
            ((HRESULT(D3DVTCC *)(void *, IDirect3DPixelShader9 *))vtable[0x1ac / 4])(device, pixelShader);
        } while (*(int *)&alwaysfails);
        state->pixelShader = pixelShader;
    }
}

static inline char *RB_GetActiveMatrices(void)
{
    char *be = (char *)imp_backEnd;
    int idx = ((r_backEndGlobals_t *)be)->codeMatrixStackLevel;
    return (char *)&((r_backEndGlobals_t *)be)->codeMatrixStack[idx];
}

static void RB_ScaleWorldMatrix(const float *src, float *dst, float invScale)
{
    int i;
    for (i = 0; i < 3; i++) {
        dst[i * 4 + 0] = src[i * 4 + 0] * invScale;
        dst[i * 4 + 1] = src[i * 4 + 1] * invScale;
        dst[i * 4 + 2] = src[i * 4 + 2] * invScale;
        dst[i * 4 + 3] = src[i * 4 + 3];
    }
    dst[12] = src[12];
    dst[13] = src[13];
    dst[14] = src[14];
    dst[15] = src[15];
}

static const float *RB_GetCodeMatrix_impl(int source, int firstRow)
{
    char *am = RB_GetActiveMatrices();
    int sourceType = source & ~3;
    int matrixIndex = source & 3;
    char *codeMatrix;
    int transposeIndex;

    switch (sourceType) {
    case 0xBC:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, world));
        break;
    case 0xC0:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, view));
        break;
    case 0xC4:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, projection));
        break;

    case 0xC8:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, worldView));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            MatrixMultiply44((vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, world))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, view))), (vec4_t *)(codeMatrix));
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xCC:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, viewProjection));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            if (!*(byte *)((char *)am + offsetof(GfxCodeMatrices, worldView.valid[0]))) {

                MatrixMultiply44((vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, world))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, view))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, worldView))));
                *(byte *)((char *)am + offsetof(GfxCodeMatrices, worldView.valid[0])) = 1;
            }
            MatrixMultiply44((vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, worldView))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, projection))), (vec4_t *)(codeMatrix));
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xD0:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, worldViewProjection));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            if (!*(byte *)((char *)am + offsetof(GfxCodeMatrices, worldView.valid[0]))) {
                MatrixMultiply44((vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, world))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, view))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, worldView))));
                *(byte *)((char *)am + offsetof(GfxCodeMatrices, worldView.valid[0])) = 1;
            }
            MatrixMultiply44((vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, worldView))), (vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, projection))), (vec4_t *)(codeMatrix));
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xD4: {
        float OGLWorld[16], OGLView[16], OGLWorldView[16], OGLProjection[16];
        GfxViewport viewport;
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, OGLworldViewProjection));

        {
            const float *w = RB_GetCodeMatrix_impl(0xBC, 0);
            const float *v = RB_GetCodeMatrix_impl(0xC0, 0);
            const float *p = RB_GetCodeMatrix_impl(0xC4, 0);
            int i;
            for (i = 0; i < 16; i++)
                OGLWorld[i] = w[i];
            for (i = 0; i < 16; i++)
                OGLView[i] = v[i];
            for (i = 0; i < 16; i++)
                OGLProjection[i] = p[i];
        }

        OGLView[2] = -OGLView[2];
        OGLView[6] = -OGLView[6];
        OGLView[10] = -OGLView[10];
        OGLView[14] = -OGLView[14];
        MatrixMultiply44((vec4_t *)(OGLWorld), (vec4_t *)(OGLView), (vec4_t *)(OGLWorldView));
        RB_GetViewport(&viewport);
        MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(OGLProjection,
                                                          (float)viewport.width, (float)viewport.height);
        MatrixMultiply44((vec4_t *)(OGLWorldView), (vec4_t *)(OGLProjection), (vec4_t *)(codeMatrix));
        ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 1] = 0;
        ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 2] = 0;
        ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 3] = 0;
        break;
    }

    case 0xD8:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, normalizedWorld));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            const float *w = RB_GetCodeMatrix_impl(0xBC, 0);
            float invScale = 1.0f / *(float *)am;
            RB_ScaleWorldMatrix(w, (float *)codeMatrix, invScale);
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xDC:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, normalizedWorldView));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            const float *wv = RB_GetCodeMatrix_impl(0xC8, 0);
            float invScale = 1.0f / *(float *)am;
            RB_ScaleWorldMatrix(wv, (float *)codeMatrix, invScale);
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xE0:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, normalizedWorldView));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            const float *wvp = RB_GetCodeMatrix_impl(0xD0, 0);
            float invScale = 1.0f / *(float *)am;
            RB_ScaleWorldMatrix(wvp, (float *)codeMatrix, invScale);
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xE4:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, shadowLookupMatrix));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            char *be = (char *)imp_backEnd;
            MatrixMultiply44((vec4_t *)(((char *)am + offsetof(GfxCodeMatrices, world))), (vec4_t *)((char *)&((r_backEndGlobals_t *)be)->shadowLookupMatrix), (vec4_t *)(codeMatrix));
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xE8:
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, lightGridLookupMatrix));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            const float *vp = RB_GetCodeMatrix_impl(0xBF, 0);
            MatrixMultiply44((vec4_t *)(vp), (vec4_t *)(lightGridLookupMatrix), (vec4_t *)(codeMatrix));
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;

    case 0xEC: {
        float biasVec[4], biasWorld[4], biasResult[4];
        codeMatrix = ((char *)am + offsetof(GfxCodeMatrices, worldOutdoorLookup));
        if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0]) {
            const float *worldMat = RB_GetCodeMatrix_impl(0xBC, 0);
            const float *viewProj = RB_GetCodeMatrix_impl(0xC1, 0);
            char *rgp = (char *)imp_rgp;
            int awayBias = r_outdoorAwayBias->current.integer;
            float downBias = r_outdoorDownBias->current.value;

            biasVec[0] = 0.0f;
            biasVec[1] = 0.0f;
            biasVec[2] = -*(float *)&awayBias;
            biasVec[3] = 0.0f;

            MatrixTransformVector44((float *)(biasVec), (vec4_t *)(viewProj), (float *)(biasWorld));
            biasWorld[1] += downBias;

            MatrixTransformVector44((float *)(biasWorld),
                                    (vec4_t *)((char *)(*(void **)&((r_global_permanent_t *)rgp)->world) + 0x1c0), (float *)(biasResult));

            MatrixMultiply44((vec4_t *)(worldMat),
                             (vec4_t *)((char *)(*(void **)&((r_global_permanent_t *)rgp)->world) + 0x1c0), (vec4_t *)(codeMatrix));

            *(float *)((char *)am + offsetof(GfxCodeMatrices, worldOutdoorLookup.matrix[0].m[3][0])) += biasResult[0];
            *(float *)((char *)am + offsetof(GfxCodeMatrices, worldOutdoorLookup.matrix[0].m[3][1])) += biasResult[1];
            *(float *)((char *)am + offsetof(GfxCodeMatrices, worldOutdoorLookup.matrix[0].m[3][2])) += biasResult[2];
            *(float *)((char *)am + offsetof(GfxCodeMatrices, worldOutdoorLookup.matrix[0].m[3][3])) += biasResult[3];
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + 0] = 1;
        }
        break;
    }

    default:
        return NULL;
    }

    if (!((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + matrixIndex]) {
        transposeIndex = matrixIndex ^ 2;
        if (((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + transposeIndex]) {

            MatrixTranspose44( (float *)(codeMatrix + transposeIndex * 64), (float *)(codeMatrix + matrixIndex * 64));
            ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + matrixIndex] = 1;
        } else {
            int inverseIndex = matrixIndex ^ 1;
            if (((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + inverseIndex]) {

                MatrixInverse44( (float *)(codeMatrix + inverseIndex * 64), (float *)(codeMatrix + matrixIndex * 64));
                ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + matrixIndex] = 1;
            } else {

                MatrixTranspose44( (float *)(codeMatrix + (matrixIndex ^ 3) * 64), (float *)(codeMatrix + transposeIndex * 64));
                ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + transposeIndex] = 1;
                MatrixInverse44( (float *)(codeMatrix + transposeIndex * 64), (float *)(codeMatrix + matrixIndex * 64));
                ((char *)codeMatrix)[offsetof(GfxCodeMatrix, valid) + matrixIndex] = 1;
            }
        }
    }

    return (float *)(codeMatrix + (firstRow + matrixIndex * 4) * 16);
}

#ifdef __EMSCRIPTEN__

static const float *RB_GetCodeMatrix(int source, int firstRow)
{
    return RB_GetCodeMatrix_impl(source, firstRow);
}
#else

static const float *RB_GetCodeMatrix(int source, int firstRow)
{
    return RB_GetCodeMatrix_impl(source, firstRow);
}

#endif

static BM_NOINLINE void __attribute_regparm__(3) RB_DrawSingleTechnique(MaterialTechniqueType techType, MaterialVertexDeclType vertDeclType, const GfxDrawPrimArgs *args, const GfxStateOverride *stateOverride)
{
    char *tess = RB_TessBase();
    const Material *material = ((materialCommands_t *)tess)->material;
    byte *technique;
    int passCount;
    int passIndex;
    int isDx7;
    r_backEndGlobals_t *backEnd;

    {
        /* was techSet + 4 + techType*4 (x86 ptr stride) */
        technique = (byte *)((MaterialTechniqueSet *)material->techniqueSet)->techniques[techType];
    }

    if (vertDeclType == 0x21) {
        byte *g_special = (byte *)imp_g_special;
        int debugLevel = r_debugShader->current.integer;
        byte *debugConst = (byte *)debugShaderConsts + debugLevel * 16;
        byte *backEndConst = (byte *)&((r_backEndGlobals_t *)imp_backEnd)->codeConsts[26][0];
        memcpy(backEndConst, debugConst, 16);
    }

    {
        byte *g_special = (byte *)imp_g_special;
        *g_special = (args == (const GfxDrawPrimArgs *)3) ? 1 : 0;
    }

    passCount = ((MaterialTechnique *)technique)->passCount;   /* was technique + 6 (x86) */
    if (passCount == 0)
        goto done;

    isDx7 = (r_rendererInUse->current.integer == 2);
    backEnd = (r_backEndGlobals_t *)imp_backEnd;

    for (passIndex = 0; passIndex < passCount; passIndex++) {
        byte *pass;
        int stateBits[2];

        if (isDx7) {
            /* was technique + 8 + passIndex*92 (x86) */
            pass = (byte *)&((MaterialTechnique *)technique)->passArray.dx7[passIndex];
        } else {
            /* was technique + 8 + passIndex*28 (x86); MaterialPassDx9 is 56B on x64 */
            pass = (byte *)&((MaterialTechnique *)technique)->passArray.dx9[passIndex];
        }

        if (isDx7) {

            byte *refStateBits;
            byte *stateMap;

            tess = RB_TessBase();
            {
                const Material *tessMat = ((materialCommands_t *)tess)->material;
                refStateBits = (byte *)tessMat->stateBits;
            }
            stateMap = *(byte **)(pass + 8);

            stateBits[0] = *(int *)(refStateBits + 0);
            stateBits[1] = *(int *)(refStateBits + 4);

            {
                int rsi;
                for (rsi = 0; rsi < 11; rsi++) {
                    /* was rsp+4 start + rsp+=4 stride (x86 4-byte ptrs) */
                    const byte *ruleSet = (const byte *)((MaterialStateMap *)stateMap)->ruleSet[rsi];
                    int rc = *(int *)ruleSet;
                    int ri2;
                    int matched = 0;

                    for (ri2 = 0; ri2 < rc; ri2++) {
                        const MaterialStateMapRule *rule = &((const MaterialStateMapRuleSet *)ruleSet)->rules[ri2];
                        if ((*(int *)(refStateBits + 0) & rule->stateBitsMask[0]) != rule->stateBitsValue[0])
                            continue;
                        if ((*(int *)(refStateBits + 4) & rule->stateBitsMask[1]) != rule->stateBitsValue[1])
                            continue;

                        {
                            int k;
                            for (k = 0; k < 2; k++) {
                                stateBits[k] &= rule->stateBitsClear[k];
                                stateBits[k] |= rule->stateBitsSet[k];
                            }
                        }
                        matched = 1;
                        break;
                    }

                    if (!matched) {
                        const Material *mat2 = ((materialCommands_t *)RB_TessBase())->material;
                        R_Error(0, "No rule in stateMap '%s' rule set %i matched the current mat",
                                *(char **)stateMap, rsi, mat2->info.name);
                    }
                }
            }

            if (stateOverride) {
                byte *ovr = (byte *)stateOverride;
                int k;
                for (k = 0; k < 2; k++) {
                    stateBits[k] &= *(int *)(ovr + k * 4);
                    stateBits[k] |= *(int *)(ovr + 8 + k * 4);
                }
            }

            if (((r_backEndGlobals_t *)backEnd)->projection2D)
                stateBits[1] &= 0xffffffcf;

            if (stateBits[0] != dxState.refStateBits[0]) {
                RB_ChangeState_0(stateBits[0]);
                dxState.refStateBits[0] = stateBits[0];
            }
            if (stateBits[1] != dxState.refStateBits[1]) {
                RB_ChangeState_1(stateBits[1]);
                dxState.refStateBits[1] = stateBits[1];
            }

            {
                int fogSrc = (*(byte *)(pass + 8) == 1) ? 0 : 2;
                RB_UpdateFogColor( (FogColorSrcEnum)(fogSrc));
            }

            {
                byte passNormalize = *(byte *)(pass + 4);
                if (passNormalize != dxState.gridLighting) {
                    byte *dx = (byte *)dx_g;
                    volatile int *af = (volatile int *)&alwaysfails;
                    do {
                        byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                        void **vt = *(void ***)dev;
                        ((void(D3DVTCC *)(void *, int, int))vt[0xe4 / 4])(dev, 0x89,
                                                                          passNormalize ? 1 : 0);
                    } while (*af);
                    dxState.gridLighting = passNormalize;
                }
            }

            {
                DWORD fvf = s_fvfForVertDeclType[vertDeclType];
                if (dxState.vertexDecl != NULL) {
                    byte *dx = (byte *)dx_g;
                    volatile int *af = (volatile int *)&alwaysfails;
                    do {
                        byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                        void **vt = *(void ***)dev;
                        ((void(D3DVTCC *)(void *, DWORD))vt[0x164 / 4])(dev, fvf);
                    } while (*af);
                    dxState.fvf = fvf;
                    dxState.vertexDecl = NULL;
                } else if (fvf != dxState.fvf) {
                    byte *dx = (byte *)dx_g;
                    volatile int *af = (volatile int *)&alwaysfails;
                    do {
                        byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                        void **vt = *(void ***)dev;
                        ((void(D3DVTCC *)(void *, DWORD))vt[0x164 / 4])(dev, fvf);
                    } while (*af);
                    dxState.fvf = fvf;
                    dxState.vertexDecl = NULL;
                }
            }

            if (*(byte *)(pass + 7)) {

                r_backEndGlobals_t *backEnd2 = backEnd;
                float phase = backEnd2->sceneDef.floatTime;
                const dvar_t *minDvar = r_objectiveColorDx7Min;
                const dvar_t *maxDvar = r_objectiveColorDx7Max;

                float oneOver255 = 0.003921568859368563f;
                float aMin = (float)minDvar->current.color[3] * oneOver255;
                float rMin = (float)minDvar->current.color[0] * oneOver255;
                float gMin = (float)minDvar->current.color[1] * oneOver255;
                float bMin = (float)minDvar->current.color[2] * oneOver255;

                float t;
                {
                    float frac = phase - floorf(phase);
                    float angle = frac * 6.283185307179586f;
                    float s = sinf(angle);
                    t = s * -0.5f + 0.5f;
                }

                unsigned int color = 0;
                {
                    float aMax = (float)maxDvar->current.color[3] * oneOver255;
                    float rMax = (float)maxDvar->current.color[0] * oneOver255;
                    float gMax = (float)maxDvar->current.color[1] * oneOver255;
                    float bMax = (float)maxDvar->current.color[2] * oneOver255;

                    int a = (int)(((aMax - aMin) * t + aMin) * 255.0f);
                    int r = (int)(((rMax - rMin) * t + rMin) * 255.0f);
                    int g = (int)(((gMax - gMin) * t + gMin) * 255.0f);
                    int b = (int)(((bMax - bMin) * t + bMin) * 255.0f);

                    color = (a << 24) | (r << 16) | (g << 8) | b;
                }
                RB_SetSamplerConstantDx7(color);
            } else {
                RB_SetSamplerConstantDx7(0xFFFFFFFF);
            }

            {
                int samplerIndex;
                for (samplerIndex = 0; samplerIndex < 2; samplerIndex++) {
                    byte *samplerDef = (byte *)(((char *)pass + offsetof(MaterialPassDx9, pixelShader)) + samplerIndex * 8);

                }
            }

            {
                int stageIdx;
                int *stagePtr = ((MaterialPassDx7 *)pass)->colorStageBits;
                int *dxStagePtr = dxState.refColorStageBits;

                for (stageIdx = 0; stageIdx < 8; stageIdx++) {
                    int colorBits = stagePtr[stageIdx];
                    if (colorBits != dxStagePtr[stageIdx]) {
                        dxStagePtr[stageIdx] = colorBits;
                        RB_ChangeColorStageState(stageIdx, colorBits);
                    }

                    int alphaBits = ((MaterialPassDx7 *)pass)->alphaStageBits[stageIdx];
                    if (alphaBits != dxState.refAlphaStageBits[stageIdx]) {
                        dxState.refAlphaStageBits[stageIdx] = alphaBits;
                        RB_ChangeAlphaStageState(stageIdx, alphaBits);
                    }
                }
            }
        } else {

            byte *refStateBits;
            byte *stateMap;
            int textureRoutingCount;
            int constantRoutingCount;
            byte *textureRouting;
            byte *constantRouting;

            /* was technique + 8 + passIndex*28 (x86) */
            pass = (byte *)&((MaterialTechnique *)technique)->passArray.dx9[passIndex];

            {
                MaterialVertexDeclaration *declArray = ((MaterialPassDx9 *)pass)->vertexDecl;
                void *decl = (void *)declArray->decl[vertDeclType];
                if (!decl) {
                    tess = RB_TessBase();
                    const Material *mat3 = ((materialCommands_t *)tess)->material;
                    MaterialShader *pgm = ((MaterialPassDx9 *)pass)->vertexShader;

                    {

                        static const char *warned[64];
                        static int nWarned;
                        int wi, seen = 0;
                        for (wi = 0; wi < nWarned; wi++)
                            if (warned[wi] == mat3->info.name) {
                                seen = 1;
                                break;
                            }
                        if (!seen && nWarned < (int)(sizeof(warned) / sizeof(warned[0]))) {
                            warned[nWarned++] = mat3->info.name;
                            ((void (*)(int, const char *, ...)) * (void **)&ri)(0,
                                                                                   "^3WARN: vertex type %i has no decl for shader %s in material %s -- skipping (static-model cache not populated)\n",
                                                                                   vertDeclType, pgm->name, mat3->info.name);
                        }
                    }
                    tess = RB_TessBase();
                    continue;
                }
            }

            tess = RB_TessBase();
            {
                const Material *tessMat = ((materialCommands_t *)tess)->material;
                refStateBits = (byte *)tessMat->stateBits;
            }
            stateMap = (byte *)((MaterialPassDx9 *)pass)->stateMap;
            stateBits[0] = *(int *)(refStateBits + 0);
            stateBits[1] = *(int *)(refStateBits + 4);

            {
                int rsi;
                for (rsi = 0; rsi < 11; rsi++) {
                    /* was rsp+4 start + rsp+=4 stride (x86 4-byte ptrs) */
                    const byte *ruleSet = (const byte *)((MaterialStateMap *)stateMap)->ruleSet[rsi];
                    int rc = *(int *)ruleSet;
                    int ri2;
                    int matched = 0;

                    for (ri2 = 0; ri2 < rc; ri2++) {
                        const MaterialStateMapRule *rule = &((const MaterialStateMapRuleSet *)ruleSet)->rules[ri2];
                        if ((*(int *)(refStateBits + 0) & rule->stateBitsMask[0]) != rule->stateBitsValue[0])
                            continue;
                        if ((*(int *)(refStateBits + 4) & rule->stateBitsMask[1]) != rule->stateBitsValue[1])
                            continue;

                        {
                            int k;
                            for (k = 0; k < 2; k++) {
                                stateBits[k] &= rule->stateBitsClear[k];
                                stateBits[k] |= rule->stateBitsSet[k];
                            }
                        }
                        matched = 1;
                        break;
                    }

                    if (!matched) {
                        const Material *mat4 = ((materialCommands_t *)RB_TessBase())->material;
                        R_Error(0, "No rule in stateMap '%s' rule set %i matched the current mat",
                                *(char **)stateMap, rsi, mat4->info.name);
                    }
                }
            }

            if (stateOverride) {
                byte *ovr = (byte *)stateOverride;
                int k;
                for (k = 0; k < 2; k++) {
                    stateBits[k] &= *(int *)(ovr + k * 4);
                    stateBits[k] |= *(int *)(ovr + 8 + k * 4);
                }
            }

            if (((r_backEndGlobals_t *)backEnd)->projection2D)
                stateBits[1] &= 0xffffffcf;

            if (stateBits[0] != dxState.refStateBits[0]) {
                RB_ChangeState_0(stateBits[0]);
                dxState.refStateBits[0] = stateBits[0];
            }
            if (stateBits[1] != dxState.refStateBits[1]) {
                RB_ChangeState_1(stateBits[1]);
                dxState.refStateBits[1] = stateBits[1];
            }

            RB_UpdateFogColor( (FogColorSrcEnum)(0));

            { extern volatile int g_rigid_step; g_rigid_step = 11; }
            RB_SetShaderAndDecl((const MaterialPassDx9 *)pass, vertDeclType, (byte *)&dxState);
            { extern volatile int g_rigid_step; g_rigid_step = 12; }

            textureRoutingCount = ((MaterialPassDx9 *)pass)->vertexArgCount;
#ifdef GFX_REAL_D3D9
            if (getenv("REALD3D9_ARGLOG")) {
                static int al = 0;
                if (al < 6) {
                    al++;
                    fprintf(stderr, "[ARGLOG] vertexArgCount=%d pixelArgCount=%d (pass=%p)\n",
                            ((MaterialPassDx9 *)pass)->vertexArgCount,
                            ((MaterialPassDx9 *)pass)->pixelArgCount, (void *)pass);
                }
            }
#endif

            {
                int routingIndex;
                for (routingIndex = 0; routingIndex < textureRoutingCount; routingIndex++) {
                    MaterialShaderArgument *entry = &((MaterialPassDx9 *)pass)->vertexArgs[routingIndex];
                    int type = entry->type;
                    int destIndex = entry->dest;
                    const void *data = entry->u.literalConst;

                    switch (type) {
                    case 0:
                    {

                        byte *cached = (byte *)&dxState + destIndex * 16;
                        if (memcmp(cached, data, 16) != 0) {
                            memcpy(cached, data, 16);

                            {
                                byte *dx = (byte *)dx_g;
                                volatile int *af = (volatile int *)&alwaysfails;
                                do {
                                    byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                                    void **vt = *(void ***)dev;
                                    ((void(D3DVTCC *)(void *, int, void *, int))vt[0x178 / 4])(
                                        dev, (int)destIndex, (void *)data, 1);
                                } while (*af);
                            }
                        }
                        break;
                    }
                    case 1:
                    {
                        int codeIndex = entry->u.codeConst.index;
                        int rowCount = entry->u.codeConst.rowCount;
                        int firstRow = entry->u.codeConst.firstRow;
                        const float *matrixData;

                        if (codeIndex <= 0xba) {
                            matrixData = (float *)((byte *)imp_backEnd - 0x800 + codeIndex * 16);
                        } else {
                            matrixData = RB_GetCodeMatrix(codeIndex, firstRow);
                        }

                        {
                            int dataSize = rowCount * 16;
                            byte *cached = (byte *)&dxState + destIndex * 16;
#ifdef GFX_REAL_D3D9
                            if (getenv("REALD3D9_ARGLOG")) {
                                static int cl = 0;
                                if (cl < 8) {
                                    cl++;
                                    fprintf(stderr, "[ARGLOG] CODECONST dest=c%d codeIdx=%d rows=%d m0=%.3f changed=%d\n",
                                            destIndex, codeIndex, rowCount, matrixData[0],
                                            memcmp(cached, matrixData, dataSize) != 0);
                                }
                            }
#endif
                            if (memcmp(cached, matrixData, dataSize) != 0) {
                                memcpy(cached, matrixData, dataSize);
                                byte *dx = (byte *)dx_g;
                                volatile int *af = (volatile int *)&alwaysfails;
                                do {
                                    byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                                    void **vt = *(void ***)dev;
                                    ((void(D3DVTCC *)(void *, int, void *, int))vt[0x178 / 4])(
                                        dev, destIndex, (void *)matrixData, rowCount);
                                } while (*af);
                            }
                        }
                        break;
                    }
                    case 2:
                    {
                        const char *literalName = entry->u.name;
                        void *constData = NULL;
                        int rowCount2 = 1;

                        {
                            tess = RB_TessBase();
                            const Material *mat5 = ((materialCommands_t *)tess)->material;
                            int constCount = mat5->constantCount;
                            MaterialConstantDef *consts = mat5->constants;
                            int ci;
                            for (ci = 0; ci < constCount; ci++) {
                                if (consts[ci].name == literalName || strcmp(consts[ci].name, literalName) == 0) {
                                    constData = consts[ci].literal;
                                    break;
                                }
                            }
                        }

                        if (constData) {
                            int dataSize = rowCount2 * 16;
                            byte *cached = (byte *)&dxState + destIndex * 16;
                            if (memcmp(cached, constData, dataSize) != 0) {
                                memcpy(cached, constData, dataSize);
                                byte *dx = (byte *)dx_g;
                                volatile int *af = (volatile int *)&alwaysfails;
                                do {
                                    byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                                    void **vt = *(void ***)dev;
                                    ((void(D3DVTCC *)(void *, int, void *, int))vt[0x178 / 4])(
                                        dev, destIndex, constData, rowCount2);
                                } while (*af);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            }

            constantRoutingCount = ((MaterialPassDx9 *)pass)->pixelArgCount;

            {
                int routingIndex;
                for (routingIndex = 0; routingIndex < constantRoutingCount; routingIndex++) {
                    MaterialShaderArgument *entry = &((MaterialPassDx9 *)pass)->pixelArgs[routingIndex];
                    int type = entry->type;

                    switch (type) {
                    case 0:
                    {
                        const void *data = entry->u.literalConst;
                        int destIdx = entry->dest;
                        byte *cached = (byte *)&dxState.pixelShaderConsts[destIdx][0];
                        if (memcmp(cached, data, 16) != 0) {
                            memcpy(cached, data, 16);
                            byte *dx = (byte *)dx_g;
                            volatile int *af = (volatile int *)&alwaysfails;
                            do {
                                byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                                void **vt = *(void ***)dev;
                                ((void(D3DVTCC *)(void *, int, void *, int))vt[0x1b4 / 4])(
                                    dev, (int)destIdx, (void *)data, 1);
                            } while (*af);
                        }
                        break;
                    }
                    case 1:
                    {
                        int codeIndex = entry->u.codeConst.index;
                        int rowCount = entry->u.codeConst.rowCount;
                        int firstRow = entry->u.codeConst.firstRow;
                        const float *matrixData;

                        if (codeIndex <= 0xba) {
                            matrixData = (float *)((byte *)imp_backEnd - 0x800 + codeIndex * 16);
                        } else {
                            matrixData = RB_GetCodeMatrix(codeIndex, firstRow);
                        }

                        {
                            int dataSize = rowCount * 16;
                            int destIdx = entry->dest;
                            byte *cached = (byte *)&dxState.pixelShaderConsts[destIdx][0];
                            if (memcmp(cached, matrixData, dataSize) != 0) {
                                memcpy(cached, matrixData, dataSize);
                                byte *dx = (byte *)dx_g;
                                volatile int *af = (volatile int *)&alwaysfails;
                                do {
                                    byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                                    void **vt = *(void ***)dev;
                                    ((void(D3DVTCC *)(void *, int, void *, int))vt[0x1b4 / 4])(
                                        dev, destIdx, (void *)matrixData, rowCount);
                                } while (*af);
                            }
                        }
                        break;
                    }
                    case 2:
                    {
                        const char *literalName = entry->u.name;
                        void *constData = NULL;
                        int rowCount2 = 1;

                        {
                            tess = RB_TessBase();
                            const Material *mat5 = ((materialCommands_t *)tess)->material;
                            int constCount = mat5->constantCount;
                            MaterialConstantDef *consts = mat5->constants;
                            int ci;
                            for (ci = 0; ci < constCount; ci++) {
                                if (consts[ci].name == literalName || strcmp(consts[ci].name, literalName) == 0) {
                                    constData = consts[ci].literal;
                                    break;
                                }
                            }
                        }

                        if (constData) {
                            int destIdx = entry->dest;
                            int dataSize = rowCount2 * 16;
                            byte *cached = (byte *)&dxState.pixelShaderConsts[destIdx][0];
                            if (memcmp(cached, constData, dataSize) != 0) {
                                memcpy(cached, constData, dataSize);
                                byte *dx = (byte *)dx_g;
                                volatile int *af = (volatile int *)&alwaysfails;
                                do {
                                    byte *dev = (byte *)(((DxGlobals *)dx)->device /*was dx+8 (x86 device offset, x64 @16)*/);
                                    void **vt = *(void ***)dev;
                                    ((void(D3DVTCC *)(void *, int, void *, int))vt[0x1b4 / 4])(
                                        dev, destIdx, constData, rowCount2);
                                } while (*af);
                            }
                        }
                        break;
                    }
                    case 3:
                    {
                        int codeTexture = entry->u.codeSampler;
                        void *image = NULL;
                        byte samplerState = 0;

                        RB_GetTextureFromCode_impl(codeTexture, &image, &samplerState);
                        RB_X64TraceSampler("code", ((materialCommands_t *)RB_TessBase())->material,
                                           type, entry->dest, NULL, codeTexture, (GfxImage *)image, samplerState);
#ifdef GFX_REAL_D3D9
                        if (getenv("REALD3D9_SAMPLOG")) {
                            static int spl = 0;
                            if (spl < 40) {
                                spl++;
                                void *tex = image ? (void *)((GfxImage *)image)->texture.map : NULL;
                                fprintf(stderr, "[SAMPLOG] codeTex=%d -> dstSampler=s%d img=%p d3dtex=%p state=0x%x\n",
                                        codeTexture, entry->dest, image, tex, samplerState);
                            }
                        }
#endif
                        RB_SetSampler(entry->dest, samplerState, (GfxImage *)(image));
                        break;
                    }
                    case 4:
                    {
                        const char *textureName = entry->u.name;
                        void *image = NULL;
                        byte samplerState = 0;
                        int destSampler = entry->dest;

                        {
                            tess = RB_TessBase();
                            const Material *mat6 = ((materialCommands_t *)tess)->material;
                            int texCount = mat6->textureCount;
                            MaterialTextureDef *textures = mat6->textures;
                            int ti;
                            for (ti = 0; ti < texCount; ti++) {
                                MaterialTextureDef *texDef = &textures[ti];
                                if (texDef->name == textureName || strcmp(texDef->name, textureName) == 0) {

                                    byte semantic = texDef->semantic;
                                    if (semantic == 5) {

                                        MaterialWaterDef *water = texDef->u.water;
                                        image = water->map->image;
                                    } else {
                                        image = texDef->u.image;
                                    }
                                    samplerState = texDef->samplerState;

                                    if (image && *(int *)image == 3) {

                                        byte imgSemantic = ((GfxImage *)image)->semantic;

                                    }
                                    break;
                                }
                            }
                        }

                        if (image) {
                            RB_X64TraceSampler("material", ((materialCommands_t *)RB_TessBase())->material,
                                               type, destSampler, textureName, -1, (GfxImage *)image, samplerState);
                            RB_SetSampler(destSampler, samplerState, (GfxImage *)(image));
                        } else {
                            RB_X64TraceSampler("material-null", ((materialCommands_t *)RB_TessBase())->material,
                                               type, destSampler, textureName, -1, NULL, samplerState);
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }

        { extern volatile int g_rigid_step; g_rigid_step = 13; }
        if (!((r_backEndGlobals_t *)backEnd)->projection2D) {
            int primCount = args->primCount;
            int drawPrimFloor = r_drawPrimFloor->current.integer;
            int drawPrimCap = r_drawPrimCap->current.integer;

            if (primCount >= drawPrimFloor && (drawPrimCap == 0 || primCount <= drawPrimCap)) {
                { extern volatile int g_rigid_step; g_rigid_step = 14; }
                RB_DrawIndexedPrim(args, primCount);
            }
        } else {

            { extern volatile int g_rigid_step; g_rigid_step = 14; }
            RB_DrawIndexedPrim(args, args->primCount);
        }
        { extern volatile int g_rigid_step; g_rigid_step = 15; }

        if (isDx7) {
            if (((MaterialTechnique *)technique)->passArray.dx7[0].projectToInfinity) {
                RB_SetViewMatrixForWDx7(0.0f);

                RB_SetViewMatrixForWDx7(1.0f);
            }
        }
    }

done:

    {
        byte *g_special = (byte *)imp_g_special;
        *g_special = 0;
    }
}

void RB_DrawTechnique(MaterialVertexDeclType vertDeclType, const GfxDrawPrimArgs *args)
{
    r_backEndGlobals_t *backEnd = (r_backEndGlobals_t *)imp_backEnd;
    char *tess;
    MaterialTechniqueType techType;
    const GfxStateOverride *stateOverride;

    if (backEnd->viewportIsDirty != 0)
        RB_UpdateViewport();

    tess = RB_TessBase();
    techType = (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType);

    if ((unsigned)(techType - 6) <= 11) {
        RB_SetupLighting();
        techType = (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType);
    }

    if ((unsigned)(techType - 3) <= 0x17)
        RB_SetIteratorFog();

    stateOverride = NULL;
    if (r_rendererInUse->current.integer == 2 &&
        (unsigned)((*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType) - 9) <= 5)
        stateOverride = &overrideEnableRenormalize;

    RB_DrawSingleTechnique(
        (*(MaterialTechniqueType *)&((materialCommands_t *)RB_TessBase())->techType),   /* was tess + 0x5a7c0 (x86) */
        vertDeclType, args, stateOverride);
}

extern int g_rb_endsurface_count;
extern int g_rb_endsurface_nomaterial;
extern int g_rb_endsurface_notechnique;
extern int g_rb_endsurface_dxstate;
extern int g_rb_endsurface_draw;
extern int g_rb_endsurface_flag1skip;
extern int g_rb_endsurface_flag2skip;
extern int g_rb_endsurface_idxzero;
extern int g_rb_tess_type_counts[8];
extern int g_rb_tess_type_idxzero[8];
extern int g_rb_last_tess_type;
extern int g_tess_since_begin;
extern int g_begin_surface_calls;
extern int g_tt_last_cached;
extern void *g_tt_last_tess;
extern int g_tt_seq, g_tt_last_seq;
int g_es_seq = 0;
void diag_idxzero(void *tess_base)
{
    (void)tess_base;
}
void diag_endsurface_entry(void *tess_base)
{
    (void)tess_base;
}

void RB_EndSurface(void)
{
    char *tess = RB_TessBase();
    const Material *material;
    MaterialTechnique *technique;
    unsigned short techFlags;
    r_backEndGlobals_t *backEnd;
    int isDx7;
    int indexCount;
    int cachedIndexCount;
    GfxDrawPrimArgs args;
    int vertexStride;

    diag_endsurface_entry(tess);
    tess = RB_TessBase();

    material = ((materialCommands_t *)tess)->material;
    if (!material) {
        g_rb_endsurface_nomaterial++;
        goto cleanup;
    }

#if defined(COD2_X64)
    /* x64 TODO: some fonts present a truncated (32-bit) Material* via font->material
       (see the X64_FONT_WHITE workaround in RB_DrawTextWithCursor_impl). A truncated
       ptr is either zero-extended (high32==0) or sign-extended (high32==0xffffffff);
       `p + 0x1_0000_0000 < 0x2_0000_0000` catches both. Guard the deref so an editable
       text field / chat draw doesn't crash on a bad material or techniqueSet. */
    if ((uintptr_t)material + 0x100000000ULL < 0x200000000ULL) {
        g_rb_endsurface_nomaterial++;
        goto cleanup;
    }
#endif

    {
        uintptr_t ts = (uintptr_t)material->techniqueSet;   /* was unsigned int -> truncated ptr on x64 */
        if (!ts || ts < 0x08000000u) {
            g_rb_endsurface_notechnique++;
            goto cleanup;
        }
#if defined(COD2_X64)
        if (ts + 0x100000000ULL < 0x200000000ULL) {
            g_rb_endsurface_notechnique++;
            goto cleanup;
        }
#endif
    }
    technique = material->techniqueSet->techniques[(*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType)];

    g_rb_endsurface_count++;
    RB_X64TraceEndSurface("loaded", material, technique, (const materialCommands_t *)tess);

    if (!technique) {
        RB_X64TraceEndSurface("skip-notech", material, technique, (const materialCommands_t *)tess);
        g_rb_endsurface_notechnique++;
        goto cleanup;
    }

    backEnd = (r_backEndGlobals_t *)imp_backEnd;
    if (backEnd->viewportIsDirty)
        RB_UpdateViewport();

    if (dxState.viewportIsNull) {
        RB_X64TraceEndSurface("skip-viewport", material, technique, (const materialCommands_t *)tess);
        g_rb_endsurface_dxstate++;
        goto cleanup;
    }

    techFlags = technique->flags;
    if (techFlags & 1) {
        if (backEnd->resolvedPostSunTarget == 0xe) {
            g_rb_endsurface_flag1skip++;
            RB_X64TraceEndSurface("skip-flag1", material, technique, (const materialCommands_t *)tess);
            goto cleanup;
        }
    }
    if (techFlags & 2) {
        if (backEnd->resolvedSceneTarget == 0xe) {
            g_rb_endsurface_flag2skip++;
            RB_X64TraceEndSurface("skip-flag2", material, technique, (const materialCommands_t *)tess);
            goto cleanup;
        }
    }

    cachedIndexCount = ((materialCommands_t *)tess)->optimizedIndexCount;
    if (cachedIndexCount != 0) {
        int cachedVertDeclType;
        IDirect3DVertexBuffer9 *vb;

        args.firstVertexFromBase = 0;
        args.vertexCount = ((materialCommands_t *)tess)->optimizedVertexCount;
        args.primCount = cachedIndexCount / 3;
        args.u.buf.baseIndex = RB_SetIndexData(
            ((materialCommands_t *)tess)->optimizedIndices, cachedIndexCount);

        isDx7 = (r_rendererInUse->current.integer == 2);

        if ((*(int *)&((materialCommands_t *)tess)->optimizedVertexSource) == 1) {

            vertexStride = isDx7 ? 0x20 : 0x44;
            vb = rgp.world->vd.worldVb;
            cachedVertDeclType = 1;
        } else {

            vertexStride = isDx7 ? 0x18 : 0x40;
            vb = dx.smodelCacheVb;
            cachedVertDeclType = 3;
        }

        if (vb != dxState.streams[0].vb ||
            dxState.streams[0].offset != 0 ||
            dxState.streams[0].stride != vertexStride) {
            RB_ChangeStreamSource(0, vb, 0, vertexStride);
        }

        args.u.buf.baseVertex = ((materialCommands_t *)tess)->firstOptimizedVertex;

        RB_DrawTechnique( (MaterialVertexDeclType)(cachedVertDeclType), &args);

        tess = RB_TessBase();
        ((materialCommands_t *)tess)->optimizedIndexCount = 0;
        (*(int *)&((materialCommands_t *)tess)->optimizedVertexSource) = 0;
    }

    indexCount = ((materialCommands_t *)tess)->indexCount;
    if (indexCount == 0) {
        RB_X64TraceEndSurface("skip-index0", material, technique, (const materialCommands_t *)tess);
        g_rb_endsurface_idxzero++;
        g_rb_tess_type_idxzero[g_rb_last_tess_type]++;
        diag_idxzero(tess);
        return;
    }

    args.firstVertexFromBase = 0;
    args.vertexCount = ((materialCommands_t *)tess)->vertexCount;
    args.primCount = indexCount / 3;

    isDx7 = (r_rendererInUse->current.integer == 2);
    if ((*(int *)&((materialCommands_t *)tess)->declType) == 1)
        vertexStride = isDx7 ? 0x20 : 0x44;
    else
        vertexStride = isDx7 ? 0x24 : 0x40;

    args.u.buf.baseIndex = RB_SetIndexData(
        ((materialCommands_t *)tess)->indices, indexCount);

    {
        char *dx = (char *)dx_g;
        int *lockSlot = (*(int **)&((DxGlobals *)dx)->dynamicVertexBuffer);
        if (!lockSlot) {
            static int lockslot_warn = 0;
            if (lockslot_warn++ < 5)
                fprintf(stderr, "[EndSurf] lockSlot NULL at dx->dynamicVertexBuffer, skipping draw\n");
            goto cleanup;
        }
        int needed = ((materialCommands_t *)tess)->vertexCount * vertexStride + lockSlot[0];
        if (needed > lockSlot[1])
            lockSlot[0] = 0;
    }

    RB_SetVertexData(0, tess, ((materialCommands_t *)tess)->vertexCount, vertexStride);
    args.u.buf.baseVertex = 0;

    {
#define RB_GL_TEXTURE_2D 0x0DE1
#define RB_GL_TEXTURE_CUBE_MAP 0x8513
        const Material *mat = ((materialCommands_t *)tess)->material;

        if (mat) {
            int texCount = (*(unsigned short *)&((Material *)mat)->textureCount);
            byte *textures = (*(byte **)&((Material *)mat)->textures);
            if (texCount > 0 && textures) {

                void *image = NULL;
                int textureIndex;

                for (textureIndex = 0; textureIndex < texCount; ++textureIndex) {
                    /* was x86 stride 0xc + offsets 5/8; MaterialTextureDef is bigger on x64 */
                    MaterialTextureDef *texEntry = &((MaterialTextureDef *)textures)[textureIndex];
                    byte semantic = (byte)texEntry->semantic;
                    void *candidate;

                    if (semantic == 5)
                        continue;

                    candidate = (void *)texEntry->u.image;
                    if (!candidate)
                        continue;

                    if (semantic == 2) {
                        image = candidate;
                        break;
                    }

                    if (!image)
                        image = candidate;
                }

                if (image) {

                    void *d3dTexture = (void *)((GfxImage *)image)->texture.basemap;
                    if (d3dTexture) {

                        void *device = ((DxGlobals *)imp_dx)->device;
                        void **vtable = *(void ***)device;
                        ((void(D3DVTCC *)(void *, int, void *))vtable[0x104 / 4])(device, 0, d3dTexture);

                        {
                            unsigned int texID = *(unsigned int *)((byte *)d3dTexture + 0x54);
                            unsigned int texTarget = (*(int *)image == 5)
                                                         ? RB_GL_TEXTURE_CUBE_MAP
                                                         : RB_GL_TEXTURE_2D;

                            extern unsigned int g_prebind_texID;
                            extern unsigned int g_prebind_texTarget;
                            g_prebind_texID = texID;
                            g_prebind_texTarget = texTarget;
                            if (texID) {
                                if (texTarget == RB_GL_TEXTURE_CUBE_MAP) {
                                    glDisable(RB_GL_TEXTURE_2D);
                                    glBindTexture(RB_GL_TEXTURE_CUBE_MAP, texID);
                                    glEnable(RB_GL_TEXTURE_CUBE_MAP);
                                } else {
                                    glDisable(RB_GL_TEXTURE_CUBE_MAP);
                                    glBindTexture(RB_GL_TEXTURE_2D, texID);
                                    glEnable(RB_GL_TEXTURE_2D);
                                }
                            }
                        }
                    }
                } else {
                    extern unsigned int g_prebind_texID;
                    extern unsigned int g_prebind_texTarget;
                    g_prebind_texID = 0;
                    g_prebind_texTarget = RB_GL_TEXTURE_2D;
                }
            }
        }
#undef RB_GL_TEXTURE_2D
#undef RB_GL_TEXTURE_CUBE_MAP
    }

    g_rb_endsurface_draw++;
    RB_X64TraceEndSurface("draw", material, technique, (const materialCommands_t *)tess);
    RB_DrawTechnique((*(MaterialVertexDeclType *)&((materialCommands_t *)tess)->declType), &args);

    tess = RB_TessBase();
    ((materialCommands_t *)tess)->indexCount = 0;
    ((materialCommands_t *)tess)->vertexCount = 0;
    return;

cleanup:
    ((materialCommands_t *)tess)->optimizedIndexCount = 0;
    ((materialCommands_t *)tess)->indexCount = 0;
    ((materialCommands_t *)tess)->vertexCount = 0;
}

/* GfxStateOverride { int stateBitsMaskClear[2]; int stateBitsMaskSet[2]; }
 * reconstructed from the first 16 bytes; trailing 16 bytes were blob padding. */
const GfxStateOverride overrideEnableRenormalize = {
    { -1, -1 },            /* stateBitsMaskClear: 0xffffffff, 0xffffffff */
    { 0x40000000, 0 }      /* stateBitsMaskSet */
};

const vec4_t debugShaderConsts[5] = {
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f }
};

const DWORD s_fvfForVertDeclType[4] = { 0x00000152, 0x00000242, 0x00000102, 0x00000142 };

const float lightGridLookupMatrix[4][4] = { { 0.816496551f, -0.408248276f, -0.408248276f, 0.0f }, { 0.0f, 0.707106769f, -0.707106769f, 0.0f }, { 0.577350259f, 0.577350259f, 0.577350259f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } }; /* truncated 176->16 (blob over-capture) */

const unsigned char lightGridLookupMatrix_125709[64] = {
    0xeb, 0x05, 0x51, 0x3f, 0xeb, 0x05, 0xd1, 0xbe, 0xeb, 0x05, 0xd1, 0xbe, 0x00, 0x00, 0x00, 0x00,
    0xf3, 0x04, 0x35, 0x3f, 0xf3, 0x04, 0x35, 0xbf, 0x3a, 0xcd, 0x13, 0x3f, 0x3a, 0xcd, 0x13, 0x3f,
    0x3a, 0xcd, 0x13, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CD3DXBuffer[];
extern fnptr_t vtbl_CD3DXConstantTable[];

typedef struct {
    void **vtable;
    ULONG refCount;
    void *data;
    UINT32 size;
} CD3DXBufferImpl;

HRESULT CD3DXBuffer_QueryInterface(const void *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    return 0;
}

ULONG CD3DXBuffer_AddRef(const void *_this)
{
    CD3DXBufferImpl *buf = (CD3DXBufferImpl *)_this;
    return ++buf->refCount;
}

ULONG CD3DXBuffer_Release(const void *_this)
{
    CD3DXBufferImpl *buf = (CD3DXBufferImpl *)_this;
    ULONG rc = --buf->refCount;
    if (!rc) {
        free(buf->data);
        free(buf);
    }
    return rc;
}

void *CD3DXBuffer_GetBufferPointer(const void *_this)
{
    return ((CD3DXBufferImpl *)_this)->data;
}

UINT32 CD3DXBuffer_GetBufferSize(const void *_this)
{
    return ((CD3DXBufferImpl *)_this)->size;
}

void ZN11CD3DXBufferD1Ev(const void *_this)
{
    CD3DXBufferImpl *buf = (CD3DXBufferImpl *)_this;
    buf->vtable = (void**)vtbl_CD3DXBuffer;
    free(buf->data);
    buf->data = NULL;
}

void ZN11CD3DXBufferD0Ev(const void *_this)
{
    ZN11CD3DXBufferD1Ev(_this);
    free((void *)_this);
}

static CD3DXBufferImpl *CD3DXBuffer_Create(const void *data, UINT32 size)
{
    CD3DXBufferImpl *buf = (CD3DXBufferImpl *)calloc(1, sizeof(CD3DXBufferImpl));
    buf->vtable = (void**)vtbl_CD3DXBuffer;
    buf->refCount = 1;
    buf->size = size;
    if (size && data) {
        buf->data = malloc(size);
        memcpy(buf->data, data, size);
    } else if (size) {
        buf->data = calloc(1, size);
    }
    return buf;
}

typedef struct {
    void **vtable;
    ULONG refCount;
    void *data;
    UINT32 size;
} CD3DXConstantTableImpl;

HRESULT CD3DXConstantTable_QueryInterface(const void *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    return 0;
}

ULONG CD3DXConstantTable_AddRef(const void *_this)
{
    return ++((CD3DXConstantTableImpl *)_this)->refCount;
}

ULONG CD3DXConstantTable_Release(const void *_this)
{
    CD3DXConstantTableImpl *ct = (CD3DXConstantTableImpl *)_this;
    ULONG rc = --ct->refCount;
    if (!rc) {
        free(ct->data);
        free(ct);
    }
    return rc;
}

void *CD3DXConstantTable_GetBufferPointer(const void *_this)
{
    return ((CD3DXConstantTableImpl *)_this)->data;
}
UINT32 CD3DXConstantTable_GetBufferSize(const void *_this)
{
    return ((CD3DXConstantTableImpl *)_this)->size;
}
HRESULT CD3DXConstantTable_GetDesc(const void *_this, void *pDesc)
{
    (void)_this;
    memset(pDesc, 0, 32);
    return 0;
}
HRESULT CD3DXConstantTable_GetConstantDesc(const void *_this, const void *hConstant, void *pConstantDesc, UINT *pCount)
{
    (void)_this;
    (void)hConstant;
    (void)pConstantDesc;
    *pCount = 0;
    return 0;
}
UINT CD3DXConstantTable_GetSamplerIndex(const void *_this, const void *hConstant)
{
    (void)_this;
    (void)hConstant;
    return 0;
}
void *CD3DXConstantTable_GetConstant(const void *_this, const void *hConstant, UINT Index)
{
    (void)_this;
    (void)hConstant;
    (void)Index;
    return NULL;
}
void *CD3DXConstantTable_GetConstantByName(const void *_this, const void *hConstant, const char *pName)
{
    (void)_this;
    (void)hConstant;
    (void)pName;
    return NULL;
}
void *CD3DXConstantTable_GetConstantElement(const void *_this, const void *hConstant, UINT Index)
{
    (void)_this;
    (void)hConstant;
    (void)Index;
    return NULL;
}
HRESULT CD3DXConstantTable_SetDefaults(const void *_this, void *pDevice)
{
    (void)_this;
    (void)pDevice;
    return 0;
}
HRESULT CD3DXConstantTable_SetValue(const void *_this, void *pDevice, const void *hConstant, const void *pData, UINT Bytes)
{
    (void)_this;
    (void)pDevice;
    (void)hConstant;
    (void)pData;
    (void)Bytes;
    return 0;
}
HRESULT CD3DXConstantTable_SetBool(const void *_this, void *d, const void *h, BOOL b)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)b;
    return 0;
}
HRESULT CD3DXConstantTable_SetBoolArray(const void *_this, void *d, const void *h, const BOOL *b, UINT n)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)b;
    (void)n;
    return 0;
}
HRESULT CD3DXConstantTable_SetInt(const void *_this, void *d, const void *h, INT n)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)n;
    return 0;
}
HRESULT CD3DXConstantTable_SetIntArray(const void *_this, void *d, const void *h, const INT *n, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)n;
    (void)c;
    return 0;
}
HRESULT CD3DXConstantTable_SetFloat(const void *_this, void *d, const void *h, float f)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)f;
    return 0;
}
HRESULT CD3DXConstantTable_SetFloatArray(const void *_this, void *d, const void *h, const float *f, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)f;
    (void)c;
    return 0;
}
HRESULT CD3DXConstantTable_SetVector(const void *_this, void *d, const void *h, const void *v)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)v;
    return 0;
}
HRESULT CD3DXConstantTable_SetVectorArray(const void *_this, void *d, const void *h, const void *v, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)v;
    (void)c;
    return 0;
}
HRESULT CD3DXConstantTable_SetMatrix(const void *_this, void *d, const void *h, const void *m)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)m;
    return 0;
}
HRESULT CD3DXConstantTable_SetMatrixArray(const void *_this, void *d, const void *h, const void *m, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)m;
    (void)c;
    return 0;
}
HRESULT CD3DXConstantTable_SetMatrixPointerArray(const void *_this, void *d, const void *h, const void **m, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)m;
    (void)c;
    return 0;
}
HRESULT CD3DXConstantTable_SetMatrixTranspose(const void *_this, void *d, const void *h, const void *m)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)m;
    return 0;
}
HRESULT CD3DXConstantTable_SetMatrixTransposeArray(const void *_this, void *d, const void *h, const void *m, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)m;
    (void)c;
    return 0;
}
HRESULT CD3DXConstantTable_SetMatrixTransposePointerArray(const void *_this, void *d, const void *h, const void **m, UINT c)
{
    (void)_this;
    (void)d;
    (void)h;
    (void)m;
    (void)c;
    return 0;
}

void ZN18CD3DXConstantTableD1Ev(const void *_this)
{
    CD3DXConstantTableImpl *ct = (CD3DXConstantTableImpl *)_this;
    ct->vtable = (void**)vtbl_CD3DXConstantTable;
    free(ct->data);
    ct->data = NULL;
}

void ZN18CD3DXConstantTableD0Ev(const void *_this)
{
    ZN18CD3DXConstantTableD1Ev(_this);
    free((void *)_this);
}

static int hlsl_has(const char *src, int len, const char *needle)
{
    int nlen = strlen(needle);
    int i;
    for (i = 0; i <= len - nlen; i++) {
        if (memcmp(src + i, needle, nlen) == 0)
            return 1;
    }
    return 0;
}

static const char *arb_vs_lightmap =
    "!!ARBvp1.0\n"
    "PARAM mvp[4] = { state.matrix.mvp };\n"
    ""
    "ATTRIB iPos = vertex.position;\n"
    "ATTRIB iCol = vertex.color;\n"
    "ATTRIB iTc0 = vertex.texcoord[0];\n"
    "ATTRIB iTc1 = vertex.texcoord[1];\n"
    "OUTPUT oPos = result.position;\n"
    "OUTPUT oCol = result.color;\n"
    "OUTPUT oTc0 = result.texcoord[0];\n"
    "OUTPUT oTc1 = result.texcoord[1];\n"
    "OUTPUT oFog = result.fogcoord;\n"
    ""
    "DP4 oPos.x, iPos, mvp[0];\n"
    "DP4 oPos.y, iPos, mvp[1];\n"
    "DP4 oPos.z, iPos, mvp[2];\n"
    "DP4 oPos.w, iPos, mvp[3];\n"
    "MOV oCol, iCol;\n"
    "MOV oTc0, iTc0;\n"
    "MOV oTc1, iTc1;\n"
    "MOV oFog.x, {1}.x;\n"
    "END\n";

static const char *arb_vs_simple =
    "!!ARBvp1.0\n"
    "PARAM mvp[4] = { state.matrix.mvp };\n"
    ""
    "ATTRIB iPos = vertex.position;\n"
    "ATTRIB iCol = vertex.color;\n"
    "ATTRIB iTc0 = vertex.texcoord[0];\n"
    "OUTPUT oPos = result.position;\n"
    "OUTPUT oCol = result.color;\n"
    "OUTPUT oTc0 = result.texcoord[0];\n"
    "OUTPUT oFog = result.fogcoord;\n"
    ""
    "DP4 oPos.x, iPos, mvp[0];\n"
    "DP4 oPos.y, iPos, mvp[1];\n"
    "DP4 oPos.z, iPos, mvp[2];\n"
    "DP4 oPos.w, iPos, mvp[3];\n"
    "MOV oCol, iCol;\n"
    "MOV oTc0, iTc0;\n"
    "MOV oFog.x, {1}.x;\n"
    "END\n";

static const char *arb_vs_sky =
    "!!ARBvp1.0\n"
    "PARAM mvp[4] = { state.matrix.mvp };\n"
    "ATTRIB iPos = vertex.position;\n"
    "OUTPUT oPos = result.position;\n"
    "OUTPUT oTc0 = result.texcoord[0];\n"
    "DP4 oPos.x, iPos, mvp[0];\n"
    "DP4 oPos.y, iPos, mvp[1];\n"
    "DP4 oPos.z, iPos, mvp[2];\n"
    "DP4 oPos.w, iPos, mvp[3];\n"
    "MOV oTc0.xyz, iPos;\n"
    "END\n";

static const char *arb_vs_passthrough =
    "!!ARBvp1.0\n"
    "PARAM mvp[4] = { state.matrix.mvp };\n"
    "ATTRIB iPos = vertex.position;\n"
    "ATTRIB iCol = vertex.color;\n"
    "ATTRIB iTc0 = vertex.texcoord[0];\n"
    "OUTPUT oPos = result.position;\n"
    "OUTPUT oCol = result.color;\n"
    "OUTPUT oTc0 = result.texcoord[0];\n"
    "DP4 oPos.x, iPos, mvp[0];\n"
    "DP4 oPos.y, iPos, mvp[1];\n"
    "DP4 oPos.z, iPos, mvp[2];\n"
    "DP4 oPos.w, iPos, mvp[3];\n"
    "MOV oCol, iCol;\n"
    "MOV oTc0, iTc0;\n"
    "END\n";

static const char *arb_ps_lightmap =
    "!!ARBfp1.0\n"
    "OPTION ARB_precision_hint_fastest;\n"
    "OUTPUT oC0AfterFog = result.color;\n"
    "TEMP r0, r1, oC0;\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB t0 = fragment.texcoord[0];\n"
    "ATTRIB t1 = fragment.texcoord[1];\n"
    "TEX r0, t0, texture[0], 2D;\n"
    "TEX r1, t1, texture[1], 2D;\n"
    "MUL r0, r0, v0;\n"
    "MUL oC0.rgb, r0, r1;\n"
    "MOV oC0.a, r0.a;\n"

    "MAX r1.x, fragment.fogcoord.x, {0}.x;\n"
    "MIN r1.x, r1.x, {1}.x;\n"
    "LRP oC0AfterFog.xyz, r1.x, oC0, state.fog.color;\n"
    "MOV oC0AfterFog.w, oC0.w;\n"
    "END\n";

static const char *arb_ps_lightmap_alpha =
    "!!ARBfp1.0\n"
    "OPTION ARB_precision_hint_fastest;\n"
    "OUTPUT oC0AfterFog = result.color;\n"
    "TEMP r0, r1, oC0;\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB t0 = fragment.texcoord[0];\n"
    "ATTRIB t1 = fragment.texcoord[1];\n"
    "TEX r0, t0, texture[0], 2D;\n"
    "TEX r1, t1, texture[1], 2D;\n"
    "MUL r0, r0, v0;\n"
    "MUL oC0.rgb, r0, r1;\n"
    "MOV oC0.a, r0.a;\n"
    "MAX r1.x, fragment.fogcoord.x, {0}.x;\n"
    "MIN r1.x, r1.x, {1}.x;\n"
    "LRP oC0AfterFog.xyz, r1.x, oC0, state.fog.color;\n"
    "MOV oC0AfterFog.w, oC0.w;\n"
    "END\n";

static const char *arb_ps_textured =
    "!!ARBfp1.0\n"
    "OPTION ARB_precision_hint_fastest;\n"
    "OUTPUT oC0AfterFog = result.color;\n"
    "TEMP r0, r1;\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB t0 = fragment.texcoord[0];\n"
    "TEX r0, t0, texture[0], 2D;\n"
    "MUL r0, r0, v0;\n"
    "MAX r1.x, fragment.fogcoord.x, {0}.x;\n"
    "MIN r1.x, r1.x, {1}.x;\n"
    "LRP oC0AfterFog.xyz, r1.x, r0, state.fog.color;\n"
    "MOV oC0AfterFog.w, r0.w;\n"
    "END\n";

static const char *arb_ps_vertcolor =
    "!!ARBfp1.0\n"
    "OPTION ARB_precision_hint_fastest;\n"
    "OUTPUT oC0AfterFog = result.color;\n"
    "TEMP r0, oC0;\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "MOV oC0, v0;\n"
    "MAX r0.x, fragment.fogcoord.x, {0}.x;\n"
    "MIN r0.x, r0.x, {1}.x;\n"
    "LRP oC0AfterFog.xyz, r0.x, oC0, state.fog.color;\n"
    "MOV oC0AfterFog.w, oC0.w;\n"
    "END\n";

static const char *arb_ps_sky =
    "!!ARBfp1.0\n"
    "OUTPUT oC0 = result.color;\n"
    "TEMP r0;\n"
    "ATTRIB t0 = fragment.texcoord[0];\n"
    "TEX r0, t0, texture[0], CUBE;\n"
    "MOV r0.w, {0}.x;\n"
    "MOV oC0, r0;\n"
    "END\n";

static const char *arb_ps_passthrough =
    "!!ARBfp1.0\n"
    "OUTPUT oC0 = result.color;\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "MOV oC0, v0;\n"
    "END\n";

static const char *arb_ps_multiply =
    "!!ARBfp1.0\n"
    "OPTION ARB_precision_hint_fastest;\n"
    "PARAM matColor = program.env[23];\n"
    "OUTPUT oC0AfterFog = result.color;\n"
    "TEMP r0, r1, oC0;\n"
    "ATTRIB v0 = fragment.color.primary;\n"
    "ATTRIB t0 = fragment.texcoord[0];\n"
    "TEX r0, t0, texture[0], 2D;\n"
    "MUL r1.xyz, v0, r0;\n"
    "MAD oC0.xyz, matColor.w, r1, r0;\n"
    "MOV oC0.w, r0.w;\n"
    "MAX r1.x, fragment.fogcoord.x, {0}.x;\n"
    "MIN r1.x, r1.x, {1}.x;\n"
    "LRP oC0AfterFog.xyz, r1.x, oC0, state.fog.color;\n"
    "MOV oC0AfterFog.w, oC0.w;\n"
    "END\n";

static const char *arb_ps_depth =
    "!!ARBfp1.0\n"
    "OUTPUT oC0 = result.color;\n"
    "MOV oC0, {1, 1, 1, 1};\n"
    "END\n";

static const char *arb_ps_debug_green =
    "!!ARBfp1.0\n"
    "OUTPUT oC0 = result.color;\n"
    "MOV oC0, {0, 1, 0, 1};\n"
    "END\n";

HRESULT D3DXCompileShader(
    const char *pSrcData, UINT SrcDataLen,
    const void *pDefines, void *pInclude,
    const char *pFunctionName, const char *pProfile,
    DWORD Flags, void **ppShader, void **ppErrorMsgs,
    void **ppConstantTable)
{
    const char *arbCode = NULL;
    int isVS = 0;
    int len = (int)SrcDataLen;

#ifdef GFX_REAL_D3D9

    {
        typedef HRESULT(__stdcall * PFN_D3DXCompile)(const char *, UINT, const void *, void *,
                                                     const char *, const char *, DWORD, void **, void **, void **);
        extern void *__stdcall LoadLibraryA(const char *);
        extern void *__stdcall GetProcAddress(void *, const char *);
        static PFN_D3DXCompile s_real;
        static int s_tried;
        if (!s_tried) {
            /* Try every shipped d3dx9 version (24..43) plus the generic name, so we
             * find whatever is on the box -- CoD2 itself ships an early one
             * (d3dx9_2x), while the DX redist installs the later d3dx9_4x. A too-short
             * list here means no real compiler -> null shaders -> black screen. */
            const char *libs[] = {
                "d3dx9_43.dll", "d3dx9_42.dll", "d3dx9_41.dll", "d3dx9_40.dll", "d3dx9_39.dll",
                "d3dx9_38.dll", "d3dx9_37.dll", "d3dx9_36.dll", "d3dx9_35.dll", "d3dx9_34.dll",
                "d3dx9_33.dll", "d3dx9_32.dll", "d3dx9_31.dll", "d3dx9_30.dll", "d3dx9_29.dll",
                "d3dx9_28.dll", "d3dx9_27.dll", "d3dx9_26.dll", "d3dx9_25.dll", "d3dx9_24.dll",
                "d3dx9.dll", 0 };
            int li;
            s_tried = 1;
            for (li = 0; libs[li]; li++) {
                void *m = LoadLibraryA(libs[li]);
                if (m) {
                    s_real = (PFN_D3DXCompile)GetProcAddress(m, "D3DXCompileShader");
                    if (s_real)
                        break;
                }
            }
            fprintf(stderr, "[real-d3dx9] D3DXCompileShader=%p (%s)\n",
                    (void *)s_real, s_real ? libs[li] : "NOT FOUND");
            fflush(stderr);
        }
        if (s_real)
            return s_real(pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pProfile,
                          Flags, ppShader, ppErrorMsgs, ppConstantTable);
    }
#endif

    (void)pDefines;
    (void)pInclude;
    (void)Flags;

    if (ppErrorMsgs)
        *ppErrorMsgs = NULL;

    if (pProfile && pProfile[0] == 'v' && pProfile[1] == 's')
        isVS = 1;

    if (isVS) {

        if (hlsl_has(pSrcData, len, "texCUBE") || hlsl_has(pSrcData, len, "cubeMapSampler")) {
            arbCode = arb_vs_sky;
        } else if (hlsl_has(pSrcData, len, "lmapCoords") || hlsl_has(pSrcData, len, "lightmap")) {
            arbCode = arb_vs_lightmap;
        } else if (hlsl_has(pSrcData, len, "texCoords") || hlsl_has(pSrcData, len, "colorMapSampler")) {
            arbCode = arb_vs_simple;
        } else {
            arbCode = arb_vs_passthrough;
        }
    } else {

        if (hlsl_has(pSrcData, len, "texCUBE")) {
            arbCode = arb_ps_sky;
        } else if (hlsl_has(pSrcData, len, "lightmapSampler") || hlsl_has(pSrcData, len, "lmapCoords") || hlsl_has(pSrcData, len, "lightmap")) {
            arbCode = arb_ps_lightmap;
        } else if (hlsl_has(pSrcData, len, "colorMapSampler") || hlsl_has(pSrcData, len, "tex2D")) {
            arbCode = arb_ps_textured;
        } else {
            arbCode = arb_ps_vertcolor;
        }
    }

    if (ppShader) {
        int arbLen = strlen(arbCode) + 1;
        *ppShader = CD3DXBuffer_Create(arbCode, arbLen);
    }

    if (ppConstantTable) {
        CD3DXConstantTableImpl *ct = (CD3DXConstantTableImpl *)calloc(1, sizeof(CD3DXConstantTableImpl));
        ct->vtable = (void**)vtbl_CD3DXConstantTable;
        ct->refCount = 1;
        *ppConstantTable = ct;
    }

    return 0;
}

static int IncludeClass_Open(void *_this, int IncludeType, const char *pFileName,
                             const void *pParentData, const void **ppData, UINT *pBytes)
{
    (void)_this;
    (void)IncludeType;
    (void)pParentData;
    (void)pFileName;

    *ppData = "";
    *pBytes = 0;
    return 0;
}

static int IncludeClass_Close(void *_this, const void *pData)
{
    (void)_this;
    (void)pData;
    return 0;
}

#ifdef GFX_REAL_D3D9

extern int FS_ReadFile(const char *path, void **buffer);
extern void FS_FreeFile(void *buffer);

static void StripConstRegisters(char *s, int len)
{
    int i = 0;
    while (i < len) {
        if (s[i] == ':') {
            int j = i + 1;
            while (j < len && (s[j] == ' ' || s[j] == '\t'))
                j++;
            if (j + 8 <= len && memcmp(s + j, "register", 8) == 0) {
                int k = j + 8;
                while (k < len && s[k] != '(')
                    k++;
                int p = k + 1;
                while (p < len && (s[p] == ' ' || s[p] == '\t'))
                    p++;
                if (p < len && (s[p] == 'c' || s[p] == 'C')) {
                    while (k < len && s[k] != ')')
                        k++;
                    if (k < len) {
                        int m;
                        for (m = i; m <= k; m++)
                            s[m] = ' ';
                        i = k + 1;
                        continue;
                    }
                }
            }
        }
        i++;
    }
}

static int __attribute__((stdcall)) RealInclude_Open(void *_this, int IncludeType,
                                                     const char *pFileName, const void *pParentData, const void **ppData, UINT *pBytes)
{
    char path[160];
    void *data = NULL;
    int len;
    (void)_this;
    (void)IncludeType;
    (void)pParentData;

    snprintf(path, sizeof(path), "materials/shaders/%s", pFileName);
    len = FS_ReadFile(path, &data);
    if ((len < 0 || !data) && !strchr(pFileName, '/')) {
        snprintf(path, sizeof(path), "materials/shaders/lib/%s", pFileName);
        len = FS_ReadFile(path, &data);
    }
    if (getenv("REALD3D9_MATDIAG"))
        fprintf(stderr, "[INCLUDE] '%s' -> '%s' =%d\n", pFileName, path, len), fflush(stderr);
    if (len < 0 || !data) {
        *ppData = "";
        *pBytes = 0;
        return 0;
    }
    StripConstRegisters((char *)data, len);
    *ppData = data;
    *pBytes = (UINT)len;
    return 0;
}
static int __attribute__((stdcall)) RealInclude_Close(void *_this, const void *pData)
{
    (void)_this;
    if (pData && ((const char *)pData)[0])
        FS_FreeFile((void *)pData);
    return 0;
}

void *g_realIncludeVtbl[2] = { (void *)RealInclude_Open, (void *)RealInclude_Close };
#endif

fnptr_t vtbl_CD3DXBuffer[] = { (fnptr_t)CD3DXBuffer_QueryInterface, (fnptr_t)CD3DXBuffer_AddRef, (fnptr_t)CD3DXBuffer_Release, (fnptr_t)CD3DXBuffer_GetBufferPointer, (fnptr_t)CD3DXBuffer_GetBufferSize, (fnptr_t)ZN11CD3DXBufferD1Ev, (fnptr_t)ZN11CD3DXBufferD0Ev };
fnptr_t vtbl_CD3DXConstantTable[] = { (fnptr_t)CD3DXConstantTable_QueryInterface, (fnptr_t)CD3DXConstantTable_AddRef, (fnptr_t)CD3DXConstantTable_Release, (fnptr_t)CD3DXConstantTable_GetBufferPointer, (fnptr_t)CD3DXConstantTable_GetBufferSize, (fnptr_t)CD3DXConstantTable_GetDesc, (fnptr_t)CD3DXConstantTable_GetConstantDesc, (fnptr_t)CD3DXConstantTable_GetSamplerIndex, (fnptr_t)CD3DXConstantTable_GetConstant, (fnptr_t)CD3DXConstantTable_GetConstantByName, (fnptr_t)CD3DXConstantTable_GetConstantElement, (fnptr_t)CD3DXConstantTable_SetDefaults, (fnptr_t)CD3DXConstantTable_SetValue, (fnptr_t)CD3DXConstantTable_SetBool, (fnptr_t)CD3DXConstantTable_SetBoolArray, (fnptr_t)CD3DXConstantTable_SetInt, (fnptr_t)CD3DXConstantTable_SetIntArray, (fnptr_t)CD3DXConstantTable_SetFloat, (fnptr_t)CD3DXConstantTable_SetFloatArray, (fnptr_t)CD3DXConstantTable_SetVector, (fnptr_t)CD3DXConstantTable_SetVectorArray, (fnptr_t)CD3DXConstantTable_SetMatrix, (fnptr_t)CD3DXConstantTable_SetMatrixArray, (fnptr_t)CD3DXConstantTable_SetMatrixPointerArray, (fnptr_t)CD3DXConstantTable_SetMatrixTranspose, (fnptr_t)CD3DXConstantTable_SetMatrixTransposeArray, (fnptr_t)CD3DXConstantTable_SetMatrixTransposePointerArray, (fnptr_t)ZN18CD3DXConstantTableD1Ev, (fnptr_t)ZN18CD3DXConstantTableD0Ev };

#include "common_types.h"
#include <stdio.h>
#include <stdlib.h>
#include "imports.h"
#include "bytematch.h"

extern const dvar_t *r_showGroundLit;

extern struct r_globals_load_t rgl;
extern GfxWorld s_world;

COD2_ASSERT_FIELD(GfxWorld, name, 0);
COD2_ASSERT_FIELD(GfxWorld, surfaces, 20);
COD2_ASSERT_FIELD(GfxWorld, cullGroupCount, 236);
COD2_ASSERT_FIELD(GfxWorld, cullGroups, 240);
COD2_ASSERT_FIELD(GfxWorld, cellCount, 252);
COD2_ASSERT_FIELD(GfxWorld, cells, 256);
COD2_ASSERT_FIELD(struct r_globals_load_t, occluders, 8);
COD2_ASSERT_FIELD(struct r_globals_load_t, portalVerts, 12);
COD2_ASSERT_FIELD(struct r_globals_load_t, aabbTrees, 16);
COD2_ASSERT_FIELD(struct r_globals_load_t, aabbTreeCount, 20);

#ifndef __EMSCRIPTEN__
typedef struct daabbnode_ondisk_s {
    float mins[3];
    float maxs[3];
    int startSurfIndex;
    int surfaceCount;
} daabbnode_ondisk_t;

typedef struct dcell_ondisk_s {
    float mins[3];
    float maxs[3];
    int aabbTreeIndex;
    int firstPortal;
    int portalCount;
    int firstOccluder;
    int occluderCount;
    int firstReflectionProbe;
    int reflectionProbeCount;
} dcell_ondisk_t;

static int R_FinishLoadingAabbTrees_r(GfxAabbTree *tree, int totalTreesUsed);
#endif
const char *R_ParseSunLight(SunLightParseParams *params, const char *text);
void R_InterpretSunLightParseParamsIntoLights(SunLightParseParams *sunParse, GfxLight *sunLight);
#ifndef __EMSCRIPTEN__
static Bool R_IsValidStaticModel(char *spawnVars[64][2], int spawnVarCount, struct XModel **model, vec_t *origin);
#endif
#ifdef __EMSCRIPTEN__
static void R_SetParentAndCell_r(mnode_t *node, int parent);
#else
static void __attribute_regparm__(2) R_SetParentAndCell_r(mnode_t *node, int parent);
#endif
static void R_SetParentAndCell_r_impl(mnode_t *node, int parent);
void __attribute_regparm__(1) R_LoadEntities(GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadNodesAndLeafs(GfxBspLoad *load);
static void R_LoadNodesAndLeafs_impl(const GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadPortals(GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadCells(GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadAabbTrees(GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadOccluders(GfxBspLoad *load);
static void R_LoadOccluders_impl(const GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadPortalVerts(GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadCullGroups(GfxBspLoad *load);
void __attribute_regparm__(1) R_LoadSurfaces(GfxBspLoad *load);
GfxWorld *R_LoadWorldInternal(const char *name);

extern void R_Error(int level, const char *msg, ...);
extern void ClearBounds(float *mins, float *maxs);
extern void AddPointToBounds(const vec_t *v, vec_t *mins, vec_t *maxs);
extern void ExpandBounds(const float *mins, const float *maxs, float *dstMins, float *dstMaxs);
extern const char *Com_Parse(const char **text);
extern void I_strncpyz(char *dest, const char *src, int size);
extern int I_stricmp(const char *a, const char *b);
extern float ColorNormalize(const float *color, float *out);
extern void Com_Printf(const char *fmt, ...);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void *Hunk_AllocInternal(int size);
extern MaterialHandle Material_Register(const char *name, int imageTrack);
extern IDirect3DVertexBuffer9 *R_CreateWorldVertexBuffer(GfxWorldVertex *vertices, int vertexCount);
extern void *CColorConverter_GetColorConverter(int mode);

static int R_ValidateLump(const GfxBspLoad *load, int lumpOfs, int elemSize, const byte **outData)
{
    const byte *header = (const byte *)load->header;
    int lumpSize = *(int *)(header + lumpOfs);
    int lumpFileOfs = *(int *)(header + lumpOfs + 4);
    int count;

    if (lumpFileOfs + lumpSize > load->fileSize)
        R_Error(1, "LoadMap: lump extends past end of file in %s", s_world.name);
    if (lumpFileOfs <= 3) {

        if (lumpSize == 0) {
            if (outData)
                *outData = NULL;
            return 0;
        }
        R_Error(1, "LoadMap: funny lump offset in %s", s_world.name);
    }

    count = lumpSize / elemSize;
    if (lumpSize < 0)
        count = (lumpSize + elemSize - 1) / elemSize;
    if (count * elemSize != lumpSize)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);

    if (outData)
        *outData = load->fileBase + lumpFileOfs;
    return count;
}

static inline __attribute__((always_inline)) int R_ValidateLumpInline(const GfxBspLoad *load, int lumpOfs, int elemSize, const byte **outData)
{
    const byte *header = (const byte *)load->header;
    const int *lump = (const int *)(header + lumpOfs);
    int lumpFileOfs = lump[1];
    int lumpSize = lump[0];
    int count;

    if (lumpFileOfs + lumpSize > load->fileSize)
        R_Error(1, "LoadMap: lump extends past end of file in %s", s_world.name);
    if (lumpFileOfs <= 3)
        R_Error(1, "LoadMap: funny lump offset in %s", s_world.name);

    count = lumpSize / elemSize;
    if (lumpSize < 0 || count * elemSize != lumpSize)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);

    if (outData)
        *outData = load->fileBase + lumpFileOfs;
    return count;
}
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern struct XModel * R_RegisterModel(const char *name);
extern int XModelBad(const struct XModel *model);
extern Bool R_ValidateStaticModel(struct XModel *model);
extern int strnicmp(const char *a, const char *b, size_t n);
extern int stricmp(const char *a, const char *b);
extern cplane_t * CM_GetPlaneNum(int planeNum);
extern void PerpendicularVector(const float *plane, vec_t *perpOut);
extern void Vec3Cross(const float *plane, const vec_t *perp, vec_t *crossOut);

static int R_FinishLoadingAabbTrees_r_impl(GfxAabbTree *tree, int totalTreesUsed)
{
    byte *treeMins = (byte *)tree->mins;
    byte *treeMaxs = (byte *)tree->maxs;
    int childCount, surfCount, i;

    ClearBounds((float *)treeMins, (float *)treeMaxs);

    childCount = tree->childCount;
    if (childCount) {

        tree->children = rgl.aabbTrees + totalTreesUsed;
        totalTreesUsed += childCount;

        for (i = 0; i < childCount; i++) {
            GfxAabbTree *child = &tree->children[i];
            totalTreesUsed = R_FinishLoadingAabbTrees_r_impl(child, totalTreesUsed);
            ExpandBounds(child->mins, child->maxs, (float *)treeMins, (float *)treeMaxs);
        }
    } else {

        int firstSurf = tree->startSurfIndex;
        surfCount = tree->surfaceCount;

        for (i = 0; i < surfCount; i++) {
            srfTriangles_t *tris = s_world.surfaces[firstSurf + i].tris;
            ExpandBounds(tris->bounds[0], tris->bounds[1], (float *)treeMins, (float *)treeMaxs);
        }
    }

    return totalTreesUsed;
}

#ifdef __EMSCRIPTEN__

static int R_FinishLoadingAabbTrees_r(GfxAabbTree *tree, int totalTreesUsed)
{
    return R_FinishLoadingAabbTrees_r_impl(tree, totalTreesUsed);
}
#else

static int R_FinishLoadingAabbTrees_r(GfxAabbTree *tree, int totalTreesUsed)
{
    return R_FinishLoadingAabbTrees_r_impl(tree, totalTreesUsed);
}

#endif

const char *R_ParseSunLight(SunLightParseParams *params, const char *text)
{
    byte *p = (byte *)params;
    char keyname[0x800];
    char value[0x800];
    const char *token;

    while (1) {
        token = Com_Parse(&text);
        if (!token[0] || token[0] == '}')
            break;

        if (token[0] == '{') {

            ((SunLightParseParams *)p)->ambientScale = 0.0f;
            ((SunLightParseParams *)p)->sunLight = 0.0f;
            ((SunLightParseParams *)p)->diffuseFraction = 0.5f;
            ((SunLightParseParams *)p)->diffuseColorHasBeenSet = 0;

            ((SunLightParseParams *)p)->ambientColor[0] = 0;
            ((SunLightParseParams *)p)->ambientColor[1] = 0;
            ((SunLightParseParams *)p)->ambientColor[2] = 0;
            ((SunLightParseParams *)p)->sunColor[0] = 0;
            ((SunLightParseParams *)p)->sunColor[1] = 0;
            ((SunLightParseParams *)p)->sunColor[2] = 0;
            ((SunLightParseParams *)p)->diffuseColor[0] = 0;
            ((SunLightParseParams *)p)->diffuseColor[1] = 0;
            ((SunLightParseParams *)p)->diffuseColor[2] = 0;
            continue;
        }

        I_strncpyz(keyname, token, 0x800);

        token = Com_Parse(&text);
        if (!token[0] || token[0] == '}')
            break;
        I_strncpyz(value, token, 0x800);

        if (!I_stricmp(keyname, "ambient")) {
            ((SunLightParseParams *)p)->ambientScale = (float)atof(value);
            if (((SunLightParseParams *)p)->ambientScale > 2.0f) {
                Com_Printf("^3WARNING: ambient too big, assuming it uses the old 0-255 scale instead of the proper 0-1 scale (value = '%s')\n", value);
                ((SunLightParseParams *)p)->ambientScale *= 0.01568627543747425f;
            }
        } else if (!I_stricmp(keyname, "_color")) {
            ((SunLightParseParams *)p)->ambientColor[0] = 0;
            ((SunLightParseParams *)p)->ambientColor[1] = 0;
            ((SunLightParseParams *)p)->ambientColor[2] = 0;
            sscanf(value, "%f %f %f", (float *)&((SunLightParseParams *)p)->ambientColor[0], (float *)&((SunLightParseParams *)p)->ambientColor[1], (float *)&((SunLightParseParams *)p)->ambientColor[2]);
        } else if (!I_stricmp(keyname, "diffuseFraction")) {
            ((SunLightParseParams *)p)->diffuseFraction = (float)atof(value);
        } else if (!I_stricmp(keyname, "suncolor")) {
            float *v = (float *)&((SunLightParseParams *)p)->sunColor[0];
            v[0] = 0;
            v[1] = 0;
            v[2] = 0;
            sscanf(value, "%f %f %f", &v[0], &v[1], &v[2]);
            ColorNormalize(v, v);
        } else if (!I_stricmp(keyname, "sundiffusecolor")) {
            float *v = (float *)&((SunLightParseParams *)p)->diffuseColor[0];
            v[0] = 0;
            v[1] = 0;
            v[2] = 0;
            sscanf(value, "%f %f %f", &v[0], &v[1], &v[2]);
            ColorNormalize(v, v);
            ((SunLightParseParams *)p)->diffuseColorHasBeenSet = 1;
        } else if (!I_stricmp(keyname, "sunlight")) {
            ((SunLightParseParams *)p)->sunLight = (float)atof(value);
        } else if (!I_stricmp(keyname, "sundirection")) {
            float *v = (float *)&((SunLightParseParams *)p)->angles[0];
            v[0] = 0;
            v[1] = 0;
            v[2] = 0;
            sscanf(value, "%f %f %f", &v[0], &v[1], &v[2]);
        } else if (!I_stricmp(keyname, "name")) {
            I_strncpyz((char *)params, value, 0x40);
        }
    }

    return text;
}

void R_InterpretSunLightParseParamsIntoLights(SunLightParseParams *sunParse, GfxLight *sunLight)
{
    byte *sp = (byte *)sunParse;
    byte *sl = (byte *)sunLight;
    vec_t sunDirection[3];
    float ambient, sunIntensity, sunAngleOverride;
    float *sunColor;
    float ambientR, ambientG, ambientB;
    float scale;
    float diffR, diffG, diffB;

    AngleVectors(((SunLightParseParams *)sp)->angles, sunDirection, NULL, NULL);

    ambient = ((SunLightParseParams *)sp)->ambientScale;
    sunIntensity = ((SunLightParseParams *)sp)->diffuseFraction;
    sunAngleOverride = ((SunLightParseParams *)sp)->sunLight;
    sunColor = ((SunLightParseParams *)sp)->ambientColor;

    if (ambient != 0.0f) {
        float normLen = ColorNormalize(sunColor, sunColor);
        if (normLen != 0.0f) {
            ambientR = ambient * sunColor[0];
            ambientG = ambient * sunColor[1];
            ambientB = ambient * sunColor[2];
        } else {
            ambientR = ambientG = ambientB = 0.0f;
        }
    } else {
        ambientR = ambientG = ambientB = 0.0f;
    }

    scale = (sunAngleOverride - ambient) * (1.0f - sunIntensity);
    diffR = scale * ((SunLightParseParams *)sp)->sunColor[0];
    diffG = scale * ((SunLightParseParams *)sp)->sunColor[1];
    diffB = scale * ((SunLightParseParams *)sp)->sunColor[2];

    {
        static GfxLight sunLightScratch;

        if ((unsigned int)sunLight < 0x1000 ||
            ((unsigned int)sunLight >= 0x08200000 && (unsigned int)sunLight < 0x08800000)) {
            sunLight = &sunLightScratch;
            sl = (byte *)sunLight;
        }
    }
    if (sunLight) {
        (*(float *)&((SunLightParseParams *)sl)->name[4]) = sunDirection[0];
        (*(float *)&((SunLightParseParams *)sl)->name[8]) = sunDirection[1];
        (*(float *)&((SunLightParseParams *)sl)->name[12]) = sunDirection[2];
        (*(float *)&((SunLightParseParams *)sl)->name[16]) = 0.0f;
        (*(float *)&((SunLightParseParams *)sl)->name[20]) = diffR;
        (*(float *)&((SunLightParseParams *)sl)->name[24]) = diffG;
        (*(float *)&((SunLightParseParams *)sl)->name[28]) = diffB;
        (*(float *)&((SunLightParseParams *)sl)->name[32]) = ambientR;
        (*(float *)&((SunLightParseParams *)sl)->name[36]) = ambientG;
        (*(float *)&((SunLightParseParams *)sl)->name[40]) = ambientB;
    }
}

static Bool R_IsValidStaticModel(char *spawnVars[64][2], int spawnVarCount, struct XModel **model, vec_t *origin)
{
    vec_t tempOrigin[3];
    const char *originStr;
    const char *modelName;
    struct XModel *tempModel;
    int hasOrigin;
    int i;

    originStr = "0 0 0";
    hasOrigin = 0;
    if (spawnVarCount - 1 > 0) {
        for (i = 1; i != spawnVarCount; i++) {
            if (stricmp(spawnVars[i][0], "origin") == 0) {
                const char *val = spawnVars[i][1];
                if (val != 0) {
                    originStr = val;
                    hasOrigin = 1;
                }
                break;
            }
        }
    }

    tempOrigin[0] = 0.0f;
    tempOrigin[1] = 0.0f;
    tempOrigin[2] = 0.0f;
    sscanf(originStr, "%f %f %f", &tempOrigin[0], &tempOrigin[1], &tempOrigin[2]);

    if (!hasOrigin)
        R_Error(1, "\x15R_LoadMiscModel: no origin specified\n");

    modelName = 0;
    if (spawnVarCount > 1) {
        for (i = 1; i != spawnVarCount; i++) {
            if (stricmp(spawnVars[i][0], "model") == 0) {
                modelName = spawnVars[i][1];
                break;
            }
        }
    }

    if (modelName == 0)
        R_Error(1, "\x15R_LoadMiscModel: no model specified in misc_model at (%.0f %.0f %.0f)\n",
                (double)tempOrigin[0], (double)tempOrigin[1], (double)tempOrigin[2]);

    if (strnicmp(modelName, "xmodel/shadow_", 14) == 0)
        return 0;

    tempModel = (struct XModel *)R_RegisterModel(modelName);
    if (tempModel != 0 && XModelBad(tempModel) == 0) {
        if (!R_ValidateStaticModel(tempModel))
            goto fallback;

    } else {
    fallback:
        Com_Printf("^1bad static model '%s' at (%.0f %.0f %.0f)\n",
                   modelName,
                   (double)tempOrigin[0], (double)tempOrigin[1], (double)tempOrigin[2]);
        tempModel = (struct XModel *)R_RegisterModel("xmodel/default_static_model");
    }

    if (tempModel == 0 || XModelBad(tempModel) != 0)
        return 0;

    if (model)
        *model = tempModel;
    if (origin) {
        origin[0] = tempOrigin[0];
        origin[1] = tempOrigin[1];
        origin[2] = tempOrigin[2];
    }
    return 1;
}

static void R_SetParentAndCell_r_impl(mnode_t *node, int parent)
{
    node->parent = parent;

    if (node->contents != -1)
        return;

    R_SetParentAndCell_r_impl(node->u.node.children[0], (int)(intptr_t)node);
    R_SetParentAndCell_r_impl(node->u.node.children[1], (int)(intptr_t)node);

    node->cellIndex = -2;
    if (node->u.node.children[0]->cellIndex == node->u.node.children[1]->cellIndex)
        node->cellIndex = node->u.node.children[0]->cellIndex;
}

#ifdef __EMSCRIPTEN__

static void R_SetParentAndCell_r(mnode_t *node, int parent)
{
    R_SetParentAndCell_r_impl(node, parent);
}
#else

static void __attribute_regparm__(2) R_SetParentAndCell_r(mnode_t *node, int parent)
{
    R_SetParentAndCell_r_impl(node, parent);
}

#endif

snd_alias_list_t R_LoadEntities_stub(void)
{
    snd_alias_list_t r = { 0 };
    return r;
}

extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_ClearTempMemory(void);
extern unsigned char XModelGetFlags(struct XModel *model);
extern void R_PrepareStaticModelLightingCache(GfxWorld *world, int smodelCount);
extern void R_CreateStaticModel(GfxWorld *world, struct XModel *model, const vec_t *origin, const vec_t *angles, vec_t scale, GfxStaticModelInstance *smodelInst);
extern void R_GetStaticModelLightingFromGrid(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float *sunVisibility, vec4_t *colorForDir);
extern void R_GetStaticModelLightingFromGround(const vec_t *groundLight, float *sunVisibility, vec4_t *colorForDir);
extern void R_ScaleStaticModelLighting(float directLightScale, float indirectLightScale, float *sunVisibility, vec4_t *colorForDir);
extern void R_CacheStaticModelLighting(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float sunVisibility, vec4_t *colorForDir);
extern void R_FinishStaticModelLightingCache(GfxWorld *world);
extern int R_AllocStaticModels(GfxAabbTree *tree);
extern int R_SortGfxAabbTree(GfxWorld *world, GfxAabbTree *tree);

void __attribute_regparm__(1) R_LoadEntities(GfxBspLoad *load)
{
    const dheader_t *header = load->header;
    const char *startPos = (const char *)load->fileBase + header->lumps[37].fileofs;
    char *textPool = (char *)Hunk_AllocateTempMemoryInternal(header->lumps[37].filelen);
    const char *text;
    const char *token;
    char *spawnVars[64][2];
    int spawnVarCount;
    int smodelCount;
    int textLen;
    int i;

    text = startPos;
    smodelCount = 0;

    token = Com_Parse(&text);
    if (!text || *token != '{')
        goto allocate;

    do {
        spawnVars[0][0] = "";
        spawnVarCount = 1;
        textLen = 0;

        for (;;) {
            int slot;
            int n;

            token = Com_Parse(&text);
            if (*token == '\0' || *token == '}')
                break;

            if (__builtin_memcmp(token, "classname", 10) != 0) {
                if (spawnVarCount == 64)
                    R_Error(1, "\x15R_LoadEntities: MAX_SPAWN_VARS (%i) reached\n", 64);
                slot = spawnVarCount;
                spawnVarCount++;
            } else {
                slot = 0;
            }

            spawnVars[slot][0] = &textPool[textLen];
            n = strlen(token) + 1;
            memcpy(&textPool[textLen], token, n);
            textLen += n;

            token = Com_Parse(&text);
            spawnVars[slot][1] = &textPool[textLen];
            n = strlen(token) + 1;
            memcpy(&textPool[textLen], token, n);
            textLen += n;
        }

        if (spawnVars[0][0][0] == '\0')
            R_Error(1, "\x15R_LoadEntities: entity without a classname\n");

        if (stricmp(spawnVars[0][1], "misc_model") == 0 &&
            R_IsValidStaticModel(spawnVars, spawnVarCount, 0, 0))
            smodelCount++;

        token = Com_Parse(&text);
    } while (text && *token == '{');

allocate:
    s_world.smodelInsts =
        (GfxStaticModelInstance *)Hunk_AllocInternal(smodelCount * (int)sizeof(GfxStaticModelInstance));
    s_world.smodelCount = 0;
    R_PrepareStaticModelLightingCache(&s_world, smodelCount);

    text = startPos;
    token = Com_Parse(&text);
    if (*token != '{')
        goto finish;

    do {
        GfxStaticModelInstance *smodelInst;
        struct XModel *model;
        vec_t origin[3];
        vec_t angles[3];
        vec_t scale[3];
        vec4_t groundLight;
        float sunVisibility;
        vec4_t colorForDir[6];
        unsigned char modelGroundLit;

        spawnVars[0][0] = "";
        spawnVarCount = 1;
        textLen = 0;

        for (;;) {
            int slot;
            int n;

            token = Com_Parse(&text);
            if (*token == '\0' || *token == '}')
                break;

            if (__builtin_memcmp(token, "classname", 10) != 0) {
                if (spawnVarCount == 64)
                    R_Error(1, "\x15R_LoadEntities: MAX_SPAWN_VARS (%i) reached\n", 64);
                slot = spawnVarCount;
                spawnVarCount++;
            } else {
                slot = 0;
            }

            spawnVars[slot][0] = &textPool[textLen];
            n = strlen(token) + 1;
            memcpy(&textPool[textLen], token, n);
            textLen += n;

            token = Com_Parse(&text);
            spawnVars[slot][1] = &textPool[textLen];
            n = strlen(token) + 1;
            memcpy(&textPool[textLen], token, n);
            textLen += n;
        }

        if (spawnVars[0][0][0] == '\0')
            R_Error(1, "\x15R_LoadEntities: entity without a classname\n");

        if (stricmp(spawnVars[0][1], "misc_model") != 0) {
            token = Com_Parse(&text);
            continue;
        }
        if (!R_IsValidStaticModel(spawnVars, spawnVarCount, &model, origin)) {
            token = Com_Parse(&text);
            continue;
        }

        smodelInst = &s_world.smodelInsts[s_world.smodelCount];
        s_world.smodelCount += 1;

        {
            const char *anglesStr = "0 0 0";
            int gotAngle = 0;

            if (spawnVarCount > 1) {
                for (i = 1; i < spawnVarCount; i++) {
                    if (stricmp(spawnVars[i][0], "angle") == 0) {
                        const char *v = spawnVars[i][1];
                        if (v != 0) {
                            float yaw = (float)atof(v);
                            if (yaw != 0.0f) {
                                angles[0] = 0.0f;
                                angles[1] = yaw;
                                angles[2] = 0.0f;
                                gotAngle = 1;
                            }
                        }
                        break;
                    }
                }
            }

            if (!gotAngle) {
                if (spawnVarCount > 1) {
                    for (i = 1; i < spawnVarCount; i++) {
                        if (stricmp(spawnVars[i][0], "angles") == 0) {
                            if (spawnVars[i][1] != 0)
                                anglesStr = spawnVars[i][1];
                            break;
                        }
                    }
                }
                angles[0] = 0.0f;
                angles[1] = 0.0f;
                angles[2] = 0.0f;
                sscanf(anglesStr, "%f %f %f", &angles[0], &angles[1], &angles[2]);
            }
        }

        {
            int gotModelScaleVec = 0;

            if (spawnVarCount > 1) {
                for (i = 1; i < spawnVarCount; i++) {
                    if (stricmp(spawnVars[i][0], "modelscale_vec") == 0) {
                        const char *v = spawnVars[i][1];
                        if (v != 0) {
                            scale[0] = 0.0f;
                            scale[1] = 0.0f;
                            scale[2] = 0.0f;
                            sscanf(v, "%f %f %f", &scale[0], &scale[1], &scale[2]);
                            gotModelScaleVec = 1;
                        }
                        break;
                    }
                }
            }

            if (!gotModelScaleVec) {
                float s = 1.0f;
                scale[0] = 0.0f;
                scale[1] = 0.0f;
                scale[2] = 0.0f;
                sscanf("1 1 1", "%f %f %f", &scale[0], &scale[1], &scale[2]);
                if (spawnVarCount > 1) {
                    for (i = 1; i < spawnVarCount; i++) {
                        if (stricmp(spawnVars[i][0], "modelscale") == 0) {
                            if (spawnVars[i][1] != 0)
                                s = (float)atof(spawnVars[i][1]);
                            break;
                        }
                    }
                }
                scale[0] = s;
                scale[1] = s;
                scale[2] = s;
            }
        }

        R_CreateStaticModel(&s_world, model, origin, angles, scale[2], smodelInst);
        modelGroundLit = (unsigned char)(XModelGetFlags(model) & 1);

        {
            unsigned int valueInt[4];
            const char *hex = "FF000000";
            int gndLtSeen = 0;

            if (spawnVarCount > 1) {
                for (i = 1; i < spawnVarCount; i++) {
                    if (stricmp(spawnVars[i][0], "gndLt") == 0) {
                        if (spawnVars[i][1] != 0) {
                            hex = spawnVars[i][1];
                            gndLtSeen = 1;
                        }
                        break;
                    }
                }
            }

            if (sscanf(hex, "%02x%02x%02x%02x",
                       &valueInt[0], &valueInt[1], &valueInt[2], &valueInt[3]) != 4)
                R_Error(1, "\x15R_Vec4ForKeyHex: invalid value\n");

            for (i = 1; i < 5; i++) {
                int iv = (int)valueInt[i - 1];
                float fv;
                if (iv < 0)
                    fv = (float)(int)(((unsigned int)iv >> 1) | (iv & 1)) * 2.0f;
                else
                    fv = (float)iv;
                groundLight[i - 1] = fv / 255.0f;
            }

            if (r_showGroundLit->current.enabled) {

                if (modelGroundLit) {
                    if (gndLtSeen) {
                        groundLight[0] = 0.0f;
                        groundLight[1] = 1.0f;
                    } else {
                        groundLight[0] = 1.0f;
                        groundLight[1] = 1.0f;
                    }
                } else {
                    groundLight[0] = 1.0f;
                    groundLight[1] = 0.0f;
                }
                groundLight[2] = 0.0f;
                groundLight[3] = 0.0f;
                R_GetStaticModelLightingFromGround(groundLight, &sunVisibility, colorForDir);
                R_CacheStaticModelLighting(&s_world, smodelInst, sunVisibility, colorForDir);
            } else {
                if (modelGroundLit && gndLtSeen) {
                    R_GetStaticModelLightingFromGround(groundLight, &sunVisibility, colorForDir);
                } else {
                    R_GetStaticModelLightingFromGrid(&s_world, smodelInst, &sunVisibility, colorForDir);
                }

                {
                    float directLightScale = 1.0f;
                    float indirectLightScale;

                    if (spawnVarCount > 1) {
                        for (i = 1; i < spawnVarCount; i++) {
                            if (stricmp(spawnVars[i][0], "directLightScale") == 0) {
                                if (spawnVars[i][1] != 0) {
                                    directLightScale = (float)atof(spawnVars[i][1]);
                                    if (directLightScale < 0.0f)
                                        R_Error(1, "\x15'directLightScale' may not be negative.");
                                }
                                break;
                            }
                        }
                    }

                    indirectLightScale = 1.0f;
                    if (spawnVarCount > 1) {
                        for (i = 1; i < spawnVarCount; i++) {
                            if (stricmp(spawnVars[i][0], "indirectLightScale") == 0) {
                                if (spawnVars[i][1] != 0) {
                                    indirectLightScale = (float)atof(spawnVars[i][1]);
                                    if (indirectLightScale < 0.0f)
                                        R_Error(1, "\x15'indirectLightScale' may not be negative.");
                                }
                                break;
                            }
                        }
                    }

                    R_ScaleStaticModelLighting(directLightScale, indirectLightScale,
                                               &sunVisibility, colorForDir);
                    R_CacheStaticModelLighting(&s_world, smodelInst, sunVisibility, colorForDir);
                }
            }
        }

        token = Com_Parse(&text);
    } while (*token == '{');

finish:
    R_FinishStaticModelLightingCache(&s_world);

    if (s_world.cellCount > 0) {
        for (i = 0; i < s_world.cellCount; i++)
            R_AllocStaticModels(s_world.cells[i].aabbTree);
        for (i = 0; i < s_world.cellCount; i++)
            R_SortGfxAabbTree(&s_world, s_world.cells[i].aabbTree);
    }

    Hunk_ClearTempMemory();
}

#ifndef __EMSCRIPTEN__

static void R_LoadNodesAndLeafs_impl(const GfxBspLoad *load)
{
    const byte *inNode;
    const byte *inLeaf;
    int i;

    int nodeCount = R_ValidateLumpInline(load, 0xD0, 36, &inNode);
    int leafCount = R_ValidateLumpInline(load, 0xD8, 36, &inLeaf);

    int totalCount = nodeCount + leafCount;
    s_world.nodeCount = totalCount;
    mnode_t *nodes = (mnode_t *)Hunk_AllocInternal(totalCount * sizeof(mnode_t));
    s_world.nodes = nodes;

    for (i = 0; i < nodeCount; i++) {
        const byte *src = inNode + i * 36;
        mnode_t *node = &nodes[i];

        node->contents = -1;
        node->u.node.plane = (cplane_t *)CM_GetPlaneNum(*(int *)src);

        int j;
        for (j = 0; j < 2; j++) {
            int childIdx = *(int *)(src + 4 + j * 4);
            if (childIdx >= 0)
                node->u.node.children[j] = &nodes[childIdx];
            else
                node->u.node.children[j] = &nodes[nodeCount + (-childIdx) - 1];
        }
    }

    for (i = 0; i < leafCount; i++) {
        const byte *src = inLeaf + i * 36;
        mnode_t *leaf = &nodes[nodeCount + i];

        leaf->cellIndex = (*(const int *)&(((const dcell_ondisk_t *)src)->aabbTreeIndex));
        leaf->u.leaf.cluster = *(int *)src;
    }

    {
        mnode_t *root = &nodes[0];
        root->parent = 0;
        if (root->contents == -1) {
            R_SetParentAndCell_r(root->u.node.children[0], (int)(intptr_t)root);
            R_SetParentAndCell_r(root->u.node.children[1], (int)(intptr_t)root);
            root->cellIndex = -2;
            if (root->u.node.children[0]->cellIndex == root->u.node.children[1]->cellIndex)
                root->cellIndex = root->u.node.children[0]->cellIndex;
        }
    }
}

void __attribute_regparm__(1) R_LoadNodesAndLeafs(GfxBspLoad *load)
{
    R_LoadNodesAndLeafs_impl(load);
}

static void R_LoadPortals_impl(const GfxBspLoad *load)
{
    const byte *srcData;
    int portalCount = R_ValidateLump(load, 0xc8, 16, &srcData);
    byte *dst;
    int i;

    dst = (byte *)Hunk_AllocInternal(portalCount * sizeof(GfxPortal));

    for (i = 0; i < portalCount; i++) {
        const int *src = (const int *)(srcData + i * 16);
        byte *d = dst + i * sizeof(GfxPortal);
        const float *plane;

        plane = (const float *)CM_GetPlaneNum(src[0]);

        ((GfxPortal *)d)->plane.coeffs[0] = plane[0];
        ((GfxPortal *)d)->plane.coeffs[1] = plane[1];
        ((GfxPortal *)d)->plane.coeffs[2] = plane[2];

        (*(int *)&((GfxPortal *)d)->plane.coeffs[3]) = *(int *)&plane[3] ^ 0x80000000;

        ((GfxPortal *)d)->plane.side[0] = (plane[0] > 0.0f) ? 0x0c : 0x00;
        ((GfxPortal *)d)->plane.side[1] = (plane[1] > 0.0f) ? 0x10 : 0x04;
        ((GfxPortal *)d)->plane.side[2] = (plane[2] > 0.0f) ? 0x14 : 0x08;

        {
            int cellIdx = src[1];
            ((GfxPortal *)d)->cell = &s_world.cells[cellIdx];
        }

        {
            int vertIdx = src[2];
            (*(void **)&((GfxPortal *)d)->vertices) = (byte *)rgl.portalVerts + vertIdx * 12;
        }

        ((GfxPortal *)d)->vertexCount = (byte)src[3];
        ((GfxPortal *)d)->hullPointCount = 0;
        ((GfxPortal *)d)->hullPoints = NULL;

        PerpendicularVector(plane, (vec_t *)&((GfxPortal *)d)->hullAxis[0][0]);
        Vec3Cross(plane, (vec_t *)&((GfxPortal *)d)->hullAxis[0][0], (vec_t *)&((GfxPortal *)d)->hullAxis[1][0]);
    }

    {
        int cellCount = s_world.cellCount;
        GfxCell *cells = s_world.cells;
        for (i = 0; i < cellCount; i++) {
            GfxCell *cell = &cells[i];
            int portalCountInCell = cell->portalCount;
            if (portalCountInCell != 0) {

                cell->portals = (GfxPortal *)(dst + (intptr_t)cell->portals);
            } else {
                cell->portals = NULL;
            }
        }
    }
}

void __attribute_regparm__(1) R_LoadPortals(GfxBspLoad *load)
{
    R_LoadPortals_impl(load);
}

static void R_LoadCells_impl(const GfxBspLoad *load)
{
    const byte *srcData;
    int cellCount = R_ValidateLumpInline(load, 0xc0, 52, &srcData);
    GfxCell *dst;
    int i;

    dst = (GfxCell *)Hunk_AllocInternal(cellCount * sizeof(GfxCell));
    s_world.cells = dst;
    s_world.cellCount = cellCount;

    for (i = 0; i < cellCount; i++) {
        const byte *src = srcData + i * 52;
        const dcell_ondisk_t *disk = (const dcell_ondisk_t *)src;
        GfxCell *d = &dst[i];
        int aabbTreeIdx, portalCountAndOfs;
        int occluderCount, reflectionProbeCount;

        d->mins[0] = disk->mins[0];
        d->mins[1] = disk->mins[1];
        d->mins[2] = disk->mins[2];

        d->maxs[0] = disk->maxs[0];
        d->maxs[1] = disk->maxs[1];
        d->maxs[2] = disk->maxs[2];

        aabbTreeIdx = disk->aabbTreeIndex;
        d->aabbTree = &rgl.aabbTrees[aabbTreeIdx];

        portalCountAndOfs = disk->firstPortal;
        d->portals = (GfxPortal *)(intptr_t)(portalCountAndOfs * sizeof(GfxPortal));

        d->portalCount = disk->portalCount;

        occluderCount = disk->occluderCount;
        if (occluderCount != 0) {
            int occluderOfs = disk->firstOccluder;
            d->cullGroups = rgl.cullGroupIndices + occluderOfs;
        } else {
            d->cullGroups = NULL;
        }
        d->cullGroupCount = occluderCount;

        reflectionProbeCount = disk->reflectionProbeCount;
        if (reflectionProbeCount != 0) {
            int reflOfs = disk->firstReflectionProbe;
            d->occluders = rgl.occluderIndices + reflOfs;
        } else {
            d->occluders = NULL;
        }
        d->occluderCount = reflectionProbeCount;
    }
}

void __attribute_regparm__(1) R_LoadCells(GfxBspLoad *load)
{
    R_LoadCells_impl(load);
}

static void R_LoadAabbTrees_impl(const GfxBspLoad *load)
{
    const byte *srcData;
    int count = R_ValidateLumpInline(load, 0xb8, 12, &srcData);
    GfxAabbTree *dst;
    int i;

    dst = (GfxAabbTree *)Hunk_AllocInternal(count * sizeof(GfxAabbTree));
    rgl.aabbTrees = dst;
    rgl.aabbTreeCount = count;

    {
        const int *src = (const int *)srcData;
        GfxAabbTree *d = dst;
        for (i = 0; i < count; i++) {
            int childCount = src[1];

            if (childCount == 0) {
                d->startSurfIndex = -1;
                d->surfaceCount = childCount;
                d->childCount = src[2];
            } else {
                d->startSurfIndex = src[0];
                d->surfaceCount = childCount;
                d->childCount = src[2];
            }
            src += 3;
            d++;
        }
    }

    {
        for (i = 0; i < count;) {
            i = R_FinishLoadingAabbTrees_r(&rgl.aabbTrees[i], i + 1);
        }
    }
}

void __attribute_regparm__(1) R_LoadAabbTrees(GfxBspLoad *load)
{
    R_LoadAabbTrees_impl(load);
}

static void R_LoadOccluders_impl(const GfxBspLoad *load)
{
    const byte *bspHeader = (const byte *)load->header;
    const byte *bspData = load->fileBase;
    int fileSize = load->fileSize;
    int i, j;

    int occLumpSize = *(int *)(bspHeader + 0x98);
    if (*(int *)(bspHeader + 0x9C) + occLumpSize > fileSize)
        R_Error(1, "LoadMap: lump extends past end of file in %s", s_world.name);
    if (*(int *)(bspHeader + 0x9C) <= 3)
        R_Error(1, "LoadMap: funny lump offset in %s", s_world.name);
    int occluderCount = occLumpSize / 20;
    if (occLumpSize < 0 || occluderCount * 20 != occLumpSize)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);
    const byte *diskOcc = bspData + *(int *)(bspHeader + 0x9C);
    byte *occluders = (byte *)Hunk_AllocInternal(occluderCount * 36);

    int planeLumpSize = *(int *)(bspHeader + 0xA0);
    if (*(int *)(bspHeader + 0xA4) + planeLumpSize > fileSize)
        R_Error(1, "LoadMap: lump extends past end of file in %s", s_world.name);
    if (*(int *)(bspHeader + 0xA4) <= 3)
        R_Error(1, "LoadMap: funny lump offset in %s", s_world.name);
    int planeIdxCount = planeLumpSize / 4;
    if (planeLumpSize < 0 || planeIdxCount * 4 != planeLumpSize)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);
    const byte *planeIndices = bspData + *(int *)(bspHeader + 0xA4);
    byte *sidePlanes = (byte *)Hunk_AllocInternal(planeIdxCount * 20);

    int edgeLumpSize = *(int *)(bspHeader + 0xA8);
    if (*(int *)(bspHeader + 0xAC) + edgeLumpSize > fileSize)
        R_Error(1, "LoadMap: lump extends past end of file in %s", s_world.name);
    if (*(int *)(bspHeader + 0xAC) <= 3)
        R_Error(1, "LoadMap: funny lump offset in %s", s_world.name);
    int edgeCount = edgeLumpSize / 4;
    if (edgeLumpSize < 0 || edgeCount * 4 != edgeLumpSize)
        R_Error(1, "LoadMap: funny lump size in %s", s_world.name);
    const byte *diskEdges = bspData + *(int *)(bspHeader + 0xAC);
    byte *edges = (byte *)Hunk_AllocInternal(edgeCount * 16);

    rgl.occluders = (GfxOccluder *)occluders;
    byte *vertBase = (byte *)rgl.portalVerts;

    byte *out = (byte *)&((GfxOccluder *)occluders)->edges;
    for (i = 0; i < occluderCount; i++) {
        int nPlanes = *(short *)(diskOcc + 4);
        *(int *)(out - 0xc) = nPlanes;

        int firstPlane = *(int *)diskOcc;
        byte *planeBase = sidePlanes + firstPlane * 20;
        *(byte **)(out - 8) = planeBase;

        if (nPlanes > 0) {
            const int *plIdx = (const int *)(planeIndices + firstPlane * 4);
            for (j = 0; j < nPlanes; j++) {
                float *src = (float *)CM_GetPlaneNum(plIdx[j]);
                byte *dst = planeBase + j * 20;
                *(float *)(dst + 0) = src[0];
                *(float *)(dst + 4) = src[1];
                *(float *)(dst + 8) = src[2];
                *(int *)(dst + 12) = *(int *)(src + 3) ^ 0x80000000;
                dst[16] = (*(int *)(dst + 0) > 0) ? 0x0C : 0x00;
                dst[17] = (*(int *)(dst + 4) > 0) ? 0x10 : 0x04;
                dst[18] = (*(int *)(dst + 8) > 0) ? 0x14 : 0x08;
            }
        }

        *(byte **)(out + 8) = vertBase + *(int *)(diskOcc + 0xc) * 12;
        *(int *)(out + 4) = *(short *)(diskOcc + 0x10);

        int nEdges = *(short *)(diskOcc + 6);
        *(int *)(out - 4) = nEdges;

        int firstEdge = *(int *)(diskOcc + 8);
        byte *edgeBase = edges + firstEdge * 16;
        *(byte **)(out + 0) = edgeBase;

        const byte *de = diskEdges + firstEdge * 4;
        for (j = 0; j < nEdges; j++) {
            byte *e = edgeBase + j * 16;
            *(byte **)(e + 0) = planeBase + de[j * 4 + 0] * 20;
            *(byte **)(e + 4) = planeBase + de[j * 4 + 1] * 20;
            byte *cv = *(byte **)(out + 8);
            *(byte **)(e + 8) = cv + de[j * 4 + 2] * 12;
            *(byte **)(e + 12) = cv + de[j * 4 + 3] * 12;
        }

        *(int *)(out + 16) = 0;
        *(int *)(out + 20) = 0;

        diskOcc += 20;
        out += 36;
    }
}

void __attribute_regparm__(1) R_LoadOccluders(GfxBspLoad *load)
{
    R_LoadOccluders_impl(load);
}

static void R_LoadPortalVerts_impl(const GfxBspLoad *load)
{
    const byte *srcData;
    int vertCount = R_ValidateLumpInline(load, 0x90, 12, &srcData);
    float *dst;
    int i;

    dst = (float *)Hunk_AllocInternal(vertCount * 12);
    rgl.portalVerts = (vec3_t *)dst;

    for (i = 0; i < vertCount; i++) {
        const float *src = (const float *)(srcData + i * 12);
        dst[i * 3 + 0] = src[0];
        dst[i * 3 + 1] = src[1];
        dst[i * 3 + 2] = src[2];
    }
}

void __attribute_regparm__(1) R_LoadPortalVerts(GfxBspLoad *load)
{
    R_LoadPortalVerts_impl(load);
}

static void R_LoadCullGroups_impl(const GfxBspLoad *load)
{
    const byte *srcData;
    int count = R_ValidateLumpInline(load, 0x58, 32, &srcData);
    byte *dst;
    int i;

    dst = (byte *)Hunk_AllocInternal(count * 32);
    s_world.cullGroups = (GfxCullGroup *)dst;
    s_world.cullGroupCount = count;

    {
        const byte *src = srcData;
        byte *d = dst;
        for (i = 0; i < count; i++) {
            byte *dm = d;
            int j;

            for (j = 0; j < 3; j++) {
                *(float *)(dm) = *(const float *)(src + j * 4);
                *(float *)(dm + 12) = *(const float *)(src + j * 4 + 12);
                dm += 4;
            }

            {
                int surfCount = ((const daabbnode_ondisk_t *)src)->surfaceCount;
                if (surfCount == 0) {
                    ((GfxAabbTree *)d)->startSurfIndex = -1;
                } else {
                    ((GfxAabbTree *)d)->startSurfIndex = ((const daabbnode_ondisk_t *)src)->startSurfIndex;
                }
                ((GfxAabbTree *)d)->surfaceCount = surfCount;
            }

            src += 32;
            d += 32;
        }
    }
}

void __attribute_regparm__(1) R_LoadCullGroups(GfxBspLoad *load)
{
    R_LoadCullGroups_impl(load);
}

#endif

static int R_LoadSurfacesLump(const GfxBspLoad *load, int lumpOfs, int elemSize, const byte **outData)
{
    const byte *header = (const byte *)load->header;
    int lumpSize = *(const int *)(header + lumpOfs);
    int lumpFileOfs = *(const int *)(header + lumpOfs + 4);
    int count;

    if (lumpFileOfs + lumpSize > load->fileSize)
        R_Error(1, "\x15LoadMap: lump extends past end of file in %s", s_world.name);

    if (lumpFileOfs <= 3) {
        if (lumpSize == 0) {
            if (outData)
                *outData = NULL;
            return 0;
        }
        R_Error(1, "\x15LoadMap: funny lump offset in %s", s_world.name);
    }

    count = lumpSize / elemSize;
    if (lumpSize < 0)
        count = (lumpSize + elemSize - 1) / elemSize;
    if (count * elemSize != lumpSize)
        R_Error(1, "\x15LoadMap: funny lump size in %s", s_world.name);

    if (outData)
        *outData = load->fileBase + lumpFileOfs;
    return count;
}

void __attribute_regparm__(1) R_LoadSurfaces(GfxBspLoad *load)
{
    const DiskTriangleSoup *diskSurfaces;
    const DiskGfxVertex *diskVerts;
    const r_index_t *diskIndices;
    GfxWorldVertex *verts;
    GfxSurface *surfaces;
    srfTriangles_t *tris;
    byte *indexMem;
    int surfCount;
    int vertCount;
    int indexCount;
    int totalIndexBytes;
    int skyCount;
    const Material *skyMaterial;
    int i;

    surfCount = R_LoadSurfacesLump(load, 0x40, sizeof(DiskTriangleSoup), (const byte **)&diskSurfaces);
    if (!surfCount)
        R_Error(1, "\x15LoadMap: no surfaces in %s", s_world.name);

    vertCount = R_LoadSurfacesLump(load, 0x48, sizeof(DiskGfxVertex), (const byte **)&diskVerts);
    if (!vertCount)
        R_Error(1, "\x15LoadMap: no vertices in %s", s_world.name);

    indexCount = R_LoadSurfacesLump(load, 0x50, sizeof(r_index_t), (const byte **)&diskIndices);

    s_world.vertexCount = vertCount;
    verts = (GfxWorldVertex *)Hunk_AllocInternal(vertCount * sizeof(GfxWorldVertex));
    s_world.vd.vertices = verts;

    {
        void *colorConverter = CColorConverter_GetColorConverter(0);
        for (i = 0; i < vertCount; i++) {
            unsigned int packedColor;
            void **ccvt;

            verts[i].xyz[0] = diskVerts[i].xyz[0];
            verts[i].xyz[1] = diskVerts[i].xyz[1];
            verts[i].xyz[2] = diskVerts[i].xyz[2];
            verts[i].normal[0] = diskVerts[i].normal[0];
            verts[i].normal[1] = diskVerts[i].normal[1];
            verts[i].normal[2] = diskVerts[i].normal[2];
            packedColor = ((unsigned int)diskVerts[i].color[0] << 24) | ((unsigned int)diskVerts[i].color[1] << 16) | ((unsigned int)diskVerts[i].color[2] << 8) | (unsigned int)diskVerts[i].color[3];
            ccvt = *(void ***)colorConverter;
            ((void (*)(void *, void *, void *))ccvt[0])(colorConverter,
                                                        &verts[i].color, &packedColor);
            verts[i].texCoord[0] = diskVerts[i].texCoord[0];
            verts[i].texCoord[1] = diskVerts[i].texCoord[1];
            verts[i].lmapCoord[0] = diskVerts[i].lmapCoord[0];
            verts[i].lmapCoord[1] = diskVerts[i].lmapCoord[1];
            verts[i].tangent[0] = diskVerts[i].tangent[0];
            verts[i].tangent[1] = diskVerts[i].tangent[1];
            verts[i].tangent[2] = diskVerts[i].tangent[2];
            verts[i].binormal[0] = diskVerts[i].binormal[0];
            verts[i].binormal[1] = diskVerts[i].binormal[1];
            verts[i].binormal[2] = diskVerts[i].binormal[2];
        }
    }

    totalIndexBytes = 0;
    for (i = 0; i < surfCount; i++) {
        int bytes = (int)diskSurfaces[i].indexCount * (int)sizeof(r_index_t);
        totalIndexBytes += (bytes + 31) & ~31;
    }

    indexMem = (byte *)Hunk_AllocInternal(totalIndexBytes);
    tris = (srfTriangles_t *)Hunk_AllocInternal(surfCount * sizeof(srfTriangles_t));
    surfaces = (GfxSurface *)Hunk_AllocInternal(surfCount * sizeof(GfxSurface));

    s_world.surfaceCount = surfCount;
    s_world.surfaces = surfaces;
    s_world.skySurfCount = 0;
    s_world.skyStartSurfs = NULL;
    s_world.skyImage = NULL;
    s_world.skySamplerState = 0;

    ClearBounds(s_world.mins, s_world.maxs);

    skyCount = 0;
    skyMaterial = NULL;

    for (i = 0; i < surfCount; i++) {
        const DiskTriangleSoup *src = &diskSurfaces[i];
        srfTriangles_t *tri = &tris[i];
        GfxSurface *surf = &surfaces[i];
        int materialIndex = (short)src->materialIndex;
        int lightmapIndex = (short)src->lightmapIndex;
        const r_lightmapMerge_t *merge;
        const Material *material;
        unsigned short sortGroup;
        int bytes;
        int j;

        if (materialIndex < 0 || materialIndex >= load->materialCount)
            R_Error(1, "\x15LoadMap: material index out of range in %s", s_world.name);
        if (lightmapIndex < 0 || lightmapIndex > 31)
            R_Error(1, "\x15LoadMap: lightmap index out of range in %s", s_world.name);
        if (src->firstVertex < 0 || src->firstVertex + src->vertexCount > vertCount)
            R_Error(1, "\x15LoadMap: surface vertices out of range in %s", s_world.name);
        if (src->firstIndex < 0 || src->firstIndex + src->indexCount > indexCount)
            R_Error(1, "\x15LoadMap: surface indices out of range in %s", s_world.name);

        material = Material_Register(load->materials[materialIndex].material, 9);
        merge = &load->lmapMergeInfo[lightmapIndex];
        sortGroup = 0;

        tri->surfaceType = SF_TRIANGLES;
        tri->firstVertex = src->firstVertex;
        tri->vertexCount = src->vertexCount;
        tri->indexCount = src->indexCount;
        tri->indices = (int (*)[4])indexMem;

        bytes = (int)src->indexCount * (int)sizeof(r_index_t);
        Com_Memcpy(indexMem, &diskIndices[src->firstIndex], bytes);

        ClearBounds(tri->bounds[0], tri->bounds[1]);
        for (j = 0; j < src->indexCount; j++) {
            int vertIndex = src->firstVertex + ((const r_index_t *)indexMem)[j];
            GfxWorldVertex *v = &verts[vertIndex];

            v->lmapCoord[0] = diskVerts[vertIndex].lmapCoord[0] * merge->scale[0] + merge->shift[0];
            v->lmapCoord[1] = diskVerts[vertIndex].lmapCoord[1] * merge->scale[1] + merge->shift[1];
            AddPointToBounds(v->xyz, tri->bounds[0], tri->bounds[1]);
            AddPointToBounds(v->xyz, s_world.mins, s_world.maxs);
        }

        surf->material = material;
        surf->lightmapIndex = (short unsigned int)merge->index;
        for (j = 0; j < i; j++) {
            const srfTriangles_t *prevTris;

            if (surfaces[j].material != material)
                continue;

            prevTris = surfaces[j].tris;
            if (prevTris->firstVertex == tri->firstVertex && prevTris->vertexCount == tri->vertexCount) {
                sortGroup = surfaces[j].sortGroup;
                break;
            }

            if (sortGroup <= surfaces[j].sortGroup)
                sortGroup = surfaces[j].sortGroup + 1;
        }
        if (sortGroup > 0x7ff)
            R_Error(1, "Material %s has more than %i sort groups\n", material->info.name, 0x800);
        surf->sortGroup = sortGroup;
        surf->tris = tri;

        if (material && (material->info.surfaceFlags & 8)) {
            if (skyMaterial && skyMaterial != material)
                R_Error(1, "map has at least two different skies: %s and %s\nOnly one sky per map is supported\n",
                        skyMaterial->info.name, material->info.name);
            skyMaterial = material;
            skyCount++;
        }

        indexMem += (bytes + 31) & ~31;
    }

    if (skyCount > 0 && skyMaterial) {
        int out = 0;
        s_world.skyStartSurfs = (int *)Hunk_AllocInternal(skyCount * sizeof(int));
        for (i = 0; i < surfCount; i++) {
            if (surfaces[i].material == skyMaterial)
                s_world.skyStartSurfs[out++] = i;
        }
        s_world.skySurfCount = out;

        for (i = 0; i < skyMaterial->textureCount; i++) {
            MaterialTextureDef *tex = &skyMaterial->textures[i];
            if (stricmp(tex->name, "colorMap") == 0) {
                if (tex->semantic == 5)
                    R_Error(1, "colorMap '%s' for sky material '%s' is not a cubemap\n",
                            tex->u.image->name, skyMaterial->info.name);
                else if (tex->u.image->mapType != 5)
                    R_Error(1, "colorMap '%s' for sky material '%s' is not a cubemap\n",
                            tex->u.image->name, skyMaterial->info.name);
                s_world.skyImage = tex->u.image;
                s_world.skySamplerState = tex->samplerState;
                break;
            }
        }
    }

    s_world.vd.worldVb = R_CreateWorldVertexBuffer(s_world.vd.vertices, s_world.vertexCount);
}

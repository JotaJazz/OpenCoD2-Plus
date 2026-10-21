#include "common_types.h"
#include <math.h>
#include <stdlib.h>
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern refimport_t ri;
#include "bytematch.h"
#include <float.h>


extern unsigned char smodelLoadGlob[128];

extern void *Hunk_AllocAlignInternal(int size, int alignment);
extern GfxImage *Image_Alloc(const char *name, int category, int semantic, int imageTrack);
extern void Image_Generate3D(GfxImage *image, byte *pixels, int width, int height, int depth, D3DFORMAT imageFormat);

static int CompareStaticModels(const int *smodel0, const int *smodel1);
void R_ScaleStaticModelLighting(float directLightScale, float indirectLightScale, float *sunVisibility, vec4_t *colorForDir);
void R_GetStaticModelLightingFromGrid(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float *sunVisibility, vec4_t *colorForDir);
void R_PrepareStaticModelLightingCache(GfxWorld *world, int smodelCount);
Bool R_ValidateStaticModel(struct XModel *model);
static int R_AddStaticModelToAabbTree_r_impl(byte *world, byte *tree, int smodelIndex);
static int COD2_REGPARM(3) R_AddStaticModelToAabbTree_r(GfxWorld *world, GfxAabbTree *tree, int smodelIndex);
static int COD2_REGPARM(3) R_FilterStaticModelIntoCells_r(GfxWorld *world, mnode_t *node, GfxStaticModelInstance *smodelInst, const vec_t *mins, const vec_t *maxs);
void R_FinishStaticModelLightingCache(GfxWorld *world);
void R_GetStaticModelLightingFromGround(const vec_t *groundLight, float *sunVisibility, vec4_t *colorForDir);
void R_CreateStaticModel(GfxWorld *world, struct XModel *model, const vec_t *origin, const vec_t *angles, vec_t scale, GfxStaticModelInstance *smodelInst);
void R_CacheStaticModelLighting(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float sunVisibility, vec4_t *colorForDir);
int R_SortGfxAabbTree(GfxWorld *world, GfxAabbTree *tree);
int R_AllocStaticModels(GfxAabbTree *tree);

static int CompareStaticModels(const int *smodel0, const int *smodel1)
{
    return *smodel0 - *smodel1;
}

void R_ScaleStaticModelLighting(float directLightScale, float indirectLightScale, float *sunVisibility, vec4_t *colorForDir)
{
    int i = 6;
    *sunVisibility *= directLightScale;
    do {
        (*colorForDir)[0] *= indirectLightScale;
        (*colorForDir)[1] *= indirectLightScale;
        (*colorForDir)[2] *= indirectLightScale;
        (*colorForDir)[3] *= indirectLightScale;
        colorForDir++;
        i--;
    } while (i);
}

extern float RB_GetLightingAtPoint(const GfxLightGrid *lightGrid, const vec_t *samplePos, vec4_t *colorForDir);

void R_GetStaticModelLightingFromGrid(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float *sunVisibility, vec4_t *colorForDir)
{
    float lightingOrigin[3];

    const vec_t *t1 = smodelInst->mins;
    const vec_t *t2 = smodelInst->maxs;
    lightingOrigin[0] = (smodelInst->mins[0] + smodelInst->maxs[0]) * 0.5f;
    lightingOrigin[1] = (t1[1] + t2[1]) * 0.5f;
    lightingOrigin[2] = (t1[2] + t2[2]) * 0.5f;

    *sunVisibility = RB_GetLightingAtPoint(&world->lightGrid, lightingOrigin, colorForDir);
}

extern void *Hunk_AllocateTempMemoryInternal(int size);

void R_PrepareStaticModelLightingCache(GfxWorld *world, int smodelCount)
{
    int rendererType = r_rendererInUse->current.integer;

    if (rendererType == 2) {
        world->smodelLightingColorTable = (vec4_t(*)[6])ri.Hunk_AllocInternal(smodelCount * 3 * 32);
        world->smodelLightingSunVisTable = (float *)ri.Hunk_AllocInternal(smodelCount * 4);
    } else {
        int width;
        int size;

        ((int *)&smodelLoadGlob)[0] = 1;

        width = 1;
        while (smodelCount > width * width) {
            width += width;
            ((int *)&smodelLoadGlob)[0] = width;
        }

        ((int *)&smodelLoadGlob)[1] = width;
        if (width * ((int *)&smodelLoadGlob)[0] >= smodelCount + smodelCount)
            ((int *)&smodelLoadGlob)[1] = (width + 1) >> 1;

        ((float *)&smodelLoadGlob)[2] = 1.0f / (float)(((int *)&smodelLoadGlob)[0] + ((int *)&smodelLoadGlob)[0]);
        ((float *)&smodelLoadGlob)[3] = 1.0f / (float)(((int *)&smodelLoadGlob)[1] + ((int *)&smodelLoadGlob)[1]);

        size = ((int *)&smodelLoadGlob)[0] * ((int *)&smodelLoadGlob)[1] << 5;
        ((void **)&smodelLoadGlob)[4] = Hunk_AllocateTempMemoryInternal(size);
        memset(((void **)&smodelLoadGlob)[4], 0x80, size);
    }
}

extern int XModelGetNumLods(const XModel *model);

extern int XModelGetSurfaces(const XModel *model, XSurface ***surfaces, int lodIndex, int **partBits);
extern int XSurfaceGetBoneOffset(const XSurface *surf);
extern const char *XModelGetName(const XModel *model);
extern void Com_Printf(const char *fmt, ...);

Bool R_ValidateStaticModel(struct XModel *model)
{
    int lodCount = XModelGetNumLods(model);
    int lodIndex, surfIndex, surfCount;
    XSurface **surfaces;
    int *partBits;

    for (lodIndex = 0; lodIndex < lodCount; lodIndex++) {
        surfCount = XModelGetSurfaces(model, &surfaces, lodIndex, &partBits);

        for (surfIndex = 0; surfIndex < surfCount; surfIndex++) {
            XSurface *surf = surfaces[surfIndex];
            if (XSurfaceGetBoneOffset(surf) == -1) {
                Com_Printf("^1ERROR: model '%s' is not a valid static model, since lod %i surface %i has bone offsets\n",
                           XModelGetName(model), lodIndex, surfIndex);
                return 0;
            }
        }
    }

    return 1;
}

static BM_ALWAYS_INLINE int R_AddStaticModelToAabbTree_r_impl(byte *world, byte *tree, int smodelIndex)
{
    int smodelInstOffset;
    GfxStaticModelInstance *smodelInst;
    GfxAabbTree *aabb;
    int count;

    smodelInstOffset = smodelIndex * (int)sizeof(GfxStaticModelInstance);   /* was hardcoded 96 = x86 sizeof; GfxStaticModelInstance grows on x64 (model XModel* + padding) */

top:
    aabb = (GfxAabbTree *)tree;
    count = aabb->staticModelCount;

    {
        int test = count - 1;
        if ((test & count) == 0) {

            int allocCount;
            int *newIndices;

            if (count == 0)
                allocCount = 1;
            else
                allocCount = count * 2;

            newIndices = (int *)Hunk_AllocateTempMemoryInternal(allocCount * 4);
            memcpy(newIndices, aabb->staticModels, count * 4);
            aabb->staticModels = newIndices;
            newIndices[count] = smodelIndex;
            aabb->staticModelCount = count + 1;
        } else {

            aabb->staticModels[count] = smodelIndex;
            aabb->staticModelCount = count + 1;
        }
    }

    {
        int childCount = aabb->childCount;
        if (childCount == 0)
            return 0;

        smodelInst = (GfxStaticModelInstance *)((byte *)((GfxWorld *)world)->smodelInsts + smodelInstOffset);

        {
            GfxAabbTree *children = (GfxAabbTree *)(intptr_t)aabb->children;
            GfxAabbTree *child = children;
            int i;

            for (i = 0; i < childCount; i++, child++) {
                if (child->mins[0] > smodelInst->mins[0])
                    continue;
                if (child->mins[1] > smodelInst->mins[1])
                    continue;
                if (child->mins[2] > smodelInst->mins[2])
                    continue;
                if (smodelInst->maxs[0] > child->maxs[0])
                    continue;
                if (smodelInst->maxs[1] > child->maxs[1])
                    continue;
                if (smodelInst->maxs[2] > child->maxs[2])
                    continue;

                tree = (byte *)child;
                goto top;
            }

            {
                int si;
                for (si = 0; si < childCount; si++) {
                    if (children[si].surfaceCount == 0) {
                        child = &children[si];

                        {
                            int a;
                            for (a = 0; a < 3; a++) {
                                float smin = smodelInst->mins[a];
                                float smax = smodelInst->maxs[a];
                                if (smin < child->mins[a])
                                    child->mins[a] = smin;
                                if (smax > child->maxs[a])
                                    child->maxs[a] = smax;
                            }
                            tree = (byte *)child;
                            goto top;
                        }
                    }
                }
            }
        }

        {
            int existingCount = childCount;
            int allocSize = (existingCount + 1) * sizeof(GfxAabbTree);
            byte *newChildren = (byte *)Hunk_AllocAlignInternal(allocSize, 4);
            memcpy(newChildren, (void *)(intptr_t)aabb->children, existingCount * sizeof(GfxAabbTree));
            aabb->children = (GfxAabbTree *)newChildren;   /* was (int) -> truncated the 8-byte ptr on x64 */

            {
                GfxAabbTree *newChild = (GfxAabbTree *)(newChildren + existingCount * sizeof(GfxAabbTree));
                aabb->childCount = existingCount + 1;

                memcpy(newChild->mins, smodelInst->mins, 12);
                memcpy(newChild->maxs, smodelInst->maxs, 12);

                tree = (byte *)newChild;
                goto top;
            }
        }
    }
}

static BM_NOINLINE int COD2_REGPARM(3) R_AddStaticModelToAabbTree_r(GfxWorld *world, GfxAabbTree *tree, int smodelIndex)
{
    return R_AddStaticModelToAabbTree_r_impl((byte *)world, (byte *)tree, smodelIndex);
}

extern int BoxOnPlaneSide(const vec_t *mins, const vec_t *maxs, const cplane_t *plane);

static int COD2_REGPARM(3) R_FilterStaticModelIntoCells_r(GfxWorld *world, mnode_t *node,
                                                          GfxStaticModelInstance *smodelInst,
                                                          const vec_t *mins, const vec_t *maxs)
{
    int cellIndex = node->cellIndex;

    if (cellIndex != -2) {

        int smodelIndex;
        GfxAabbTree *aabb;
        int count;

        if (cellIndex < 0)
            return (int)(intptr_t)world;

        smodelIndex = (int)(smodelInst - world->smodelInsts);

        aabb = world->cells[cellIndex].aabbTree;
        count = aabb->staticModelCount;

        if (count != 0 && smodelIndex == aabb->staticModels[count - 1])
            return (int)(intptr_t)aabb->staticModels;

        return R_AddStaticModelToAabbTree_r(world, aabb, smodelIndex);
    }

    {
        cplane_t *plane = node->u.node.plane;
        int side = BoxOnPlaneSide(mins, maxs, plane);

        if (side != 3) {

            return R_FilterStaticModelIntoCells_r(world, node->u.node.children[side - 1],
                                                  smodelInst, mins, maxs);
        }

        if (plane->type > 2) {

            R_FilterStaticModelIntoCells_r(world, node->u.node.children[0],
                                           smodelInst, mins, maxs);
            return R_FilterStaticModelIntoCells_r(world, node->u.node.children[1],
                                                  smodelInst, mins, maxs);
        }

        {
            int axis = plane->type;
            vec3_t localmins;
            vec3_t localmaxs;

            localmins[0] = mins[0];
            localmins[1] = mins[1];
            localmins[2] = mins[2];
            localmins[axis] = plane->dist;

            localmaxs[0] = maxs[0];
            localmaxs[1] = maxs[1];
            localmaxs[2] = maxs[2];
            localmaxs[axis] = plane->dist;

            if (maxs[axis] > localmins[axis])
                R_FilterStaticModelIntoCells_r(world, node->u.node.children[0],
                                               smodelInst, localmins, maxs);

            return R_FilterStaticModelIntoCells_r(world, node->u.node.children[1],
                                                  smodelInst, mins, localmaxs);
        }
    }
}

void R_FinishStaticModelLightingCache(GfxWorld *world)
{
    if (r_rendererInUse->current.integer != 2) {
        GfxImage *image;

        image = Image_Alloc("*smodel_lighting", 2, 1, 4);
        world->smodelLightingImage = image;

        Image_Generate3D(image,
                         ((byte **)&smodelLoadGlob)[4],
                         ((int *)&smodelLoadGlob)[0] * 2,
                         ((int *)&smodelLoadGlob)[1] * 2,
                         2,
                         (D3DFORMAT)21);

        {
            float scaleY = ((float *)&smodelLoadGlob)[3] * 0.5f;
            float *lookupScale = world->smodelLightingLookupScale;
            world->smodelLightingLookupScale[0] = 0.5f * ((float *)&smodelLoadGlob)[2];
            lookupScale[1] = scaleY;
            lookupScale[2] = 0.25f;
        }

        ((int *)&smodelLoadGlob)[0] = 0;
        ((int *)&smodelLoadGlob)[1] = 0;
        ((int *)&smodelLoadGlob)[2] = 0;
        ((int *)&smodelLoadGlob)[3] = 0;
        ((int *)&smodelLoadGlob)[4] = 0;
    }
}
void R_GetStaticModelLightingFromGround(const vec_t *groundLight, float *sunVisibility, vec4_t *colorForDir)
{
    float *base = (float *)colorForDir;
    int i;
    vec_t v;

    for (i = 0; i < 3; i++) {
        v = groundLight[i];
        base[i * 8 + 0] = v;
        base[i * 8 + 1] = v;
        base[i * 8 + 2] = v;
        base[i * 8 + 3] = v;
        v = groundLight[i];
        base[i * 8 + 4] = v;
        base[i * 8 + 5] = v;
        base[i * 8 + 6] = v;
        base[i * 8 + 7] = v;
    }

    *sunVisibility = groundLight[3];
}

extern void AnglesToAxis(const vec_t *angles, vec3_t *axis);
extern void Hunk_FreeTempMemory(void *buf);
extern int XModelNumBones(const struct XModel *model);
extern const DObjAnimMat *XModelGetBasePose(const struct XModel *model);
extern int XSurfaceGetNumVerts(const XSurface *surface);
extern void XSurfaceGetVerts(const XSurface *surf, DObjSkelMat *boneMatrix,
                                      float *pVert, float *pTexCoord, float *pNormal);
extern float XModelGetLodOutDist(const struct XModel *model);

void R_CreateStaticModel(GfxWorld *world, struct XModel *model,
                         const vec_t *origin, const vec_t *angles,
                         vec_t scale, GfxStaticModelInstance *smodelInst)
{
    XSurface **surfaces;
    int *partBits;
    DObjSkelMat *matArray;
    float *vert;
    int surfaceCount;
    int numBones;
    const DObjAnimMat *basePose;
    int i;

    smodelInst->model = model;
    smodelInst->origin[0] = origin[0];
    smodelInst->origin[1] = origin[1];
    smodelInst->origin[2] = origin[2];

    AnglesToAxis(angles, smodelInst->axis);

    smodelInst->scale = scale;

    smodelInst->mins[0] = 3.40282347e+38f;
    smodelInst->mins[1] = 3.40282347e+38f;
    smodelInst->mins[2] = 3.40282347e+38f;
    smodelInst->maxs[0] = -3.40282347e+38f;
    smodelInst->maxs[1] = -3.40282347e+38f;
    smodelInst->maxs[2] = -3.40282347e+38f;

    surfaceCount = XModelGetSurfaces(model, &surfaces, 0, &partBits);

    matArray = (DObjSkelMat *)Hunk_AllocateTempMemoryInternal(0x2000);
    vert = (float *)Hunk_AllocateTempMemoryInternal(0xff78);

    numBones = XModelNumBones(model);
    basePose = XModelGetBasePose(model);

    if (numBones > 0) {
        const DObjAnimMat *b = basePose;
        DObjSkelMat *mat = matArray;
        i = 0;
        do {
            float tw = b->transWeight;
            float wx = tw * b->quat[0];
            float wy = tw * b->quat[1];
            float wz = tw * b->quat[2];

            float xx = wx * b->quat[0];
            float xy = wx * b->quat[1];
            float xz = wx * b->quat[2];
            float xw = wx * b->quat[3];

            float yy = wy * b->quat[1];
            float yz = wy * b->quat[2];
            float yw = wy * b->quat[3];

            float zz = wz * b->quat[2];
            float zw = wz * b->quat[3];

            mat->axis[0][0] = 1.0f - (yy + zz);
            mat->axis[0][1] = zw + xy;
            mat->axis[0][2] = xz - yw;
            mat->axis[0][3] = 0.0f;

            mat->axis[1][0] = xy - zw;
            mat->axis[1][1] = 1.0f - (zz + xx);
            mat->axis[1][2] = xw + yz;
            mat->axis[1][3] = 0.0f;

            mat->axis[2][0] = yw + xz;
            mat->axis[2][1] = yz - xw;
            mat->axis[2][2] = 1.0f - (xx + yy);
            mat->axis[2][3] = 0.0f;

            mat->origin[0] = b->trans[0];
            mat->origin[1] = b->trans[1];
            mat->origin[2] = b->trans[2];
            mat->origin[3] = 1.0f;

            i++;
            mat++;
            b++;
        } while (numBones != i);
    }

    if (surfaceCount > 0) {
        int s;
        for (s = 0; s < surfaceCount; s++) {
            XSurface *surf = surfaces[s];
            int vertCount = XSurfaceGetNumVerts(surf);
            XSurfaceGetVerts(surf, matArray, vert, 0, 0);

            if (vertCount > 0) {
                const float *v = vert;
                int vi;
                for (vi = 0; vi < vertCount; vi++) {
                    int j;
                    for (j = 0; j < 3; j++) {
                        float dot = v[0] * smodelInst->axis[0][j] + v[1] * smodelInst->axis[1][j] + v[2] * smodelInst->axis[2][j];
                        if (smodelInst->mins[j] > dot)
                            smodelInst->mins[j] = dot;
                        if (dot > smodelInst->maxs[j])
                            smodelInst->maxs[j] = dot;
                    }
                    v += 3;
                }
            }
        }
    }

    Hunk_FreeTempMemory(vert);
    Hunk_FreeTempMemory(matArray);

    smodelInst->mins[0] = scale * smodelInst->mins[0] + origin[0];
    smodelInst->mins[1] = scale * smodelInst->mins[1] + origin[1];
    smodelInst->mins[2] = scale * smodelInst->mins[2] + origin[2];
    smodelInst->maxs[0] = scale * smodelInst->maxs[0] + origin[0];
    smodelInst->maxs[1] = scale * smodelInst->maxs[1] + origin[1];
    smodelInst->maxs[2] = scale * smodelInst->maxs[2] + origin[2];

    smodelInst->cullDist = scale * XModelGetLodOutDist(model);

    R_FilterStaticModelIntoCells_r(world, world->nodes, smodelInst,
                                   smodelInst->mins, smodelInst->maxs);
}
void R_CacheStaticModelLighting(const GfxWorld *world, GfxStaticModelInstance *smodelInst, float sunVisibility, vec4_t *colorForDir)
{
    int idx = world->smodelCount - 1;
    const float *cfd = (const float *)colorForDir;
    int x0, y0;
    float fbase;
    int z, y, x, c;

    if (r_rendererInUse->current.integer == 2) {
        memcpy(world->smodelLightingColorTable + idx, colorForDir, 96);
        world->smodelLightingSunVisTable[idx] = sunVisibility;
        return;
    }

    x0 = 2 * (idx % ((int *)&smodelLoadGlob)[0]);
    y0 = 2 * (idx / ((int *)&smodelLoadGlob)[0]);

    fbase = 0.5f + 255.0f * sunVisibility;

    c = 0;
    for (z = 0; z < 2; z++) {
        for (y = y0; y < y0 + 2; y++) {
            int ai = (int)floorf(fbase);
            if (ai - 255 >= 0)
                ai = 255;
            else if (ai <= 0)
                ai = 0;
            for (x = x0; x < x0 + 2; x++) {
                byte *texel = &((byte **)&smodelLoadGlob)[4][4 * (x + 2 * ((int *)&smodelLoadGlob)[0] * (y + 2 * (z * ((int *)&smodelLoadGlob)[1])))];
                int ri = (int)floorf(cfd[c] * 255.0f + 0.5f);
                int gi = (int)floorf(cfd[c + 8] * 255.0f + 0.5f);
                int bi = (int)floorf(0.5f + 255.0f * cfd[c + 16]);
                byte rByte, gByte;

                if (ri - 255 >= 0)
                    rByte = 255;
                else if (ri <= 0)
                    rByte = 0;
                else
                    rByte = (byte)ri;
                if (gi - 255 >= 0)
                    gByte = 255;
                else if (gi <= 0)
                    gByte = 0;
                else
                    gByte = (byte)gi;
                if (bi - 255 >= 0)
                    bi = 255;
                else if (bi <= 0)
                    bi = 0;

                /* A8R8G8B8 is B, G, R, A in memory: the shader reads the
                 * light's color from rgb and the sun's visibility from a */
                texel[0] = (byte)bi;
                texel[1] = gByte;
                texel[2] = rByte;
                texel[3] = (byte)ai;
                c++;
            }
        }
    }

    smodelInst->baseLightingCoords[0] = ((float)x0 + 1.0f) * ((float *)&smodelLoadGlob)[2];
    smodelInst->baseLightingCoords[1] = ((float)y0 + 1.0f) * ((float *)&smodelLoadGlob)[3];
    smodelInst->baseLightingCoords[2] = 0.5f;
}
static int R_CompareStaticModelIndex(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

int R_SortGfxAabbTree(GfxWorld *world, GfxAabbTree *tree)
{
    GfxStaticModelInstance *insts;
    vec3_t localMins, localMaxs, boxLo[4], boxHi[4];
    float centerX, centerY;
    int *base, *part, remaining, partCount[4], qualifying, total, i, k, pass;
    GfxAabbTree *children, *c;

top:
    insts = world->smodelInsts;
    qsort(tree->staticModels, tree->staticModelCount, 4, R_CompareStaticModelIndex);

    if (tree->childCount) {
        GfxAabbTree *kids = (GfxAabbTree *)(intptr_t)tree->children;
        for (i = 0; i < tree->childCount; i++)
            R_SortGfxAabbTree(world, &kids[i]);
        return 0;
    }

    localMins[0] = localMins[1] = localMins[2] = FLT_MAX;
    localMaxs[0] = localMaxs[1] = localMaxs[2] = -FLT_MAX;
    for (i = 0; i < tree->staticModelCount; i++) {
        GfxStaticModelInstance *m = &insts[tree->staticModels[i]];
        for (k = 0; k < 3; k++) {
            if (localMins[k] > m->mins[k])
                localMins[k] = m->mins[k];
            if (m->maxs[k] > localMaxs[k])
                localMaxs[k] = m->maxs[k];
        }
    }
    if (tree->surfaceCount == 0)
        for (k = 0; k < 3; k++) {
            tree->mins[k] = localMins[k];
            tree->maxs[k] = localMaxs[k];
        }

    if (tree->staticModelCount <= 7)
        return 0;

    centerX = (localMins[0] + localMaxs[0]) * 0.5f;
    centerY = (localMins[1] + localMaxs[1]) * 0.5f;
    for (pass = 0; pass < 4; pass++)
        for (k = 0; k < 3; k++) {
            boxLo[pass][k] = localMins[k];
            boxHi[pass][k] = localMaxs[k];
        }
    boxHi[0][0] = centerX;
    boxLo[1][0] = centerX;
    boxHi[2][1] = centerY;
    boxLo[3][1] = centerY;

    base = tree->staticModels;
    remaining = tree->staticModelCount;
    for (pass = 0; pass < 4; pass++) {
        int count = 0;
        int *dst = base;
        for (i = 0; i < remaining; i++) {
            GfxStaticModelInstance *m = &insts[base[i]];
            if (!(boxLo[pass][0] > m->mins[0]) && !(boxLo[pass][1] > m->mins[1]) &&
                !(boxLo[pass][2] > m->mins[2]) && !(m->maxs[0] > boxHi[pass][0]) &&
                !(m->maxs[1] > boxHi[pass][1]) && !(m->maxs[2] > boxHi[pass][2])) {
                int t = *dst;
                *dst = base[i];
                base[i] = t;
                dst++;
                count++;
            }
        }
        if (count <= 1)
            count = 0;
        partCount[pass] = count;
        base += count;
        remaining -= count;
    }

    qualifying = 0;
    for (pass = 0; pass < 4; pass++)
        if (partCount[pass])
            qualifying++;
    if (qualifying == 0)
        return 0;

    total = qualifying + (tree->surfaceCount ? 1 : 0) + (remaining ? 1 : 0);
    children = (GfxAabbTree *)Hunk_AllocAlignInternal(total * (int)sizeof(GfxAabbTree), 4);
    tree->children = children;   /* was (int) -> truncated the 8-byte ptr on x64 */
    tree->childCount = 0;

    if (tree->surfaceCount) {
        c = &children[tree->childCount++];
        for (k = 0; k < 3; k++) {
            c->mins[k] = tree->mins[k];
            c->maxs[k] = tree->maxs[k];
        }
        c->surfaceCount = tree->surfaceCount;
        c->startSurfIndex = tree->startSurfIndex;
    }

    part = tree->staticModels;
    for (pass = 0; pass < 4; pass++) {
        if (partCount[pass]) {
            c = &children[tree->childCount++];
            c->staticModelCount = partCount[pass];
            c->staticModels = part;
            R_SortGfxAabbTree(world, c);
            part += partCount[pass];
        }
    }
    if (remaining) {
        c = &children[tree->childCount++];
        c->staticModelCount = remaining;
        c->staticModels = part;
        tree = c;
        goto top;
    }
    return 0;
}

int R_AllocStaticModels(GfxAabbTree *tree)
{
    if (tree->staticModelCount) {
        int *indices = (int *)Hunk_AllocAlignInternal(tree->staticModelCount << 2, 4);
        memcpy(indices, tree->staticModels, tree->staticModelCount << 2);
        tree->staticModels = indices;
    }
    if (tree->childCount > 0) {
        GfxAabbTree *children = (GfxAabbTree *)(intptr_t)tree->children;
        int i;
        for (i = 0; i < tree->childCount; i++)
            R_AllocStaticModels(&children[i]);
    }
    return 0;
}

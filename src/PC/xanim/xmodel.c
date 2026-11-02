#include "common_types.h"
#include "imports.h"

#include <string.h>

extern struct XModelDefault g_default;

extern void SL_RemoveRefToString(unsigned int name);
extern void *Hunk_FindDataForFile(int type, const char *name);
extern const char *Hunk_SetDataForFile(int type, const char *name, void *data, Alloc_t alloc);
extern XModel *XModelLoad(const char *name, Alloc_t Alloc, Alloc_t AllocColl);
extern int XModelNumBones(const XModel *model);
extern void Com_Printf(const char *fmt, ...);
extern int strnicmp(const char *s1, const char *s2, size_t n);
extern void CM_CalcTraceEntents(TraceExtents *extents);
extern qboolean CM_TraceBox(const TraceExtents *extents, const vec_t *mins, const vec_t *maxs, float fraction);

int XModelBad(const XModel *model);
void XModelPartsFree(XModelParts *model);
XModelParts *XModelPartsFindData(const char *name);
void XModelPartsSetData(const char *name, XModelParts *modelParts, Alloc_t Alloc);
XModelSurfs *XModelSurfsFindData(const char *name);
void XModelSurfsSetData(const char *name, XModelSurfs *modelSurfs, Alloc_t Alloc);
XModel *XModelPrecache(const char *name, Alloc_t Alloc, Alloc_t AllocColl);
intptr_t XModelBoneNames(XModel *model);
int XModelGetBoneIndex(const XModel *model, unsigned int name);
const char *XModelGetLodName(const XModel *model, int lod);
int XModelGetContents(const XModel *model);
const struct trXSkin_t *XModelGetSkins(const XModel *model);
int XModelGetMemUsage(const XModel *model);
qboolean Com_ValidXModelName(const char *name);
void XModelGetBounds(const XModel *model, vec_t *mins, vec_t *maxs);
void XModelFree(XModel *model);
int XModelTraceLine(const XModel *model, trace_t *results, const DObjAnimMat *boneMtxList, vec_t *localStart, vec_t *localEnd, int contentmask);

int XModelBad(const XModel *model)
{
    return model->bad;
}

void XModelPartsFree(XModelParts *model)
{
    unsigned short *boneNames = model->hierarchy->names;
    int size = model->numBones;
    int i;

    for (i = 0; i < size; i++) {
        SL_RemoveRefToString(boneNames[i]);
    }
}

XModelParts *XModelPartsFindData(const char *name)
{
    return (XModelParts *)(Hunk_FindDataForFile(3, name));
}

void XModelPartsSetData(const char *name, XModelParts *modelParts, Alloc_t Alloc)
{
    Hunk_SetDataForFile(3, name, modelParts, Alloc);
}

XModelSurfs *XModelSurfsFindData(const char *name)
{
    return (XModelSurfs *)(Hunk_FindDataForFile(2, name));
}

void XModelSurfsSetData(const char *name, XModelSurfs *modelSurfs, Alloc_t Alloc)
{
    Hunk_SetDataForFile(2, name, modelSurfs, Alloc);
}

XModel *XModelPrecache(const char *name, Alloc_t Alloc, Alloc_t AllocColl)
{
    XModel *model;
    int i;

    model = (XModel *)(Hunk_FindDataForFile(4, name));
    if (model) {
        return model;
    }

    model = XModelLoad(name, Alloc, AllocColl);
    if (model) {
        model->name = (const char *)(Hunk_SetDataForFile(4, name, model, Alloc));
        return model;
    }

    Com_Printf("^1ERROR: Cannot find xmodel '%s'.\n", name);
    model = (XModel *)(Alloc(sizeof(XModel)));
    model->bad = 1;

    g_default.hierarchy.names = g_default.boneNames;
    g_default.modelParts.hierarchy = &g_default.hierarchy;
    g_default.modelParts.quats = NULL;
    g_default.modelParts.trans = NULL;
    g_default.modelParts.numBones = 1;
    g_default.modelParts.numRootBones = 1;
    g_default.modelParts.partClassification = g_default.partClassification;
    g_default.partClassification[0] = 0;
    g_default.boneNames[0] = 0;

    model->parts = (void (*)())&g_default.modelParts;
    for (i = 0; i < 4; i++) {
        model->lodInfo[i].surfs = NULL;
        model->lodInfo[i].filename = (char *)"";
        model->lodInfo[i].dist = 0;
        model->lodInfo[i].numsurfs = 1;
        model->lodInfo[i].surfNames = g_default.surfNames;
        g_default.surfNames[0] = 0;
    }

    g_default.modelSurf.surfs = NULL;
    model->lodInfo[0].surfs = (XModelSurfs *)&g_default.modelSurf;
    model->numLods = 1;
    model->collLod = 0;
    model->name = "DEFAULT";

    g_default.boneInfo.bounds[0][0] = -16.0f;
    g_default.boneInfo.bounds[0][1] = -16.0f;
    g_default.boneInfo.bounds[0][2] = -16.0f;
    g_default.boneInfo.bounds[1][0] = 16.0f;
    g_default.boneInfo.bounds[1][1] = 16.0f;
    g_default.boneInfo.bounds[1][2] = 16.0f;
    model->boneInfo = (XBoneInfo_s *)&g_default.boneInfo;

    Hunk_SetDataForFile(4, name, model, Alloc);
    return model;
}

intptr_t XModelBoneNames(XModel *model)
{
    XModelParts *parts = (XModelParts *)model->parts;
    return (intptr_t)parts->hierarchy->names;
}

int XModelGetBoneIndex(const XModel *model, unsigned int name)
{
    XModelParts *parts = (XModelParts *)model->parts;
    unsigned short *boneNames = parts->hierarchy->names;
    int localBoneIndex;

    localBoneIndex = (int)parts->numBones - 1;
    if (localBoneIndex < 0) {
        return localBoneIndex;
    }

    do {
        if (name == boneNames[localBoneIndex]) {
            return localBoneIndex;
        }
    } while (--localBoneIndex >= 0);

    return localBoneIndex;
}

const char *XModelGetLodName(const XModel *model, int lod)
{
    return model->lodInfo[lod].filename;
}

int XModelGetContents(const XModel *model)
{
    return model->contents;
}

const struct trXSkin_t *XModelGetSkins(const XModel *model)
{
    return model->xskins;
}

int XModelGetMemUsage(const XModel *model)
{
    return model->memUsage;
}

qboolean Com_ValidXModelName(const char *name)
{
    return strnicmp(name, "xmodel", 6) == 0 && (name[6] == '/' || name[6] == '\\');
}

void XModelGetBounds(const XModel *model, vec_t *mins, vec_t *maxs)
{
    mins[0] = model->mins[0];
    mins[1] = model->mins[1];
    mins[2] = model->mins[2];
    maxs[0] = model->maxs[0];
    maxs[1] = model->maxs[1];
    maxs[2] = model->maxs[2];
}

void XModelFree(XModel *model)
{
    int i, j;

    if (model->bad) {
        return (void)0;
    }

    for (i = 0; i < 4; i++) {
        if (!model->lodInfo[i].surfNames) {
            continue;
        }
        for (j = 0; j < model->lodInfo[i].numsurfs; j++) {
            SL_RemoveRefToString(model->lodInfo[i].surfNames[j]);
        }
        model->lodInfo[i].surfNames = NULL;
    }
}

int XModelTraceLine(const XModel *model, trace_t *results, const DObjAnimMat *boneMtxList, vec_t *localStart, vec_t *localEnd, int contentmask)
{
    int i, j;
    int partIndex = -1;
    int boneIdx;
    float xx, xy, xz, xw, yy, yz, yw, zz, zw;
    float m00, m01, m02, m10, m11, m12, m20, m21, m22;
    float scale, xs, ys_val, zs_val;
    vec3_t startRel, endRel;

    union {
        vec3_t bonePos[2];
        struct TraceExtents ext;
    } bonePosU;
#    define bonePos (bonePosU.bonePos)
    vec3_t delta;
    const DObjAnimMat *mat;
    XModelCollSurf *csurf;

    XModelNumBones(model);

    if (model->numCollSurfs <= 0) {
        return -1;
    }

    for (i = 0; i < model->numCollSurfs; i++) {
        csurf = &model->collSurfs[i];

        if (!(contentmask & csurf->contents)) {
            continue;
        }

        boneIdx = csurf->boneIdx;
        mat = &boneMtxList[boneIdx];

        startRel[0] = localStart[0] - mat->trans[0];
        startRel[1] = localStart[1] - mat->trans[1];
        startRel[2] = localStart[2] - mat->trans[2];
        endRel[0] = localEnd[0] - mat->trans[0];
        endRel[1] = localEnd[1] - mat->trans[1];
        endRel[2] = localEnd[2] - mat->trans[2];

        scale = mat->transWeight;
        xs = mat->quat[0] * scale;
        ys_val = mat->quat[1] * scale;
        zs_val = mat->quat[2] * scale;

        xx = xs * mat->quat[0];
        xy = xs * mat->quat[1];
        xz = xs * mat->quat[2];
        xw = xs * mat->quat[3];
        yy = ys_val * mat->quat[1];
        yz = ys_val * mat->quat[2];
        yw = ys_val * mat->quat[3];
        zz = zs_val * mat->quat[2];
        zw = zs_val * mat->quat[3];

        m00 = 1.0f - (yy + zz);
        m01 = xy + zw;
        m02 = xz - yw;
        m10 = xy - zw;
        m11 = 1.0f - (xx + zz);
        m12 = yz + xw;
        m20 = xz + yw;
        m21 = yz - xw;
        m22 = 1.0f - (xx + yy);

        bonePos[0][0] = m00 * startRel[0] + m01 * startRel[1] + m02 * startRel[2];
        bonePos[0][1] = m10 * startRel[0] + m11 * startRel[1] + m12 * startRel[2];
        bonePos[0][2] = m20 * startRel[0] + m21 * startRel[1] + m22 * startRel[2];
        bonePos[1][0] = m00 * endRel[0] + m01 * endRel[1] + m02 * endRel[2];
        bonePos[1][1] = m10 * endRel[0] + m11 * endRel[1] + m12 * endRel[2];
        bonePos[1][2] = m20 * endRel[0] + m21 * endRel[1] + m22 * endRel[2];

        CM_CalcTraceEntents(&bonePosU.ext);
        if (CM_TraceBox(&bonePosU.ext, csurf->mins, csurf->maxs, results->fraction)) {
            continue;
        }

        delta[0] = bonePos[1][0] - bonePos[0][0];
        delta[1] = bonePos[1][1] - bonePos[0][1];
        delta[2] = bonePos[1][2] - bonePos[0][2];

        for (j = 0; j < csurf->numCollTris; j++) {
            XModelCollTri *tri = &csurf->collTris[j];
            float dot_end, dot_start, diff, frac, t;
            float hit0, hit1, hit2;
            float s, tv;

            dot_end = bonePos[1][0] * tri->plane[0] + bonePos[1][1] * tri->plane[1] +
                      bonePos[1][2] * tri->plane[2] - tri->plane[3];
            if (dot_end >= 0.0f) {
                continue;
            }

            dot_start = bonePos[0][0] * tri->plane[0] + bonePos[0][1] * tri->plane[1] +
                        bonePos[0][2] * tri->plane[2] - tri->plane[3];
            if (dot_start <= 0.0f) {
                continue;
            }

            diff = dot_start - dot_end;
            frac = (dot_start - 0.125f) / diff;
            if (frac < 0.0f) {
                frac = 0.0f;
            }
            if (frac >= results->fraction) {
                continue;
            }

            t = dot_start / diff;
            hit0 = bonePos[0][0] + t * delta[0];
            hit1 = bonePos[0][1] + t * delta[1];
            hit2 = bonePos[0][2] + t * delta[2];

            s = hit0 * tri->svec[0] + hit1 * tri->svec[1] + hit2 * tri->svec[2] - tri->svec[3];
            if (s < -0.001f) {
                continue;
            }
            if (s > 1.001f) {
                continue;
            }

            tv = hit0 * tri->tvec[0] + hit1 * tri->tvec[1] + hit2 * tri->tvec[2] - tri->tvec[3];
            if (tv < -0.001f) {
                continue;
            }
            if (s + tv > 1.001f) {
                continue;
            }

            results->startsolid = 0;
            results->allsolid = 0;
            results->fraction = frac;
            results->surfaceFlags = csurf->surfFlags;
            results->contents = csurf->contents;
            results->normal[0] = tri->plane[0];
            results->normal[1] = tri->plane[1];
            results->normal[2] = tri->plane[2];
            partIndex = boneIdx;
        }
    }

    if (partIndex >= 0) {

        mat = &boneMtxList[partIndex];
        scale = mat->transWeight;
        xs = mat->quat[0] * scale;
        ys_val = mat->quat[1] * scale;
        zs_val = mat->quat[2] * scale;

        xx = xs * mat->quat[0];
        xy = xs * mat->quat[1];
        xz = xs * mat->quat[2];
        xw = xs * mat->quat[3];
        yy = ys_val * mat->quat[1];
        yz = ys_val * mat->quat[2];
        yw = ys_val * mat->quat[3];
        zz = zs_val * mat->quat[2];
        zw = zs_val * mat->quat[3];

        {
            float n0 = results->normal[0];
            float n1 = results->normal[1];
            float n2 = results->normal[2];

            results->normal[0] = (1.0f - yy - zz) * n0 + (xy - zw) * n1 + (xz + yw) * n2;
            results->normal[1] = (xy + zw) * n0 + (1.0f - xx - zz) * n1 + (yz - xw) * n2;
            results->normal[2] = (xz - yw) * n0 + (yz + xw) * n1 + (1.0f - xx - yy) * n2;
        }
    }

    return partIndex;
}
#    undef bonePos

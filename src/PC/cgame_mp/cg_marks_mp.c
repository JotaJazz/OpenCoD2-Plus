#include "common_types.h"
#include <math.h>
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern const dvar_t *cg_marks;
extern const dvar_t *cg_marksLimit;


extern MarkPoly cg_markPolys[1024];
extern MarkPoly *cg_freeMarkPolys;
extern MarkVertAssemblyBuffer markVerts;
/* CG_ImpactMark lets CL_MarkFragments write up to 1024 vertices there */
#ifdef __cplusplus
static_assert(sizeof(MarkVertAssemblyBuffer) >= 1024 * sizeof(GfxWorldVertex), "markVerts holds 1024 vertices");
#endif

extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern void PerpendicularVector(const vec_t *src, vec_t *dst);
extern void RotatePointAroundVector(vec_t *dst, const vec_t *dir, const vec_t *point, const float degrees);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern Bool FxHelper_CullSphere(const FxHelper *_this, const vec_t *worldPos, float radius, int planeCount);
extern void CL_AddPolyToScene(MaterialHandle mtlHandle, unsigned short lmapIndex,
                              unsigned short vertCount, const GfxWorldVertex *verts);
extern int CL_MarkFragments(const vec3_t *points, const vec_t *origin, const vec3_t *axis, float radius, int maxPoints, GfxWorldVertex *verts, int maxFragments, GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial);
extern void Com_Error(errorParm_t code, const char *fmt, ...);

#define CG_MARKS_DVAR cg_marks
#define FX_HELPER_PTR (*(FxHelper **)(imp_theFxHelper))

static inline __attribute__((always_inline)) unsigned char ColorFloatToByte(float f)
{
    int val = (int)floorf(f * 255.0f + 0.5f);
    if (val - 255 >= 0)
        return 255;
    if (val <= 0)
        return 0;
    return (unsigned char)val;
}

void CG_InitMarkPolys(void);
void CG_AddMarks(void);
void CG_ImpactMark(MaterialHandle markMaterial, const vec_t *origin, const vec_t *dir, float orientation, const vec_t *color, float radius);

void CG_InitMarkPolys(void)
{
    int lasttrav;
    int i;
    MarkPoly *sentinel;

    lasttrav = cg_marksLimit->current.integer;

    memset(cg_markPolys, 0, (size_t)lasttrav * sizeof(MarkPoly));

    sentinel = &cgArray[0].activeMarkPolys;
    sentinel->nextMark = sentinel;
    sentinel->prevMark = sentinel;

    cg_freeMarkPolys = &cg_markPolys[0];

    {
        MarkPoly *nxt = &cg_markPolys[1];
        MarkPoly *cur = &cg_markPolys[0];
        for (i = 0; i < lasttrav - 1; i++) {
            cur->nextMark = nxt;
            cur = nxt;
            nxt = nxt + 1;
        }
    }
}

void CG_AddMarks(void)
{
    MarkPoly *markPoly;
    FxHelper **fxSlot;

    if (!CG_MARKS_DVAR->current.enabled)
        return;

    markPoly = cg->activeMarkPolys.nextMark;

    if (markPoly == &cg->activeMarkPolys)
        return;

    fxSlot = (FxHelper **)(imp_theFxHelper);

    do {

        if (!FxHelper_CullSphere(*fxSlot, markPoly->origin, markPoly->radius,
                                 (*fxSlot)->mCamera.numPlanes)) {

            markPoly->lastFrameDrawn = cg->clientFrame;

            CL_AddPolyToScene(markPoly->mtlHandle, (int)markPoly->lmapIndex,
                              (int)markPoly->vertCount, markPoly->verts);
        }

        markPoly = markPoly->nextMark;
    } while (markPoly != &cg->activeMarkPolys);
}

void CG_ImpactMark(MaterialHandle markMaterial, const vec_t *origin, const vec_t *dir, float orientation, const vec_t *color, float radius)
{
    vec3_t axisRight;
    vec3_t axisCross;
    vec3_t axisDir;
    vec3_t originalPoints[4];
    GfxMarkFragment markFragments[384];
    int fragmentCount;
    int fragmentIndex;
    GfxColor nativeColor;
    int j;
    MarkPoly *sentinel;
    MarkPoly *markPoly;
    MarkPoly *oldest;
    MarkPoly *current;
    float a, b;

    if (!CG_MARKS_DVAR->current.enabled)
        return;

    if (((cg_t *)cg)->nomarks != 0)
        return;

    Vec3NormalizeTo(dir, axisDir);
    PerpendicularVector(axisDir, axisCross);
    RotatePointAroundVector(axisRight, axisDir, axisCross, orientation);
    Vec3Cross(axisDir, axisRight, axisCross);

    for (j = 0; j < 3; j++) {
        a = radius * axisCross[j];
        b = radius * axisRight[j];
        originalPoints[0][j] = origin[j] - a - b;
        originalPoints[1][j] = origin[j] + a - b;
        originalPoints[2][j] = origin[j] + a + b;
        originalPoints[3][j] = origin[j] - a + b;
    }

    fragmentCount = CL_MarkFragments(
        originalPoints,
        origin,
        (const vec3_t *)axisDir,
        radius,
        1024,
        (GfxWorldVertex *)&markVerts,
        384,
        markFragments,
        markMaterial);

    nativeColor.array[1] = ColorFloatToByte(color[0]);
    nativeColor.array[2] = ColorFloatToByte(color[1]);
    nativeColor.array[3] = ColorFloatToByte(color[2]);
    nativeColor.array[0] = ColorFloatToByte(color[3]);

    if (fragmentCount <= 0)
        return;

    for (fragmentIndex = 0; fragmentIndex < fragmentCount; fragmentIndex++) {
        GfxMarkFragment *frag = &markFragments[fragmentIndex];
        GfxWorldVertex *verts = (GfxWorldVertex *)((char *)&markVerts + frag->firstPoint * (int)sizeof(GfxWorldVertex));
        int vertCount = (int)frag->pointCount;

        for (j = 0; j < vertCount; j++) {
            verts[j].color.packed = *(unsigned int *)nativeColor.array;
        }

        if (cg_freeMarkPolys == NULL) {

            sentinel = &cg->activeMarkPolys;
            oldest = sentinel->prevMark;

            if (oldest != sentinel) {
                current = oldest;
                while (1) {
                    if (current->lastFrameDrawn < oldest->lastFrameDrawn) {
                        oldest = current;
                    }
                    current = current->prevMark;
                    if (current == sentinel)
                        break;
                }
            }

            if (oldest->prevMark == NULL) {
                Com_Error(ERR_DROP, "CG_FreeLocalEntity: not active");

            }

            {
                MarkPoly *prev = oldest->prevMark;
                MarkPoly *next = oldest->nextMark;
                prev->nextMark = next;
                next->prevMark = prev;
            }

            oldest->nextMark = cg_freeMarkPolys;
            cg_freeMarkPolys = oldest;
        }

        markPoly = cg_freeMarkPolys;
        cg_freeMarkPolys = markPoly->nextMark;

        sentinel = &cg->activeMarkPolys;
        {
            MarkPoly *oldFirst = sentinel->nextMark;
            markPoly->nextMark = oldFirst;
            markPoly->prevMark = sentinel;
            oldFirst->prevMark = markPoly;
            sentinel->nextMark = markPoly;
        }

        markPoly->origin[0] = origin[0];
        markPoly->origin[1] = origin[1];
        markPoly->origin[2] = origin[2];
        markPoly->radius = radius;
        markPoly->mtlHandle = frag->markMaterial;
        markPoly->lmapIndex = frag->lmapIndex;
        markPoly->vertCount = frag->pointCount;
        markPoly->lastFrameDrawn = *(int *)cg;

        memcpy(markPoly->verts, verts, (size_t)frag->pointCount * sizeof(GfxWorldVertex));
    }
}

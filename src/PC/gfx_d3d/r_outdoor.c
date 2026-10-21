#include "common_types.h"
#include <math.h>
extern dvar_t *r_rendererInUse;
#include "imports.h"

extern GfxImage * Image_Register(const char *imageName, int semantic, int imageTrack);
extern void ClearBounds(float *mins, float *maxs);
extern void ExpandBounds(const vec_t *mins, const vec_t *maxs, vec_t *boundsMin, vec_t *boundsMax);
extern void MatrixIdentity44(float (*out)[4]);
extern void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
extern void *Hunk_AllocateTempMemoryInternal(int size);
extern void Hunk_FreeTempMemory(void *buf);

typedef int (*CM_BoxTraceFn)(trace_t *results, const vec_t *start, const vec_t *end,
                             const vec_t *mins, const vec_t *maxs, clipHandle_t model, int brushmask);

/* the outdoor map's texels per axis, as the 1.3 renderer's (0x10190398): left
 * zero, the map's scale was negative and nothing was outdoors */
static const int outdoorMapSize[3] = { 512, 512, 256 };

static OutdoorGlob outdoorGlob;

void R_RegisterOutdoorImage(GfxWorld *world)
{
    const dvar_t *rendererInUse = r_rendererInUse;
    if (rendererInUse->current.integer == 2) {
        world->outdoorImage = NULL;
        return;
    }

    ClearBounds(outdoorGlob.bbox[0], outdoorGlob.bbox[1]);

    int surfCount = world->surfaceCount;
    int i;
    for (i = 0; i < surfCount; i++) {
        const GfxSurface *surf = &world->surfaces[i];
        const Material *material = surf->material;

        if (material->info.gameFlags & 8)
            continue;

        ExpandBounds(surf->tris->bounds[0], surf->tris->bounds[1],
                     outdoorGlob.bbox[0], outdoorGlob.bbox[1]);
    }

    int axis;
    for (axis = 0; axis < 3; axis++) {
        if (outdoorGlob.bbox[0][axis] == 131072.0f) {
            outdoorGlob.bbox[0][axis] = 0.0f;
            outdoorGlob.bbox[1][axis] = 0.0f;
        }

        float extent = outdoorGlob.bbox[1][axis] - outdoorGlob.bbox[0][axis];
        if (extent < 1.0f) {
            outdoorGlob.bbox[0][axis] -= 0.5f;
            outdoorGlob.bbox[1][axis] += 0.5f;
        }
    }

    for (axis = 0; axis < 3; axis++) {
        float range = outdoorGlob.bbox[1][axis] - outdoorGlob.bbox[0][axis];
        float s = (float)(outdoorMapSize[axis] - 1) / range;
        outdoorGlob.scale[axis] = s;
        outdoorGlob.invScale[axis] = 1.0f / s;
        outdoorGlob.add[axis] = -outdoorGlob.bbox[0][axis] * s;
    }

    float outdoorScale[3];
    float outdoorTranslate[3];
    for (axis = 0; axis < 3; axis++) {
        float range = outdoorGlob.bbox[1][axis] - outdoorGlob.bbox[0][axis];
        outdoorScale[axis] = 1.0f / range;
        outdoorTranslate[axis] = -outdoorGlob.bbox[0][axis] * outdoorScale[axis];
    }

    float *matrix = (float *)world->outdoorLookupMatrix;
    MatrixIdentity44( (float (*)[4])(matrix));
    matrix[0] = outdoorScale[0];
    matrix[5] = outdoorScale[1];
    matrix[10] = outdoorScale[2];
    matrix[12] = outdoorTranslate[0];
    matrix[13] = outdoorTranslate[1];
    matrix[14] = outdoorTranslate[2];

    world->outdoorImage = (GfxImage *)Image_Register("$outdoor", 1, 0);
}

void R_GenerateOutdoorImage(GfxImage *outdoorImage)
{
    byte *pic = (byte *)Hunk_AllocateTempMemoryInternal(0x40000);
    outdoorGlob.pic = pic;

    int row;
    for (row = 0; row < 512; row++) {
        float rowF = (float)row + 0.5f - outdoorGlob.add[1];
        rowF *= outdoorGlob.invScale[1];

        int col;
        for (col = 0; col < 512; col++) {
            float colF = (float)col + 0.5f - outdoorGlob.add[0];
            colF *= outdoorGlob.invScale[0];

            float ceilZ = outdoorGlob.bbox[1][2] + 1.0f;
            float floorZ = outdoorGlob.bbox[0][2] - 1.0f;

            vec3_t start, end;
            start[0] = colF;
            start[1] = rowF;
            start[2] = ceilZ;

            end[0] = colF;
            end[1] = rowF;
            end[2] = floorZ;

            trace_t trace;
            memset(&trace, 0, sizeof(trace));
            trace.fraction = 1.0f;

            vec3_t nullVec = { 0, 0, 0 };
            if (imp_CM_BoxTrace)
                ((CM_BoxTraceFn)imp_CM_BoxTrace)(&trace, start, end, nullVec, nullVec, 0, 0x2001);

            float hitZ = floorZ + (ceilZ - floorZ) * trace.fraction;
            hitZ = hitZ * outdoorGlob.scale[2] + outdoorGlob.add[2];

            float fval = floorf(hitZ);
            int ival = (int)fval;
            byte bval;
            if (ival < 0) {
                bval = 0;
            } else {
                bval = (ival > 255) ? 255 : (byte)ival;
            }

            pic[col] = bval;
        }

        pic += 512;
    }

    Image_Generate2D(outdoorImage, outdoorGlob.pic, 512, 512, 0x32);
    Hunk_FreeTempMemory(outdoorGlob.pic);
}

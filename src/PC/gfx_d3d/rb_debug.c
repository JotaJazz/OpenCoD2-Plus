#include "common_types.h"
#include <math.h>
extern materialCommands_t tess;
/* File-scope alias: bound where no local can shadow `tess`, so uses below
   always reach the global even inside functions that declare their own `tess`. */
static materialCommands_t * const tess_g = &tess;
extern GfxBackEndData *backEndData;
#include "imports.h"
#include "bytematch.h"

extern refimport_t ri;
extern dvar_t *r_rendererInUse;
extern r_global_permanent_t rgp;

extern r_backEndGlobals_t backEnd;

extern void R_ConvertColorToBytes(const vec_t *colorFloat, byte *colorBytes);
extern void RB_DrawLines3D(int count, int width, const GfxPointVertex *verts, int depthTest);
extern void RB_EndSurface(void);
extern void RB_Set3D(void);
extern void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex);
extern void RB_DrawTextInSpace(const char *text, FontHandle font, const vec_t *org, const vec_t *xPixelStep, const vec_t *yPixelStep, D3DCOLOR color);
extern void R_AddDebugString(DebugGlobals *debugGlobalsEntry, const vec_t *origin, const vec_t *color, float scale, const char *string);
extern const char *va(const char *fmt, ...);

static int RB_AddDebugLine(const vec_t *start, const vec_t *end, const vec_t *color, int depthTest, int vertCount, int vertLimit, GfxPointVertex *verts);
static void RB_DrawDebugStrings(trDebugString_t *strings, int stringCount);
static void RB_DrawPolyOutlines(void);
static void RB_DrawDebugLines(trDebugLine_t *lines, int lineCount);
static void RB_DrawPolyInteriors(void);
void RB_DrawDebug(const GfxViewParms *viewParms);

static int RB_AddDebugLine(const vec_t *start, const vec_t *end, const vec_t *color, int depthTest, int vertCount, int vertLimit, GfxPointVertex *verts)
{
    int newVertCount = vertCount + 2;

    if (newVertCount > vertLimit) {

        RB_DrawLines3D(vertCount / 2, 1, verts, depthTest);
        vertCount = 0;
        newVertCount = 2;
    }

    {
        GfxPointVertex *v0 = &verts[vertCount];
        GfxPointVertex *v1 = &verts[vertCount + 1];

        R_ConvertColorToBytes(color, v0->color);

        *(int *)v1->color = *(int *)v0->color;

        v0->xyz[0] = start[0];
        v0->xyz[1] = start[1];
        v0->xyz[2] = start[2];

        v1->xyz[0] = end[0];
        v1->xyz[1] = end[1];
        v1->xyz[2] = end[2];
    }

    return newVertCount;
}

static void RB_DrawDebugStrings(trDebugString_t *strings, int stringCount)
{
    int stringIndex;
    materialCommands_t *tess;

    if (stringCount == 0) {
        return;
    }

    if (backEnd.projection2D != 0) {
        RB_Set3D();
    }

    if (stringCount > 0) {
        for (stringIndex = 0; stringIndex < stringCount; stringIndex++) {
            trDebugString_t *s = &strings[stringIndex];
            byte colorBytes[4];
            vec3_t xStep, yStep;

            R_ConvertColorToBytes(s->color, colorBytes);

            float scale = -s->scale;

            const GfxViewParms *vp = backEnd.viewParms;

            xStep[0] = scale * vp->axis[1][0];
            xStep[1] = scale * vp->axis[1][1];
            xStep[2] = scale * vp->axis[1][2];

            yStep[0] = scale * vp->axis[2][0];
            yStep[1] = scale * vp->axis[2][1];
            yStep[2] = scale * vp->axis[2][2];

            RB_DrawTextInSpace(
                s->text,
                backEnd.debugFont,
                s->xyz,
                xStep,
                yStep,
                *(D3DCOLOR *)colorBytes);
        }
    }

    tess = (materialCommands_t *)tess_g;
    if (tess->indexCount != 0 || tess->optimizedIndexCount != 0) {
        RB_EndSurface();
    }

    return;
}

static void RB_DrawPolyOutlines(void)
{
    GfxPointVertex *verts;
    int polyIndex;
    int vertCount = 0;
    int polyCount;

    verts = (GfxPointVertex *)ri.Z_MallocInternal(0xaa50);

    DebugGlobals *dbg = &backEndData->debugGlobals;
    polyCount = dbg->polyCount;

    if (polyCount <= 0) {
        ri.Z_FreeInternal(verts);
        return;
    }

    for (polyIndex = 0; polyIndex < polyCount; polyIndex++) {
        vec3_t *polyVerts;
        int polyVertCount;
        int vertIndex;
        int lastVert;

        dbg = &backEndData->debugGlobals;
        GfxDebugPoly *poly = &dbg->polys[polyIndex];

        polyVerts = &dbg->verts[poly->firstVert];

        polyVertCount = poly->vertCount;
        if (polyVertCount <= 0) {
            continue;
        }

        lastVert = polyVertCount - 1;
        {
            vec3_t *currentEdgeStart = &polyVerts[lastVert];
            for (vertIndex = 0; vertIndex < polyVertCount; vertIndex++) {
                vertCount = RB_AddDebugLine(
                    (const vec_t *)currentEdgeStart,
                    (const vec_t *)&polyVerts[vertIndex],
                    (const vec_t *)poly->color,
                    0,
                    vertCount,
                    0xaa5,
                    verts);
                currentEdgeStart = &polyVerts[vertIndex];
            }
        }
    }

    {
        int lineCount = vertCount / 2;
        if (lineCount != 0) {
            RB_DrawLines3D(lineCount, 1, verts, 0);
        }
    }

    ri.Z_FreeInternal(verts);

    return;
}

static void RB_DrawDebugLines(trDebugLine_t *lines, int lineCount)
{
    GfxPointVertex *verts;
    int depthTest;
    int lineIndex;
    int vertCount;
    int lineDepthTest;

    if (lineCount == 0) {
        return;
    }

    if (backEnd.projection2D != 0) {
        RB_Set3D();
    }

    verts = (GfxPointVertex *)ri.Z_MallocInternal(0xaa50);

    depthTest = (lines->depthTest != 0) ? 1 : 0;

    if (lineCount > 0) {
        trDebugLine_t *line = lines;
        lineIndex = 0;
        vertCount = 0;

        while (lineIndex < lineCount) {
            lineDepthTest = (line->depthTest != 0) ? 1 : 0;

            if (lineDepthTest != depthTest) {

                int count = vertCount / 2;
                if (count != 0) {
                    RB_DrawLines3D(count, 1, verts, depthTest);
                }
                depthTest = lineDepthTest;
                vertCount = 0;
            }

            vertCount = RB_AddDebugLine(
                line->start,
                line->end,
                line->color,
                lineDepthTest,
                vertCount,
                0xaa5,
                verts);

            lineIndex++;
            line++;
        }
    } else {
        lineDepthTest = depthTest;
        vertCount = 0;
    }

    {
        int count = vertCount / 2;
        if (count != 0) {
            RB_DrawLines3D(count, 1, verts, lineDepthTest);
        }
    }

    ri.Z_FreeInternal(verts);

    return;
}

static void RB_DrawPolyInteriors(void)
{
    GfxBackEndData *data;
    byte *drawSurf;
    int polyCount;
    int polyIndex;

    RB_BeginSurface(rgp.whiteMaterial, (MaterialTechniqueType)(3), 0);

    data = backEndData;
    polyCount = data->debugGlobals.polyCount;

    if (polyCount <= 0) {
        RB_EndSurface();
        return;
    }

    drawSurf = (byte *)imp_tess;

    for (polyIndex = 0; polyIndex < polyCount; polyIndex++) {
        GfxDebugPoly *poly;
        vec3_t *polyVerts;
        int polyVertCount;
        int indexCount;
        int vertIndex;
        D3DCOLOR colorBytes;

        poly = &data->debugGlobals.polys[polyIndex];

        {
            vec3_t *vertsBase = data->debugGlobals.verts;
            polyVerts = &vertsBase[poly->firstVert];
        }

        R_ConvertColorToBytes((const vec_t *)poly->color, (byte *)&colorBytes);

        polyVertCount = poly->vertCount;
        indexCount = polyVertCount * 3 - 6;

        {
            materialCommands_t *tess = (materialCommands_t *)drawSurf;
            int newVertTotal = polyVertCount + tess->vertexCount;
            int newIndexTotal = indexCount + tess->indexCount;

            if (newVertTotal > 0x154a || newIndexTotal > 0x100000) {
                int oldBatch = tess->declType;

                RB_EndSurface();
                RB_BeginSurface(
                    tess->material,
                    tess->techType,
                    tess->lmapIndex);

                tess = (materialCommands_t *)drawSurf;
                if (oldBatch != tess->declType) {

                    if (tess->indexCount != 0 || tess->optimizedIndexCount != 0) {
                        RB_EndSurface();
                    }
                    drawSurf = (byte *)imp_tess;
                    tess = (materialCommands_t *)drawSurf;
                    tess->declType = (MaterialVertexDeclType)(oldBatch);
                }
            }
        }

        if (polyVertCount > 0) {
            int surfaceType = r_rendererInUse->current.integer;
            vec3_t *pv = polyVerts;

            for (vertIndex = 0; vertIndex < polyVertCount; vertIndex++) {
                int baseVert = vertIndex + ((materialCommands_t *)drawSurf)->vertexCount;

                if (surfaceType == 2) {

                    int vertOff = baseVert * 9 * 4;
                    byte *to = drawSurf + vertOff;

                    *(float *)(to + 0) = (*pv)[0];
                    *(float *)(to + 4) = (*pv)[1];
                    *(float *)(to + 8) = (*pv)[2];

                    ((GfxVertexDx7 *)to)->normal[0] = 0.0f;
                    ((GfxVertexDx7 *)to)->normal[1] = 0.0f;
                    ((GfxVertexDx7 *)to)->normal[2] = 1.0f;

                    ((GfxVertexDx7 *)to)->color.packed = colorBytes;

                    ((GfxVertexDx7 *)to)->texCoord[0] = 0.0f;
                    ((GfxVertexDx7 *)to)->texCoord[1] = 0.0f;
                } else {

                    int vertOff = baseVert * 64;
                    byte *to = drawSurf + vertOff;

                    *(float *)(to + 0) = (*pv)[0];
                    *(float *)(to + 4) = (*pv)[1];
                    *(float *)(to + 8) = (*pv)[2];

                    ((GfxVertex *)to)->xyzw[3] = 1.0f;

                    ((GfxVertex *)to)->normal[0] = 0.0f;
                    ((GfxVertex *)to)->normal[1] = 0.0f;
                    ((GfxVertex *)to)->normal[2] = 1.0f;

                    ((GfxVertex *)to)->color.packed = colorBytes;

                    ((GfxVertex *)to)->texCoord[0] = 0.0f;
                    ((GfxVertex *)to)->texCoord[1] = 0.0f;

                    ((GfxVertex *)to)->binormal[0] = 0.0f;

                    ((GfxVertex *)to)->tangent[0] = 1.0f;
                    ((GfxVertex *)to)->tangent[1] = 0.0f;
                    ((GfxVertex *)to)->tangent[2] = 0.0f;
                }

                pv++;
            }
        }

        if (polyVertCount > 2) {
            for (vertIndex = 2; vertIndex < polyVertCount; vertIndex++) {
                materialCommands_t *tess = (materialCommands_t *)drawSurf;
                r_index_t *indices;
                int indexBase;
                r_index_t baseVertIdx;

                indexBase = tess->indexCount;
                indices = tess->indices;
                baseVertIdx = (r_index_t)tess->vertexCount;
                indices[indexBase] = baseVertIdx;

                indexBase = tess->indexCount;
                indices = tess->indices;
                indices[indexBase + 1] = (r_index_t)(vertIndex + tess->vertexCount);

                indexBase = tess->indexCount;
                indices = tess->indices;
                indices[indexBase + 2] = (r_index_t)(vertIndex + tess->vertexCount - 1);

                tess->indexCount += 3;
            }
        }

        ((materialCommands_t *)drawSurf)->vertexCount += polyVertCount;

        data = backEndData;
        drawSurf = (byte *)imp_tess;
    }

    RB_EndSurface();

    return;
}

void RB_DrawDebug(const GfxViewParms *viewParms)
{
    GfxBackEndData *data;
    int plumeIndex;
    int plumeCount;

    data = backEndData;
    {
        DebugGlobals *dbg = &data->debugGlobals;
        plumeCount = dbg->plumeCount;
    }

    if (plumeCount > 0) {
        const vec_t *dir = viewParms->axis[1];
        for (plumeIndex = 0; plumeIndex < plumeCount; plumeIndex++) {
            GfxDebugPlume *plume;
            int time;
            int startTime;
            int duration;
            int elapsed;
            vec3_t org;

            data = backEndData;
            plume = &data->debugGlobals.plumes[plumeIndex];

            time = backEnd.sceneDef.time;

            startTime = plume->startTime;
            elapsed = time - startTime;

            if (elapsed < 0) {
                continue;
            }

            duration = plume->duration;
            if (elapsed > duration) {
                continue;
            }

            plume->color[3] = 1.0f;

            data = backEndData;
            {
                GfxDebugPlume *plumeData = &data->debugGlobals.plumes[plumeIndex];

                if (elapsed * 2 > duration) {

                    float fElapsed = (float)elapsed;
                    float fDuration = (float)duration;
                    plumeData->color[3] = fElapsed * -2.0f / fDuration + 2.0f;
                }
            }

            {
                double angle = (double)elapsed * 0.012566370614359173 + (double)plumeIndex;
                float sinVal = (float)sin(angle);
                float height = sinVal * 4.0f;

                data = backEndData;
                {
                    GfxDebugPlume *plumeOrigin = &data->debugGlobals.plumes[plumeIndex];

                    org[0] = height * viewParms->axis[1][0] + plumeOrigin->origin[0];
                    org[1] = height * viewParms->axis[1][1] + plumeOrigin->origin[1];
                    org[2] = height * viewParms->axis[1][2] + plumeOrigin->origin[2];

                    org[2] = (float)elapsed * 0.06400000303983688f + org[2];
                }

                data = backEndData;
                {
                    GfxDebugPlume *plumeData2 = &data->debugGlobals.plumes[plumeIndex];
                    const char *scoreStr = va("%i", plumeData2->score);

                    R_AddDebugString(
                        &data->debugGlobals,
                        org,
                        (const vec_t *)plumeData2->color,
                        0.5f,
                        scoreStr);
                }
            }
        }
    }

    data = backEndData;
    if (data->debugGlobals.polyCount != 0) {
        if (backEnd.projection2D != 0) {
            RB_Set3D();
        }
        RB_DrawPolyInteriors();
        RB_DrawPolyOutlines();

        data = backEndData;
        data->debugGlobals.polyCount = 0;
    }

    data = backEndData;
    {
        DebugGlobals *dbg = &data->debugGlobals;
        RB_DrawDebugLines(dbg->lines, dbg->lineCount);
    }

    data = backEndData;
    {
        DebugGlobals *dbg = &data->debugGlobals;
        RB_DrawDebugLines(dbg->externLines, dbg->externLineCount);
    }

    data = backEndData;
    data->debugGlobals.lineCount = 0;

    data = backEndData;
    {
        DebugGlobals *dbg = &data->debugGlobals;
        RB_DrawDebugStrings(dbg->strings, dbg->stringCount);
    }

    data = backEndData;
    {
        DebugGlobals *dbg = &data->debugGlobals;
        RB_DrawDebugStrings(dbg->externStrings, dbg->externStringCount);
    }

    return;
}

#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

typedef struct GfxSurfaceInternal {
    const Material *material;
    unsigned short lightmapIndex;
    unsigned short sortGroup;
    srfTriangles_t *tris;
} GfxSurfaceInternal;

typedef struct ClipPoint {
    vec3_t xyz;
    vec2_t lmapCoord;
    vec3_t normal;
} ClipPoint;

extern r_global_permanent_t rgp;
extern r_globals_t rg; /* "tr" was a 64-byte stub; the renderer's globals are rg */

extern int BoxOnPlaneSide(const vec_t *mins, const vec_t *maxs, const cplane_t *plane);
extern void ClearBounds(vec_t *mins, vec_t *maxs);
extern void AddPointToBounds(const vec_t *point, vec_t *mins, vec_t *maxs);
extern void Vec3Cross(const vec_t *a, const vec_t *b, vec_t *out);
extern const vec_t Vec3Normalize(vec_t *v);

#define R_MARKS_REGPARM3_ABI COD2_REGPARM(3)

static void R_MARKS_REGPARM3_ABI R_AABBTreeSurfaces_r(GfxAabbTree *tree, const vec_t *mins, const vec_t *maxs,
                                                      GfxSurfaceInternal **surfList, int surfListSize, int *surfCount);
static void R_MARKS_REGPARM3_ABI R_CellSurfaces(GfxCell *cell, const vec_t *mins, const vec_t *maxs,
                                                GfxSurfaceInternal **surfList, int surfListSize, int *surfCount);
static void R_MARKS_REGPARM3_ABI R_BoxSurfaces_r(mnode_t *node, const vec_t *mins, const vec_t *maxs,
                                                 GfxSurfaceInternal **surfList, int surfListSize, int *surfCount);
int R_MarkFragments(vec3_t *points, const vec_t *origin, vec3_t *axis, float radius,
                    int maxPoints, GfxWorldVertex *verts, int maxFragments,
                    GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial);

static void R_MARKS_REGPARM3_ABI R_AABBTreeSurfaces_r(GfxAabbTree *tree, const vec_t *mins, const vec_t *maxs,
                                                      GfxSurfaceInternal **surfList, int surfListSize, int *surfCount)
{
    GfxWorld *world;
    GfxSurfaceInternal *surfNodes;
    int surfNodeIndex;
    int childIndex;

    if (mins[0] > tree->maxs[0])
        return;
    if (tree->mins[0] > maxs[0])
        return;
    if (mins[1] > tree->maxs[1])
        return;
    if (tree->mins[1] > maxs[1])
        return;
    if (mins[2] > tree->maxs[2])
        return;
    if (tree->mins[2] > maxs[2])
        return;

    if (tree->childCount == 0) {

        world = rgp.world;
        surfNodes = (GfxSurfaceInternal *)world->surfaces + tree->startSurfIndex;

        if (tree->surfaceCount <= 0)
            return;
        if (*surfCount >= surfListSize)
            return;

        for (surfNodeIndex = 0; surfNodeIndex < tree->surfaceCount; surfNodeIndex++) {
            GfxSurfaceInternal *surf = &surfNodes[surfNodeIndex];
            srfTriangles_t *tris;

            if (surf->material->info.gameFlags & 4)
                goto next_surf;

            tris = surf->tris;

            if (mins[0] > tris->bounds[1][0])
                goto next_surf;
            if (tris->bounds[0][0] > maxs[0])
                goto next_surf;
            if (mins[1] > tris->bounds[1][1])
                goto next_surf;
            if (tris->bounds[0][1] > maxs[1])
                goto next_surf;
            if (mins[2] > tris->bounds[1][2])
                goto next_surf;
            if (tris->bounds[0][2] > maxs[2])
                goto next_surf;

            surfList[*surfCount] = surf;
            (*surfCount)++;

        next_surf:
            if (*surfCount >= surfListSize)
                return;
        }
    } else if (tree->childCount > 0) {

        GfxAabbTree *children = (GfxAabbTree *)(unsigned long)tree->children;
        for (childIndex = 0; childIndex < tree->childCount; childIndex++) {
            R_AABBTreeSurfaces_r(&children[childIndex], mins, maxs,
                                 surfList, surfListSize, surfCount);
        }
    }
}

static void R_MARKS_REGPARM3_ABI R_CellSurfaces(GfxCell *cell, const vec_t *mins, const vec_t *maxs,
                                                GfxSurfaceInternal **surfList, int surfListSize, int *surfCount)
{
    GfxWorld *world;
    int cullGroupIndex;
    GfxAabbTree *tree;

    if (cell->writable.markCount == rg.markCount)
        return;
    cell->writable.markCount = rg.markCount;

    world = rgp.world;

    if (cell->cullGroupCount > 0) {
        for (cullGroupIndex = 0; cullGroupIndex < cell->cullGroupCount; cullGroupIndex++) {
            GfxCullGroup *cg;
            GfxSurfaceInternal *surfNodes;
            int triSurfIndex;

            cg = &world->cullGroups[cell->cullGroups[cullGroupIndex]];

            if (mins[0] > cg->maxs[0])
                continue;
            if (cg->mins[0] > maxs[0])
                continue;
            if (mins[1] > cg->maxs[1])
                continue;
            if (cg->mins[1] > maxs[1])
                continue;
            if (mins[2] > cg->maxs[2])
                continue;
            if (cg->mins[2] > maxs[2])
                continue;

            surfNodes = (GfxSurfaceInternal *)world->surfaces + cg->startSurfIndex;
            if (cg->surfaceCount <= 0)
                continue;
            if (*surfCount >= surfListSize)
                continue;

            for (triSurfIndex = 0; triSurfIndex < cg->surfaceCount; triSurfIndex++) {
                GfxSurfaceInternal *surf = &surfNodes[triSurfIndex];
                srfTriangles_t *tris;
                int listIndex;

                if (surf->material->info.gameFlags & 4)
                    goto next_cg_surf;

                tris = surf->tris;

                if (mins[0] > tris->bounds[1][0])
                    goto next_cg_surf;
                if (tris->bounds[0][0] > maxs[0])
                    goto next_cg_surf;
                if (mins[1] > tris->bounds[1][1])
                    goto next_cg_surf;
                if (tris->bounds[0][1] > maxs[1])
                    goto next_cg_surf;
                if (mins[2] > tris->bounds[1][2])
                    goto next_cg_surf;
                if (tris->bounds[0][2] > maxs[2])
                    goto next_cg_surf;

                for (listIndex = 0; listIndex < *surfCount; listIndex++) {
                    if (surfList[listIndex] == surf)
                        goto next_cg_surf;
                }

                surfList[*surfCount] = surf;
                (*surfCount)++;

            next_cg_surf:
                if (*surfCount >= surfListSize)
                    break;
            }
        }
    }

    tree = cell->aabbTree;
    R_AABBTreeSurfaces_r(tree, mins, maxs, surfList, surfListSize, surfCount);
}

static void R_MARKS_REGPARM3_ABI R_BoxSurfaces_r(mnode_t *node, const vec_t *mins, const vec_t *maxs,
                                                 GfxSurfaceInternal **surfList, int surfListSize, int *surfCount)
{
    GfxWorld *world;
    GfxCell *cell;

    while (1) {
        if (node->cellIndex == -2) {

            int side = BoxOnPlaneSide(mins, maxs, node->u.node.plane);
            if (side == 1) {
                node = node->u.node.children[0];
            } else if (side == 2) {
                node = node->u.node.children[1];
            } else {

                R_BoxSurfaces_r(node->u.node.children[0], mins, maxs,
                                surfList, surfListSize, surfCount);
                node = node->u.node.children[1];
            }
        } else if (node->cellIndex == -1) {

            return;
        } else {

            world = rgp.world;
            cell = &world->cells[node->cellIndex];
            R_CellSurfaces(cell, mins, maxs, surfList, surfListSize, surfCount);
            return;
        }
    }
}

int R_MarkFragments(vec3_t *points, const vec_t *origin, vec3_t *axis, float radius,
                    int maxPoints, GfxWorldVertex *verts, int maxFragments,
                    GfxMarkFragment *fragmentBuffer, MaterialHandle markMaterial)
{
    vec3_t mins, maxs;
    vec3_t xyz;
    vec3_t edge01, edge21;
    vec3_t scaledNormal;
    float axisScaled[3];
    int surfCount;
    int returnedPoints;
    int returnedFragments;
    GfxSurfaceInternal *surfaces[4300];
    GfxWorld *world;
    mnode_t *node;
    int pointIndex;
    int surfIndex;
    float texCoordScale;

    vec3_t normals[6];

    float planeDists[8];

    ClipPoint clipPoints[2][9];

    ClearBounds(mins, maxs);

    axisScaled[0] = radius * (*axis)[0];
    axisScaled[1] = radius * (*axis)[1];
    axisScaled[2] = radius * (*axis)[2];

    for (pointIndex = 0; pointIndex < 4; pointIndex++) {
        float *from = (float *)points + pointIndex * 3;

        xyz[0] = axisScaled[0] + from[0];
        xyz[1] = axisScaled[1] + from[1];
        xyz[2] = axisScaled[2] + from[2];
        AddPointToBounds(xyz, mins, maxs);

        xyz[0] = from[0] - axisScaled[0];
        xyz[1] = from[1] - axisScaled[1];
        xyz[2] = from[2] - axisScaled[2];
        AddPointToBounds(xyz, mins, maxs);
    }

    rg.markCount++;

    surfCount = 0;
    world = rgp.world;
    node = world->nodes;

    while (1) {
        if (node->cellIndex == -2) {
            int side = BoxOnPlaneSide(mins, maxs, node->u.node.plane);
            if (side == 1) {
                node = node->u.node.children[0];
            } else if (side == 2) {
                node = node->u.node.children[1];
            } else {
                R_BoxSurfaces_r(node->u.node.children[0], mins, maxs,
                                surfaces, 4096, &surfCount);
                node = node->u.node.children[1];
            }
        } else if (node->cellIndex == -1) {
            break;
        } else {
            GfxCell *cell = &world->cells[node->cellIndex];
            R_CellSurfaces(cell, mins, maxs, surfaces, 4096, &surfCount);
            break;
        }
    }

    {
        float *pts = (float *)points;
        for (pointIndex = 0; pointIndex < 4; pointIndex++) {
            int next = (pointIndex + 1) % 4;
            float *from = &pts[pointIndex * 3];
            float *to = &pts[next * 3];

            edge01[0] = to[0] - from[0];
            edge01[1] = to[1] - from[1];
            edge01[2] = to[2] - from[2];

            Vec3Cross(edge01, (vec_t *)axis, normals[pointIndex]);
            Vec3Normalize(normals[pointIndex]);

            planeDists[pointIndex + 1] = normals[pointIndex][0] * from[0] + normals[pointIndex][1] * from[1] + normals[pointIndex][2] * from[2];
        }
    }

    normals[4][0] = (*axis)[0];
    normals[4][1] = (*axis)[1];
    normals[4][2] = (*axis)[2];

    {
        float *firstPt = (float *)points;
        planeDists[5] = normals[4][0] * firstPt[0] + normals[4][1] * firstPt[1] + normals[4][2] * firstPt[2] - radius;
    }

    normals[5][0] = -normals[4][0];
    normals[5][1] = -normals[4][1];
    normals[5][2] = -normals[4][2];

    {
        float *firstPt = (float *)points;
        planeDists[6] = normals[5][0] * firstPt[0] + normals[5][1] * firstPt[1] + normals[5][2] * firstPt[2] - radius;
    }

    texCoordScale = 0.5f / radius;

    if (surfCount <= 0) {
        return 0;
    }

    returnedPoints = 0;
    returnedFragments = 0;

    for (surfIndex = 0; surfIndex < surfCount; surfIndex++) {
        GfxSurfaceInternal *surfNode = surfaces[surfIndex];
        srfTriangles_t *tris;
        GfxWorldVertex *triVerts;
        int triIndex;

        if (surfNode->material->stateBits[1] & 0x30)
            continue;

        tris = surfNode->tris;
        triVerts = world->vd.vertices + tris->firstVertex;

        if (tris->indexCount == 0)
            continue;

        for (triIndex = 0; triIndex < tris->indexCount; triIndex += 3) {
            unsigned short *indices;
            float dot, lenSq;
            int v;
            int planeIdx;
            int numClipPoints;
            int pingPong;
            GfxWorldVertex *outVerts;
            int curMaxPoints;
            GfxMarkFragment *curFragment;

            indices = (unsigned short *)tris->indices + triIndex;

            {
                GfxWorldVertex *vert0 = &triVerts[indices[0]];
                GfxWorldVertex *vert1 = &triVerts[indices[1]];
                edge01[0] = vert0->xyz[0] - vert1->xyz[0];
                edge01[1] = vert0->xyz[1] - vert1->xyz[1];
                edge01[2] = vert0->xyz[2] - vert1->xyz[2];
            }

            {
                GfxWorldVertex *vert2 = &triVerts[indices[2]];
                GfxWorldVertex *vert1 = &triVerts[indices[1]];
                edge21[0] = vert2->xyz[0] - vert1->xyz[0];
                edge21[1] = vert2->xyz[1] - vert1->xyz[1];
                edge21[2] = vert2->xyz[2] - vert1->xyz[2];
            }

            Vec3Cross(edge01, edge21, scaledNormal);

            dot = scaledNormal[0] * (*axis)[0] + scaledNormal[1] * (*axis)[1] + scaledNormal[2] * (*axis)[2];

            if (dot < 0.0f)
                continue;

            lenSq = scaledNormal[0] * scaledNormal[0] + scaledNormal[1] * scaledNormal[1] + scaledNormal[2] * scaledNormal[2];
            if (dot * dot < 0.25f * lenSq)
                continue;

            for (v = 0; v < 3; v++) {
                GfxWorldVertex *vert = &triVerts[indices[v]];
                clipPoints[0][v].xyz[0] = vert->xyz[0];
                clipPoints[0][v].xyz[1] = vert->xyz[1];
                clipPoints[0][v].xyz[2] = vert->xyz[2];
                clipPoints[0][v].lmapCoord[0] = vert->lmapCoord[0];
                clipPoints[0][v].lmapCoord[1] = vert->lmapCoord[1];
                clipPoints[0][v].normal[0] = vert->normal[0];
                clipPoints[0][v].normal[1] = vert->normal[1];
                clipPoints[0][v].normal[2] = vert->normal[2];
            }

            outVerts = &verts[returnedPoints];
            curMaxPoints = maxPoints - returnedPoints;
            pingPong = 0;
            numClipPoints = 3;

            for (planeIdx = 1; planeIdx <= 6; planeIdx++) {
                float dists[10];
                int sides[10];
                int sideCount[3];
                int j, k;
                float planeDist;
                float *planeNorm;
                int outCount;
                ClipPoint *inPts, *outPts;

                planeNorm = normals[planeIdx - 1];
                planeDist = planeDists[planeIdx];

                inPts = clipPoints[pingPong];
                outPts = clipPoints[pingPong ^ 1];

                sideCount[0] = 0;
                sideCount[1] = 0;
                sideCount[2] = 0;

                for (j = 0; j < numClipPoints; j++) {
                    float d = planeNorm[0] * inPts[j].xyz[0] + planeNorm[1] * inPts[j].xyz[1] + planeNorm[2] * inPts[j].xyz[2] - planeDist;
                    dists[j] = d;
                    if (d > 0.5f) {
                        sides[j] = 0;
                    } else if (d >= -0.5f) {
                        sides[j] = 2;
                    } else {
                        sides[j] = 1;
                    }
                    sideCount[sides[j]]++;
                }

                dists[numClipPoints] = dists[0];
                sides[numClipPoints] = sides[0];

                if (sideCount[0] == 0) {

                    numClipPoints = 0;
                    break;
                }

                if (sideCount[1] == 0) {

                    memcpy(outPts, inPts, numClipPoints * sizeof(ClipPoint));
                    goto next_plane;
                }

                outCount = 0;
                for (j = 0; j < numClipPoints; j++) {
                    k = j + 1;

                    if (sides[j] == 2) {

                        outPts[outCount] = inPts[j];
                        outCount++;
                        goto next_clip_point;
                    } else if (sides[j] == 0) {

                        outPts[outCount] = inPts[j];
                        outCount++;
                    }

                    if (sides[k] != 2 && sides[k] != sides[j]) {

                        float frac = dists[j] / (dists[j] - dists[k]);
                        int to_idx = k % numClipPoints;
                        float *fromF = (float *)&inPts[j];
                        float *toF = (float *)&inPts[to_idx];
                        float *outF = (float *)&outPts[outCount];
                        int c;
                        for (c = 0; c < 8; c++) {
                            outF[c] = fromF[c] + frac * (toF[c] - fromF[c]);
                        }
                        outCount++;
                    }

                next_clip_point:;
                }
                numClipPoints = outCount;

            next_plane:
                pingPong ^= 1;
                if (numClipPoints == 0)
                    break;
            }

            if (numClipPoints == 0 || pingPong < 0)
                continue;

            if (numClipPoints > curMaxPoints)
                continue;

            curFragment = &fragmentBuffer[returnedFragments];
            curFragment->pointCount = (byte)numClipPoints;

            if (curFragment->pointCount != 0) {
                ClipPoint *finalPts = clipPoints[pingPong];

                for (v = 0; v < numClipPoints; v++) {
                    GfxWorldVertex *outV = &outVerts[v];
                    outV->xyz[0] = finalPts[v].xyz[0];
                    outV->xyz[1] = finalPts[v].xyz[1];
                    outV->xyz[2] = finalPts[v].xyz[2];
                    outV->lmapCoord[0] = finalPts[v].lmapCoord[0];
                    outV->lmapCoord[1] = finalPts[v].lmapCoord[1];
                    outV->normal[0] = finalPts[v].normal[0];
                    outV->normal[1] = finalPts[v].normal[1];
                    outV->normal[2] = finalPts[v].normal[2];
                }

                for (v = 0; v < (int)curFragment->pointCount; v++) {
                    GfxWorldVertex *outV = &outVerts[v];
                    float dx, dy, dz;
                    float u, t;

                    dx = outV->xyz[0] - origin[0];
                    dy = outV->xyz[1] - origin[1];
                    dz = outV->xyz[2] - origin[2];

                    u = (dx * (*axis)[3] + dy * (*axis)[4] + dz * (*axis)[5]) * texCoordScale + 0.5f;
                    outV->texCoord[0] = u;

                    t = (dx * (*axis)[6] + dy * (*axis)[7] + dz * (*axis)[8]) * texCoordScale + 0.5f;
                    outV->texCoord[1] = t;

                    Vec3Cross(outV->normal, &(*axis)[3], outV->binormal);

                    Vec3Cross(outV->binormal, outV->normal, outV->tangent);
                }
            }

            curFragment->markMaterial = markMaterial;
            curFragment->lmapIndex = surfNode->lightmapIndex;
            curFragment->firstPoint = returnedPoints;

            returnedPoints += curFragment->pointCount;
            returnedFragments++;

            if (returnedFragments == maxFragments)
                return returnedFragments;

            if (returnedPoints > maxPoints - 3)
                return returnedFragments;
        }
    }

    return returnedFragments;
}

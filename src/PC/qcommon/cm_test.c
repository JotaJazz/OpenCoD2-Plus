#include "common_types.h"
#include "imports.h"

extern clipMap_t cm;

extern int BoxOnPlaneSide(const vec_t *mins, const vec_t *maxs, const cplane_t *plane);
extern cmodel_t *CM_ClipHandleToModel(clipHandle_t handle);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);

byte *CM_ClusterPVS(int cluster);
void CM_BoxLeafnums_r(leafList_t *ll, int nodenum);
int CM_BoxLeafnums(const vec_t *mins, const vec_t *maxs, int *list, int listsize, int *lastLeaf);
int CM_PointLeafnum(const vec_t *p);
static int CM_PointContentsLeafBrushNode_r(cLeafBrushNode_t *node);
int CM_PointContents(const vec_t *p, clipHandle_t model);
int CM_TransformedPointContents(const vec_t *p, clipHandle_t model, const vec_t *origin, const vec_t *angles);

byte *CM_ClusterPVS(int cluster)
{
    if (cluster < 0)
        return cm.visibility;

    if (cluster < cm.numClusters) {
        if (cm.vised)
            return cm.visibility + cluster * cm.clusterBytes;
    }
    return cm.visibility;
}

static int CM_PointContentsLeafBrushNode_r_impl(const vec_t *p, cLeafBrushNode_t *node)
{
    int contents;
    int k;
    short int leafBrushCount;
    unsigned short *brushes;
    cbrush_t *brushBase;
    cbrush_t *b;
    int childIdx;

    contents = 0;

    for (;;) {
        leafBrushCount = node->leafBrushCount;

        if (leafBrushCount == 0) {

            float val = p[node->axis];
            if (node->data.children.dist >= val) {
                childIdx = node->data.children.childOffset[1];
            } else {
                childIdx = node->data.children.childOffset[0];
            }
            node = node + childIdx;
            continue;
        }

        if (leafBrushCount < 0) {

            contents |= CM_PointContentsLeafBrushNode_r_impl(p, node + 1);

            {
                float val = p[node->axis];
                if (node->data.children.dist >= val) {
                    childIdx = node->data.children.childOffset[1];
                } else {
                    childIdx = node->data.children.childOffset[0];
                }
                node = node + childIdx;
            }
            continue;
        }

        {
            int numBrushes = (int)leafBrushCount;
            if (numBrushes <= 0) {
                break;
            }

            brushes = node->data.leaf.brushes;
            brushBase = cm.brushes;

            for (k = 0; k < numBrushes; k++) {
                int brushIdx = brushes[k];
                b = &brushBase[brushIdx];

                {
                    int j;
                    float *bptr = (float *)b;
                    int outside = 0;
                    for (j = 0; j < 3; j++) {
                        if (bptr[j] > p[j]) {
                            outside = 1;
                            break;
                        }
                        if (p[j] > bptr[4 + j]) {
                            outside = 1;
                            break;
                        }
                    }
                    if (outside) {
                        continue;
                    }
                }

                {
                    int numSides = b->numsides;
                    if (numSides > 0) {
                        cbrushside_t *sides = b->sides;
                        cplane_t *plane;
                        float dist;
                        int inside = 1;

                        plane = sides[0].plane;
                        dist = p[0] * plane->normal[0] + p[1] * plane->normal[1] + p[2] * plane->normal[2];
                        if (dist > plane->dist) {
                            inside = 0;
                        }

                        if (inside) {
                            int i;
                            for (i = 1; i < numSides; i++) {
                                plane = sides[i].plane;
                                dist = p[0] * plane->normal[0] + p[1] * plane->normal[1] + p[2] * plane->normal[2];
                                if (dist > plane->dist) {
                                    inside = 0;
                                    break;
                                }
                            }
                        }

                        if (inside) {
                            contents |= b->contents;
                        }
                    } else {
                        contents |= b->contents;
                    }
                }
            }
        }
        break;
    }

    return contents;
}

static int CM_PointContentsLeafBrushNode_r(cLeafBrushNode_t *node)
{

    (void)node;
    return 0;
}

void CM_BoxLeafnums_r(leafList_t *ll, int nodenum)
{
    cNode_t *node;
    int s;

    while (nodenum >= 0) {
        node = &cm.nodes[nodenum];
        s = BoxOnPlaneSide(ll->bounds[0], ll->bounds[1], node->plane);

        if (s == 1) {
            nodenum = node->children[0];
        } else if (s == 2) {
            nodenum = node->children[1];
        } else {

            CM_BoxLeafnums_r(ll, node->children[0]);
            nodenum = node->children[1];
        }
    }

    {
        int leafNum = ~nodenum;
        if (cm.leafs[leafNum].cluster != -1) {
            ll->lastLeaf = leafNum;
        }

        if (ll->count >= ll->maxcount) {
            ll->overflowed = 1;
            return;
        }

        ll->list[ll->count] = leafNum;
        ll->count++;
    }
}

int CM_BoxLeafnums(const vec_t *mins, const vec_t *maxs, int *list, int listsize, int *lastLeaf)
{
    leafList_t ll;

    ll.bounds[0][0] = mins[0];
    ll.bounds[0][1] = mins[1];
    ll.bounds[0][2] = mins[2];
    ll.bounds[1][0] = maxs[0];
    ll.bounds[1][1] = maxs[1];
    ll.bounds[1][2] = maxs[2];
    ll.count = 0;
    ll.maxcount = listsize;
    ll.list = list;
    ll.lastLeaf = 0;
    ll.overflowed = 0;

    CM_BoxLeafnums_r(&ll, 0);

    *lastLeaf = ll.lastLeaf;
    return ll.count;
}

int CM_PointLeafnum(const vec_t *p)
{
    int nodenum;
    cNode_t *nodes;
    cNode_t *node;
    cplane_t *plane;
    float dist;

    nodes = cm.nodes;
    nodenum = 0;

    while (nodenum >= 0) {
        node = &nodes[nodenum];
        plane = node->plane;

        if (plane->type <= 2) {

            dist = p[plane->type] - plane->dist;
        } else {

            dist = plane->normal[0] * p[0] + plane->normal[1] * p[1] + plane->normal[2] * p[2] - plane->dist;
        }

        if (dist < 0.0f) {
            nodenum = node->children[1];
        } else {
            nodenum = node->children[0];
        }
    }

    return ~nodenum;
}

int CM_PointContents(const vec_t *p, clipHandle_t model)
{
    int leafNum;
    cLeaf_t *leaf;
    cLeafBrushNode_t *node;
    int contents;
    int k;
    unsigned short *brushIdxList;
    cbrush_t *brushBase;
    cbrush_t *b;

    if (model != 0) {
        cmodel_t *cmod = CM_ClipHandleToModel(model);
        leaf = &cmod->leaf;
        goto check_leaf;
    }

    {
        int nodenum;
        cNode_t *nodes;
        cNode_t *nd;
        cplane_t *plane;
        float dist;

        nodes = cm.nodes;
        nodenum = 0;

        while (nodenum >= 0) {
            nd = &nodes[nodenum];
            plane = nd->plane;

            if (plane->type <= 2) {
                dist = p[plane->type] - plane->dist;
            } else {
                dist = plane->normal[0] * p[0] + plane->normal[1] * p[1] + plane->normal[2] * p[2] - plane->dist;
            }

            if (dist < 0.0f) {
                nodenum = nd->children[1];
            } else {
                nodenum = nd->children[0];
            }
        }

        leafNum = ~nodenum;
        leaf = &cm.leafs[leafNum];
    }

check_leaf:

    /* a leaf without brushes holds nothing (CoD2rev-Plus's CM_PointContents) */
    if (!leaf->leafBrushNode)
        return 0;

    {
        int i;
        for (i = 0; i < 3; i++) {
            if (leaf->mins[i] >= p[i]) {
                contents = 0;
                return contents;
            }
            if (p[i] >= leaf->maxs[i]) {
                contents = 0;
                return contents;
            }
        }
    }

    {
        int leafBrushNodeIdx = leaf->leafBrushNode;
        node = &cm.leafbrushNodes[leafBrushNodeIdx];
    }

    contents = 0;

    contents = CM_PointContentsLeafBrushNode_r_impl(p, node);

    return contents;
}

int CM_TransformedPointContents(const vec_t *p, clipHandle_t model, const vec_t *origin, const vec_t *angles)
{
    vec3_t p_l;
    vec3_t forward, right, up;
    float tmp0, tmp1, tmp2;

    p_l[0] = p[0] - origin[0];
    p_l[1] = p[1] - origin[1];
    p_l[2] = p[2] - origin[2];

    if (angles[0] != 0.0f || angles[1] != 0.0f || angles[2] != 0.0f) {
        AngleVectors(angles, forward, right, up);

        tmp0 = p_l[0];
        tmp1 = p_l[1];
        tmp2 = p_l[2];

        p_l[0] = tmp0 * forward[0] + tmp1 * forward[1] + tmp2 * forward[2];
        p_l[1] = -(tmp0 * right[0] + tmp1 * right[1] + tmp2 * right[2]);
        p_l[2] = tmp0 * up[0] + tmp1 * up[1] + tmp2 * up[2];
    }

    return CM_PointContents(p_l, model);
}

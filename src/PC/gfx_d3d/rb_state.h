#ifndef PC_GFX_D3D_RB_STATE_H_H
#define PC_GFX_D3D_RB_STATE_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mac/DirectX 9/MacOpenGLUtils.h"

struct GfxCodeMatrices;
struct GfxCodeMatrix;
struct XBlendInfo_s;
struct XRigidSurface;
struct XSurface_s;
struct XVertexBuffer;
struct XVertexInfo_s;

struct GfxCodeMatrices {
    float worldScale;
    struct GfxCodeMatrix world;
    GfxCodeMatrix normalizedWorld;
    GfxCodeMatrix view;
    GfxCodeMatrix projection;
    GfxCodeMatrix worldView;
    GfxCodeMatrix normalizedWorldView;
    GfxCodeMatrix viewProjection;
    GfxCodeMatrix worldViewProjection;
    GfxCodeMatrix normalizedWorldViewProjection;
    GfxCodeMatrix shadowLookupMatrix;
    GfxCodeMatrix lightGridLookupMatrix;
    GfxCodeMatrix worldOutdoorLookup;
    GfxCodeMatrix OGLworldViewProjection;
};

struct GfxCodeMatrix {
    D3DMATRIX matrix[4];
    Bool valid[4];
};

struct XBlendInfo_s {
    vec3_t offset;
    short int boneOffset;
    short unsigned int boneWeight;
};

struct XRigidSurface {
    IDirect3DVertexBuffer9 *vb;
};

struct XSurface_s {
    byte tileMode;
    short int vertCount;
    short int triCount;
    short int boneOffset;
    int (*triIndices)[4];
    XVertexBuffer *verts;
    struct XRigidSurface surfRigid;
    IDirect3DIndexBuffer9 *indexBuffer;
};

struct XVertexBuffer {
    XVertexInfo v;
    XBlendInfo w[1];
};

struct XVertexInfo_s {
    vec3_t normal;
    byte color[4];
    vec3_t binormal;
    float texCoordX;
    vec3_t tangent;
    float texCoordY;
    vec3_t offset;
    byte numWeights;
    byte boneWeight;
    short int boneOffset;
};

#ifdef __cplusplus
}
#endif
#endif

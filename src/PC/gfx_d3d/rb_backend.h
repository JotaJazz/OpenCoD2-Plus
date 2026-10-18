#ifndef PC_GFX_D3D_RB_BACKEND_H_H
#define PC_GFX_D3D_RB_BACKEND_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mac/DirectX 9/MacOpenGLUtils.h"
#include "PC/gfx_d3d/rb_state.h"
#include "PC/universal/com_math.h"
#include "PC/universal/com_vector.h"

struct DebugGlobals;
struct Font_s;
struct GfxBackEndData;
struct GfxBackEndLightSettings;
struct GfxCmdArray;
struct GfxCmdDrawLines;
struct GfxCmdDrawPoints;
struct GfxCmdDrawSprite;
struct GfxCmdDrawTriangles;
struct GfxCmdSetStencilRefValue;
struct GfxCmdStencilPlanes;
union GfxColor;
struct GfxDebugPlume;
struct GfxDebugPoly;
struct GfxEntityLighting;
struct GfxFog;
struct GfxPointVertex;
struct GfxVertex;
struct GfxVertexDx7;
struct GfxWorldVertex;
struct GfxWorldVertexDx7;
struct Glyph;
struct SunFlareDynamic;
struct materialCommands_t;
struct r_backEndGlobals_t;
struct r_vb_state_t;
struct srfPoly_t;
struct trDebugLine_t;
struct trDebugString_t;

enum GfxOptimizedVertexSource {
    OPT_VERTS_NONE = 0,
    OPT_VERTS_WORLD = 1,
    OPT_VERTS_SMC = 2,
    OPT_VERTS_COUNT = 3
};

enum MaterialVertexDeclType {
    VERTDECL_GENERIC = 0,
    VERTDECL_WORLD = 1,
    VERTDECL_POS_TEX = 2,
    VERTDECL_STATICMODELCACHE = 3,
    VERTDECL_COUNT = 4
};

struct DebugGlobals {
    vec3_t *verts;
    int vertCount;
    int vertLimit;
    GfxDebugPoly *polys;
    int polyCount;
    int polyLimit;
    trDebugString_t *strings;
    int stringCount;
    int stringLimit;
    trDebugString_t *externStrings;
    int externStringCount;
    int externMaxStringCount;
    trDebugLine_t *lines;
    int lineCount;
    int lineLimit;
    trDebugLine_t *externLines;
    int externLineCount;
    int externMaxLineCount;
    GfxDebugPlume *plumes;
    int plumeCount;
    int plumeLimit;
};

struct Font_s {
    const char *name;
    int pixelHeight;
    int glyphCount;
    MaterialHandle material;
    Glyph *glyphs;
};

struct GfxBackEndData {
    int frameCount;
    int drawSurfCount;
    int drawSurfs;
    int surfPos;
    byte surfsBuffer[131072];
    int entityCount;
    int entities;
    int polyCount;
    srfPoly_t polys[2048];
    int polyVertCount;
    int polyVerts;
    r_vb_state_t *skinnedCacheVb;
    int viewParmCount;
    GfxViewParms viewParms[25];
    struct GfxFog fogSettings;
    struct GfxCmdArray commands;
    DebugGlobals debugGlobals;
};

struct GfxBackEndLightSettings {
    vec4_t position;
    vec4_t ambient;
    vec4_t color;
    vec4_t specular;
    GfxLightDef *def;
};

struct GfxCmdArray {
    byte cmds[196608];
    int usedTotal;
    int usedCritical;
    GfxCmdHeader *lastCmd;
};

struct GfxCmdDrawLines {
    GfxCmdHeader header;
    short int lineCount;
    short int width;
    GfxPointVertex verts[2];
};

struct GfxCmdDrawPoints {
    GfxCmdHeader header;
    short int pointCount;
    short int size;
    GfxPointVertex verts[1];
};

struct GfxCmdDrawSprite {
    GfxCmdHeader header;
    Material *material;
    GfxColor rgbaColor;
    vec3_t pos;
    float radius;
    float minScreenRadius;
    int renderFxFlags;
};

struct GfxCmdDrawTriangles {
    GfxCmdHeader header;
    const Material *material;
    MaterialTechniqueType techType;
    short int indexCount;
    short int vertexCount;
};

struct GfxCmdSetStencilRefValue {
    GfxCmdHeader header;
    int refValue;
};

struct GfxCmdStencilPlanes {
    GfxCmdHeader header;
    vec_t nearDist;
    int planeCount;
    vec_t planeDists[1];
};

union GfxColor {
    unsigned int packed;
    byte array[4];
};

struct GfxDebugPlume {
    vec3_t origin;
    vec4_t color;
    int score;
    int startTime;
    int duration;
};

struct GfxDebugPoly {
    vec4_t color;
    int firstVert;
    int vertCount;
};

struct GfxEntityLighting {
    int viewCount;
    float sunVisibility;
    vec4_t colorForDir[6];
};

struct GfxFog {
    GfxFogOffset techniqueOffset;
    int startTime;
    int finishTime;
    union GfxColor color;
    float fogStart;
    float fogEnd;
    float density;
    Bool registered;
    Bool drawSky;
    Bool clearScreen;
};

struct GfxPointVertex {
    vec3_t xyz;
    byte color[4];
};

struct GfxVertex {
    vec4_t xyzw;
    vec3_t normal;
    GfxColor color;
    vec2_t texCoord;
    vec3_t binormal;
    vec3_t tangent;
};

struct GfxVertexDx7 {
    vec3_t xyz;
    vec3_t normal;
    GfxColor color;
    vec2_t texCoord;
};

struct GfxWorldVertex {
    vec3_t xyz;
    GfxPackedVector normal;
    union GfxColor color;
    vec2_t texCoord;
    vec2_t lmapCoord;
    GfxPackedVector binormal;
    GfxPackedVector tangent;
};

struct GfxWorldVertexDx7 {
    vec3_t xyz;
    GfxColor color;
    vec2_t texCoord;
    vec2_t lmapCoord;
};

struct Glyph {
    short unsigned int letter;
    signed char x0;
    signed char y0;
    byte dx;
    byte pixelWidth;
    byte pixelHeight;
    float s0;
    float t0;
    float s1;
    float t1;
};

struct SunFlareDynamic {
    float flareIntensity;
    float currentBlind;
    float currentGlare;
    int lastTime;
    Bool error;
    float cumulVisibility;
    float lastVisibility;
    float lastDot;
    int hitNum;
    IDirect3DQuery9 *sunQuery[2];
    Bool sunQueryIssued[2];
};

struct materialCommands_t {
    int verts;
    r_index_t *indices;
    r_index_t *optimizedIndices;
    GfxOptimizedVertexSource optimizedVertexSource;
    const Material *material;
    MaterialTechniqueType techType;
    int lmapIndex;
    float materialTime;
    MaterialVertexDeclType declType;
    int indexCount;
    int vertexCount;
    int firstVertex;
    int lastVertex;
    int optimizedIndexCount;
    int optimizedVertexCount;
    int firstOptimizedVertex;
};

struct r_backEndGlobals_t {
    vec4_t codeConsts[59];
    int frameCount;
    int viewCount;
    GfxSceneDef sceneDef;
    const GfxViewParms *viewParms;
    GfxLodParms lodParms;
    GfxViewport sceneViewport;
    D3DMATRIX viewProjectionMatrix;
    int tileIndex;
    int tileCount;
    int width;
    int height;
    GfxEntity *currentEntity;
    GfxEntityLighting *currentEntityLighting;
    GfxEntity worldEntity;
    Bool viewportIsDirty;
    Bool projection2D;
    GfxSamplerState defaultSamplerState;
    GfxColor color_axis;
    GfxColor color_allies;
    GfxColor fogColor;
    int glowIndexFirst;
    int glowCount;
    GfxImage *glowImage[2];
    float texScrollAmountDx7;
    GfxCodeMatrices codeMatrixStack[3];
    int codeMatrixStackLevel;
    char _pad_backend_2c90[504];
    GfxRenderTargetId resolvedPostSunTarget;
    GfxRenderTargetId resolvedSceneTarget;
    GfxImage *currentFeedbackImage;
    GfxBackEndLightSettings light[2];
    GfxEntityLighting entityLighting[2046];
    D3DMATRIX shadowLookupMatrix;
    FontHandle debugFont;
};

struct r_vb_state_t {
    int used;
    int total;
    void (*buffer)();
};

struct srfPoly_t {
    surfaceType_t surfaceType;
    MaterialHandle material;
    short unsigned int lmapIndex;
    short unsigned int vertCount;
    GfxWorldVertex *verts;
};

struct trDebugLine_t {
    vec3_t start;
    vec3_t end;
    vec4_t color;
    qboolean depthTest;
};

struct trDebugString_t {
    vec3_t xyz;
    vec4_t color;
    float scale;
    char text[96];
};

#ifdef __cplusplus
}
#endif
#endif

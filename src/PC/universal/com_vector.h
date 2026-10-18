#ifndef PC_UNIVERSAL_COM_VECTOR_H_H
#define PC_UNIVERSAL_COM_VECTOR_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mac/DirectX 9/MacOpenGLUtils.h"
#include "PC/gfx_d3d/rb_backend.h"
#include "PC/universal/com_math.h"

struct CollisionAabbTree;
union CollisionAabbTreeIndex;
struct CollisionBorder;
struct CollisionEdge;
struct CollisionPartition;
struct CollisionTriangle;
struct CollisionVertex;
struct DObjSkelMat_s;
union DvarLimits;
union DvarValue;
struct GfxAabbTree;
struct GfxCell;
struct GfxCellWritable;
struct GfxCmdDrawQuadPic;
struct GfxCmdDrawTextInSpace;
struct GfxCmdHeader;
struct GfxCmdStretchPicRotate;
struct GfxCullGroup;
struct GfxDrawPrimArgs;
struct GfxLightGrid;
struct GfxLightGridColors;
struct GfxLightGridPoint;
struct GfxMarkFragment;
struct GfxModelSurface;
struct GfxOccluder;
struct GfxOccluderEdge;
struct GfxPortal;
struct GfxPortalWritable;
struct GfxSModelCachedVertex;
struct GfxSModelCachedVertexDx7;
struct GfxSceneModelCellRef;
struct GfxStaticModelInstance;
struct GfxSurface;
struct GfxWorld;
struct GfxWorldVertexData;
struct MantleResults;
struct MantleState;
struct MaterialMemory;
struct SkinBuffers;
struct SunLightParseParams;
struct XAnimNotify_s;
struct cLeafBrushNodeChildren_t;
union cLeafBrushNodeData_t;
struct cLeafBrushNodeLeaf_t;
struct cLeafBrushNode_s;
struct cNode_t;
struct cStaticModelWritable;
struct cStaticModel_s;
struct cbrush_t;
struct cbrushside_t;
struct centity_s;
struct clientState_s;
struct cmodel_t;
struct corpseInfo_t;
struct cplane_s;
struct dmaterial_t;
struct dvar_s;
struct gameState_t;
union hudelem_color_t;
struct hudelem_s;
struct itemInfo_t;
struct leafList_s;
struct locTraceWork_t;
struct localEntity_s;
union mnode_info_t;
struct mnode_leaf_t;
struct mnode_node_t;
struct mnode_t;
struct orientation_t;
struct playerEntity_t;
struct playerState_s;
struct pml_t;
struct pmove_t;
struct pointtrace_t;
struct score_t;
struct snapshot_t;
struct srfTriangles_t;
struct trStatistics_t;
struct trace_t;
struct usercmd_s;
struct weaponState_t;

enum GfxFogOffset {
    FOG_OFFSET_NONE = 0,
    FOG_OFFSET_LINEAR = 1,
    FOG_OFFSET_EXP = 2,
    FOG_OFFSET_COUNT = 3
};

enum GfxPrimStatsTarget {
    GFX_PRIM_STATS_WORLD = 0,
    GFX_PRIM_STATS_SMODELCACHED = 1,
    GFX_PRIM_STATS_SMODELRIGID = 2,
    GFX_PRIM_STATS_XMODELCACHED = 3,
    GFX_PRIM_STATS_XMODELRIGID = 4,
    GFX_PRIM_STATS_XMODELSKINNED = 5,
    GFX_PRIM_STATS_BMODEL = 6,
    GFX_PRIM_STATS_FX = 7,
    GFX_PRIM_STATS_HUD = 8,
    GFX_PRIM_STATS_DEBUG = 9,
    GFX_PRIM_STATS_CODE = 10,
    GFX_PRIM_STATS_COUNT = 11
};

enum he_type_t {
    HE_TYPE_FREE = 0,
    HE_TYPE_TEXT = 1,
    HE_TYPE_VALUE = 2,
    HE_TYPE_PLAYERNAME = 3,
    HE_TYPE_MAPNAME = 4,
    HE_TYPE_GAMETYPE = 5,
    HE_TYPE_MATERIAL = 6,
    HE_TYPE_TIMER_DOWN = 7,
    HE_TYPE_TIMER_UP = 8,
    HE_TYPE_TENTHS_TIMER_DOWN = 9,
    HE_TYPE_TENTHS_TIMER_UP = 10,
    HE_TYPE_CLOCK_DOWN = 11,
    HE_TYPE_CLOCK_UP = 12,
    HE_TYPE_WAYPOINT = 13,
    HE_TYPE_COUNT = 14
};

enum hitLocation_t {
    HITLOC_NONE = 0,
    HITLOC_HELMET = 1,
    HITLOC_HEAD = 2,
    HITLOC_NECK = 3,
    HITLOC_TORSO_UPR = 4,
    HITLOC_TORSO_LWR = 5,
    HITLOC_R_ARM_UPR = 6,
    HITLOC_L_ARM_UPR = 7,
    HITLOC_R_ARM_LWR = 8,
    HITLOC_L_ARM_LWR = 9,
    HITLOC_R_HAND = 10,
    HITLOC_L_HAND = 11,
    HITLOC_R_LEG_UPR = 12,
    HITLOC_L_LEG_UPR = 13,
    HITLOC_R_LEG_LWR = 14,
    HITLOC_L_LEG_LWR = 15,
    HITLOC_R_FOOT = 16,
    HITLOC_L_FOOT = 17,
    HITLOC_GUN = 18,
    HITLOC_NUM = 19
};

enum leType_t {
    LE_MOVING_TRACER = 0
};

enum objectiveState_t {
    OBJST_EMPTY = 0,
    OBJST_ACTIVE = 1,
    OBJST_INVISIBLE = 2,
    OBJST_DONE = 3,
    OBJST_CURRENT = 4,
    OBJST_FAILED = 5,
    OBJST_NUMSTATES = 6
};

enum surfaceType_t {
    SF_BAD = 0,
    SF_POLY = 1,
    SF_ENTITY = 2,
    SF_XMODEL_SKINNED = 3,
    SF_XMODEL_RIGID = 4,
    SF_STATICMODEL_CACHED = 5,
    SF_TRIANGLES = 6,
    SF_RAW_GEOMETRY = 7,
    SF_NUM_SURFACE_TYPES = 8,
    SF_MAX = 4294967295
};

struct CollisionAabbTree {
    vec3_t origin;
    vec3_t halfSize;
    short unsigned int materialIndex;
    short unsigned int childCount;
    union CollisionAabbTreeIndex u;
};

union CollisionAabbTreeIndex {
    int firstChildIndex;
    int partitionIndex;
};

struct CollisionBorder {
    vec3_t distEq;
    vec_t zBase;
    vec_t zSlope;
    vec_t start;
    vec_t length;
};

struct CollisionEdge {
    vec3_t origin;
    vec3_t axis[3];
};

struct CollisionPartition {
    byte triCount;
    byte borderCount;
    CollisionTriangle *tris;
    CollisionBorder *borders;
};

struct CollisionTriangle {
    vec4_t plane;
    vec4_t svec;
    vec4_t tvec;
    int verts[3];
    int edges[3];
};

struct CollisionVertex {
    vec3_t xyz;
};

struct DObjSkelMat_s {
    float axis[3][4];
    float origin[4];
};

union DvarLimits {
    struct {
        int stringCount;
        const char **strings;
    } enumeration;
    struct {
        int min;
        int max;
    } integer;
    struct {
        float min;
        float max;
    } value;
    struct {
        vec_t min;
        vec_t max;
    } vector;
};

union DvarValue {
    Bool enabled;
    int integer;
    float value;
    vec_t *vector;
    const char *string;
    byte color[4];
};

struct GfxAabbTree {
    vec3_t mins;
    vec3_t maxs;
    int surfaceCount;
    int startSurfIndex;
    int staticModelCount;
    int *staticModels;
    int childCount;
    int children;
};

struct GfxCell {
    struct GfxCellWritable writable;
    vec3_t mins;
    vec3_t maxs;
    GfxAabbTree *aabbTree;
    int portalCount;
    GfxPortal *portals;
    int cullGroupCount;
    int *cullGroups;
    int occluderCount;
    GfxOccluder **occluders;
    GfxSceneModelCellRef *modelRefs;
};

struct GfxCellWritable {
    int markCount;
};

struct GfxCmdDrawQuadPic {
    GfxCmdHeader header;
    const Material *material;
    vec2_t verts[4];
    GfxColor color;
};

struct GfxCmdDrawTextInSpace {
    GfxCmdHeader header;
    vec3_t org;
    FontHandle font;
    vec3_t xPixelStep;
    vec3_t yPixelStep;
    GfxColor color;
    char text[4];
};

struct GfxCmdHeader {
    short unsigned int id;
    short unsigned int byteCount;
};

struct GfxCmdStretchPicRotate {
    GfxCmdHeader header;
    const Material *material;
    float x;
    float y;
    float w;
    float h;
    float s0;
    float t0;
    float s1;
    float t1;
    GfxColor color;
    float rotation;
};

struct GfxCullGroup {
    vec3_t mins;
    vec3_t maxs;
    int surfaceCount;
    int startSurfIndex;
};

struct GfxDrawPrimArgs {
    int firstVertexFromBase;
    int vertexCount;
    int primCount;
    int u;
};

struct GfxLightGrid {
    int pointCount;
    GfxLightGridPoint *points;
    int colorCount;
    GfxLightGridColors *colors;
};

struct GfxLightGridColors {
    byte rgb[3][8];
};

struct GfxLightGridPoint {
    unsigned int xyzHighBits;
    unsigned char xyzLowBitsAndSunVisible;
    unsigned char needsTrace;
    short unsigned int colorsIndex;
};

struct GfxMarkFragment {
    MaterialHandle markMaterial;
    short unsigned int lmapIndex;
    byte pointCount;
    int firstPoint;
};

struct GfxModelSurface {
    surfaceType_t surfType;
    XSurface *xsurf;
};

struct GfxOccluder {
    int planeCount;
    DpvsPlane *planes;
    int edgeCount;
    GfxOccluderEdge *edges;
    int vertexCount;
    vec3_t *vertices;
    int ignoreStackLevel;
    int viewPlaneCount;
    DpvsPlane *viewPlanes;
};

struct GfxOccluderEdge {
    DpvsPlane *plane[2];
    vec3_t *vertex[2];
};

struct GfxPortal {
    struct GfxPortalWritable writable;
    struct DpvsPlane plane;
    GfxCell *cell;
    vec3_t *vertices;
    byte vertexCount;
    byte hullPointCount;
    vec2_t *hullPoints;
    vec3_t hullAxis[2];
};

struct GfxPortalWritable {
    Bool isQueued;
    Bool isAncestor;
    GfxPortal *queuedParent;
};

struct GfxSModelCachedVertex {
    vec3_t xyz;
    vec3_t normal;
    GfxColor color;
    vec2_t texCoord;
    short unsigned int baseLightingCoords[2];
    vec3_t binormal;
    vec3_t tangent;
};

struct GfxSModelCachedVertexDx7 {
    vec3_t xyz;
    GfxColor color;
    vec2_t texCoord;
};

struct GfxSceneModelCellRef {
    int entIndex;
    vec3_t mins;
    vec3_t maxs;
    int next;
};

struct GfxStaticModelInstance {
    float cullDist;
    vec3_t origin;
    struct XModel *model;
    vec3_t mins;
    vec3_t maxs;
    vec3_t axis[3];
    float scale;
    vec3_t baseLightingCoords;
};

struct GfxSurface {
    const Material *material;
    short unsigned int lightmapIndex;
    short unsigned int sortGroup;
};

struct GfxWorld {
    const char *name;
    const char *baseName;
    int nodeCount;
    mnode_t *nodes;
    int surfaceCount;
    GfxSurface *surfaces;
    int skySurfCount;
    int *skyStartSurfs;
    GfxImage *skyImage;
    GfxSamplerState skySamplerState;
    int vertexCount;
    struct GfxWorldVertexData vd;
    struct SunLightParseParams sunParse;
    struct GfxLight sunLight;
    vec3_t sunColorFromBsp;
    int cullGroupCount;
    GfxCullGroup *cullGroups;
    int smodelCount;
    GfxStaticModelInstance *smodelInsts;
    int cellCount;
    GfxCell *cells;
    int lightmapCount;
    GfxLightmapArray *lightmaps;
    GfxImage *smodelLightingImage;
    vec3_t smodelLightingLookupScale;
    struct GfxLightGrid lightGrid;
    vec4_t (*smodelLightingColorTable)[6];
    float *smodelLightingSunVisTable;
    int modelCount;
    GfxBrushModel *models;
    vec3_t mins;
    vec3_t maxs;
    unsigned int checksum;
    int materialMemoryCount;
    MaterialMemory *materialMemory;
    struct sunflare_t sun;
    vec4_t outdoorLookupMatrix[4];
    GfxImage *outdoorImage;
};

struct GfxWorldVertexData {
    GfxWorldVertex *vertices;
    IDirect3DVertexBuffer9 *worldVb;
};

struct MantleResults {
    vec3_t dir;
    vec3_t startPos;
    vec3_t ledgePos;
    vec3_t endPos;
    int flags;
    int duration;
};

struct MantleState {
    float yaw;
    int timer;
    int transIndex;
    int flags;
};

struct MaterialMemory {
    const Material *material;
    int memory;
};

struct SkinBuffers {
    DObjSkelMat matArrayBuf[128];
    int verts;
};

struct SunLightParseParams {
    char name[64];
    float ambientScale;
    vec3_t ambientColor;
    float diffuseFraction;
    float sunLight;
    vec3_t sunColor;
    vec3_t diffuseColor;
    Bool diffuseColorHasBeenSet;
    vec3_t angles;
};

struct XAnimNotify_s {
    const char *name;
    unsigned int type;
    float timeFrac;
};

struct cLeafBrushNodeChildren_t {
    float dist;
    float range;
    short unsigned int childOffset[2];
};

union cLeafBrushNodeData_t {
    struct cLeafBrushNodeLeaf_t leaf;
    struct cLeafBrushNodeChildren_t children;
};

struct cLeafBrushNodeLeaf_t {
    short unsigned int *brushes;
};

struct cLeafBrushNode_s {
    byte axis;
    short int leafBrushCount;
    int contents;
    union cLeafBrushNodeData_t data;
};

struct cNode_t {
    cplane_t *plane;
    short int children[2];
};

struct cStaticModelWritable {
    short unsigned int nextModelInWorldSector;
};

struct cStaticModel_s {
    struct cStaticModelWritable writable;
    struct XModel *xmodel;
    vec3_t origin;
    vec3_t invAxis[3];
    vec3_t absmin;
    vec3_t absmax;
};

struct cbrush_t {
    vec3_t mins;
    vec3_t maxs;
    int contents;
    int numsides;
    cbrushside_t *sides;
    short int axialMaterialNum[2][3];
};

struct cbrushside_t {
    cplane_t *plane;
    int materialNum;
};

struct centity_s {
    entityState_t currentState;
    entityState_t nextState;
    Bool nextValid;
    byte cullIn;
    Bool bMuzzleFlash;
    Bool bTrailMade;
    int previousEventSequence;
    int miscTime;
    vec3_t lerpOrigin;
    vec3_t lerpAngles;
    vec3_t lightingOrigin;
    int dl_time;
    MaterialHandle voiceChatSprite;
    int voiceChatSpriteTime;
    struct XAnimTree_s *tree;
    int localClientNum;
};

struct clientState_s {
    int clientIndex;
    team_t team;
    int modelindex;
    int attachModelIndex[6];
    int attachTagIndex[6];
    char name[32];
};

struct cmodel_t {
    vec3_t mins;
    vec3_t maxs;
    float radius;
    struct cLeaf_t leaf;
};

struct corpseInfo_t {
    struct XAnimTree_s *tree;
    int entnum;
    int time;
    struct clientInfo_t ci;
    Bool falling;
};

struct cplane_s {
    vec3_t normal;
    float dist;
    byte type;
    byte signbits;
    byte pad[2];
};

struct dmaterial_t {
    char material[64];
    int surfaceFlags;
    int contentFlags;
};

struct dvar_s {
    const char *name;
    short unsigned int flags;
    byte type;
    Bool modified;
    union DvarValue current;
    DvarValue latched;
    DvarValue reset;
    union DvarLimits domain;
#if defined(__x86_64__) || defined(_M_X64)

    intptr_t next;
    intptr_t hashNext;
#else
    int next;
    int hashNext;
#endif
};

struct gameState_t {
    int stringOffsets[2048];
    char stringData[16000];
    int dataCount;
};

union hudelem_color_t {
    int rgba;
};

struct hudelem_s {
    he_type_t type;
    float x;
    float y;
    float z;
    float fontScale;
    int font;
    int alignOrg;
    int alignScreen;
    union hudelem_color_t color;
    hudelem_color_t fromColor;
    int fadeStartTime;
    int fadeTime;
    int label;
    int width;
    int height;
    int materialIndex;
    int fromWidth;
    int fromHeight;
    int scaleStartTime;
    int scaleTime;
    float fromX;
    float fromY;
    int fromAlignOrg;
    int fromAlignScreen;
    int moveStartTime;
    int moveTime;
    int time;
    int duration;
    float value;
    int text;
    float sort;
    qboolean foreground;
};

struct itemInfo_t {
    qboolean registered;
    struct XModel *models[2];
    MaterialHandle icons[4];
    snd_alias_list_t *pickupSound;
    snd_alias_list_t *ammoPickupSound;
};

struct leafList_s {
    int count;
    int maxcount;
    qboolean overflowed;
    int *list;
    vec3_t bounds[2];
    int lastLeaf;
};

struct locTraceWork_t {
    int contents;
    TraceExtents extents;
};

struct localEntity_s {
    struct localEntity_s *prev;
    struct localEntity_s *next;
    leType_t leType;
    int endTime;
    trajectory_t pos;
    float color[4];
    float tracerClipDist;
    struct GfxEntity refEntity;
};

union mnode_info_t {
    struct mnode_node_t node;
    struct mnode_leaf_t leaf;
};

struct mnode_leaf_t {
    int cluster;
};

struct mnode_node_t {
    cplane_t *plane;
    mnode_t *children[2];
};

struct mnode_t {
    int contents;
    int parent;
    int cellIndex;
    union mnode_info_t u;
};

struct orientation_t {
    vec3_t origin;
    vec3_t axis[3];
};

struct playerEntity_t {
    float fLastWeaponPosFrac;
    qboolean bPositionToADS;
    vec3_t vPositionLastOrg;
    float fLastIdleFactor;
    vec3_t vLastMoveOrg;
    vec3_t vLastMoveAng;
};

struct playerState_s {
    int commandTime;
    int pm_type;
    int bobCycle;
    int pm_flags;
    int pm_time;
    vec3_t origin;
    vec3_t velocity;
    vec2_t oldVelocity;
    int weaponTime;
    int weaponDelay;
    int grenadeTimeLeft;
    int weaponRestrictKickTime;
    int foliageSoundTime;
    int gravity;
    float leanf;
    int speed;
    int delta_angles[3];
    int groundEntityNum;
    vec3_t vLadderVec;
    int jumpTime;
    float jumpOriginZ;
    int legsTimer;
    int legsAnim;
    int torsoTimer;
    int torsoAnim;
    int legsAnimDuration;
    int torsoAnimDuration;
    int damageTimer;
    int damageDuration;
    int flinchYaw;
    int movementDir;
    int eFlags;
    int eventSequence;
    int events[4];
    int eventParms[4];
    int oldEventSequence;
    int clientNum;
    int offHandIndex;
    int weapon;
    int weaponstate;
    float fWeaponPosFrac;
    int adsDelayTime;
    int viewmodelIndex;
    vec3_t viewangles;
    int viewHeightTarget;
    float viewHeightCurrent;
    int viewHeightLerpTime;
    int viewHeightLerpTarget;
    qboolean viewHeightLerpDown;
    float viewHeightLerpPosAdj;
    vec2_t viewAngleClampBase;
    vec2_t viewAngleClampRange;
    int damageEvent;
    int damageYaw;
    int damagePitch;
    int damageCount;
    int stats[6];
    int ammo[128];
    int ammoclip[128];
    int weapons[4];
    char weaponslots[8];
    int weaponrechamber[4];
    vec3_t mins;
    vec3_t maxs;
    float proneDirection;
    float proneDirectionPitch;
    float proneTorsoPitch;
    int viewlocked;
    int viewlocked_entNum;
    int cursorHint;
    int cursorHintString;
    int cursorHintEntIndex;
    int iCompassFriendInfo;
    float fTorsoHeight;
    float fTorsoPitch;
    float fWaistPitch;
    float holdBreathScale;
    int holdBreathTimer;
    struct MantleState mantleState;
    int entityEventSequence;
    int weapAnim;
    float aimSpreadScale;
    int shellshockIndex;
    int shellshockTime;
    int shellshockDuration;
    objective_t objective[16];
    int deltaTime;
    int hud;
};

struct pml_t {
    vec3_t forward;
    vec3_t right;
    vec3_t up;
    float frametime;
    int msec;
    qboolean walking;
    qboolean groundPlane;
    qboolean almostGroundPlane;
    trace_t groundTrace;
    float impactSpeed;
    vec3_t previous_origin;
    vec3_t previous_velocity;
    int previous_waterlevel;
};

struct pmove_t {
    playerState_t *ps;
    usercmd_t cmd;
    usercmd_t oldcmd;
    int tracemask;
    int numtouch;
    int touchents[32];
    vec3_t mins;
    vec3_t maxs;
    float xyspeed;
    qboolean proneChange;
    byte handler;
    Bool mantleStarted;
    vec3_t mantleEndPos;
    int mantleDuration;
};

struct pointtrace_t {
    TraceExtents extents;
    int passEntityNum;
    int passOwnerNum;
    int contentmask;
    qboolean bLocational;
    unsigned char *priorityMap;
};

struct score_t {
    int client;
    int score;
    int ping;
    int deaths;
    int team;
    MaterialHandle hStatusIcon;
};

struct snapshot_t {
    int snapFlags;
    int ping;
    int serverTime;
    playerState_t ps;
    int numEntities;
    int numClients;
    entityState_t entities[256];
    clientState_t clients[64];
    int serverCommandSequence;
};

struct srfTriangles_t {
    surfaceType_t surfaceType;
    vec3_t bounds[2];
    int firstVertex;
    short unsigned int vertexCount;
    short unsigned int indexCount;
    int (*indices)[4];
};

struct trStatistics_t {
    int c_indexes;
    int c_fxIndexes;
    int c_viewIndexes;
    int c_shadowIndexes;
    int c_vertexes;
    int c_batches;
    int c_refents;
    float dc;
    struct Image_MemUsage c_imageUsage;
};

struct trace_t {
    float fraction;
    vec3_t normal;
    int surfaceFlags;
    int contents;
    const char *material;
    short unsigned int entityNum;
    short unsigned int partName;
    short unsigned int partGroup;
    byte allsolid;
    byte startsolid;
};

struct usercmd_s {
    int serverTime;
    int buttons;
    byte weapon;
    byte offHandIndex;
    int angles[3];
    signed char forwardmove;
    signed char rightmove;
};

struct weaponState_t {
    playerState_t *ps;
    float xyspeed;
    float frametime;
    vec3_t vLastMoveAng;
    float fLastIdleFactor;
    int time;
    int damageTime;
    float v_dmg_pitch;
    float v_dmg_roll;
    vec3_t vGunOffset;
    vec3_t vGunSpeed;
    vec3_t swayAngles;
    int *weapIdleTime;
};

#ifdef __cplusplus
}
#endif
#endif

#ifndef PC_EFFECTSCORE_FXMEMMGR_H_H
#define PC_EFFECTSCORE_FXMEMMGR_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct Cloud;
struct Cylinder;
struct Emitter;
struct FxMemMgr_Cloud;
struct FxMemMgr_Cylinder;
struct FxMemMgr_Emitter;
struct FxMemMgr_Line;
struct FxMemMgr_OrientedParticle;
struct FxMemMgr_Tail;
struct Line;
struct OrientedParticle;
struct Tail;

struct Cloud {
    Bool useLength;
    float randomLengthWeight;
    FxChannelInstance lengthChannelInstance;
    FxChannelInstance lengthRandChannelInstance;
};

struct Cylinder {
    int _placeholder;
};

struct Emitter {
    int _placeholder;
};

struct FxMemMgr_Cloud {
    int _placeholder;
};

struct FxMemMgr_Cylinder {
    int _placeholder;
};

struct FxMemMgr_Emitter {
    int _placeholder;
};

struct FxMemMgr_Line {
    int _placeholder;
};

struct FxMemMgr_OrientedParticle {
    int _placeholder;
};

struct FxMemMgr_Tail {
    int _placeholder;
};

struct Line {
    int _placeholder;
};

struct OrientedParticle {
    int _placeholder;
};

struct Tail {
    float randomLengthWeight;
    FxChannelInstance lengthChannelInstance;
    FxChannelInstance lengthRandChannelInstance;
};

#ifdef __cplusplus
}
#endif
#endif

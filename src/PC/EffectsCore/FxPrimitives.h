#ifndef PC_EFFECTSCORE_FXPRIMITIVES_H_H
#define PC_EFFECTSCORE_FXPRIMITIVES_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct Effect;
struct Flash;
struct FxBoltFrame;
struct FxBoltFramePtr;
struct FxMemMgr_Effect;
struct FxMemMgr_FxBoltFrame;
struct FxMemMgr_Light;
struct FxMemMgr_Particle;
struct Light;
struct Particle;

struct Effect {
    int _vptr$Effect;
    struct FxGfxEntity mRefEnt;
    int mFlags;
    int mClusterId;
    int mSortGroup;
    struct XModel *mModel;
    int mTimeStart;
    int mTimeEnd;
    struct FxBoltFramePtr mBolt;
};

struct Flash {
    int _placeholder;
};

struct FxBoltFrame {
    struct FxBoltInfo mBolt;
};

struct FxBoltFramePtr {
    int _placeholder;
};

struct FxMemMgr_Effect {
    int _placeholder;
};

struct FxMemMgr_FxBoltFrame {
    int _placeholder;
};

struct FxMemMgr_Light {
    int _placeholder;
};

struct FxMemMgr_Particle {
    int _placeholder;
};

struct Light {
    FxChannelInstance colorChannelInstance;
    FxChannelInstance colorRandChannelInstance;
    FxChannelInstance sizeChannelInstance;
    FxChannelInstance sizeRandChannelInstance;
};

struct Particle {
    FxChannelInstance colorChannelInstance;
    FxChannelInstance colorRandChannelInstance;
    FxChannelInstance alphaChannelInstance;
    FxChannelInstance alphaRandChannelInstance;
    FxChannelInstance sizeChannelInstance;
    FxChannelInstance sizeRandChannelInstance;
    FxChannelInstance size2ChannelInstance;
    FxChannelInstance size2RandChannelInstance;
    FxChannelInstance rotationDeltaChannelInstance;
    FxChannelInstance rotationDeltaRandChannelInstance;
    FxChannelInstance velocityXChannelInstance;
    FxChannelInstance velocityYChannelInstance;
    FxChannelInstance velocityZChannelInstance;
    FxChannelInstance velocityXRandChannelInstance;
    FxChannelInstance velocityYRandChannelInstance;
    FxChannelInstance velocityZRandChannelInstance;
    FxChannelInstance velocity2XChannelInstance;
    FxChannelInstance velocity2YChannelInstance;
    FxChannelInstance velocity2ZChannelInstance;
    FxChannelInstance velocity2XRandChannelInstance;
    FxChannelInstance velocity2YRandChannelInstance;
    FxChannelInstance velocity2ZRandChannelInstance;
};

#ifdef __cplusplus
}
#endif
#endif

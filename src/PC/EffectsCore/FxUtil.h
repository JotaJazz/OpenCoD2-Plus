#ifndef PC_EFFECTSCORE_FXUTIL_H_H
#define PC_EFFECTSCORE_FXUTIL_H_H

#ifdef __cplusplus
extern "C" {
#endif

#include "PC/EffectsCore/FxPrimitives.h"

struct EffectPrimitive;

struct EffectPrimitive {
    const EffectTemplate *fx;
    const PrimitiveTemplate *primTemp;
    FxBoltFramePtr boltFrame;
};

#ifdef __cplusplus
}
#endif
#endif

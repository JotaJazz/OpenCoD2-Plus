#include "common_types.h"
#include <math.h>
#include "imports.h"

extern const FxCurve *FxCurve_AllocAndCreateWithKeys(const float *keyArray, int dimensionCount, int keyCount);
extern void FxRange_SetRange(const FxRange *_this, float min, float max);
extern float FxRange_GetValPct(const FxRange *_this, float percent);
extern float flrand(float min, float max);

void FxChannel_CreateDefault(FxChannel *createe, int dimensions, float value1, float value2)
{
    int keySize = dimensions + 1;
    float keys[16];

    keys[0] = 0.0f;

    keys[keySize] = 1.0f;

    int d;
    if (dimensions != 0) {
        for (d = 0; d != dimensions; d++) {
            keys[1 + d] = value1;
            keys[keySize + 1 + d] = value2;
        }
    }

    createe->curve = FxCurve_AllocAndCreateWithKeys(keys, dimensions, 2);

    FxRange_SetRange(&createe->scaleRange, 1.0f, 1.0f);
}

void FxChannel_CreateViaMigration(const FxChannelBackwardCompatible *source, int dimensions, float lifetime, int forceUnitScale, FxChannel *target)
{
    float keys[132];
    float startVals[3];
    float endVals[3];
    float scaleFactor;
    float delayFraction;
    float waveParm;
    int flags;
    int flagMode;
    int randomize;
    int hasData;
    int keySize;
    int dimensionId;
    int timeStep;
    float t;
    float val;

    if (forceUnitScale) {

        scaleFactor = 1.0f;
    } else {

        float valLT = FxRange_GetValPct( (const FxRange *)((void *)&source->end[0]), 0.5f);
        float valStart = FxRange_GetValPct( (const FxRange *)((void *)&source->start[0]), 0.5f);

        float diff = valStart - valLT;
        float maxVal;
        if (diff < 0.0f) {
            maxVal = valLT;
        } else {
            maxVal = valStart;
        }

        maxVal *= 1.3333333730697632f;

        if (1.0f > maxVal) {
            scaleFactor = 1.0f;
        } else {
            scaleFactor = 1.0f / maxVal;
        }
    }

    {
        float halfRange = (source->start[0].mMax - source->start[0].mMin) * 0.5f;
        float rangeMax = scaleFactor + halfRange;
        float rangeMin = scaleFactor - halfRange;
        FxRange_SetRange(&target->scaleRange, rangeMin, rangeMax);
    }

    if (dimensions == 0) {
        delayFraction = 0.0f;
        waveParm = 0.0f;
    } else {

        const FxRange *startPtr = &source->start[0];
        const FxRange *endPtr = &source->end[0];
        const FxRange *parmPtr = &source->parm;

        float lastParmVal = 0.0f;
        for (dimensionId = 0; dimensionId < dimensions; dimensionId++) {

            startVals[dimensionId] = FxRange_GetValPct( (const FxRange *)((void *)&startPtr[dimensionId]), 0.5f) * scaleFactor;

            endVals[dimensionId] = FxRange_GetValPct( (const FxRange *)((void *)&endPtr[dimensionId]), 0.5f) * scaleFactor;

            lastParmVal = FxRange_GetValPct( (const FxRange *)((void *)parmPtr), 0.5f) * scaleFactor;
        }
        delayFraction = lastParmVal * 0.009999999776482582f;
        waveParm = lastParmVal * 0.0031415929552167654f;
    }

    waveParm *= lifetime;

    flags = source->flags;

    hasData = flags & 1;

    if (!hasData) {

        flagMode = flags & 0xC;
        if (flagMode == 4) {

            keySize = dimensions + 1;
            keys[0] = 0.0f;
            keys[keySize] = delayFraction;
            *(int *)&keys[keySize * 2] = 0x3f800000;
            for (dimensionId = 0; dimensionId < dimensions; dimensionId++) {
                keys[1 + dimensionId] = startVals[dimensionId];
                keys[keySize + 1 + dimensionId] = startVals[dimensionId];
                keys[keySize * 2 + 1 + dimensionId] = endVals[dimensionId];
            }
            target->curve = FxCurve_AllocAndCreateWithKeys(keys, dimensions, 3);
            return;
        }

    } else {

        flagMode = flags & 0xC;
        if (flagMode == 0) {

            keySize = dimensions + 1;
            keys[0] = 0.0f;
            *(int *)&keys[keySize] = 0x3f800000;
            for (dimensionId = 0; dimensionId < dimensions; dimensionId++) {
                keys[1 + dimensionId] = startVals[dimensionId];
                keys[keySize + 1 + dimensionId] = endVals[dimensionId];
            }
            target->curve = FxCurve_AllocAndCreateWithKeys(keys, dimensions, 2);
            return;
        }

    }

    keySize = dimensions + 1;
    randomize = (flags >> 1) & 1;
    t = 0.0f;

    for (timeStep = 0; timeStep < 16; timeStep++) {
        float oneMinusT;
        float t2;

        if (hasData) {

            oneMinusT = 1.0f - t;
        } else {
            oneMinusT = 1.0f;
        }

        if (flagMode == 4) {

            if (t > delayFraction) {

                float progress = (t - delayFraction) / (1.0f - delayFraction);
                t2 = 1.0f - progress;
            } else {
                t2 = 1.0f;
            }
        } else if (flagMode == 8) {

            t2 = cosf(waveParm * t) * oneMinusT;
            goto store_key;
        } else if (flagMode == 0xC) {

            if (delayFraction > t) {
                t2 = (delayFraction - t) / delayFraction;
            } else {
                t2 = 0.0f;
            }
        } else {

            t2 = oneMinusT;
            goto store_key;
        }

        if (hasData) {
            oneMinusT *= 0.5f;
            t2 = t2 * 0.5f + oneMinusT;
        }

    store_key:

        keys[keySize * timeStep] = t;

        if (dimensions > 0) {
            float oneMinusT2 = 1.0f - t2;

            if (randomize) {

                if (t2 > 1.0f) {

                    int d;
                    for (d = 0; d < dimensions; d++) {
                        float blended = t2 * startVals[d] + oneMinusT2 * endVals[d];
                        float maxVal;
                        if (0.0f > blended) {
                            maxVal = 0.0f;
                        } else {
                            maxVal = 1.0f;
                        }
                        keys[keySize * timeStep + 1 + d] = flrand(0.0f, maxVal);
                    }
                } else {

                    int d;
                    for (d = 0; d < dimensions; d++) {
                        float blended = t2 * startVals[d] + oneMinusT2 * endVals[d];
                        if (0.0f > blended) {
                            blended = 0.0f;
                        }
                        keys[keySize * timeStep + 1 + d] = flrand(0.0f, blended);
                    }
                }
            } else if (t2 > 1.0f) {

                int d;
                for (d = 0; d < dimensions; d++) {
                    float blended = t2 * startVals[d] + oneMinusT2 * endVals[d];
                    if (0.0f > blended) {
                        keys[keySize * timeStep + 1 + d] = 0.0f;
                    } else {
                        keys[keySize * timeStep + 1 + d] = 1.0f;
                    }
                }
            } else {

                int d;
                for (d = 0; d < dimensions; d++) {
                    float blended = oneMinusT2 * endVals[d] + t2 * startVals[d];
                    if (0.0f > blended) {
                        blended = 0.0f;
                    }
                    keys[keySize * timeStep + 1 + d] = blended;
                }
            }
        }

        t += 0.06666667014360428f;

        if (timeStep < 15) {
            if (1.0f < t) {
                t = 1.0f;
            }
        }
    }

    target->curve = FxCurve_AllocAndCreateWithKeys(keys, dimensions, 16);
}

#include "common_types.h"
#include "imports.h"
#include <math.h>

void CSoundObject_CSoundObject(char *_this, CSoundEngine *engine, unsigned int busIndex);
void *CSoundEngine_GetMixerUnit(CSoundEngine *engine);

void CSampleSound_CSampleSound(CSampleSound *_this, CSoundEngine *inEngine, UInt32 inBusIndex);
void CSampleSound_TheadIdle(const CSampleSound *_this);
void CSampleSound_stop_sample(const CSampleSound *_this);
void CSampleSound_end_sample(const CSampleSound *_this);
bool CSampleSound_open_stream(const CSampleSound *_this, const char *filename);
void CSampleSound_close_stream(const CSampleSound *_this);
void CSampleSound_ChangedFormat(CSampleSound *_this);
void CSampleSound_Changed3DPosition(CSampleSound *_this);
OSStatus CSampleSound_DoConvert(const CSampleSound *_this, UInt32 *ioNumberDataPackets, AudioBufferList *ioData);
OSStatus CSampleSound_AudioConverterProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData);
void CSampleSound_UpdateStreamFormat(const CSampleSound *_this);
void CSampleSound_ChangedVolume(CSampleSound *_this);
void CSampleSound_Update3DValues(const CSampleSound *_this);
OSStatus CSampleSound_DoRender(const CSampleSound *_this, AudioUnitRenderActionFlags *ioActionFlags, UInt32 inNumberFrames, AudioBufferList *ioData);
OSStatus CSampleSound_RenderCallbackProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
void CSampleSound_resume_sample(const CSampleSound *_this);
OSStatus CSampleSound_InitSample(const CSampleSound *_this, UInt32 inSoundType, UInt32 inSpatializationAlgorithm, UInt32 in3DRenderFlags, UInt32 inDopplerShift);
void CSampleSound_Changed3DDistances(const CSampleSound *_this);
OSStatus CSampleSound_RenderNotifyProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
void ZN12CSampleSoundD2Ev(void);
void ZN12CSampleSoundD1Ev(void);
void ZN12CSampleSoundD0Ev(void);
OSStatus CSampleSound_DoPreRender(const CSampleSound *_this);
OSStatus CSampleSound_DoPostRender(const CSampleSound *_this);

void CSampleSound_CSampleSound(CSampleSound *_this, CSoundEngine *inEngine, UInt32 inBusIndex)
{
    char *p = (char *)_this;

    CSoundObject_CSoundObject((char *)_this, inEngine, inBusIndex);

    *(int *)(p + 0x00) = 0x3322c8;
    *(int *)(p + 0x108) = 0;
    *(char *)(p + 0x10c) = 0;
    *(char *)(p + 0x10d) = 0;
    *(char *)(p + 0x10e) = 0;
    *(char *)(p + 0x10f) = 1;
    *(char *)(p + 0x110) = 0;
    *(char *)(p + 0x111) = 0;
    *(int *)(p + 0x114) = 0;
    *(int *)(p + 0x118) = 0;
    *(char *)(p + 0x11c) = 0;

    int *slices = (int *)(p + 0xe0);
    for (int i = 0; i < 10; i++)
        slices[i] = -1;
}

void CSampleSound_TheadIdle(const CSampleSound *_this)
{
}

void CSampleSound_stop_sample(const CSampleSound *_this)
{
}

void CSampleSound_end_sample(const CSampleSound *_this)
{
}

bool CSampleSound_open_stream(const CSampleSound *_this, const char *filename)
{
    return 0;
}

void CSampleSound_close_stream(const CSampleSound *_this)
{
}

void CSampleSound_ChangedFormat(CSampleSound *_this)
{
    *(char *)((char *)_this + 0x10f) = 1;
}

void CSampleSound_Changed3DPosition(CSampleSound *_this)
{
    *(char *)((char *)_this + 0x110) = 1;
}

OSStatus CSampleSound_DoConvert(const CSampleSound *_this, UInt32 *ioNumberDataPackets, AudioBufferList *ioData)
{
    return 0;
}

OSStatus CSampleSound_AudioConverterProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
    if (!inUserData)
        return -50;

    return CSampleSound_DoConvert((const CSampleSound*)inUserData, ioNumberDataPackets, ioData);
}

void CSampleSound_UpdateStreamFormat(const CSampleSound *_this)
{
}

void CSampleSound_ChangedVolume(CSampleSound *_this)
{
    char *p = (char *)_this;

    float leftLevel = *(float *)(p + 0x68);
    float rightLevel = *(float *)(p + 0x6c);
    float masterVolume = *(float *)(p + 0x64);

    float realVolume = (leftLevel + rightLevel) * 0.5f * masterVolume;

    if (realVolume > 1.0f)
        realVolume = 1.0f;

    if (realVolume <= 0.0f) {
        *(char *)(p + 0x111) = 1;
        return;
    }

    float db = log10f(realVolume) * 20.0f;
    if (db < -120.0f)
        db = -120.0f;

    UInt32 busIndex = *(UInt32 *)(p + 0x08);
    CSoundEngine *engine = *(CSoundEngine **)(p + 0x04);
    AudioUnit mixerUnit = (AudioUnit)CSoundEngine_GetMixerUnit(engine);

    AudioUnitSetParameter(mixerUnit, 3 , 1 , busIndex, db, 0);

    *(char *)(p + 0x111) = 0;
}

void CSampleSound_Update3DValues(const CSampleSound *_this)
{
}

OSStatus CSampleSound_DoRender(const CSampleSound *_this, AudioUnitRenderActionFlags *ioActionFlags, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    return 0;
}

OSStatus CSampleSound_RenderCallbackProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    if (!inRefCon)
        return -50;

    return CSampleSound_DoRender((const CSampleSound*)inRefCon, ioActionFlags, inNumberFrames, ioData);
}

void CSampleSound_resume_sample(const CSampleSound *_this)
{
}

OSStatus CSampleSound_InitSample(const CSampleSound *_this, UInt32 inSoundType, UInt32 inSpatializationAlgorithm, UInt32 in3DRenderFlags, UInt32 inDopplerShift)
{
    return 0;
}

void CSampleSound_Changed3DDistances(const CSampleSound *_this)
{
}

OSStatus CSampleSound_RenderNotifyProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    return 0;
}

void ZN12CSampleSoundD2Ev(void)
{
}

void ZN12CSampleSoundD1Ev(void)
{
}

void ZN12CSampleSoundD0Ev(void)
{
}

OSStatus CSampleSound_DoPreRender(const CSampleSound *_this)
{
    return 0;
}

OSStatus CSampleSound_DoPostRender(const CSampleSound *_this)
{
    return 0;
}

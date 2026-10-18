#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

void *__Znwm(unsigned long size);
void __ZdlPv(void *ptr);
void __ZdaPv(void *ptr);

void CStreamSound_CStreamSound(COD2_KR);
void CStreamSound_UpdateAllStreams(void);
void StThreadLock_StThreadLock();
void ZN12StThreadLockD1Ev();
void CThread_Stop();
void CSoundObject_Release();
Boolean CSoundObject_IsAvailable(char *_this);
UInt32 CSoundObject_GetSoundType(char *_this);
OSStatus CSampleSound_InitSample(const CSampleSound *_this, UInt32 inSoundType, UInt32 inSpatializationAlgorithm, UInt32 in3DRenderFlags, UInt32 inDopplerShift);
void ZN6CMutexD1Ev(void *mutex);
void ZN7CThreadD2Ev();

static char *sDeviceName;

long int CSoundEngine_mixer_count(const CSoundEngine *_this);
long int CSoundEngine_get_cpu_percent(const CSoundEngine *_this);
long int CSoundEngine_mixer_count_3D(const CSoundEngine *_this);
void CSoundEngine_set_3D_room_type(const CSoundEngine *_this, long int room_type);
void CSoundEngine_set_digital_master_reverb_levels(const CSoundEngine *_this, float dry_level, float wet_level);
long int CSoundEngine_minimum_sample_buffer_size(const CSoundEngine *_this, long int playback_rate, long int format);
long int CSoundEngine_size_processed_digital_audio(const CSoundEngine *_this, long unsigned int dest_rate, long unsigned int dest_format, const AILMIXINFO *src);
long int CSoundEngine_process_digital_audio(const CSoundEngine *_this, void *dest_buffer, long int dest_buffer_size, long unsigned int dest_rate, long unsigned int dest_format, const AILMIXINFO *src);
char *CSoundEngine_device_name(const CSoundEngine *_this);
void CSoundEngine_set_3D_rolloff_factor(const CSoundEngine *_this, float factor);
float CSoundEngine_get_3D_distance_factor(const CSoundEngine *_this);
void CSoundEngine_set_3D_distance_factor(const CSoundEngine *_this, float factor);
void CSoundEngine_set_3D_position(const CSoundEngine *_this, float X, float Y, float Z);
void CSoundEngine_get_3D_position(const CSoundEngine *_this, float *X, float *Y, float *Z);
CSoundObject *CSoundEngine_NewStreamSound(const CSoundEngine *_this);
CSoundObject *CSoundEngine_GetSampleSound(const CSoundEngine *_this, HSAMPLE inSample);
CSoundObject *CSoundEngine_GetSampleSound3D(const CSoundEngine *_this, void *in3DSample);
CSoundObject *CSoundEngine_GetStreamSound(const CSoundEngine *_this, HSTREAM inStream);
void CSoundEngine_UpdateAllSounds(const CSoundEngine *_this);
AudioUnit CSoundEngine_GetMixerUnit(const CSoundEngine *_this);
Boolean CSoundEngine_IsDistanceScalingRequired(const CSoundEngine *_this);
Float32 CSoundEngine_GetDefaultReferenceDistance(const CSoundEngine *_this);
Float32 CSoundEngine_GetDefaultMaxDistance(const CSoundEngine *_this);
void CSoundEngine_GetListenerFaceVector(const CSoundEngine *_this, D3DXVECTOR3 *outFace);
void CSoundEngine_GetListenerUpVector(const CSoundEngine *_this, D3DXVECTOR3 *outUp);
CSoundObject *CSoundEngine_GetAvailableSampleBus(const CSoundEngine *_this, UInt32 inSoundType);
CSoundObject *CSoundEngine_NewSampleSound3D(const CSoundEngine *_this);
CSoundObject *CSoundEngine_NewSampleSound(const CSoundEngine *_this);
void CSoundEngine_Execute(const CSoundEngine *_this, void *inArg);
void CSoundEngine_RemoveSoundObject(const CSoundEngine *_this, CSoundObject *inSound);
void CSoundEngine_set_digital_master_room_type(const CSoundEngine *_this, long int room_type);
void ZN12CSoundEngineD0Ev(CSoundEngine *_this);
void CSoundEngine_AddSoundObject(const CSoundEngine *_this, CSoundObject *inSound);
void ZN12CSoundEngineD2Ev(void);
void ZN12CSoundEngineD1Ev(CSoundEngine *_this);
UInt32 CSoundEngine_GetPreferredChannelCount(const CSoundEngine *_this);
void CSoundEngine_CSoundEngine(const CSoundEngine *_this, UInt32 inBusCount, int inHighQuality);
void *ZNSt6vectorIP12CSoundObjectSaIS1_EE5eraseEN9__gnu_cxx17__normal_iteratorIPS1_S3_EE(void *_this, CSoundObject **__position);
void *ZNSt6vectorIP12CSoundObjectSaIS1_EE5eraseEN9__gnu_cxx17__normal_iteratorIPS1_S3_EES7_(void *_this, CSoundObject **__first, CSoundObject **__last);
void ZNSt6vectorIP12CSoundObjectSaIS1_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS1_S3_EERKS1_(void);

long int CSoundEngine_mixer_count(const CSoundEngine *_this)
{
    return *(long int *)((char *)_this + 0x50);
}

long int CSoundEngine_get_cpu_percent(const CSoundEngine *_this)
{
    float cpuLoad;
    OSStatus error = AUGraphGetCPULoad(*(void **)((char *)_this + 0x3c), &cpuLoad);
    if (error != 0)
        return 1;

    int percent = (int)(cpuLoad * 100.0f);
    if (percent <= 0)
        return 1;

    return percent;
}

long int CSoundEngine_mixer_count_3D(const CSoundEngine *_this)
{
    return *(long int *)((char *)_this + 0x50);
}

void CSoundEngine_set_3D_room_type(const CSoundEngine *_this, long int room_type)
{
    *(long int *)((char *)_this + 0x1a8) = room_type;
}

void CSoundEngine_set_digital_master_reverb_levels(const CSoundEngine *_this, float dry_level, float wet_level)
{

}

long int CSoundEngine_minimum_sample_buffer_size(const CSoundEngine *_this, long int playback_rate, long int format)
{
    return 0x800;
}

long int CSoundEngine_size_processed_digital_audio(const CSoundEngine *_this, long unsigned int dest_rate, long unsigned int dest_format, const AILMIXINFO *src)
{
    return 0;
}

long int CSoundEngine_process_digital_audio(const CSoundEngine *_this, void *dest_buffer, long int dest_buffer_size, long unsigned int dest_rate, long unsigned int dest_format, const AILMIXINFO *src)
{
    return 0;
}

char *CSoundEngine_device_name(const CSoundEngine *_this)
{
    if (sDeviceName)
        return sDeviceName;

    sDeviceName = strdup((const char *)_this + 0x58);
    return sDeviceName;
}

void CSoundEngine_set_3D_rolloff_factor(const CSoundEngine *_this, float factor)
{
    *(float *)((char *)_this + 0x1ac) = factor;
}

float CSoundEngine_get_3D_distance_factor(const CSoundEngine *_this)
{
    return *(float *)((char *)_this + 0x1b0);
}

void CSoundEngine_set_3D_distance_factor(const CSoundEngine *_this, float factor)
{
    *(float *)((char *)_this + 0x1b0) = factor;
}

void CSoundEngine_set_3D_position(const CSoundEngine *_this, float X, float Y, float Z)
{
    char *p = (char *)_this;
    unsigned int zi;
    zi = *(unsigned int *)&Z;
    *(float *)(p + 0x1b4) = X;
    *(float *)(p + 0x1b8) = Y;
    zi ^= 0x80000000u;
    *(unsigned int *)(p + 0x1bc) = zi;
}

void CSoundEngine_get_3D_position(const CSoundEngine *_this, float *X, float *Y, float *Z)
{
    const char *p = (const char *)_this;
    *X = *(float *)(p + 0x1b4);
    *Y = *(float *)(p + 0x1b8);

    unsigned int zi = *(unsigned int *)(p + 0x1bc);
    zi ^= 0x80000000u;
    *(unsigned int *)Z = zi;
}

CSoundObject *CSoundEngine_NewStreamSound(const CSoundEngine *_this)
{
    void *sound = __Znwm(0xf0);
    CStreamSound_CStreamSound(sound, _this);
    return (CSoundObject *)sound;
}

CSoundObject *CSoundEngine_GetSampleSound(const CSoundEngine *_this, HSAMPLE inSample)
{
    return (CSoundObject *)inSample;
}

CSoundObject *CSoundEngine_GetSampleSound3D(const CSoundEngine *_this, void *in3DSample)
{
    return (CSoundObject *)in3DSample;
}

CSoundObject *CSoundEngine_GetStreamSound(const CSoundEngine *_this, HSTREAM inStream)
{
    return (CSoundObject *)inStream;
}

void CSoundEngine_UpdateAllSounds(const CSoundEngine *_this)
{
    CStreamSound_UpdateAllStreams();
}

AudioUnit CSoundEngine_GetMixerUnit(const CSoundEngine *_this)
{
    return *(AudioUnit *)((char *)_this + 0x44);
}

Boolean CSoundEngine_IsDistanceScalingRequired(const CSoundEngine *_this)
{
    return *(unsigned char *)((char *)_this + 0x190);
}

Float32 CSoundEngine_GetDefaultReferenceDistance(const CSoundEngine *_this)
{
    const char *p = (const char *)_this;
    float refDist = *(float *)(p + 0x194);
    float distFactor = *(float *)(p + 0x1b0);
    return refDist / distFactor;
}

Float32 CSoundEngine_GetDefaultMaxDistance(const CSoundEngine *_this)
{
    const char *p = (const char *)_this;
    float maxDist = *(float *)(p + 0x198);
    float distFactor = *(float *)(p + 0x1b0);
    return maxDist / distFactor;
}

void CSoundEngine_GetListenerFaceVector(const CSoundEngine *_this, D3DXVECTOR3 *outFace)
{
    const char *p = (const char *)_this;
    *(float *)((char *)outFace + 0) = *(float *)(p + 0x1c0);
    *(float *)((char *)outFace + 4) = *(float *)(p + 0x1c4);
    *(float *)((char *)outFace + 8) = *(float *)(p + 0x1c8);
}

void CSoundEngine_GetListenerUpVector(const CSoundEngine *_this, D3DXVECTOR3 *outUp)
{
    const char *p = (const char *)_this;
    *(float *)((char *)outUp + 0) = *(float *)(p + 0x1cc);
    *(float *)((char *)outUp + 4) = *(float *)(p + 0x1d0);
    *(float *)((char *)outUp + 8) = *(float *)(p + 0x1d4);
}

CSoundObject *CSoundEngine_GetAvailableSampleBus(const CSoundEngine *_this, UInt32 inSoundType)
{
    char *engine = (char *)_this;
    UInt32 mixerCount = *(UInt32 *)(engine + 0x50);
    CSoundObject **sampleBuses = *(CSoundObject ***)(engine + 0x54);
    UInt32 spatializationAlgorithm = *(UInt32 *)(engine + 0x19c);
    UInt32 renderFlags = *(UInt32 *)(engine + 0x1a0);
    UInt32 dopplerShift = *(UInt32 *)(engine + 0x1a4);
    UInt32 i;

    for (i = 0; i < mixerCount; ++i) {
        CSoundObject *sample = sampleBuses[i];

        if (CSoundObject_IsAvailable((char *)sample) &&
            CSoundObject_GetSoundType((char *)sample) == inSoundType &&
            CSampleSound_InitSample((CSampleSound *)sample, inSoundType,
                                    spatializationAlgorithm, renderFlags, dopplerShift) == 0) {
            return sample;
        }
    }

    for (i = 0; i < mixerCount; ++i) {
        CSoundObject *sample = sampleBuses[i];

        if (CSoundObject_IsAvailable((char *)sample) &&
            CSampleSound_InitSample((CSampleSound *)sample, inSoundType,
                                    spatializationAlgorithm, renderFlags, dopplerShift) == 0) {
            return sample;
        }
    }

    return 0;
}

CSoundObject *CSoundEngine_NewSampleSound3D(const CSoundEngine *_this)
{
    return CSoundEngine_GetAvailableSampleBus(_this, 1);
}

CSoundObject *CSoundEngine_NewSampleSound(const CSoundEngine *_this)
{
    return CSoundEngine_GetAvailableSampleBus(_this, 0);
}

void CSoundEngine_set_digital_master_room_type(const CSoundEngine *_this, long int room_type)
{
    *(long int *)((char *)_this + 0x1a8) = room_type;
    return (void)(unsigned long)_this;
}

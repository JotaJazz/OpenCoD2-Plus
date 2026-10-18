#include "common_types.h"
#include "imports.h"

int g_sound_recordFrequency;
int g_sound_recordVolume;
int g_sound_channels;
char g_recording_initialized;
char g_currently_recording;
extern int (*current_audioCallback)(COD2_KR);
static recordingSample_t s_recordingSamples[65];
static recordingSample_t *s_recordingSamplePtr;
static recordingSample_t *g_current_sample;
static CAudioRecorder *sAudioRecorder;

Boolean CAudioRecorder_Start(CAudioRecorder *rec);
void CAudioRecorder_Stop(CAudioRecorder *rec);
void CAudioRecorder_CAudioRecorder(void *rec, double freq, int channels, int bits);
void *CAudioRecorder_GetBuffer(CAudioRecorder *rec, int *outSize);
Float32 CAudioRecorder_GetRecordLevel(CAudioRecorder *rec);
int CAudioRecorder_Initialize(void *rec);
void CAudioRecorder_Mute(CAudioRecorder *rec, int mute);
void CAudioRecorder_SetRecordLevel(CAudioRecorder *rec, float level);
void CAudioRecorder_Shutdown(CAudioRecorder *rec);
void ZN14CAudioRecorderD1Ev(void *rec);
void *__Znwm(unsigned int size);
void __ZdlPv(void *ptr);
void Com_Printf(const char *fmt, ...);
int DSOUNDRecord_Start(recordingSample_t *pRecSample);
int DSOUNDRecord_Stop(recordingSample_t *pRecSample);
recordingSample_t *DSOUNDRecord_NewSample(void);
int DSOUNDRecord_DestroySample(recordingSample_t *pRecSample);
int DSOUNDRecord_Init(void);
void DSOUNDRecord_Shutdown(void);
void DSOUNDRecord_Frame(void);
int mixerSetRecordLevel(char *SrcName, unsigned short newLevel);
int mixerGetRecordLevel(char *SrcName);
int mixerGetRecordSource(char *srcName);
int mixerSetRecordSource(char *SrcName);
int mixerSetMicrophoneMute(char bMute);

int DSOUNDRecord_Start(recordingSample_t *pRecSample)
{
    if (!g_recording_initialized)
        return -1;

    if (!sAudioRecorder)
        return -1;

    if (!CAudioRecorder_Start(sAudioRecorder)) {
        Com_Printf("error: Unable to Read to Buffer\n");
        return -1;
    }

    g_current_sample = pRecSample;
    g_currently_recording = 1;
    return 0;
}

int DSOUNDRecord_Stop(recordingSample_t *pRecSample)
{
    if (!g_recording_initialized)
        return -1;

    if (!sAudioRecorder)
        return -1;

    CAudioRecorder_Stop(sAudioRecorder);
    g_current_sample = 0;
    g_currently_recording = 0;
    return 0;
}

recordingSample_t *DSOUNDRecord_NewSample(void)
{
    recordingSample_t *sample;

    if (!g_recording_initialized)
        return NULL;

    sample = s_recordingSamplePtr;
    s_recordingSamplePtr = (recordingSample_t *)((char *)s_recordingSamplePtr + 0x24);

    memset(sample, 0, 9 * sizeof(int));

    sample->frequency = g_sound_recordFrequency;
    sample->volume = g_sound_recordVolume;
    sample->pan = 0x80;
    sample->channels = g_sound_channels;
    sample->playMode = 2;

    return sample;
}

int DSOUNDRecord_DestroySample(recordingSample_t *pRecSample)
{
    return g_recording_initialized != 0;
}

int DSOUNDRecord_Init(void)
{
    void *recorder;

    s_recordingSamplePtr = (recordingSample_t *)0xff2220;
    g_recording_initialized = 0;

    recorder = __Znwm(0x78);
    CAudioRecorder_CAudioRecorder(recorder, (double)g_sound_recordFrequency, g_sound_channels, 16);
    sAudioRecorder = (CAudioRecorder*)recorder;

    if (!CAudioRecorder_Initialize(recorder)) {
        if (sAudioRecorder) {
            ZN14CAudioRecorderD1Ev(sAudioRecorder);
            __ZdlPv(sAudioRecorder);
        }
        sAudioRecorder = 0;
        return 0;
    }

    g_recording_initialized = 1;
    return 1;
}

void DSOUNDRecord_Shutdown(void)
{

    CAudioRecorder *rec = sAudioRecorder;
    if (rec) {
        CAudioRecorder_Shutdown(rec);
        rec = sAudioRecorder;
        if (rec) {
            ZN14CAudioRecorderD1Ev(rec);
            __ZdlPv(rec);
        }
        sAudioRecorder = 0;
    }
    g_recording_initialized = 0;
}

void DSOUNDRecord_Frame(void)
{
    void *buffer;
    unsigned int size;
    audioSample_t audioSample;

    if (!g_recording_initialized)
        return;
    if (!g_currently_recording)
        return;
    if (!g_current_sample)
        return;

    buffer = CAudioRecorder_GetBuffer(sAudioRecorder, (int *)&size);
    if (!buffer)
        return;
    if (!size)
        return;

    audioSample.stereo = g_sound_channels > 1;
    audioSample.bytesPerSample = 2;
    audioSample.frequency = g_sound_recordFrequency;
    audioSample.sampleOffset = 0;
    audioSample.channels = g_sound_channels;
    audioSample.lengthInSamples = size / (g_sound_channels * 2);
    audioSample.lengthInBytes = size;
    audioSample.buffer = (unsigned char*)buffer;

    current_audioCallback(&audioSample);
}

int mixerSetRecordLevel(char *SrcName, unsigned short newLevel)
{
    if (sAudioRecorder) {
        float level = (float)newLevel / 65535.0f;
        CAudioRecorder_SetRecordLevel(sAudioRecorder, level);
    }
    return 0;
}

int mixerGetRecordLevel(char *SrcName)
{
    if (!sAudioRecorder)
        return 0xFFFF;

    float level = CAudioRecorder_GetRecordLevel(sAudioRecorder);
    return (int)(level * 65535.0);
}

int mixerGetRecordSource(char *srcName)
{
    *(int *)srcName = 0x63614d;
    return 1;
}

int mixerSetRecordSource(char *SrcName)
{
    return 1;
}

int mixerSetMicrophoneMute(char bMute)
{
    if (sAudioRecorder) {
        CAudioRecorder_Mute(sAudioRecorder, !bMute);
    }
    return 0;
}

#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <stdio.h>

int g_voice_initialized = 0x0;

extern const dvar_t *Dvar_RegisterBool(const char *name, int defaultValue, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float defaultValue, float min, float max, int flags);
extern int Record_Init(int callInit, const void *handle);
extern void Sound_Init(int freq);
extern Bool Encode_Init(int freq);
extern unsigned char Decode_Init(int freq);
extern void Record_Shutdown(void);
extern Bool Encode_Shutdown(void);
extern void Decode_Shutdown(void);
extern void Sound_Shutdown(void);
extern dsound_sample_t *Sound_NewSample(void);
extern void Sound_SampleFrame(dsound_sample_t *sample);
extern int Sound_DestroySample(dsound_sample_t *sample);
extern int Sound_UpdateSample(dsound_sample_t *sample, char *data, unsigned int size);
extern void Sound_Frame(void);
extern void Record_Frame(void);
extern int Record_Start(recordingSample_t *sample);
extern int Record_Stop(recordingSample_t *sample);
extern recordingSample_t *Record_NewSample(void);
extern int Record_DestroySample(recordingSample_t *sample);
extern void mixerGetRecordSource(char *source);
extern void mixerSetRecordSource(const char *source);
extern int mixerGetRecordLevel(const char *source);
extern void mixerSetRecordLevel(const char *source, int level);
extern void mixerSetMicrophoneMute(int mute);
extern int Sys_Milliseconds(void);
extern int Decode_Sample(char *inData, int inSize, short *outData, int maxOutSize);

extern byte *voice_freq_ptr;
extern byte *voice_maxframe_ptr;

extern const dvar_t *winvoice_mic_mute;
extern const dvar_t *winvoice_mic_reclevel;
extern const dvar_t *winvoice_save_voice;
extern const dvar_t *winvoice_mic_scaler;
extern int mic_old_reclevel;
extern int mic_current_reclevel;
extern char old_rec_source[256];
extern float voice_current_voicelevel;
extern float voice_current_scaler;
extern int g_voice_initialized;
extern void Com_Printf(const char *fmt, ...);
extern float levelSamples[6];
extern int sampleCount;
static int count;
static Bool recording;
static Bool playing;
static recordingSample_t *currentRecordingSample;

static short unsigned int *(*s_clientSamples[128])[4];
static int s_clientTalkTime[64];
static int s_voiceBandwidth;

Bool Voice_Init(void);
float Voice_GetVoiceLevel(void);
void Voice_Playback(void);
void Voice_IncomingVoiceData(int talker, unsigned char *data, int packetDataSize);
Bool Voice_IsClientTalking(int clientNum);
void Voice_Shutdown(void);
int Voice_GetLocalVoiceData(ClientVoicePacket_t *voiceData);

Bool Voice_Init(void)
{
    int *freq;
    int i;
    int level;

    winvoice_mic_mute = Dvar_RegisterBool("winvoice_mic_mute", 1, 0x1001);
    winvoice_mic_reclevel = Dvar_RegisterFloat("winvoice_mic_reclevel", 65535.0f, 0.0f, 65535.0f, 0x1001);
    winvoice_save_voice = Dvar_RegisterBool("winvoice_save_voice", 0, 0x1001);
    winvoice_mic_scaler = Dvar_RegisterFloat("winvoice_mic_scaler", 1.0f, 0.25f, 2.0f, 0x1001);

    /* Voice chat is not in this build yet: its recording, encoding and
     * decoding still read zeroed stand-ins for the original's data
     * (record_callback_ptr, encode_vol_ptr, voice_scale_ptr,
     * voice_maxframe_ptr, the speex mode and quality pointers) as pointers,
     * and wrote through NULL as soon as the game's sound started. The rest
     * of the sound runs without it. */
    Com_Printf("Voice chat is not available in this build\n");
    return 0;

    /* not reached until voice chat is done; then: the original zeroes a
     * global it hands Encode_Init and Decode_Init (0x19a1c2c), where the
     * reconstruction read voice_freq_ptr, a zeroed blob, as a pointer to it
     * and wrote through NULL */
    freq = &s_voiceBandwidth;
    *freq = 0;

    if (!Record_Init(0, 0))
        return 0;

    Sound_Init(0);
    Encode_Init(*freq);
    Decode_Init(*freq);

    mixerGetRecordSource(old_rec_source);
    mixerSetRecordSource("Mic");
    mic_old_reclevel = mixerGetRecordLevel("Mic");

    level = (unsigned short)(int)winvoice_mic_reclevel->current.value;
    mic_current_reclevel = level;

    level = (unsigned short)(int)winvoice_mic_reclevel->current.value;
    mixerSetRecordLevel("Mic", level);

    mixerSetMicrophoneMute(winvoice_mic_mute->current.enabled);

    g_voice_initialized = 1;

    memset(s_clientTalkTime, 0, sizeof(s_clientTalkTime));

    for (i = 0; i < 64; i++) {
        s_clientSamples[i] = (unsigned short *(*)[4])(Sound_NewSample());
    }

    return 0;
}

float Voice_GetVoiceLevel(void)
{
    float result;
    int idx;
    int i;

    if (!g_voice_initialized)
        return 0.0f;

    idx = sampleCount % 6;
    result = voice_current_voicelevel / 32767.0f / 6.0f;
    levelSamples[idx] = result;
    sampleCount++;

    result = 0.0f;
    for (i = 0; i < 6; i++) {
        result += levelSamples[i];
    }

    return result;
}

void Voice_Playback(void)
{
    int level;
    int i;

    if (!g_voice_initialized)
        return;

    level = (unsigned short)(int)winvoice_mic_reclevel->current.value;
    if (level != mic_current_reclevel) {
        mic_current_reclevel = level;
        level = (unsigned short)(int)winvoice_mic_reclevel->current.value;
        mixerSetRecordLevel("Mic", level);
    }

    for (i = 0; i < 64; i++) {
        Sound_SampleFrame((dsound_sample_t *)s_clientSamples[i]);
    }

    Sound_Frame();
}

void Voice_IncomingVoiceData(int talker, unsigned char *data, int packetDataSize)
{
    short decodedData[4096];
    int talkerIdx;
    int processedBytes;
    int maxFrameSize;
    int remaining;
    int bytesToProcess;
    int decodedLen;
    FILE *fptr;

    if (!playing)
        playing = 1;

    talkerIdx = (unsigned char)talker;
    s_clientTalkTime[talkerIdx] = Sys_Milliseconds();

    if (!g_voice_initialized)
        return;  /* the talker is shown, but there is nothing to play it with */

    decodedLen = 0;
    if (packetDataSize > 0) {
        processedBytes = 0;
        maxFrameSize = *(int *)voice_maxframe_ptr;

        while (processedBytes < packetDataSize) {
            remaining = packetDataSize - processedBytes;
            bytesToProcess = remaining < maxFrameSize ? remaining : maxFrameSize;

            decodedLen = Decode_Sample((char *)(data + processedBytes), bytesToProcess, decodedData, maxFrameSize);
            if (decodedLen > 0) {
                Sound_UpdateSample((dsound_sample_t *)s_clientSamples[talkerIdx], (char *)decodedData, decodedLen);
            }
            processedBytes += bytesToProcess;
        }
    }

    if (winvoice_save_voice->current.enabled) {
        fptr = fopen("voice.wav", "a");
        if (fptr != NULL) {
            fwrite(decodedData, decodedLen, 1, fptr);
            fclose(fptr);
        }
    }
}

Bool Voice_IsClientTalking(int clientNum)
{
    return (Sys_Milliseconds() - s_clientTalkTime[clientNum]) <= 299;
}

void Voice_Shutdown(void)
{
    int i;

    if (!g_voice_initialized)
        return;

    for (i = 0; i < 64; i++) {
        Sound_DestroySample((dsound_sample_t *)s_clientSamples[i]);
    }

    if (recording) {
        Record_Stop(currentRecordingSample);
        Record_DestroySample(currentRecordingSample);
        recording = 0;
    }

    Record_Shutdown();
    Encode_Shutdown();
    Decode_Shutdown();
    Sound_Shutdown();

    mixerSetMicrophoneMute(1);
    mixerSetRecordLevel("Mic", (unsigned short)mic_old_reclevel);
    mixerSetRecordSource(old_rec_source);

    g_voice_initialized = 0;
}

int Voice_GetLocalVoiceData(ClientVoicePacket_t *voiceData)
{
    float scaler;

    if (!g_voice_initialized)
        return 0;

    if (!recording) {
        currentRecordingSample = Record_NewSample();
        Record_Start(currentRecordingSample);
        recording = 1;
        count++;
    }

    scaler = winvoice_mic_scaler->current.value;
    if (scaler != voice_current_scaler) {
        voice_current_scaler = scaler;
        if (scaler < 0.5f)
            voice_current_scaler = 0.5f;
        else if (scaler > 1.5f)
            voice_current_scaler = 1.5f;
    }

    Record_Frame();
    return 0;
}

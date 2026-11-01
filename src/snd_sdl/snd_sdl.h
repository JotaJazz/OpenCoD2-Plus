#ifndef SND_SDL_H
#define SND_SDL_H

#define SND_SMP_FREE 1
#define SND_SMP_DONE 2
#define SND_SMP_PLAYING 4
#define SND_SMP_STOPPED 8

typedef struct SndVoice {
    const unsigned char *data; unsigned dataLen;
    int bits, channels, rate;      /* rate 0 = silent */
    int loopCount;                 /* 0 = forever, as Miles */
    float left, right, wet;
    double pos;                    /* source frames */
    int status;
} SndVoice;

/* Writes the mix of every SND_SMP_PLAYING voice to out (interleaved PCM16, saturated).
   wetOut may be NULL; it receives the mono mix sent to the reverb. */
void SndMix_Render(SndVoice *voices, int count, int outRate, int outChannels,
                   short *out, float *wetOut, int frames);

/* Layout-compatible with AILSOUNDINFO / AILMIXINFO (src/headers/cod2_defs.h), so the AIL layer can
   cast; kept local so the engine and its test build without the game headers. long as in the game. */
typedef struct SndSoundInfo {
    long format;                   /* WAVE tag: 1 PCM, 0x11 IMA ADPCM */
    const void *data_ptr;
    unsigned long data_len, rate;
    long bits, channels;
    unsigned long samples, block_size;
    const void *initial_ptr;
} SndSoundInfo;

typedef struct SndMixInfo {
    SndSoundInfo Info;
    unsigned long adpcm[8];        /* ADPCMDATA, unused here */
    unsigned short moresamples[16];
    unsigned long src_fract;
    long left_val, right_val;
} SndMixInfo;

/* Reads a WAV (PCM or IMA ADPCM) held in data[0..fileLen). Returns 1, or 0 if it is malformed. */
long SndWav_Info(const void *data, long fileLen, SndMixInfo *info);
/* format: 0/1/2/3 = mono8/mono16/stereo8/stereo16 (PCM sources), 5/7 = ADPCM mono/stereo (copied as is).
   Both return 0 on anything unsupported or invalid. */
long SndWav_ConvertedSize(unsigned rate, unsigned format, const SndMixInfo *src);
long SndWav_Convert(void *dest, long size, unsigned rate, unsigned format, const SndMixInfo *src);
/* IMA ADPCM to interleaved PCM16; returns the frames written (at most maxFrames). */
int SndAdpcm_Decode(const unsigned char *src, int srcLen, int block, int channels, short *dst, int maxFrames);

#endif

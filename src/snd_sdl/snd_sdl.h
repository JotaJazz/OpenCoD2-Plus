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

/* Adds every SND_SMP_PLAYING voice into out (interleaved PCM16, saturated).
   wetOut may be NULL; it receives the mono mix sent to the reverb. */
void SndMix_Render(SndVoice *voices, int count, int outRate, int outChannels,
                   short *out, float *wetOut, int frames);

#endif

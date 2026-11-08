#include <string.h>
#include "snd_sdl.h"

/* Source sample of frame i, channel c, as a float in PCM16 units. */
static float Sample(const SndVoice *v, unsigned i, int c)
{
    if (v->bits == 8)
        return ((int)v->data[i * v->channels + c] - 128) * 256.0f;
    {
        const unsigned char *p = v->data + ((size_t)i * v->channels + c) * 2;
        return (float)(short)(p[0] | p[1] << 8);
    }
}

#define CHUNK 512  /* the audio thread must not allocate: accumulate on the stack */

static void Chunk(SndVoice *voices, int count, int outRate, int outChannels,
                  short *out, float *wetOut, int frames)
{
    float acc[CHUNK * 2] = { 0 };
    int n, f;

    if (wetOut)
        memset(wetOut, 0, (size_t)frames * sizeof(float));
    for (n = 0; n < count; n++) {
        SndVoice *v = &voices[n];
        unsigned bytes = (unsigned)(v->bits / 8) * v->channels, total;
        double step;

        if (v->status != SND_SMP_PLAYING || v->rate <= 0 || bytes == 0 || outRate <= 0)
            continue;
        total = v->dataLen / bytes;
        step = (double)v->rate / outRate;
        for (f = 0; f < frames; f++) {
            unsigned i = (unsigned)v->pos, j = i + 1;
            float t = (float)(v->pos - i), l, r;

            if (i >= total) {
                v->status = SND_SMP_DONE;
                break;
            }
            if (j >= total)
                j = v->loopCount == 1 ? i : 0;
            l = Sample(v, i, 0);
            l += (Sample(v, j, 0) - l) * t;
            r = l;
            if (v->channels == 2) {
                r = Sample(v, i, 1);
                r += (Sample(v, j, 1) - r) * t;
            }
            acc[f * 2] += l * v->left;
            acc[f * 2 + 1] += r * v->right;
            if (wetOut)
                wetOut[f] += (l * v->left + r * v->right) * 0.5f * v->wet;

            v->pos += step;
            if (v->pos >= total) {
                if (v->loopCount != 0 && --v->loopCount == 0) {
                    v->status = SND_SMP_DONE;
                    break;
                }
                v->pos -= total * (double)(unsigned)(v->pos / total);
            }
        }
    }
    for (f = 0; f < frames; f++) {
        float s[2] = { acc[f * 2], acc[f * 2 + 1] };
        int c;

        if (outChannels == 1)
            s[0] = (s[0] + s[1]) * 0.5f;
        for (c = 0; c < outChannels; c++) {
            float x = s[c < 2 ? c : 0];
            out[f * outChannels + c] = x >= 32767.0f ? 32767 : x <= -32768.0f ? -32768 : (short)(x < 0 ? x - 0.5f : x + 0.5f);
        }
    }
}

void SndMix_Render(SndVoice *voices, int count, int outRate, int outChannels,
                   short *out, float *wetOut, int frames)
{
    while (frames > 0) {
        int n = frames < CHUNK ? frames : CHUNK;

        Chunk(voices, count, outRate, outChannels, out, wetOut, n);
        out += n * outChannels;
        if (wetOut)
            wetOut += n;
        frames -= n;
    }
}

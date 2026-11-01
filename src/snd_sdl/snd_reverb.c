/* The reverb bus: Freeverb's shape, mono in, stereo out. Not Miles' filter and no data of Miles: the
   one intended difference in sound (ADR 0012). The buffers are static for the audio thread. */
#include <math.h>
#include <string.h>
#include "snd_sdl.h"

enum {
    COMBS = 8,
    ALLPASSES = 4,
    SPREAD = 23,                   /* frames more on the right, for width */
    ROOMS = 26,
    TUNING_RATE = 44100,
    MAX_RATE = 96000,              /* past it the delays stop growing, and the decay still holds */
    MAX_COMB = (1617 + SPREAD) * MAX_RATE / TUNING_RATE + 1,
    MAX_ALLPASS = (556 + SPREAD) * MAX_RATE / TUNING_RATE + 1,
    MAX_PREDELAY = MAX_RATE / 2,
    FADE_MS = 10
};

#define INPUT_GAIN 0.0625f         /* half of one over COMBS: a tail at that level ran about 8 dB hot */
#define ALLPASS_FEEDBACK 0.5f
#define SILENT_IN 1e-3f            /* PCM16 units: below it a send is no send */
#define SILENT_OUT 0.5f
#define SLEEP_SECONDS 0.6          /* past the longest predelay and comb */
#define HF_REFERENCE 5000.0        /* EFX's */

/* The public EAX 2 environments, as in OpenAL Soft's efx-presets.h: decay is flDecayTime, hfRatio
   flDecayHFRatio, predelay flReflectionsDelay + flLateReverbDelay, level flGain * flLateReverbGain *
   sqrt(flGainHF), which stands in for a filter on the input. Generic's level is 0 as Miles' room 0
   (0x21155838); the rest are for tuning by ear: when in doubt, lower. */
static const struct { float decay, hfRatio, predelay, level; } rooms[ROOMS] = {
    { 1.49f, 0.83f, 0.018f, 0.0f },     /* generic */
    { 0.17f, 0.10f, 0.003f, 0.013f },   /* paddedcell */
    { 0.40f, 0.83f, 0.005f, 0.259f },   /* room */
    { 1.49f, 0.54f, 0.018f, 0.519f },   /* bathroom */
    { 0.50f, 0.10f, 0.007f, 0.003f },   /* livingroom */
    { 2.31f, 0.64f, 0.029f, 0.293f },   /* stoneroom */
    { 4.32f, 0.59f, 0.050f, 0.172f },   /* auditorium */
    { 3.92f, 0.70f, 0.049f, 0.237f },   /* concerthall */
    { 2.91f, 1.30f, 0.037f, 0.223f },   /* cave */
    { 7.24f, 0.33f, 0.050f, 0.216f },   /* arena */
    { 10.05f, 0.23f, 0.050f, 0.223f },  /* hangar */
    { 0.30f, 0.10f, 0.032f, 0.005f },   /* carpetedhallway */
    { 1.49f, 0.59f, 0.018f, 0.442f },   /* hallway */
    { 2.70f, 0.79f, 0.033f, 0.435f },   /* stonecorridor */
    { 1.49f, 0.86f, 0.018f, 0.269f },   /* alley */
    { 1.49f, 0.54f, 0.250f, 0.036f },   /* forest */
    { 1.49f, 0.67f, 0.018f, 0.028f },   /* city */
    { 1.49f, 0.21f, 0.400f, 0.014f },   /* mountains */
    { 1.49f, 0.83f, 0.086f, 0.316f },   /* quarry */
    { 1.49f, 0.50f, 0.279f, 0.011f },   /* plain */
    { 1.65f, 1.50f, 0.020f, 0.084f },   /* parkinglot */
    { 2.81f, 0.14f, 0.035f, 0.577f },   /* sewerpipe */
    { 1.49f, 0.10f, 0.018f, 0.45f },    /* underwater: doubled, so INPUT_GAIN leaves shock/default.shock's level as it was */
    { 8.39f, 1.39f, 0.032f, 0.983f },   /* drugged */
    { 17.23f, 0.56f, 0.050f, 0.124f },  /* dizzy */
    { 7.56f, 0.91f, 0.050f, 0.707f }    /* psychotic */
};

static const short combFrames[COMBS] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static const short allpassFrames[ALLPASSES] = { 556, 441, 341, 225 };

typedef struct Comb {
    int len, pos;
    float feedback, damp, lowpass;
    float buf[MAX_COMB];
} Comb;

typedef struct Allpass {
    int len, pos;
    float buf[MAX_ALLPASS];
} Allpass;

static struct {
    Comb comb[2][COMBS];
    Allpass allpass[2][ALLPASSES];
    float pre[MAX_PREDELAY];
    int prePos, preFrames;
    int room, active;              /* asked for, and playing */
    int rate, quiet;               /* quiet: frames with nothing going in or out */
    float dry, wet, fade, fadeStep;
} rv = { .dry = 1 };

static float Cap(float x) { return x > 0 ? (x < 1 ? x : 1) : 0; }   /* NaN goes to 0 */

static double DelayRate(void) { return rv.rate < MAX_RATE ? rv.rate : MAX_RATE; }

/* Each comb loses 60 dB in decay seconds whatever its length, so a room lasts the same at any rate.
   damp: the one-pole in the loop that makes hfRatio * decay of that at HF_REFERENCE. Below 1 the loop
   gain stays under 1 at every frequency: any table is stable. */
static void Apply(int room)
{
    double decay = rooms[room].decay, hf = rooms[room].hfRatio;
    double w = 2 * M_PI * (HF_REFERENCE < rv.rate * 0.45 ? HF_REFERENCE : rv.rate * 0.45) / rv.rate;
    int c, i;

    for (c = 0; c < 2; c++)
        for (i = 0; i < COMBS; i++) {
            Comb *comb = &rv.comb[c][i];
            double seconds = (double)comb->len / rv.rate, damp = 0;

            comb->feedback = (float)pow(10, -3 * seconds / decay);
            if (hf < 1) {
                double a = pow(10, -3 * seconds / decay * (1 / hf - 1));
                double b = (1 - a * a * cos(w)) / (1 - a * a);

                damp = b - sqrt(b * b - 1);
            }
            comb->damp = (float)damp;
        }
    rv.preFrames = (int)(rooms[room].predelay * DelayRate());
    rv.active = room;
}

static void Start(int rate)
{
    int c, i;

    rv.rate = rate;
    rv.prePos = 0;
    rv.quiet = 0;
    memset(rv.pre, 0, sizeof rv.pre);
    for (c = 0; c < 2; c++) {
        for (i = 0; i < COMBS; i++) {
            Comb *comb = &rv.comb[c][i];

            comb->len = (int)((combFrames[i] + c * SPREAD) * DelayRate() / TUNING_RATE) + 1;
            comb->pos = 0;
            comb->lowpass = 0;
            memset(comb->buf, 0, sizeof comb->buf);
        }
        for (i = 0; i < ALLPASSES; i++) {
            Allpass *ap = &rv.allpass[c][i];

            ap->len = (int)((allpassFrames[i] + c * SPREAD) * DelayRate() / TUNING_RATE) + 1;
            ap->pos = 0;
            memset(ap->buf, 0, sizeof ap->buf);
        }
    }
    Apply(rv.room);
    rv.fade = 1;
    rv.fadeStep = 1000.0f / FADE_MS / rate;
}

static float Channel(int c, float in)
{
    float sum = 0;
    int i;

    for (i = 0; i < COMBS; i++) {
        Comb *comb = &rv.comb[c][i];
        float y = comb->buf[comb->pos];

        comb->lowpass = y + (comb->lowpass - y) * comb->damp;
        comb->buf[comb->pos] = in + comb->lowpass * comb->feedback;
        if (++comb->pos == comb->len) comb->pos = 0;
        sum += y;
    }
    for (i = 0; i < ALLPASSES; i++) {
        Allpass *ap = &rv.allpass[c][i];
        float y = ap->buf[ap->pos];

        ap->buf[ap->pos] = sum + y * ALLPASS_FEEDBACK;
        if (++ap->pos == ap->len) ap->pos = 0;
        sum = y - sum;
    }
    return sum;
}

/* Adds the tail; returns its peak. A new room waits for the input to fade, as its predelay is another
   read point; what already rings rings on. */
static float Tail(const float *wetIn, short *out, int frames, int outChannels)
{
    float peak = 0;
    int f, c;

    for (f = 0; f < frames; f++) {
        int read = rv.prePos - rv.preFrames;
        float in = wetIn[f] * rv.wet, s[2];

        if (rv.room != rv.active) {
            rv.fade -= rv.fadeStep;
            if (rv.fade <= 0) {
                rv.fade = 0;
                Apply(rv.room);
            }
        } else if (rv.fade < 1) {
            rv.fade += rv.fadeStep;
            if (rv.fade > 1) rv.fade = 1;
        }
        if (read < 0) read += MAX_PREDELAY;
        rv.pre[rv.prePos] = in > 32767.0f ? 32767.0f : in < -32768.0f ? -32768.0f : in;   /* it saturates as the dry mix */
        in = rv.pre[read] * rv.fade * INPUT_GAIN;   /* preFrames 0 reads the frame just written */
        if (++rv.prePos == MAX_PREDELAY) rv.prePos = 0;
        s[0] = Channel(0, in);
        s[1] = Channel(1, in);
        if (fabsf(s[0]) > peak) peak = fabsf(s[0]);
        if (fabsf(s[1]) > peak) peak = fabsf(s[1]);
        if (outChannels == 1)
            s[0] = (s[0] + s[1]) * 0.5f;
        for (c = 0; c < outChannels; c++) {
            short *o = &out[f * outChannels + c];
            float x = *o + s[c < 2 ? c : 0];

            *o = x >= 32767.0f ? 32767 : x <= -32768.0f ? -32768 : (short)(x < 0 ? x - 0.5f : x + 0.5f);
        }
    }
    return peak;
}

void SndReverb_SetRoom(int room)   /* 0x21115930 */
{
    rv.room = room >= 0 && room < ROOMS ? room : 0;
    rv.wet = rooms[rv.room].level;
}

void SndReverb_SetLevels(float dry, float wet)
{
    rv.dry = Cap(dry);
    rv.wet = Cap(wet);
}

/* Asleep once quiet: a tail stops before its floats could reach denormals, which would be hundreds of
   dB and many of its decay times below the last bit of PCM16. */
void SndReverb_Process(const float *wetIn, short *out, int frames, int outChannels, int outRate)
{
    float send = 0;
    int i;

    if (outRate <= 0) {
        rv.rate = 0;
        return;
    }
    if (outRate != rv.rate) Start(outRate);
    if (rv.dry < 1)
        for (i = 0; i < frames * outChannels; i++) out[i] = (short)(out[i] * rv.dry);
    for (i = 0; i < frames; i++)
        if (fabsf(wetIn[i]) > send) send = fabsf(wetIn[i]);
    if (send * rv.wet < SILENT_IN && rv.quiet >= rv.rate * SLEEP_SECONDS) {
        if (rv.room != rv.active) Apply(rv.room);
        rv.fade = 1;
        return;
    }
    if (Tail(wetIn, out, frames, outChannels) >= SILENT_OUT || send * rv.wet >= SILENT_IN)
        rv.quiet = 0;
    else if (rv.quiet < rv.rate)
        rv.quiet += frames;
}

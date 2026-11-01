/* The AIL_* calls snd_driver.c makes, on our own engine and an SDL device. Addresses are mss32.dll's
   (base 0x21100000) and msssoft.m3d's (base 0x22400000). Handles are pointers into samples[]; streams
   are samples too. The audio callback runs under SDL's device lock, so everything it reads is written
   under Lock(). The game's thread is the only caller of the AIL_* functions. */
#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snd_sdl.h"

/* Task 8: Sdl_##name and __stdcall on Windows, where mss32_dll.c keeps the AIL_* names */
#define SA_NAME(name) name
#define SA_API

extern void Com_Printf(const char *fmt, ...);
extern void Cmd_AddCommand(const char *cmdName, void (*function)(void));
extern void Cmd_RemoveCommand(const char *cmdName);
extern long sndWavFileLen;         /* snd_driver.c: AIL_WAV_info's signature has no length */
void SA_API SA_NAME(AIL_close_stream)(void *S);

enum {
    MAX_SAMPLES = 54,              /* 8 2D, 32 3D, 13 streams, raw */
    MAX_3D = 32,
    DEVICE_FRAMES = 1024,
    BLOCK = 256,                   /* frames between refills of the stream and raw voices */
    PCM_FRAMES = BLOCK * 16 + 2,   /* a source up to 16 times the device: 2x at 11 kHz is 8; past it, gaps */
    MAX_DECODED = 256 * 1024 * 1024,   /* ADPCM to PCM16, against a file that claims too much */
    DEFAULT_RATE = 11025,          /* 0x2112a771 */
    DEFAULT_BLOCK = 0x100,         /* 0x2112a78d */
    FORMAT_ADPCM = 4               /* the bit in the DIG_F_* types */
};

typedef struct Sample {
    SndVoice *v;                   /* voices[] has to be contiguous for SndMix_Render */
    int used, format, block, rate;
    float left, right;             /* as set: the mixer's are capped */
    const unsigned char *src;
    unsigned srcLen;
    short *decoded;                /* ADPCM's PCM16, owned here */

    float pos[3], volume, minDist, maxDist;   /* 3D */

    /* a stream or raw voice plays pcm[0..frames), refilled in Feed like a window on its source */
    SndStream *stream;
    int raw;
    const unsigned char *buf[2];   /* raw's two buffers, as S+0xc to S+0x28 */
    unsigned len[2], off[2];
    int done[2], cur;
    int frames;
    short pcm[PCM_FRAMES * 2];     /* room for 8 bit too */
} Sample;

static Sample samples[MAX_SAMPLES];
static SndVoice voices[MAX_SAMPLES];
static struct {
    SDL_AudioDeviceID dev;
    int rate, channels, underruns;
    double bufferMs, lastMs, worstMs;
    Uint32 lastRender;             /* SDL_GetTicks */
} dig;
static SndFileCallbacks files;
static float rolloff, distFactor;
static char lastError[256];

static void Lock(void) { if (dig.dev) SDL_LockAudioDevice(dig.dev); }
static void Unlock(void) { if (dig.dev) SDL_UnlockAudioDevice(dig.dev); }

static float Cap(float x) { return x > 0 ? (x < 1 ? x : 1) : 0; }   /* 0x2112be21; NaN goes to 0 */

/* What a sample has to play from. Caller locks. */
static void Detach(Sample *s)
{
    SndVoice *v = s->v;

    free(s->decoded);
    s->decoded = NULL;
    s->src = NULL;
    s->srcLen = 0;
    s->raw = s->frames = 0;
    v->data = NULL;
    v->dataLen = 0;
    v->pos = 0;
    if (v->status != SND_SMP_FREE) v->status = SND_SMP_DONE;
}

static void Reset(Sample *s)   /* 0x2112a700 */
{
    SndVoice *v = s->v;

    Detach(s);
    s->format = 0;
    s->block = DEFAULT_BLOCK;
    s->rate = v->rate = DEFAULT_RATE;
    v->bits = 8;
    v->channels = v->loopCount = 1;
    SndSpatial_VolumePan(1.0f, 0.5f, &s->left, &s->right);
    v->left = s->left;
    v->right = s->right;
    v->wet = 0;
    s->cur = 0;
    s->done[0] = s->done[1] = 1;
    v->status = SND_SMP_DONE;
}

/* A free sample, reset; NULL when all are taken. */
static Sample *Allocate(void)
{
    int i;

    for (i = 0; i < MAX_SAMPLES; i++)
        if (!samples[i].used) {
            samples[i].used = 1;
            samples[i].v = &voices[i];
            Reset(&samples[i]);
            return &samples[i];
        }
    snprintf(lastError, sizeof lastError, "out of sample handles");
    return NULL;
}

/* raw: reads from the two buffers, flipping as the mixer does. Returns the bytes read. */
static unsigned RawRead(Sample *s, unsigned char *out, unsigned want)
{
    unsigned got = 0;

    while (want) {
        int c = s->cur;
        unsigned n;

        if (s->done[c]) {
            if (s->done[c ^ 1]) break;
            s->cur = c ^= 1;
        }
        n = s->len[c] - s->off[c];
        if (n > want) n = want;
        memcpy(out + got, s->buf[c] + s->off[c], n);
        s->off[c] += n;
        got += n;
        want -= n;
        if (s->off[c] == s->len[c]) {
            s->done[c] = 1;
            s->cur = c ^ 1;
        }
    }
    return got;
}

/* Gives a stream or raw voice what the mixer takes in the next frames, plus a frame to interpolate
   towards, keeping what it has not played. The source is short only on an underrun: silence. */
static void Feed(Sample *s, int frames)
{
    SndVoice *v = s->v;
    int fb = v->bits / 8 * v->channels, drop = (int)v->pos, have, need, got;
    unsigned char *pcm = (unsigned char *)s->pcm;

    if (v->status == SND_SMP_STOPPED || v->status == SND_SMP_FREE) return;
    if (drop > s->frames) drop = s->frames;
    have = s->frames - drop;
    memmove(pcm, pcm + (size_t)drop * fb, (size_t)have * fb);
    v->pos -= drop;
    need = (int)(v->pos + (double)frames * v->rate / dig.rate) + 2;
    if (need > PCM_FRAMES) need = PCM_FRAMES;
    if (have < need) {
        got = s->stream ? SndStream_Read(s->stream, (short *)(pcm + (size_t)have * fb), need - have)
                        : (int)(RawRead(s, pcm + (size_t)have * fb, (unsigned)(need - have) * fb) / fb);
        /* counted when it runs dry, not for each silent block; a seek empties it first */
        if (s->stream && got < need - have && have > 0 && SndStream_Status(s->stream) == SND_SMP_PLAYING)
            dig.underruns++;
        have += got;
    }
    s->frames = have;
    v->data = pcm;
    v->dataLen = (unsigned)have * fb;
    v->loopCount = 1;
    v->status = v->pos < have ? SND_SMP_PLAYING : SND_SMP_DONE;
}

static void Render(short *out, int frames)
{
    while (frames > 0) {
        int n = frames < BLOCK ? frames : BLOCK, i;

        for (i = 0; i < MAX_SAMPLES; i++)
            if (samples[i].stream || samples[i].raw) Feed(&samples[i], n);
        SndMix_Render(voices, MAX_SAMPLES, dig.rate, dig.channels, out, NULL, n);
        out += n * dig.channels;
        frames -= n;
    }
}

static void SDLCALL Callback(void *unused, Uint8 *out, int len)
{
    Uint64 start = SDL_GetPerformanceCounter();

    (void)unused;
    Render((short *)out, len / (2 * dig.channels));
    dig.lastMs = (double)(SDL_GetPerformanceCounter() - start) * 1000 / (double)SDL_GetPerformanceFrequency();
    if (dig.lastMs > dig.worstMs) dig.worstMs = dig.lastMs;
    dig.lastRender = SDL_GetTicks();
}

/* A device that is gone calls back no more: the voices run on the clock, so that they end and the
   game gets its channels back. Caller locks. */
static void Advance(void)
{
    short scratch[BLOCK * 2];
    Uint32 now, ms;
    int frames;

    if (!dig.dev || SDL_GetAudioDeviceStatus(dig.dev) == SDL_AUDIO_PLAYING) return;
    now = SDL_GetTicks();
    ms = now - dig.lastRender;
    if (ms <= (Uint32)dig.bufferMs) return;
    dig.lastRender = now;
    frames = ms < 1000 ? (int)(ms * (Uint64)dig.rate / 1000) : dig.rate;   /* the next poll goes on */
    for (; frames > 0; frames -= BLOCK) Render(scratch, frames < BLOCK ? frames : BLOCK);
}

static void Stats_f(void)
{
    double worst, last;
    int underruns;

    Lock();
    worst = dig.worstMs;
    last = dig.lastMs;
    underruns = dig.underruns;
    Unlock();
    Com_Printf("snd_stats: worst callback %.3f ms, last %.3f ms, buffer %.3f ms, underruns %i\n",
               worst, last, dig.bufferMs, underruns);
}

/* The bytes of a frame of what the voice holds. */
static unsigned FrameBytes(const SndVoice *v) { return (unsigned)(v->bits / 8 * v->channels); }

/* Caller locks. Clamped: the mixer casts pos to unsigned. */
static void SetPosition(SndVoice *v, double frame)
{
    double total = v->dataLen / FrameBytes(v);

    v->pos = frame > 0 ? (frame < total ? frame : total) : 0;
}

static long Milliseconds(double frames, int rate)   /* 0x2112bac1 rounds; a rate of 0 has no length */
{
    double ms = rate > 0 ? frames * 1000 / rate + 0.5 : 0;

    return ms < 2147483647.0 ? (long)ms : 2147483647L;
}

/* Publishes the sample's source, with ADPCM decoded first and off the lock. Its block size comes
   after its address (snd_driver.c:1138), so both land here. A bad size just leaves it silent. */
static void SetData(Sample *s)
{
    SndVoice *v = s->v;
    int adpcm = s->format & FORMAT_ADPCM;
    long hdr = 4L * v->channels;
    long long frames = 0;
    short *pcm = NULL, *old;

    if (adpcm && s->src && s->block >= hdr) {
        frames = ((long long)s->srcLen / s->block + 1) * ((s->block - hdr) / hdr * 8 + 1);
        if (frames * v->channels * 2 <= MAX_DECODED) pcm = malloc((size_t)frames * v->channels * 2);
        if (pcm) frames = SndAdpcm_Decode(s->src, (int)s->srcLen, s->block, v->channels, pcm, (int)frames);
    }
    Lock();
    old = s->decoded;
    s->decoded = pcm;
    v->data = adpcm ? (const unsigned char *)pcm : s->src;
    v->dataLen = adpcm ? (pcm ? (unsigned)frames * v->channels * 2 : 0) : s->srcLen;
    v->pos = 0;
    Unlock();
    free(old);
}

static void SetGains(Sample *s)
{
    Lock();
    s->v->left = Cap(s->left);
    s->v->right = Cap(s->right);
    Unlock();
}

static void Spatialize(Sample *s)
{
    SndSpatial_Levels(s->pos, s->minDist, s->maxDist, rolloff, distFactor, s->volume, &s->left, &s->right);
    SetGains(s);
}

/* 0x2112a838: the volume and pan that would give these levels; the getter undoes the curve, 0x21115447 */
static void DerivedVolumePan(const Sample *s, float *volume, float *pan)
{
    double vol = s->right, p = 0;

    if (s->left > 1e-4f) {
        double x = pow(s->right / s->left, 10.0 / 3.0);
        p = x / (1 + x);
        vol = p > 1e-4f ? s->right * pow(p, -0.3) : s->left;
    }
    if (volume) *volume = vol > 0 ? (float)pow(vol, 0.6) : 0;
    if (pan) *pan = (float)p;
}

/* Setup */

int SA_API SA_NAME(AIL_startup)(int flags) { (void)flags; return 1; }
void SA_API SA_NAME(AIL_set_redist_directory)(const char *dir) { (void)dir; }
int SA_API SA_NAME(AIL_set_preference)(int pref, int value) { (void)pref; (void)value; return 0; }
const char *SA_API SA_NAME(AIL_last_error)(void) { return lastError; }

void SA_API SA_NAME(AIL_set_file_callbacks)(void *open_cb, void *close_cb, void *seek_cb, void *read_cb)
{
    files.open = (unsigned long (*)(const char *, unsigned long *))open_cb;
    files.close = (void (*)(unsigned long))close_cb;
    files.seek = (long (*)(unsigned long, long, unsigned long))seek_cb;
    files.read = (unsigned long (*)(unsigned long, void *, unsigned long))read_cb;
}

/* bits comes as bytes from snd_driver.c:1671 and matters not: samples are converted at load, the
   device is S16. A failure is for snd_driver.c's failure path. */
void *SA_API SA_NAME(AIL_open_digital_driver)(int freq, int bits, int channels, int flags)
{
    SDL_AudioSpec want, got;

    (void)bits;
    (void)flags;
    if (dig.dev) return &dig;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        snprintf(lastError, sizeof lastError, "%s", SDL_GetError());
        return NULL;
    }
    memset(&want, 0, sizeof want);
    want.freq = freq;
    want.format = AUDIO_S16SYS;
    want.channels = (Uint8)channels;
    want.samples = DEVICE_FRAMES;
    want.callback = Callback;
    /* no allowed changes: SDL converts if the device is otherwise */
    dig.dev = SDL_OpenAudioDevice(NULL, 0, &want, &got, 0);
    if (!dig.dev) {
        snprintf(lastError, sizeof lastError, "%s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return NULL;
    }
    dig.rate = freq;
    dig.channels = channels;
    dig.bufferMs = got.samples * 1000.0 / freq;
    dig.lastRender = SDL_GetTicks();
    Cmd_AddCommand("snd_stats", Stats_f);
    SDL_PauseAudioDevice(dig.dev, 0);
    return &dig;
}

int SA_API SA_NAME(AIL_digital_CPU_percent)(void *d)
{
    int percent;

    (void)d;
    Lock();
    percent = dig.bufferMs > 0 ? (int)(dig.lastMs * 100 / dig.bufferMs) : 0;
    Unlock();
    return percent;
}

void SA_API SA_NAME(AIL_shutdown)(void)
{
    int i;

    for (i = 0; i < MAX_SAMPLES; i++)
        if (samples[i].stream) SA_NAME(AIL_close_stream)(&samples[i]);
    if (dig.dev) {
        SDL_CloseAudioDevice(dig.dev);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        Cmd_RemoveCommand("snd_stats");
    }
    for (i = 0; i < MAX_SAMPLES; i++) free(samples[i].decoded);
    memset(samples, 0, sizeof samples);
    memset(voices, 0, sizeof voices);
    memset(&dig, 0, sizeof dig);
}

/* The one provider: its name is what snd_driver.c and config files look for. Its handle only has to
   be something other than NULL. */
int SA_API SA_NAME(AIL_enumerate_3D_providers)(void *next, void **handle, const char **name)
{
    void **it = next;

    if (*it) return 0;
    *it = *handle = &dig;
    *name = "Miles Fast 2D Positional Audio";
    return 1;
}

int SA_API SA_NAME(AIL_open_3D_provider)(void *handle) { (void)handle; return 0; }
void SA_API SA_NAME(AIL_close_3D_provider)(void *lib) { (void)lib; }

void SA_API SA_NAME(AIL_3D_provider_attribute)(void *provider, const char *name, void *val)
{
    (void)provider;
    if (!strcmp(name, "Maximum supported samples")) *(int *)val = MAX_3D;
}

void SA_API SA_NAME(AIL_set_3D_distance_factor)(void *provider, float factor) { (void)provider; distFactor = factor; }
void SA_API SA_NAME(AIL_set_3D_rolloff_factor)(void *provider, float factor) { (void)provider; rolloff = factor; }

/* Task 7 */
void SA_API SA_NAME(AIL_set_3D_room_type)(void *provider, int room_type) { (void)provider; (void)room_type; }
void SA_API SA_NAME(AIL_set_digital_master_room_type)(void *d, int room_type) { (void)d; (void)room_type; }
void SA_API SA_NAME(AIL_set_digital_master_reverb_levels)(void *d, float dry, float wet) { (void)d; (void)dry; (void)wet; }
void SA_API SA_NAME(AIL_set_sample_reverb_levels)(void *S, float dry, float wet) { (void)S; (void)dry; (void)wet; }
void SA_API SA_NAME(AIL_set_3D_sample_effects_level)(void *S, float level) { (void)S; (void)level; }
void SA_API SA_NAME(AIL_set_stream_reverb_levels)(void *S, float dry, float wet) { (void)S; (void)dry; (void)wet; }

/* Loaded sounds */

int SA_API SA_NAME(AIL_WAV_info)(void *data, void *info) { return (int)SndWav_Info(data, sndWavFileLen, info); }

int SA_API SA_NAME(AIL_size_processed_digital_audio)(int rate, int format, int num, void *info)
{
    (void)num;
    return (int)SndWav_ConvertedSize((unsigned)rate, (unsigned)format, info);
}

long SA_API SA_NAME(AIL_process_digital_audio)(void *dest, long size, unsigned long rate, unsigned long format,
                                               long num, void *info)
{
    (void)num;
    return SndWav_Convert(dest, size, (unsigned)rate, (unsigned)format, info);
}

/* Samples */

void *SA_API SA_NAME(AIL_allocate_sample_handle)(void *d)
{
    Sample *s;

    (void)d;
    Lock();
    s = Allocate();
    Unlock();
    return s;
}

void SA_API SA_NAME(AIL_release_sample_handle)(void *S)
{
    Sample *s = S;

    if (!s) return;
    Lock();
    Detach(s);
    s->v->status = SND_SMP_FREE;
    s->used = 0;
    Unlock();
}

void SA_API SA_NAME(AIL_init_sample)(void *S)
{
    if (!S) return;
    Lock();
    Reset(S);
    Unlock();
}

void SA_API SA_NAME(AIL_set_sample_type)(void *S, int format, int flags)
{
    Sample *s = S;

    (void)flags;
    if (!s) return;
    Lock();
    s->format = format;
    s->v->bits = format & 1 ? 16 : 8;
    s->v->channels = format & 2 ? 2 : 1;
    Unlock();
}

void SA_API SA_NAME(AIL_set_sample_address)(void *S, void *ptr, int len)
{
    Sample *s = S;

    if (!s) return;
    s->src = ptr;
    s->srcLen = (unsigned)len;
    SetData(s);
}

void SA_API SA_NAME(AIL_set_sample_adpcm_block_size)(void *S, int blocksize)
{
    Sample *s = S;

    if (!s) return;
    s->block = blocksize;
    if (s->format & FORMAT_ADPCM) SetData(s);
}

void SA_API SA_NAME(AIL_set_sample_loop_count)(void *S, int count)
{
    Sample *s = S;

    if (!s) return;
    Lock();
    s->v->loopCount = count > 0 ? count : 0;
    Unlock();
}

void SA_API SA_NAME(AIL_set_sample_playback_rate)(void *S, int rate)
{
    Sample *s = S;

    if (!s) return;
    Lock();
    s->rate = s->v->rate = rate;
    Unlock();
}

int SA_API SA_NAME(AIL_sample_playback_rate)(void *S) { return S ? ((Sample *)S)->rate : 0; }

void SA_API SA_NAME(AIL_set_sample_volume_levels)(void *S, float left, float right)
{
    Sample *s = S;

    if (!s) return;
    s->left = left;
    s->right = right;
    SetGains(s);
}

void SA_API SA_NAME(AIL_sample_volume_levels)(void *S, float *left, float *right)
{
    const Sample *s = S;

    if (!s) return;
    if (left) *left = s->left;
    if (right) *right = s->right;
}

void SA_API SA_NAME(AIL_sample_volume_pan)(void *S, float *volume, float *pan)
{
    if (S) DerivedVolumePan(S, volume, pan);
}

void SA_API SA_NAME(AIL_resume_sample)(void *S)   /* 0x2112b760 */
{
    Sample *s = S;

    if (!s) return;
    Lock();
    if (s->v->status == SND_SMP_STOPPED || (s->v->status == SND_SMP_DONE && s->v->dataLen))
        s->v->status = SND_SMP_PLAYING;
    Unlock();
}

void SA_API SA_NAME(AIL_stop_sample)(void *S)   /* 0x21114d4f */
{
    Sample *s = S;

    if (!s) return;
    Lock();
    if (s->v->status == SND_SMP_PLAYING) s->v->status = SND_SMP_STOPPED;
    Unlock();
}

/* The hunk behind the sample may be freed from here on, so the voice keeps no pointer to it. */
void SA_API SA_NAME(AIL_end_sample)(void *S)
{
    if (!S) return;
    Lock();
    Detach(S);
    Unlock();
}

unsigned long SA_API SA_NAME(AIL_sample_status)(void *S)
{
    Sample *s = S;
    unsigned long status;

    if (!s) return SND_SMP_FREE;
    Lock();
    Advance();
    status = (unsigned long)s->v->status;
    Unlock();
    return status;
}

void SA_API SA_NAME(AIL_sample_ms_position)(void *S, long *total, long *current)
{
    Sample *s = S;

    if (!s) return;
    Lock();
    if (total) *total = Milliseconds(s->v->dataLen / FrameBytes(s->v), s->rate);
    if (current) *current = Milliseconds(s->v->pos, s->rate);
    Unlock();
}

void SA_API SA_NAME(AIL_set_sample_ms_position)(void *S, int ms)
{
    Sample *s = S;

    if (!s) return;
    Lock();
    SetPosition(s->v, (double)ms * s->rate / 1000);
    Unlock();
}

unsigned long SA_API SA_NAME(AIL_sample_position)(void *S)   /* bytes into the current buffer, 0x21116104 */
{
    Sample *s = S;
    unsigned long pos;

    if (!s) return 0;
    Lock();
    pos = s->raw ? s->off[s->cur] : (unsigned long)s->v->pos * FrameBytes(s->v);
    Unlock();
    return pos;
}

int SA_API SA_NAME(AIL_minimum_sample_buffer_size)(void *d, int rate, int format)
{
    int bytes = (format & 1 ? 2 : 1) * (format & 2 ? 2 : 1);

    (void)d;
    /* a device buffer's worth at that rate, up to 256 as 0x2112b8eb */
    return dig.rate > 0 ? (int)(((long long)DEVICE_FRAMES * bytes * rate / dig.rate + 255) & ~255LL) : 0;
}

/* raw's double buffer. Like 0x21115cf0 it hands out both, then each as the mixer leaves it; this one
   looks at what is played rather than at the flips, as two flips between calls would stall it, and
   snd_driver.c:654 spins until a buffer comes. */
int SA_API SA_NAME(AIL_sample_buffer_ready)(void *S)
{
    Sample *s = S;
    int ready = -1;

    if (!s) return -1;
    Lock();
    Advance();
    if (s->done[s->cur]) ready = s->cur;
    else if (s->done[s->cur ^ 1]) ready = s->cur ^ 1;
    Unlock();
    return ready;
}

void SA_API SA_NAME(AIL_load_sample_buffer)(void *S, int buf_num, void *buffer, int size)   /* 0x2112b940 */
{
    Sample *s = S;

    if (!s) return;
    buf_num &= 1;
    Lock();
    s->buf[buf_num] = buffer;
    s->len[buf_num] = size > 0 ? (unsigned)size : 0;
    s->off[buf_num] = 0;
    s->done[buf_num] = size <= 0;
    s->raw = 1;
    if (size > 0 && s->v->status != SND_SMP_PLAYING) s->v->status = SND_SMP_PLAYING;
    Unlock();
}

/* 3D samples: the "Miles Fast 2D Positional Audio" provider on a sample */

void *SA_API SA_NAME(AIL_allocate_3D_sample_handle)(void *provider)
{
    (void)provider;
    return SA_NAME(AIL_allocate_sample_handle)(NULL);
}

/* The provider's M3D_set_3D_sample_data, 0x22401d30: mono linear PCM only (0x22401d4c, 0x22401d62),
   with what it sets up in 0x22401da3 to 0x22401df7. Refused, a sample stays silent, as its channel
   reads free. The game ignores the answer. */
void SA_API SA_NAME(AIL_set_3D_sample_info)(void *S, void *info)
{
    Sample *s = S;
    const SndSoundInfo *i = info;
    int ok;

    if (!s) return;
    ok = i->format == 1 && i->channels == 1 && (i->bits == 8 || i->bits == 16);
    Lock();
    if (ok) Reset(s); else Detach(s);
    Unlock();
    if (!ok) return;
    s->volume = 1.0f;
    s->maxDist = 200.0f;
    s->minDist = 1.0f;
    SA_NAME(AIL_set_sample_type)(s, i->bits == 16, 0);
    SA_NAME(AIL_set_sample_playback_rate)(s, (int)i->rate);
    SA_NAME(AIL_set_sample_address)(s, (void *)i->data_ptr, (int)i->data_len);
    Spatialize(s);
}

void SA_API SA_NAME(AIL_set_3D_sample_volume)(void *S, float volume)
{
    Sample *s = S;

    if (!s) return;
    s->volume = volume;
    Spatialize(s);
}

float SA_API SA_NAME(AIL_3D_sample_volume)(void *S) { return S ? ((Sample *)S)->volume : 0; }

void SA_API SA_NAME(AIL_set_3D_sample_distances)(void *S, float max_dist, float min_dist)
{
    Sample *s = S;

    if (!s) return;
    s->maxDist = max_dist;
    s->minDist = min_dist;
    Spatialize(s);
}

void SA_API SA_NAME(AIL_set_3D_position)(void *S, float x, float y, float z)
{
    Sample *s = S;

    if (!s) return;
    s->pos[0] = x;
    s->pos[1] = y;
    s->pos[2] = z;
    Spatialize(s);
}

void SA_API SA_NAME(AIL_3D_position)(void *S, float *x, float *y, float *z)
{
    const Sample *s = S;

    if (!s) return;
    if (x) *x = s->pos[0];
    if (y) *y = s->pos[1];
    if (z) *z = s->pos[2];
}

void SA_API SA_NAME(AIL_set_3D_sample_offset)(void *S, int offset)
{
    Sample *s = S;

    if (!s) return;
    Lock();
    SetPosition(s->v, offset / (double)FrameBytes(s->v));
    Unlock();
}

unsigned long SA_API SA_NAME(AIL_3D_sample_offset)(void *S) { return SA_NAME(AIL_sample_position)(S); }
int SA_API SA_NAME(AIL_3D_sample_length)(void *S) { return S ? (int)((Sample *)S)->srcLen : 0; }   /* bytes: S+4, 0x22401dba */
int SA_API SA_NAME(AIL_3D_sample_playback_rate)(void *S) { return SA_NAME(AIL_sample_playback_rate)(S); }
void SA_API SA_NAME(AIL_set_3D_sample_playback_rate)(void *S, int rate) { SA_NAME(AIL_set_sample_playback_rate)(S, rate); }
void SA_API SA_NAME(AIL_set_3D_sample_loop_count)(void *S, int count) { SA_NAME(AIL_set_sample_loop_count)(S, count); }
void SA_API SA_NAME(AIL_resume_3D_sample)(void *S) { SA_NAME(AIL_resume_sample)(S); }
void SA_API SA_NAME(AIL_stop_3D_sample)(void *S) { SA_NAME(AIL_stop_sample)(S); }
void SA_API SA_NAME(AIL_end_3D_sample)(void *S) { SA_NAME(AIL_end_sample)(S); }
unsigned long SA_API SA_NAME(AIL_3D_sample_status)(void *S) { return SA_NAME(AIL_sample_status)(S); }

/* Streams */

/* Opened stopped: the game starts it with AIL_pause_stream(0), as 1.3 after its setup. */
void *SA_API SA_NAME(AIL_open_stream)(void *d, const char *filename, int flags)
{
    SndStream *stream;
    Sample *s;
    long rate, channels;

    (void)d;
    (void)flags;
    if (!files.open) {   /* shortcut: no fopen fallback for mss_q3fs 0, as iwds would stay out of reach anyway */
        snprintf(lastError, sizeof lastError, "no file callbacks");
        return NULL;
    }
    stream = SndStream_Open(&files, filename);
    if (!stream) {
        snprintf(lastError, sizeof lastError, "could not open the stream");
        return NULL;
    }
    SndStream_SetPaused(stream, 1);
    SndStream_Info(stream, &rate, &channels, NULL, NULL);
    Lock();
    s = Allocate();
    if (s) {
        s->stream = stream;
        s->rate = s->v->rate = (int)rate;
        s->v->bits = 16;
        s->v->channels = (int)channels;
        s->v->status = SND_SMP_STOPPED;
    }
    Unlock();
    if (!s) SndStream_Close(stream);
    return s;
}

/* The lock only detaches: Close waits for the reader, and the callback is out of the stream by then. */
void SA_API SA_NAME(AIL_close_stream)(void *S)
{
    Sample *s = S;
    SndStream *stream;

    if (!s) return;
    Lock();
    stream = s->stream;
    s->stream = NULL;
    Detach(s);
    s->v->status = SND_SMP_FREE;
    s->used = 0;
    Unlock();
    SndStream_Close(stream);
}

void SA_API SA_NAME(AIL_pause_stream)(void *S, long onoff)
{
    Sample *s = S;

    if (!s) return;
    SndStream_SetPaused(s->stream, (int)onoff);
    Lock();
    s->v->status = onoff ? SND_SMP_STOPPED : SND_SMP_PLAYING;
    Unlock();
}

long SA_API SA_NAME(AIL_stream_status)(void *S)
{
    Sample *s = S;
    long status;

    if (!s) return SND_SMP_FREE;
    Lock();
    Advance();
    status = SndStream_Status(s->stream);   /* of what Feed left, a couple of frames go unmixed */
    Unlock();
    return status;
}

void SA_API SA_NAME(AIL_set_stream_loop_count)(void *S, int count)
{
    if (S) SndStream_SetLoop(((Sample *)S)->stream, count > 0 ? count : 0);
}

void SA_API SA_NAME(AIL_set_stream_ms_position)(void *S, int ms)
{
    Sample *s = S;

    if (!s) return;
    SndStream_SeekMs(s->stream, ms);
    Lock();
    s->frames = 0;
    s->v->pos = 0;
    s->v->dataLen = 0;
    Unlock();
}

/* The length is from the bitrate, as Miles: file bytes / S+0x84 (0x211118dd), which an ASI gives once
   at open (0x21110815). */
void SA_API SA_NAME(AIL_stream_ms_position)(void *S, long *total, long *current)
{
    if (S) SndStream_Info(((Sample *)S)->stream, NULL, NULL, total, current);
}

/* 0x2111b520. sndtype is what the voice plays: decoded, so never with Miles' ADPCM flag. */
void SA_API SA_NAME(AIL_stream_info)(void *S, int *datarate, int *sndtype, int *length, int *memory)
{
    long rate, channels, ms;

    if (!S) return;
    SndStream_Info(((Sample *)S)->stream, &rate, &channels, &ms, NULL);
    if (datarate) *datarate = (int)rate;
    if (sndtype) *sndtype = channels == 2 ? 3 : 1;
    if (length) *length = (int)((long long)ms * rate / 1000 * channels * 2);
    if (memory) *memory = 0;
}

int SA_API SA_NAME(AIL_stream_playback_rate)(void *S) { return SA_NAME(AIL_sample_playback_rate)(S); }
void SA_API SA_NAME(AIL_set_stream_playback_rate)(void *S, int rate) { SA_NAME(AIL_set_sample_playback_rate)(S, rate); }
void SA_API SA_NAME(AIL_set_stream_volume_levels)(void *S, float left, float right) { SA_NAME(AIL_set_sample_volume_levels)(S, left, right); }
void SA_API SA_NAME(AIL_stream_volume_levels)(void *S, float *left, float *right) { SA_NAME(AIL_sample_volume_levels)(S, left, right); }
void SA_API SA_NAME(AIL_stream_volume_pan)(void *S, float *volume, float *pan) { SA_NAME(AIL_sample_volume_pan)(S, volume, pan); }

/* Not Miles exports: mss32_dll.c:192,393 */
int SA_API SA_NAME(AIL_is_3D_stream)(void *S) { (void)S; return 0; }
void SA_API SA_NAME(AIL_set_3D_stream_position)(void *S, float x, float y, float z) { (void)S; (void)x; (void)y; (void)z; }

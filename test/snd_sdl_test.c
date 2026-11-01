#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "snd_sdl.h"

static int failed;
#define CHECK(name, cond) do { int ok_ = (cond); printf("%s %s\n", name, ok_ ? "PASS" : "FAIL"); if (!ok_) failed = 1; } while (0)

/* test_snd_sdl.py links with --wrap: only our objects land here, not libSDL's or libc's own calls */
static int allocs;
void *__real_malloc(size_t n);
void *__real_calloc(size_t n, size_t size);
void *__real_realloc(void *p, size_t n);
void *__wrap_malloc(size_t n) { __atomic_add_fetch(&allocs, 1, __ATOMIC_RELAXED); return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t size) { __atomic_add_fetch(&allocs, 1, __ATOMIC_RELAXED); return __real_calloc(n, size); }
void *__wrap_realloc(void *p, size_t n) { __atomic_add_fetch(&allocs, 1, __ATOMIC_RELAXED); return __real_realloc(p, n); }

static SndVoice voice(const void *data, unsigned len, int bits, int ch, int rate)
{
    SndVoice v;
    memset(&v, 0, sizeof v);
    v.data = data; v.dataLen = len; v.bits = bits; v.channels = ch; v.rate = rate;
    v.loopCount = 1; v.left = v.right = 1.0f; v.status = SND_SMP_PLAYING;
    return v;
}

#define NEAR(a, b) (fabs((a) - (b)) < 2e-6)

static int levels(float x, float y, float z, float minD, float maxD, float rolloff, float volume, float wantL, float wantR)
{
    float pos[3], l = -1, r = -1;
    pos[0] = x; pos[1] = y; pos[2] = z;
    SndSpatial_Levels(pos, minD, maxD, rolloff, 0.0254f, volume, &l, &r);
    return NEAR(l, wantL) && NEAR(r, wantR);
}

static int vol_pan(float volume, float pan, float wantL, float wantR)
{
    float l = -1, r = -1;
    SndSpatial_VolumePan(volume, pan, &l, &r);
    return NEAR(l, wantL) && NEAR(r, wantR);
}

static void put16(unsigned char *p, unsigned v) { p[0] = v & 255; p[1] = v >> 8 & 255; }
static void put32(unsigned char *p, unsigned long v) { put16(p, v & 65535); put16(p + 2, v >> 16); }

/* RIFF/WAVE: optional LIST chunk (odd size, so padded), fmt, data. Returns the file length. */
static long mkwav(unsigned char *f, int tag, int ch, int rate, int bits, int block, const void *data, long dlen, int list)
{
    unsigned char *p = f + 12;
    memcpy(f, "RIFF", 4); memcpy(f + 8, "WAVE", 4);
    if (list) { memcpy(p, "LIST", 4); put32(p + 4, 3); memcpy(p + 8, "abc\0", 4); p += 12; }
    memcpy(p, "fmt ", 4); put32(p + 4, 16); put16(p + 8, tag); put16(p + 10, ch); put32(p + 12, rate);
    put32(p + 16, rate * block); put16(p + 20, block); put16(p + 22, bits); p += 24;
    memcpy(p, "data", 4); put32(p + 4, dlen); memcpy(p + 8, data, dlen); p += 8 + dlen;
    put32(f + 4, p - f - 8);
    return p - f;
}

#ifdef SND_TEST_STREAMS
#include <SDL2/SDL.h>
#include "snd_mp3_fixture.h"

/* Files in memory. The name is the index in files[]. Everything the reader sees goes through here. */
typedef struct MemFile { const unsigned char *data; long len, pos; } MemFile;
static MemFile files[16];
static int stall, readFails, seekFails, stalled, closes, readDelay;

static unsigned long mem_open(const char *name, unsigned long *handle)
{
    if (!strcmp(name, "missing")) return 0;
    *handle = strtoul(name, NULL, 10) + 1;
    return 1;
}
static void mem_close(unsigned long h) { (void)h; closes++; }
static long mem_seek(unsigned long h, long off, unsigned long type)
{
    MemFile *f = &files[h - 1];
    if (__atomic_load_n(&seekFails, __ATOMIC_ACQUIRE)) return -1;
    f->pos = off + (type == 1 ? f->pos : type == 2 ? f->len : 0);
    if (f->pos > f->len) f->pos = f->len;
    return f->pos;
}
static unsigned long mem_read(unsigned long h, void *buf, unsigned long n)
{
    MemFile *f = &files[h - 1];
    if (__atomic_load_n(&readFails, __ATOMIC_ACQUIRE)) return (unsigned long)-1;   /* FS_Read's -1 */
    if (__atomic_load_n(&readDelay, __ATOMIC_ACQUIRE)) SDL_Delay(1);
    while (__atomic_load_n(&stall, __ATOMIC_ACQUIRE)) {
        __atomic_store_n(&stalled, 1, __ATOMIC_RELEASE);
        SDL_Delay(1);
    }
    if ((long)n > f->len - f->pos) n = f->len - f->pos;
    memcpy(buf, f->data + f->pos, n);
    f->pos += n;
    return n;
}
static const SndFileCallbacks memCb = { mem_open, mem_close, mem_seek, mem_read };

static SndStream *mem_stream(int index, const void *data, long len)
{
    char name[8];
    files[index].data = data; files[index].len = len; files[index].pos = 0;
    sprintf(name, "%d", index);
    return SndStream_Open(&memCb, name);
}

/* Takes want frames, or what is there once the stream is done or 5 s pass. */
static int pull(SndStream *s, short *out, int want, int channels)
{
    int got = 0;
    Uint32 start = SDL_GetTicks();
    while (got < want && SDL_GetTicks() - start < 5000) {
        int n = SndStream_Read(s, out + got * channels, want - got);
        got += n;
        if (!n) {
            if (SndStream_Status(s) == SND_SMP_DONE) break;
            SDL_Delay(1);
        }
    }
    return got;
}

/* Plays s out, discarding the sound; returns the frames, or -1 if it never ends. */
static long drain(SndStream *s)
{
    static short sink[8192];
    long total = 0;
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < 5000) {
        int n = SndStream_Read(s, sink, 4096);
        total += n;
        if (!n) {
            if (SndStream_Status(s) == SND_SMP_DONE) return total;
            SDL_Delay(1);
        }
    }
    return -1;
}

static int wait_stalled(void)
{
    Uint32 start = SDL_GetTicks();
    while (!__atomic_load_n(&stalled, __ATOMIC_ACQUIRE))
        if (SDL_GetTicks() - start > 5000) return 0; else SDL_Delay(1);
    return 1;
}

static int ramp_at(short *p, long frame, int n) { int i; for (i = 0; i < n; i++) if (p[i] != (short)((frame + i) * 7)) return 0; return 1; }
static int cmp_int(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }

/* Stands for the audio callback: uses cur with the device lock held, as the real one will. */
static SDL_mutex *device;
static SndStream *cur;
static int audioQuit;
static int audio_thread(void *unused)
{
    static short buf[1024];
    (void)unused;
    while (!__atomic_load_n(&audioQuit, __ATOMIC_ACQUIRE)) {
        SDL_LockMutex(device);
        if (cur) { SndStream_Read(cur, buf, 256); SndStream_Status(cur); }
        SDL_UnlockMutex(device);
        SDL_Delay(0);
    }
    return 0;
}

static void set_cur(SndStream *s) { SDL_LockMutex(device); cur = s; SDL_UnlockMutex(device); }

static void stream_tests(void)
{
    enum { WAV_FRAMES = 66150, MP3_FRAMES = 2194 / 104 * 1152 };
    static short ramp[WAV_FRAMES], got[WAV_FRAMES * 3];
    static unsigned char wav[WAV_FRAMES * 2 + 64], small[16384], noise[65536];
    static unsigned char mp3[sizeof mp3_fixture];
    static const unsigned char ima[8] = { 0xe8, 0x03, 0, 0, 0x04, 0x00, 0, 0 };
    static unsigned char u8s[4] = { 128, 255, 0, 192 };
    long rate = 0, ch = 0, total = 0, pos = 0, len, frames;
    long i;
    int n, k, ok;
    SndStream *s, *open13[13];
    SDL_Thread *audio;

    for (i = 0; i < WAV_FRAMES; i++) ramp[i] = (short)(i * 7);
    len = mkwav(wav, 1, 1, 22050, 16, 2, ramp, WAV_FRAMES * 2, 0);

    /* three seconds: more than the ring holds, so it wraps */
    s = mem_stream(0, wav, len);
    ok = s != NULL;
    if (s) {
        SndStream_Info(s, &rate, &ch, &total, &pos);
        ok = rate == 22050 && ch == 1 && total == 3000 && pos == 0;
        ok = ok && pull(s, got, WAV_FRAMES, 1) == WAV_FRAMES && ramp_at(got, 0, WAV_FRAMES);
        SndStream_Info(s, NULL, NULL, NULL, &pos);
        ok = ok && pos == 3000 && drain(s) == 0;
        SndStream_Close(s);
    }
    len = mkwav(small, 1, 1, 44100, 8, 1, u8s, 4, 0);
    s = mem_stream(0, small, len);
    ok = ok && s && pull(s, got, 4, 1) == 4 && got[0] == 0 && got[1] > 32000 && got[2] < -32000 && got[3] == 16384;
    if (s) SndStream_Close(s);
    len = mkwav(small, 0x11, 1, 22050, 4, 8, ima, 8, 0);
    s = mem_stream(0, small, len);
    ok = ok && s && pull(s, got, 9, 1) == 9 && got[0] == 1000 && got[1] == 1007 && got[2] == 1008;
    if (s) SndStream_Close(s);
    len = mkwav(small, 0x11, 1, 22050, 4, 8, ima, 4, 0);     /* a short last block plays, as SndWav_Info counts it */
    s = mem_stream(0, small, len);
    ok = ok && s && pull(s, got, 2, 1) == 1 && got[0] == 1000;
    if (s) SndStream_Close(s);
    CHECK("stream_wav_plays", ok);

    s = mem_stream(0, mp3_fixture, sizeof mp3_fixture);
    ok = s != NULL;
    if (s) {
        int peak = 0, crossings = 0;
        SndStream_Info(s, &rate, &ch, &total, NULL);
        ok = rate == 44100 && ch == 1 && total > 480 && total < 580;
        frames = pull(s, got, WAV_FRAMES, 1);
        for (i = 1; i < frames; i++) {
            if (abs(got[i]) > peak) peak = abs(got[i]);
            crossings += (got[i - 1] < 0) != (got[i] < 0);
        }
        /* 1 kHz: two crossings per ms, but for the encoder's delay at the start */
        ok = ok && frames >= MP3_FRAMES - 1152 && frames <= MP3_FRAMES + 1152 && peak > 2000;
        ok = ok && crossings > 800 && crossings < 1100;
        ok = ok && SndStream_Status(s) == SND_SMP_DONE;
        SndStream_Close(s);
    }
    {   /* seven frames and an ID3v1 tag, as sound/ambient/silentsong.mp3: too few for minimp3's sync alone */
        enum { SEVEN_FRAMES = 731 };
        static unsigned char tagged[SEVEN_FRAMES + 128];
        memcpy(tagged, mp3_fixture, SEVEN_FRAMES);
        memcpy(tagged + SEVEN_FRAMES, "TAG", 3);
        s = mem_stream(0, tagged, sizeof tagged);
        ok = ok && s && drain(s) == 7 * 1152;
        if (s) SndStream_Close(s);
    }
    CHECK("stream_mp3_plays", ok);

    len = mkwav(small, 1, 1, 22050, 16, 2, ramp, 200, 0);
    s = mem_stream(0, small, len);
    ok = s != NULL;
    if (s) {
        SndStream_SetLoop(s, 3);
        ok = pull(s, got, 400, 1) == 300 && SndStream_Status(s) == SND_SMP_DONE;
        for (i = 0; i < 300; i++) ok = ok && got[i] == ramp[i % 100];
        SndStream_Close(s);
    }
    s = mem_stream(0, small, len);   /* loops set after the reader reached the end */
    ok = ok && s;
    if (s) {
        ok = ok && pull(s, got, 50, 1) == 50;
        SDL_Delay(50);
        SndStream_SetLoop(s, 2);
        ok = ok && drain(s) == 150;
        SndStream_Close(s);
    }
    s = mem_stream(0, small, len);
    ok = ok && s;
    if (s) {
        SndStream_SetLoop(s, 0);
        ok = ok && pull(s, got, 1000, 1) == 1000 && SndStream_Status(s) == SND_SMP_PLAYING;
        for (i = 0; i < 1000; i++) ok = ok && got[i] == ramp[i % 100];
        SndStream_Info(s, NULL, NULL, &total, &pos);
        ok = ok && total == 4 && pos >= 0 && pos < 5;
        SndStream_Close(s);
    }
    s = mem_stream(0, mp3_fixture, sizeof mp3_fixture);
    ok = ok && s;
    if (s) {
        SndStream_SetLoop(s, 2);
        frames = drain(s);
        ok = ok && frames >= 2 * (MP3_FRAMES - 1152) && frames <= 2 * (MP3_FRAMES + 1152);
        SndStream_Close(s);
    }
    CHECK("stream_loop", ok);

    len = mkwav(wav, 1, 1, 22050, 16, 2, ramp, WAV_FRAMES * 2, 0);
    s = mem_stream(0, wav, len);
    ok = s != NULL;
    if (s) {
        ok = pull(s, got, 1000, 1) == 1000;
        SndStream_SeekMs(s, 1500);
        ok = ok && pull(s, got, 2000, 1) == 2000 && ramp_at(got, 33075, 2000);
        SndStream_Info(s, NULL, NULL, NULL, &pos);
        ok = ok && pos >= 1500 && pos <= 1592;
        SndStream_SeekMs(s, 0);       /* behind the reader: the rest of the ring is gone */
        ok = ok && pull(s, got, 100, 1) == 100 && ramp_at(got, 0, 100);
        SndStream_SeekMs(s, 1000000);
        ok = ok && drain(s) == 0 && SndStream_Status(s) == SND_SMP_DONE;
        SndStream_SeekMs(s, 2900);    /* a finished stream plays again, and is not done from here on */
        ok = ok && SndStream_Status(s) == SND_SMP_PLAYING;
        frames = drain(s);
        ok = ok && frames >= 2200 && frames <= 2206;
        SndStream_Close(s);
    }
    {
        static unsigned char blocks[8 * 3];
        short dec[9];
        for (i = 0; i < 3; i++) memcpy(blocks + i * 8, ima, 8);
        SndAdpcm_Decode(ima, 8, 8, 1, dec, 9);
        len = mkwav(small, 0x11, 1, 22050, 4, 8, blocks, 24, 0);
        s = mem_stream(0, small, len);
        ok = ok && s;
        if (s) {
            SndStream_SeekMs(s, 1);   /* frame 22: the fifth of the third block */
            ok = ok && pull(s, got, 5, 1) == 5 && !memcmp(got, dec + 4, 10);
            SndStream_Close(s);
        }
    }
    s = mem_stream(0, mp3_fixture, sizeof mp3_fixture);
    ok = ok && s;
    if (s) {
        SndStream_Info(s, NULL, NULL, &total, NULL);
        SndStream_SeekMs(s, total / 2);
        frames = drain(s);
        ok = ok && frames > MP3_FRAMES / 2 - 2304 && frames < MP3_FRAMES / 2 + 2304;
        SndStream_Close(s);
    }
    CHECK("stream_seek_ms", ok);

    s = mem_stream(0, wav, len = mkwav(wav, 1, 1, 22050, 16, 2, ramp, WAV_FRAMES * 2, 0));
    ok = s != NULL;
    if (s) {
        static int ticks[101];
        ok = pull(s, got, 100, 1) == 100;
        __atomic_store_n(&stall, 1, __ATOMIC_RELEASE);
        ok = ok && wait_stalled();                       /* the reader is stuck in read */
        while (SndStream_Read(s, got, 4096)) continue;   /* the ring empties */
        for (i = 0; i < 101; i++) {
            Uint64 t = SDL_GetPerformanceCounter();
            n = SndStream_Read(s, got, 4096);
            t = SDL_GetPerformanceCounter() - t;
            ok = ok && n == 0 && SndStream_Status(s) == SND_SMP_PLAYING;
            ticks[i] = (int)(t * 1000000 / SDL_GetPerformanceFrequency());
        }
        qsort(ticks, 101, sizeof ticks[0], cmp_int);
        ok = ok && ticks[50] < 1000;                     /* microseconds */
        __atomic_store_n(&stall, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&stalled, 0, __ATOMIC_RELEASE);
        ok = ok && pull(s, got, 100, 1) == 100;          /* and it comes back */
        SndStream_Close(s);
    }
    CHECK("stream_underrun_silence", ok);

    /* every game-thread call against a live reader and a live callback; TSan's case */
    device = SDL_CreateMutex();
    audio = SDL_CreateThread(audio_thread, "audio", NULL);
    ok = 1;
    for (k = 0; k < 100; k++) {
        const int isMp3 = k & 1;
        closes = 0;
        s = isMp3 ? mem_stream(k % 4, mp3_fixture, sizeof mp3_fixture) : mem_stream(k % 4, wav, len);
        ok = ok && s;
        if (!s) continue;
        __atomic_store_n(&readDelay, k % 3 == 0, __ATOMIC_RELEASE);
        set_cur(s);
        SndStream_SetLoop(s, k & 2 ? 0 : 1);
        SndStream_SetPaused(s, k % 5 == 0);
        if (k % 7 == 0) SndStream_SeekMs(s, 100);
        SDL_Delay(k % 3);
        SndStream_Info(s, &rate, &ch, &total, &pos);
        set_cur(NULL);
        SndStream_Close(s);
        ok = ok && closes == 1;
    }
    __atomic_store_n(&readDelay, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&audioQuit, 1, __ATOMIC_RELEASE);
    SDL_WaitThread(audio, NULL);
    SDL_DestroyMutex(device);
    CHECK("stream_close_while_reading", ok);

    /* Every hostile file must end: Open fails, or it plays to DONE within the time limit. */
    ok = 1;
    srand(1);
    for (k = 0; k < 40; k++) {
        for (i = 0; i < (long)sizeof noise; i++) noise[i] = (unsigned char)rand();
        s = mem_stream(0, noise, sizeof noise);
        if (s) { ok = ok && drain(s) >= 0; SndStream_Close(s); }
    }
    memset(noise, 0, sizeof noise);
    s = mem_stream(0, noise, sizeof noise);
    if (s) { ok = ok && drain(s) >= 0; SndStream_Close(s); }
    for (i = 0; i < (long)sizeof mp3_fixture; i += 97) {     /* truncated */
        unsigned char *exact = malloc(i ? i : 1);            /* exact size: ASan sees any read past it */
        memcpy(exact, mp3_fixture, i);
        s = mem_stream(0, exact, i);
        if (s) { ok = ok && drain(s) >= 0; SndStream_Close(s); }
        free(exact);
    }
    for (k = 0; k < 300; k++) {                              /* damaged */
        memcpy(mp3, mp3_fixture, sizeof mp3);
        for (n = 0; n < 1 + k % 8; n++) mp3[rand() % sizeof mp3] ^= (unsigned char)(1 << rand() % 8);
        s = mem_stream(0, mp3, sizeof mp3);
        if (s) { SndStream_SetLoop(s, k & 1 ? 2 : 1); ok = ok && drain(s) >= 0; SndStream_Close(s); }
    }
    memcpy(mp3, mp3_fixture, sizeof mp3);                    /* turns stereo mid stream: buffers fit one layout */
    for (i = 1000; i + 3 < (long)sizeof mp3; i++)
        if (mp3[i] == 0xff && mp3[i + 1] == 0xfb) mp3[i + 3] ^= 0xc0;
    s = mem_stream(0, mp3, sizeof mp3);
    ok = ok && s;
    if (s) { frames = drain(s); ok = ok && frames >= 0 && frames <= MP3_FRAMES; SndStream_Close(s); }
    n = (int)mkwav(small, 1, 1, 22050, 16, 2, ramp, 8, 0);   /* data_len lies */
    put32(small + 40, 0xffffffffUL);
    s = mem_stream(0, small, n);
    ok = ok && s && drain(s) == 4;
    if (s) { SndStream_Info(s, NULL, NULL, &total, NULL); ok = ok && total > 0; SndStream_Close(s); }
    n = (int)mkwav(small, 0x11, 1, 22050, 4, 8, ima, 8, 0);  /* and in frames too, on a 32-bit long */
    put32(small + 40, 0xffffffffUL);
    s = mem_stream(0, small, n);
    ok = ok && s && drain(s) == 9;
    if (s) { SndStream_Info(s, NULL, NULL, &total, NULL); ok = ok && total > 0; SndStream_Close(s); }
    n = (int)mkwav(small, 1, 1, 22050, 16, 2, ramp, 0, 0);   /* nothing to play, forever */
    s = mem_stream(0, small, n);
    ok = ok && s;
    if (s) { SndStream_SetLoop(s, 0); ok = ok && drain(s) == 0; SndStream_Close(s); }
    n = (int)mkwav(small, 1, 1, 0, 16, 2, ramp, 8, 0);       /* rate 0 */
    ok = ok && !mem_stream(0, small, n);
    n = (int)mkwav(small, 1, 1, 0x7ffffff, 16, 2, ramp, 8, 0);
    ok = ok && !mem_stream(0, small, n);
    n = (int)mkwav(small, 1, 1, 22050, 16, 2, ramp, 200, 0);
    for (k = 0; k < 2; k++) {                                /* the file breaks while playing */
        int *fail = k ? &seekFails : &readFails;
        s = mem_stream(0, small, n);
        ok = ok && s;
        if (!s) continue;
        SndStream_SetLoop(s, 0);
        __atomic_store_n(fail, 1, __ATOMIC_RELEASE);
        ok = ok && drain(s) >= 0;
        __atomic_store_n(fail, 0, __ATOMIC_RELEASE);
        SndStream_Close(s);
    }
    CHECK("stream_garbage", ok);

    closes = 0;
    ok = SndStream_Open(&memCb, "missing") == NULL && closes == 0;
    for (k = 0; k < 13; k++) {
        open13[k] = mem_stream(k, wav, len);
        ok = ok && open13[k];
    }
    ok = ok && mem_stream(13, wav, len) == NULL;
    for (k = 0; k < 13; k++) if (open13[k]) SndStream_Close(open13[k]);
    /* a second generation of the reader, after the last close stopped the first */
    s = mem_stream(0, wav, len);
    ok = ok && s && pull(s, got, 10, 1) == 10 && ramp_at(got, 0, 10);
    if (s) SndStream_Close(s);
    CHECK("stream_open_fails", ok);
}
#endif

#ifdef SND_TEST_AIL
#include <stdarg.h>
/* snd_ail.c's calls, with snd_driver.c's prototypes, and what it needs from the engine */
int AIL_startup(int flags);
void AIL_shutdown(void);
const char *AIL_last_error(void);
void *AIL_open_digital_driver(int freq, int bits, int channels, int flags);
int AIL_enumerate_3D_providers(void *next, void **handle, const char **name);
int AIL_open_3D_provider(void *handle);
void AIL_3D_provider_attribute(void *provider, const char *name, void *val);
void AIL_set_3D_rolloff_factor(void *provider, float factor);
void AIL_set_file_callbacks(void *open_cb, void *close_cb, void *seek_cb, void *read_cb);
int AIL_WAV_info(void *data, void *info);
void *AIL_allocate_sample_handle(void *dig);
void AIL_release_sample_handle(void *S);
void AIL_init_sample(void *S);
void AIL_set_sample_type(void *S, int format, int flags);
void AIL_set_sample_address(void *S, void *ptr, int len);
void AIL_set_sample_adpcm_block_size(void *S, int blocksize);
void AIL_set_sample_loop_count(void *S, int count);
void AIL_set_sample_playback_rate(void *S, int rate);
int AIL_sample_playback_rate(void *S);
void AIL_set_sample_volume_levels(void *S, float left, float right);
void AIL_sample_volume_levels(void *S, float *left, float *right);
void AIL_sample_volume_pan(void *S, float *volume, float *pan);
void AIL_resume_sample(void *S);
void AIL_stop_sample(void *S);
void AIL_end_sample(void *S);
unsigned long AIL_sample_status(void *S);
void AIL_sample_ms_position(void *S, long *total, long *current);
void AIL_set_sample_ms_position(void *S, int ms);
unsigned long AIL_sample_position(void *S);
int AIL_minimum_sample_buffer_size(void *dig, int rate, int format);
int AIL_sample_buffer_ready(void *S);
void AIL_load_sample_buffer(void *S, int buf_num, void *buffer, int size);
void *AIL_allocate_3D_sample_handle(void *provider);
void AIL_set_digital_master_room_type(void *d, int room_type);
void AIL_set_digital_master_reverb_levels(void *d, float dry, float wet);
void AIL_set_3D_room_type(void *provider, int room_type);
void AIL_set_sample_reverb_levels(void *S, float dry, float wet);
void AIL_set_3D_sample_effects_level(void *S, float level);
void AIL_set_stream_reverb_levels(void *S, float dry, float wet);
void AIL_set_3D_sample_info(void *S, void *info);
void AIL_set_3D_sample_volume(void *S, float volume);
void AIL_set_3D_sample_distances(void *S, float max_dist, float min_dist);
void AIL_set_3D_position(void *S, float x, float y, float z);
void AIL_set_3D_sample_loop_count(void *S, int count);
void AIL_set_3D_sample_offset(void *S, int offset);
unsigned long AIL_3D_sample_offset(void *S);
int AIL_3D_sample_length(void *S);
void AIL_resume_3D_sample(void *S);
void AIL_end_3D_sample(void *S);
unsigned long AIL_3D_sample_status(void *S);
void *AIL_open_stream(void *dig, const char *filename, int flags);
void AIL_close_stream(void *S);
void AIL_pause_stream(void *S, long onoff);
long AIL_stream_status(void *S);
void AIL_set_stream_loop_count(void *S, int count);
void AIL_set_stream_ms_position(void *S, int ms);
void AIL_stream_ms_position(void *S, long *total, long *current);
void AIL_stream_info(void *S, int *datarate, int *sndtype, int *length, int *memory);
int AIL_stream_playback_rate(void *S);
void AIL_set_stream_playback_rate(void *S, int rate);
void AIL_set_stream_volume_levels(void *S, float left, float right);
int AIL_is_3D_stream(void *S);

long sndWavFileLen;
static void (*statsCmd)(void);
static char printed[256];
void Com_Printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(printed, sizeof printed, fmt, ap);
    va_end(ap);
}
void Cmd_AddCommand(const char *name, void (*f)(void)) { if (!strcmp(name, "snd_stats")) statsCmd = f; }
void Cmd_RemoveCommand(const char *name) { if (!strcmp(name, "snd_stats")) statsCmd = NULL; }

static char rawPath[512];

/* dummy is a device with no output; disk writes it to rawPath, quicker than real time */
static void *device_open(const char *driver, int rate)
{
    setenv("SDL_AUDIODRIVER", driver, 1);
    setenv("SDL_DISKAUDIOFILE", rawPath, 1);
    setenv("SDL_DISKAUDIODELAY", "1", 1);
    return AIL_open_digital_driver(rate, 2, 2, 0);
}

static long raw_read(short *out, long maxFrames)
{
    FILE *f = fopen(rawPath, "rb");
    long n = f ? (long)fread(out, 4, (size_t)maxFrames, f) : 0;
    if (f) fclose(f);
    return n;
}

/* Frames per cycle of a tone on the left channel, over the part that sounds. */
static double period(const short *p, long frames)
{
    long i, first = -1, last = -1, n = 0;
    for (i = 1; i < frames; i++)
        if ((p[(i - 1) * 2] < 0) != (p[i * 2] < 0) && (abs(p[i * 2]) > 0 || abs(p[(i - 1) * 2]) > 0)) {
            if (first < 0) first = i;
            last = i;
            n++;
        }
    return n > 1 ? 2.0 * (double)(last - first) / (double)(n - 1) : 0;
}

static int status_reaches(void *S, unsigned long want, int ms)
{
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < (Uint32)ms) {
        if (AIL_sample_status(S) == want) return 1;
        SDL_Delay(1);
    }
    return 0;
}

static int stream_reaches(void *S, long want, int ms)
{
    Uint32 start = SDL_GetTicks();
    while (SDL_GetTicks() - start < (Uint32)ms) {
        if (AIL_stream_status(S) == want) return 1;
        SDL_Delay(1);
    }
    return 0;
}

static void *mono16(void *dig, void *data, int bytes, int rate)
{
    void *s = AIL_allocate_sample_handle(dig);
    AIL_set_sample_type(s, 1, 0);
    AIL_set_sample_address(s, data, bytes);
    AIL_set_sample_playback_rate(s, rate);
    return s;
}

static void *ail_stream(void *dig, int index, const void *data, long len)
{
    char name[8];
    files[index].data = data; files[index].len = len; files[index].pos = 0;
    sprintf(name, "%d", index);
    return AIL_open_stream(dig, name, 0);
}

static void silent(void *user, int category, SDL_LogPriority priority, const char *text) { (void)user; (void)category; (void)priority; (void)text; }
static void set_callbacks(void) { AIL_set_file_callbacks((void *)mem_open, (void *)mem_close, (void *)mem_seek, (void *)mem_read); }

static void ail_tests(void)
{
    static short tone[44100], dc[4096], out[1 << 20], ramp[24576];
    static unsigned char wav[200000];
    static unsigned char ima[256];
    void *dig, *provider = NULL, *handle = NULL, *s, *t;
    void *handles[53];
    const char *name = NULL;
    SndMixInfo mi;
    SndSoundInfo info;
    float a, b;
    long total, cur, len, n;
    int i, k, ok, value;

    if (getenv("SND_TEST_DIR")) snprintf(rawPath, sizeof rawPath, "%s/out.raw", getenv("SND_TEST_DIR"));
    for (i = 0; i < 44100; i++) tone[i] = (short)(16000 * sin(2 * 3.14159265358979 * 1000 * i / 44100));
    for (i = 0; i < 4096; i++) dc[i] = 32767;
    set_callbacks();
    SDL_LogSetOutputFunction(silent, NULL);   /* the disk driver's warning */

    ok = AIL_startup(0x75) == 1;
    ok = ok && AIL_enumerate_3D_providers(&provider, &handle, &name) == 1 && handle && AIL_open_3D_provider(handle) == 0;
    ok = ok && !strcmp(name, "Miles Fast 2D Positional Audio");
    ok = ok && AIL_enumerate_3D_providers(&provider, &handle, &name) == 0;
    value = 0;
    AIL_3D_provider_attribute(handle, "Maximum supported samples", &value);
    CHECK("ail_provider", ok && value == 32 && AIL_is_3D_stream(handle) == 0);

    n = mkwav(wav, 1, 1, 22050, 16, 2, tone, 8, 0);
    sndWavFileLen = n;
    ok = AIL_WAV_info(wav, &mi) == 1 && mi.Info.samples == 4;
    sndWavFileLen = n - 1;   /* the length that counts is the file's, not what the header says */
    CHECK("ail_wav_length", ok && AIL_WAV_info(wav, &mi) == 0);

    AIL_shutdown();          /* snd_driver.c does this after a failed start too */
    dig = device_open("nonexistent", 44100);
    ok = !dig && AIL_last_error()[0] && !SDL_WasInit(SDL_INIT_AUDIO);
    AIL_shutdown();
    CHECK("ail_no_device", ok);

    /* Nothing here may crash: a stream channel keeps calling on a closed handle */
    AIL_close_stream(NULL); AIL_end_sample(NULL); AIL_resume_sample(NULL); AIL_set_stream_volume_levels(NULL, 1, 1);
    AIL_set_sample_volume_levels(NULL, 1, 1); AIL_pause_stream(NULL, 1); AIL_stream_ms_position(NULL, &total, NULL);
    AIL_set_3D_position(NULL, 0, 0, 0); AIL_load_sample_buffer(NULL, 0, NULL, 0); AIL_release_sample_handle(NULL);

    dig = device_open("dummy", 44100);
    ok = dig != NULL && statsCmd != NULL;
    s = mono16(dig, tone, 2000, 44100);
    ok = ok && AIL_sample_status(s) == SND_SMP_DONE;
    AIL_resume_sample(s);
    ok = ok && status_reaches(s, SND_SMP_DONE, 1000);    /* 1000 frames, once */
    AIL_init_sample(s);
    AIL_resume_sample(s);
    ok = ok && AIL_sample_status(s) == SND_SMP_DONE;     /* no address: 0x2112b77c */
    AIL_set_sample_type(s, 1, 0);
    AIL_set_sample_address(s, tone, 2000);
    AIL_set_sample_loop_count(s, 0);
    AIL_resume_sample(s);
    ok = ok && AIL_sample_status(s) == SND_SMP_PLAYING;
    AIL_stop_sample(s);
    ok = ok && AIL_sample_status(s) == SND_SMP_STOPPED;
    AIL_resume_sample(s);
    ok = ok && AIL_sample_status(s) == SND_SMP_PLAYING;
    AIL_end_sample(s);
    ok = ok && AIL_sample_status(s) == SND_SMP_DONE;
    AIL_resume_sample(s);
    ok = ok && AIL_sample_status(s) == SND_SMP_DONE;     /* ended is ended: its memory may be gone */
    CHECK("ail_status", ok);

    /* A hunk freed right after the end: ASan reads over the callback's shoulder */
    {
        short *hunk = malloc(sizeof tone);
        void *raw;
        memcpy(hunk, tone, sizeof tone);
        AIL_init_sample(s);
        AIL_set_sample_type(s, 1, 0);
        AIL_set_sample_address(s, hunk, sizeof tone);
        AIL_set_sample_loop_count(s, 0);
        AIL_resume_sample(s);
        t = AIL_allocate_3D_sample_handle(handle);
        memset(&info, 0, sizeof info);
        info.format = 1; info.data_ptr = hunk; info.data_len = sizeof tone; info.rate = 44100; info.bits = 16; info.channels = 1;
        AIL_set_3D_sample_info(t, &info);
        AIL_set_3D_sample_loop_count(t, 0);
        AIL_resume_3D_sample(t);
        AIL_load_sample_buffer(raw = AIL_allocate_sample_handle(dig), 0, hunk, sizeof tone);
        AIL_set_sample_playback_rate(raw, 100);
        SDL_Delay(60);
        ok = AIL_sample_status(s) == SND_SMP_PLAYING && AIL_3D_sample_status(t) == SND_SMP_PLAYING;
        AIL_end_sample(s);
        AIL_end_3D_sample(t);
        AIL_end_sample(raw);
        AIL_release_sample_handle(raw);
        free(hunk);
        SDL_Delay(60);
        CHECK("ail_hunk_freed", ok);
    }

    AIL_init_sample(s);
    AIL_set_sample_type(s, 1, 0);
    AIL_set_sample_address(s, tone, 44100 * 2);
    AIL_set_sample_playback_rate(s, 22050);
    AIL_sample_ms_position(s, &total, &cur);
    ok = total == 2000 && cur == 0;
    AIL_set_sample_playback_rate(s, 44100);
    AIL_sample_ms_position(s, &total, NULL);
    ok = ok && total == 1000;
    AIL_set_sample_ms_position(s, 500);
    AIL_sample_ms_position(s, NULL, &cur);
    ok = ok && cur == 500 && AIL_sample_position(s) == 22050 * 2;
    AIL_set_sample_ms_position(s, -7);                   /* the mixer casts pos to unsigned */
    AIL_sample_ms_position(s, NULL, &cur);
    ok = ok && cur == 0;
    AIL_set_sample_ms_position(s, 0x7fffffff);
    AIL_sample_ms_position(s, NULL, &cur);
    ok = ok && cur == 1000;
    AIL_set_sample_loop_count(s, -3);
    AIL_resume_sample(s);
    SDL_Delay(30);
    AIL_end_sample(s);
    AIL_init_sample(s);
    AIL_set_sample_type(s, 1, 0);
    AIL_set_sample_address(s, tone, 2000);
    AIL_set_sample_playback_rate(s, 0);                  /* no division by it */
    AIL_sample_ms_position(s, &total, &cur);
    ok = ok && total == 0 && cur == 0;
    AIL_set_sample_ms_position(s, 100);
    AIL_set_sample_playback_rate(s, 0x7fffffff);         /* one step eats it all */
    AIL_resume_sample(s);
    CHECK("ail_positions", ok && status_reaches(s, SND_SMP_DONE, 1000));

    /* ADPCM: the address comes before the block size, and its PCM is ours to free */
    memset(ima, 0, sizeof ima);
    AIL_init_sample(s);
    AIL_set_sample_type(s, 5, 0);
    AIL_set_sample_address(s, ima, 256);
    AIL_set_sample_adpcm_block_size(s, 256);
    AIL_set_sample_playback_rate(s, 505);                /* one block: 505 frames */
    AIL_sample_ms_position(s, &total, NULL);
    ok = total == 1000;
    AIL_set_sample_adpcm_block_size(s, 0);               /* from a hostile file */
    AIL_sample_ms_position(s, &total, NULL);
    AIL_resume_sample(s);
    ok = ok && total == 0 && AIL_sample_status(s) == SND_SMP_DONE;
    AIL_set_sample_adpcm_block_size(s, 6);               /* below 2 headers it is no block */
    AIL_set_sample_type(s, 7, 0);
    AIL_set_sample_adpcm_block_size(s, 6);
    AIL_sample_ms_position(s, &total, NULL);
    ok = ok && total == 0;
    AIL_set_sample_adpcm_block_size(s, 12);              /* not a whole number of groups */
    AIL_resume_sample(s);
    CHECK("ail_adpcm", ok && AIL_sample_status(s) == SND_SMP_DONE);

    /* 3D as the provider: mono PCM only; left is -X; levels above 1 stay as set, and are capped when mixed */
    AIL_set_3D_rolloff_factor(handle, 0);
    t = AIL_allocate_3D_sample_handle(handle);
    memset(&info, 0, sizeof info);
    info.format = 1; info.data_ptr = tone; info.data_len = 2000; info.rate = 44100; info.bits = 16; info.channels = 2;
    AIL_set_3D_sample_info(t, &info);
    AIL_resume_3D_sample(t);
    ok = AIL_3D_sample_status(t) == SND_SMP_DONE;
    info.channels = 1; info.format = 0x11;
    AIL_set_3D_sample_info(t, &info);
    AIL_resume_3D_sample(t);
    ok = ok && AIL_3D_sample_status(t) == SND_SMP_DONE;
    info.format = 1;
    AIL_set_3D_sample_info(t, &info);
    ok = ok && AIL_3D_sample_length(t) == 2000;
    AIL_set_3D_sample_offset(t, 400);
    ok = ok && AIL_3D_sample_offset(t) == 400;
    AIL_set_3D_sample_offset(t, 0x7fffffff);
    ok = ok && AIL_3D_sample_offset(t) == 2000;
    AIL_set_3D_sample_offset(t, -5);
    ok = ok && AIL_3D_sample_offset(t) == 0;
    AIL_set_3D_sample_volume(t, 1);
    AIL_set_3D_sample_distances(t, 100, 1);
    AIL_set_3D_position(t, -10, 0, 0);
    AIL_sample_volume_levels(t, &a, &b);
    ok = ok && NEAR(a, 1) && NEAR(b, 0);
    AIL_set_3D_position(t, 1000, 0, 0);                  /* past max: muted (0x22401058) */
    AIL_sample_volume_levels(t, &a, &b);
    ok = ok && a == 0 && b == 0;
    AIL_set_sample_volume_levels(s, 3, .5f);
    AIL_sample_volume_levels(s, &a, &b);
    ok = ok && a == 3 && b == .5f;
    AIL_set_sample_volume_levels(s, 1, 1);
    AIL_sample_volume_pan(s, &a, &b);                    /* 0x2112a838, 0x21115447: (.5^-.3)^.6 */
    CHECK("ail_3d", ok && NEAR(a, 1.1328839f) && NEAR(b, .5f));
    AIL_shutdown();
    ok = statsCmd == NULL;

    /* Device gone: nothing calls back, yet the voices end on time and raw takes more */
    dig = device_open("dummy", 44100);
    s = mono16(dig, tone, 8820, 44100);                  /* 100 ms */
    t = ail_stream(dig, 0, wav, mkwav(wav, 1, 1, 44100, 16, 2, tone, 8820, 0));
    ok = ok && t != NULL;
    AIL_pause_stream(t, 0);
    SDL_Delay(20);
    for (i = 1; i < 8; i++) SDL_PauseAudioDevice((SDL_AudioDeviceID)i, 1);
    AIL_resume_sample(s);
    {
        Uint32 start = SDL_GetTicks();
        ok = ok && status_reaches(s, SND_SMP_DONE, 2000) && stream_reaches(t, SND_SMP_DONE, 2000);
        ok = ok && SDL_GetTicks() - start >= 70;         /* the clock's, not at once */
    }
    AIL_close_stream(t);
    AIL_release_sample_handle(s);
    s = AIL_allocate_sample_handle(dig);
    AIL_set_sample_type(s, 1, 0);
    AIL_set_sample_playback_rate(s, 44100);
    ok = ok && AIL_sample_buffer_ready(s) == 0;
    AIL_load_sample_buffer(s, 0, tone, 882);
    ok = ok && AIL_sample_buffer_ready(s) == 1;
    AIL_load_sample_buffer(s, 1, tone, 882);
    k = -1;
    for (i = 0; i < 1000 && k < 0; i++) { SDL_Delay(1); k = AIL_sample_buffer_ready(s); }
    CHECK("ail_device_gone", ok && k == 0);
    AIL_end_sample(s);
    AIL_release_sample_handle(s);
    AIL_shutdown();

    /* Raw: two buffers play seamlessly, and each is given back as the other starts */
    for (i = 0; i < 24576; i++) ramp[i] = (short)(i + 1);
    remove(rawPath);
    dig = device_open("disk", 44100);
    s = AIL_allocate_sample_handle(dig);
    ok = AIL_minimum_sample_buffer_size(dig, 22050, 3) == 2048;
    AIL_set_sample_type(s, 1, 0);
    AIL_set_sample_playback_rate(s, 44100);
    AIL_set_sample_volume_levels(s, 1, 1);
    ok = ok && AIL_sample_buffer_ready(s) == 0;
    AIL_load_sample_buffer(s, 0, ramp, 16384);
    ok = ok && AIL_sample_status(s) == SND_SMP_PLAYING && AIL_sample_buffer_ready(s) == 1;
    AIL_load_sample_buffer(s, 1, ramp + 8192, 16384);
    k = -1;
    {
        Uint32 start = SDL_GetTicks();
        while (k < 0 && SDL_GetTicks() - start < 2000) k = AIL_sample_buffer_ready(s);
    }
    ok = ok && k == 0;
    AIL_load_sample_buffer(s, 0, ramp + 16384, 16384);
    ok = ok && status_reaches(s, SND_SMP_DONE, 2000);
    AIL_end_sample(s);
    AIL_release_sample_handle(s);
    AIL_shutdown();
    len = raw_read(out, 1 << 19);
    for (i = 0; i < len && !out[i * 2]; i++) {}
    n = i;
    for (i = 0; i < 24576 && i + n < len; i++) ok = ok && out[(i + n) * 2] == (short)(i + 1);
    CHECK("ail_raw", ok && i == 24576);

    /* Pitch: what comes out of a device at another rate */
    for (k = 0; k < 3; k++) {
        static const int device[3] = { 22050, 44100, 11025 };
        static const double want[3] = { 22.05, 44.1, 5.5125 };
        remove(rawPath);
        dig = device_open("disk", device[k]);
        if (k == 0) {                                    /* a sample at 44.1, 1 kHz */
            s = mono16(dig, tone, 8820, 44100);
            AIL_set_sample_volume_levels(s, 1, 1);
            AIL_resume_sample(s);
            ok = status_reaches(s, SND_SMP_DONE, 5000);
        } else {                                         /* a stream at 2x: tone's 44.1 frames a cycle go by in half */
            int fileRate = k == 1 ? 22050 : 44100, rate = fileRate * 2;
            long wl = mkwav(wav, 1, 1, fileRate, 16, 2, tone, 44100, 0);
            s = ail_stream(dig, 0, wav, wl);
            AIL_set_stream_playback_rate(s, rate);
            AIL_set_stream_volume_levels(s, 1, 1);
            AIL_set_stream_ms_position(s, 0);
            AIL_pause_stream(s, 0);
            ok = stream_reaches(s, SND_SMP_DONE, 5000);
            AIL_close_stream(s);
        }
        AIL_shutdown();
        len = raw_read(out, 1 << 19);
        printf("# period %d: %g want %g, %ld frames\n", k, period(out, len), want[k], len);
        CHECK(k == 0 ? "ail_pitch_sample" : k == 1 ? "ail_pitch_stream_2x" : "ail_pitch_stream_11k",
              ok && fabs(period(out, len) - want[k]) < want[k] * 0.01);
    }

    /* Miles caps each gain at 1 (0x2112be21), the mixer does not */
    for (i = 0; i < 2000; i++) ramp[i] = 10000;
    remove(rawPath);
    dig = device_open("disk", 44100);
    s = mono16(dig, ramp, 4000, 44100);
    AIL_set_sample_volume_levels(s, 3, 3);
    AIL_resume_sample(s);
    ok = status_reaches(s, SND_SMP_DONE, 2000);
    AIL_shutdown();
    len = raw_read(out, 1 << 19);
    k = 0;
    for (i = 0; i < len; i++) if (out[i * 2] > k) k = out[i * 2];
    CHECK("ail_gain_cap", ok && k == 10000);

    /* All 53 at once, at full scale: 8 2D, 32 3D (left), 13 streams */
    remove(rawPath);
    dig = device_open("disk", 44100);
    for (i = 0; i < 8; i++) {
        handles[i] = mono16(dig, dc, sizeof dc, 44100);
        AIL_set_sample_loop_count(handles[i], 0);
        AIL_set_sample_volume_levels(handles[i], 1, 1);
    }
    memset(&info, 0, sizeof info);
    info.format = 1; info.data_ptr = dc; info.data_len = sizeof dc; info.rate = 44100; info.bits = 16; info.channels = 1;
    for (; i < 40; i++) {
        handles[i] = AIL_allocate_3D_sample_handle(handle);
        AIL_set_3D_sample_info(handles[i], &info);
        AIL_set_3D_sample_loop_count(handles[i], 0);
        AIL_set_3D_sample_volume(handles[i], 1);
        AIL_set_3D_position(handles[i], -10, 0, 0);
    }
    n = mkwav(wav, 1, 2, 44100, 16, 4, dc, sizeof dc, 0);
    ok = 1;
    for (; i < 53; i++) {
        handles[i] = ail_stream(dig, i - 40, wav, n);
        ok = ok && handles[i];
        if (!handles[i]) continue;
        AIL_set_stream_loop_count(handles[i], 0);
        AIL_set_stream_volume_levels(handles[i], 1, 1);
        AIL_pause_stream(handles[i], 0);
    }
    /* the rate that would ask a stream for the most: bounded, ASan would see it */
    AIL_set_stream_playback_rate(handles[52], 0x7fffffff);
    ok = ok && !AIL_open_stream(dig, "0", 0);            /* past the game's 13 */
    AIL_set_digital_master_room_type(dig, 10);           /* with the reverb working */
    for (i = 0; i < 8; i++) AIL_set_sample_reverb_levels(handles[i], 1, 1);
    for (; i < 40; i++) AIL_set_3D_sample_effects_level(handles[i], 1);
    for (; i < 53; i++) AIL_set_stream_reverb_levels(handles[i], 1, 1);
    for (i = 0; i < 8; i++) AIL_resume_sample(handles[i]);
    for (; i < 40; i++) AIL_resume_3D_sample(handles[i]);
    SDL_Delay(200);
    statsCmd();
    {
        double worst = 0, buffer = 0;
        sscanf(printed, "snd_stats: worst callback %lf ms, last %*f ms, buffer %lf ms", &worst, &buffer);
#if !defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__)
        ok = ok && worst < buffer / 2;
#endif
        printf("# worst callback %g of %g ms\n", worst, buffer);
        ok = ok && buffer > 23 && buffer < 24;
    }
    AIL_shutdown();                                      /* with all of it running */
    ok = ok && closes;
    len = raw_read(out, 1 << 19);
    k = 0;
    for (i = 0; i < len * 2; i++) {
        if (i > 1 && abs(out[i] - out[i - 2]) > 60000) ok = 0;   /* saturated, not wrapped; the tail may swing */
        if (out[i] == 32767) k++;
    }
    CHECK("ail_53_voices", ok && k > 1000);

    /* A corrupt stream in an iwd: snd_driver.c:1357 retries with FS_ShortOSFilePath, NULL for a file in a pack */
    dig = device_open("dummy", 44100);
    memset(wav, 0x55, 4096);
    closes = 0;
    ok = ail_stream(dig, 0, wav, 4096) == NULL && closes == 1 && AIL_last_error()[0];
    AIL_set_file_callbacks(NULL, NULL, NULL, NULL);
    ok = ok && !AIL_open_stream(dig, "0", 0);            /* no callbacks: mss_q3fs 0 */
    set_callbacks();
    ok = ok && !AIL_open_stream(dig, NULL, 0) && AIL_last_error()[0] && closes == 1;
    n = mkwav(wav, 1, 1, 44100, 16, 2, tone, 8, 0);
    s = ail_stream(dig, 0, wav, n);                      /* and it goes on */
    ok = ok && s != NULL;
    AIL_close_stream(s);
    AIL_shutdown();
    CHECK("ail_stream_retry", ok);

    /* A stream's life, with the voice's tail mixed before it reads done */
    dig = device_open("dummy", 44100);
    closes = 0;
    n = mkwav(wav, 1, 2, 44100, 16, 4, tone, 8820, 0);
    s = ail_stream(dig, 0, wav, n);
    ok = s != NULL && AIL_stream_status(s) == SND_SMP_STOPPED;
    AIL_stream_info(s, &i, &k, NULL, NULL);
    ok = ok && i == 44100 && k == 3 && AIL_stream_playback_rate(s) == 44100;
    AIL_stream_ms_position(s, &total, NULL);
    ok = ok && total == 50;
    AIL_set_stream_loop_count(s, -1);                    /* forever */
    AIL_set_stream_ms_position(s, -3);
    AIL_pause_stream(s, 0);
    ok = ok && stream_reaches(s, SND_SMP_PLAYING, 1000);
    SDL_Delay(120);
    ok = ok && AIL_stream_status(s) == SND_SMP_PLAYING;
    AIL_pause_stream(s, 1);
    ok = ok && AIL_stream_status(s) == SND_SMP_STOPPED;
    AIL_pause_stream(s, 0);
    AIL_set_stream_loop_count(s, 1);
    AIL_set_stream_ms_position(s, 0);
    ok = ok && stream_reaches(s, SND_SMP_DONE, 2000);
    AIL_close_stream(s);
    ok = ok && closes == 1;
    t = ail_stream(dig, 1, wav, n);                      /* a slot freed by the close, a stream left open */
    ok = ok && t != NULL;
    AIL_pause_stream(t, 0);
    ok = ok && !AIL_open_stream(dig, "missing", 0) && AIL_last_error()[0];
    AIL_shutdown();
    ok = ok && closes == 2;
    dig = device_open("dummy", 44100);                   /* a second life after shutdown */
    s = mono16(dig, tone, 2000, 44100);
    AIL_resume_sample(s);
    ok = ok && status_reaches(s, SND_SMP_DONE, 1000);
    AIL_shutdown();
    AIL_shutdown();
    CHECK("ail_stream_life", ok);

    /* The driver's calls in SND_SetRoomtype's order, on each kind of voice: what rings after it ends */
    for (k = 0; k < 4; k++) {
        int room = k == 3 ? 0 : 3;                       /* generic: the 3D call leaves it off */
        long tail = 0;
        remove(rawPath);
        dig = device_open("disk", 44100);
        AIL_set_digital_master_room_type(dig, room);
        AIL_set_digital_master_reverb_levels(dig, 1, .5f);
        AIL_set_3D_room_type(handle, room);
        if (k == 1) {
            memset(&info, 0, sizeof info);
            info.format = 1; info.data_ptr = tone; info.data_len = 8820; info.rate = 44100; info.bits = 16; info.channels = 1;
            s = AIL_allocate_3D_sample_handle(handle);
            AIL_set_3D_sample_info(s, &info);
            AIL_set_3D_sample_volume(s, 1);
            AIL_set_3D_sample_effects_level(s, 1);
            AIL_resume_3D_sample(s);
        } else if (k == 2) {
            s = ail_stream(dig, 0, wav, mkwav(wav, 1, 1, 44100, 16, 2, tone, 8820, 0));
            AIL_set_stream_volume_levels(s, 1, 1);
            AIL_set_stream_reverb_levels(s, 1, 1);
            AIL_pause_stream(s, 0);
        } else {
            s = mono16(dig, tone, 8820, 44100);
            AIL_set_sample_volume_levels(s, 1, 1);
            AIL_set_sample_reverb_levels(s, 1, 1);
            AIL_resume_sample(s);
        }
        SDL_Delay(600);
        if (k == 2) AIL_close_stream(s);
        AIL_shutdown();
        len = raw_read(out, 1 << 19);
        for (i = 0; i < len && !out[i * 2]; i++) {}
        for (i += 4500 + 50; i < len; i++) if (out[i * 2]) tail++;
        printf("# reverb kind %d: %ld of %ld frames ring after the sound\n", k, tail, len);
        ok = len > 4410 + 11025 && (k == 3 ? tail == 0 : tail > 2000);
        CHECK(k == 0 ? "ail_reverb_2d" : k == 1 ? "ail_reverb_3d" : k == 2 ? "ail_reverb_stream" : "ail_reverb_generic", ok);
    }
    remove(rawPath);
}
#endif

/* Peak of each 50 ms after an impulse, on both channels: when the tail last was within 60 dB of its
   start, in seconds. *grew: a later window passed the first 600 ms, which hold any predelay. */
static void reverb_fresh(int room, float wet)
{
    SndReverb_Process(NULL, NULL, 0, 2, 0);
    SndReverb_SetRoom(room);
    SndReverb_SetLevels(1, wet);
}

static double reverb_tail(int room, int rate, int *grew)
{
    static float in[256];
    static short out[512];
    long done = 0, last = 0, window = rate / 20 / 256 * 256;
    int top = 0, peak = 0, i;

    reverb_fresh(room, 1);
    in[0] = 32767;
    if (window < 256) window = 256;
    while (done < rate * 40L && (done < rate || done < last + rate)) {
        memset(out, 0, sizeof out);
        SndReverb_Process(in, out, 256, 2, rate);
        in[0] = 0;
        for (i = 0; i < 512; i++) if (abs(out[i]) > peak) peak = abs(out[i]);
        done += 256;
        if (done % window) continue;
        if (done <= rate * 3 / 5) { if (peak > top) top = peak; }
        else if (peak * 2 > top * 3 && grew) *grew = 1;   /* echoes of a slow room can line up: a bit above is no growth */
        if (peak * 1000 >= top) last = done;
        peak = 0;
    }
    return (double)last / rate;
}

static void reverb_tests(void)
{
    static const int rates[5] = { 11025, 22050, 44100, 48000, 96000 };
    static float in[4410];
    static short out[8820];
    double t0, t1, slow = 0;
    int room, r, i, n, ok, grew = 0, before;
    long sum;
    clock_t start, idle, busy;

    /* Miles' generic has a tail of 1.493 s and a wet of 0 (0x21155838), which is what the 3D call leaves */
    reverb_fresh(0, 0);
    SndReverb_SetRoom(0);
    in[0] = 32767;
    memset(out, 0, sizeof out);
    SndReverb_Process(in, out, 4410, 2, 44100);
    for (sum = 0, i = 0; i < 8820; i++) sum += abs(out[i]);
    in[0] = 0;
    CHECK("reverb_generic_off", sum == 0);

    t0 = reverb_tail(3, 44100, NULL);
    printf("# bathroom %g s\n", t0);
    /* SetRoom alone gives the room's own level, as Miles' does */
    reverb_fresh(3, 0);
    SndReverb_SetRoom(3);
    in[0] = 32767;
    memset(out, 0, sizeof out);
    SndReverb_Process(in, out, 4410, 2, 44100);
    in[0] = 0;
    for (sum = 0, i = 0; i < 8820; i++) sum += abs(out[i]);
    CHECK("reverb_impulse_tail", sum > 0 && t0 > .2 && t0 < 3);

    t1 = reverb_tail(10, 44100, NULL);
    printf("# hangar %g s\n", t1);
    CHECK("reverb_room_differs", t1 > t0 * 3);

    /* every room, at every rate the device may open */
    for (ok = 1, r = 0; r < 5; r++)
        for (room = 0; room < 26; room++) {
            double t = reverb_tail(room, rates[r], &grew);
            if (t > slow) slow = t;
            ok = ok && t > 0 && t < 30;
        }
    printf("# slowest room %g s\n", slow);
    CHECK("reverb_decays", ok && !grew);

    before = __atomic_load_n(&allocs, __ATOMIC_RELAXED);
    reverb_fresh(5, 1);
    in[0] = 32767;
    for (i = 0; i < 100; i++) {
        memset(out, 0, sizeof out);
        SndReverb_Process(in, out, 256, 2, i < 50 ? 44100 : 22050);
        SndReverb_SetRoom(i % 26);
    }
    in[0] = 0;
    CHECK("reverb_no_alloc_in_callback", __atomic_load_n(&allocs, __ATOMIC_RELAXED) == before);

    t0 = reverb_tail(3, 11025, NULL);
    t1 = reverb_tail(3, 44100, NULL);
    ok = fabs(t0 - t1) < t1 * .25;
    t0 = reverb_tail(10, 11025, NULL);
    t1 = reverb_tail(10, 44100, NULL);
    printf("# hangar 11 kHz %g s, 44 kHz %g s\n", t0, t1);
    CHECK("reverb_rate_change", ok && t1 < 20 && fabs(t0 - t1) < t1 * .25);

    /* A room's predelay is another read point: no jump in what goes to the tail */
    reverb_fresh(12, 1);
    {
        int steps[3] = { 0, 0, 0 }, k;
        for (n = 0, k = 0; k < 30; k++) {
            for (i = 0; i < 2205; i++, n++) in[i] = (float)(20000 * sin(2 * 3.14159265358979 * 200 * n / 44100));
            memset(out, 0, sizeof out);
            SndReverb_Process(in, out, 2205, 2, 44100);
            for (i = 2; k >= 10 && i < 4410; i++)
                if (abs(out[i] - out[i - 2]) > steps[(k > 20) + (k > 22)]) steps[(k > 20) + (k > 22)] = abs(out[i] - out[i - 2]);
            if (k == 20) {
                SndReverb_SetRoom(17);
                SndReverb_SetLevels(1, 1);
            }
        }
        printf("# steps before %d, at the change %d, after %d\n", steps[0], steps[1], steps[2]);
        CHECK("reverb_room_fade", steps[1] > 0 && steps[1] * 10 < (steps[0] > steps[2] ? steps[0] : steps[2]) * 11);
    }
    memset(in, 0, sizeof in);

    /* A dead tail costs nothing: it sleeps long before its floats go denormal */
    reverb_fresh(1, 1);
    in[0] = 32767;
    memset(out, 0, sizeof out);
    SndReverb_Process(in, out, 4410, 2, 44100);
    in[0] = 0;
    for (i = 0; i < 40; i++) SndReverb_Process(in, out, 4410, 2, 44100);
    start = clock();
    for (i = 0; i < 2000; i++) SndReverb_Process(in, out, 256, 2, 44100);
    idle = clock() - start;
    in[0] = 32767;
    start = clock();
    for (i = 0; i < 2000; i++) SndReverb_Process(in, out, 256, 2, 44100);
    busy = clock() - start;
    in[0] = 0;
    printf("# idle %ld, busy %ld ticks\n", (long)idle, (long)busy);
    CHECK("reverb_sleeps", idle * 5 < busy);

    /* Added to what is there, saturating; dry scales it; mono is one channel */
    reverb_fresh(3, 1);
    SndReverb_SetLevels(.5f, 1);
    memset(in, 0, sizeof in);
    for (i = 0; i < 256; i++) out[i] = 1000;
    SndReverb_Process(in, out, 128, 2, 44100);
    ok = out[0] == 500 && out[255] == 500;
    reverb_fresh(3, 1);
    for (i = 0; i < 2205; i++) in[i] = 32767 * 8.0f;
    for (i = 0; i < 4410; i++) out[i] = 32767;
    SndReverb_Process(in, out, 2205, 2, 44100);
    for (i = 0; i < 4410; i++) ok = ok && out[i] > 0;
    reverb_fresh(3, 1);
    for (i = 0; i < 2205; i++) out[i] = 0;
    out[2205] = 77;
    SndReverb_Process(in, out, 2205, 1, 44100);
    memset(in, 0, sizeof in);
    CHECK("reverb_mixes", ok && out[2205] == 77);

    /* Out of the table, and rates no device gives: ASan reads along */
    for (i = 0; i < 4; i++) {
        static const int rooms[4] = { -1, 26, 1000, 0x7fffffff }, odd[4] = { 1, 8000, 192000, 0x7fffffff };
        reverb_fresh(rooms[i], 1);
        in[0] = 32767;
        SndReverb_Process(in, out, 4410, 2, odd[i]);
        SndReverb_SetLevels(-1, 1e30f);
        SndReverb_Process(in, out, 4410, 2, odd[i]);
    }
    in[0] = 0;
    reverb_fresh(0, 0);
    CHECK("reverb_bad_input", 1);
}

int main(void)
{
    static short pcm[4410], out[8820];
    static unsigned char u8[3] = { 128, 255, 0 };
    static short st[4] = { 1000, 0, 0, -2000 };   /* L,R,L,R: left then right */
    static short big[1] = { 32767 };
    static SndVoice many[53];
    SndVoice v, w;
    int i, n;

    for (i = 0; i < 100; i++) pcm[i] = (short)(i * 300 - 15000);

    v = voice(pcm, 0, 16, 1, 44100);
    v.status = SND_SMP_STOPPED;
    memset(out, 1, sizeof out);
    SndMix_Render(&v, 1, 44100, 2, out, NULL, 10);
    for (n = 0, i = 0; i < 20; i++) n |= out[i];
    CHECK("mix_silence", n == 0 && v.pos == 0);

    v = voice(pcm, 200, 16, 1, 44100);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 100);
    CHECK("mix_pcm16_unity", memcmp(out, pcm, 200) == 0 && v.status == SND_SMP_DONE);

    v = voice(u8, 3, 8, 1, 44100);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 3);
    CHECK("mix_pcm8_unsigned", out[0] == 0 && out[1] > 32000 && out[2] < -32000);

    v = voice(st, 8, 16, 2, 44100);
    v.right = 0;
    SndMix_Render(&v, 1, 44100, 2, out, NULL, 2);
    CHECK("mix_stereo_source", out[0] == 1000 && out[1] == 0 && out[2] == 0 && out[3] == 0);
    v = voice(st, 8, 16, 2, 44100);
    v.left = 0;
    SndMix_Render(&v, 1, 44100, 2, out, NULL, 2);
    CHECK("mix_stereo_source_r", out[0] == 0 && out[1] == 0 && out[3] == -2000);

    v = voice(pcm, 200, 16, 1, 88200);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 49);
    n = v.status == SND_SMP_PLAYING && v.pos == 98;
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 1);
    CHECK("mix_resample_2x", n && v.status == SND_SMP_DONE);

    v = voice(pcm, 200, 16, 1, 22050);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 199);
    n = v.status == SND_SMP_PLAYING;
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 1);
    CHECK("mix_resample_half", n && v.status == SND_SMP_DONE);

    for (i = 0; i < 2205; i++) pcm[i] = (short)(16000 * sin(2 * 3.14159265358979 * 1000 * i / 22050));
    v = voice(pcm, 2205 * 2, 16, 1, 22050);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 4410);
    for (n = 0, i = 1; i < 4410; i++) n += (out[i - 1] < 0) != (out[i] < 0);
    /* 0.1 s at 1 kHz = 100 cycles = 200 crossings */
    CHECK("mix_rate_22_to_44", n >= 198 && n <= 202);

    for (i = 0; i < 10; i++) pcm[i] = (short)(i * 100 + 100);
    v = voice(pcm, 20, 16, 1, 44100);
    v.loopCount = 0;
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 100);
    n = v.status == SND_SMP_PLAYING;
    w = voice(pcm, 20, 16, 1, 44100);
    w.loopCount = 2;
    SndMix_Render(&w, 1, 44100, 1, out, NULL, 19);
    n = n && w.status == SND_SMP_PLAYING;
    SndMix_Render(&w, 1, 44100, 1, out + 19, NULL, 1);
    for (i = 0; i < 20; i++) n = n && out[i] == pcm[i % 10];
    CHECK("mix_loop_wraps", n && w.status == SND_SMP_DONE);

    for (i = 0; i < 53; i++) many[i] = voice(big, 2, 16, 1, 44100);
    big[0] = 32767;
    for (i = 0; i < 53; i++) many[i].loopCount = 0;
    SndMix_Render(many, 53, 44100, 2, out, NULL, 600);
    for (n = 1, i = 0; i < 1200; i++) n = n && out[i] == 32767;
    CHECK("mix_saturates", n);

    v = voice(pcm, 20, 16, 1, 0);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 10);
    CHECK("mix_rate_zero", v.status == SND_SMP_PLAYING);

    v = voice(st, 8, 16, 2, 44100);
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 2);
    CHECK("mix_mono_output", out[0] == 500 && out[1] == -1000);

    {
        static float wet[2];
        v = voice(big, 2, 16, 1, 44100);
        v.wet = 0.5f;
        SndMix_Render(&v, 1, 44100, 1, out, wet, 1);
        n = wet[0] == 16383.5f;
        v = voice(big, 2, 16, 1, 44100);                 /* past max distance it feeds no room */
        v.wet = 1;
        v.left = v.right = 0;
        SndMix_Render(&v, 1, 44100, 1, out, wet, 1);
        CHECK("mix_wet", n && wet[0] == 0);
    }

    {
        static unsigned char f[512];
        static const unsigned char ima[8] = { 0xe8, 0x03, 0, 0, 0x04, 0x00, 0, 0 };   /* pred 1000, idx 0, nibbles 4,0,0.. */
        static const unsigned char ima2[16] = { 0xe8, 0x03, 0, 0, 0x18, 0xfc, 0, 0, 0x04, 0, 0, 0, 0x08, 0, 0, 0 };
        static short st44[8] = { 100, -100, 200, -200, 300, -300, 400, -400 };
        static unsigned char cv[64];
        SndMixInfo mi, mo;
        short dec[20];
        long len, size;
        unsigned char *exact;

        len = mkwav(f, 1, 1, 22050, 16, 2, pcm, 8, 0);
        CHECK("wav_pcm16_mono", SndWav_Info(f, len, &mi) && mi.Info.format == 1 && mi.Info.rate == 22050 &&
              mi.Info.bits == 16 && mi.Info.channels == 1 && mi.Info.data_len == 8 && mi.Info.samples == 4 &&
              mi.Info.block_size == 2 && mi.Info.data_ptr == f + 44 && mi.Info.initial_ptr == f + 44);
        len = mkwav(f, 1, 2, 44100, 16, 4, st44, 16, 0);
        CHECK("wav_stereo_44", SndWav_Info(f, len, &mi) && mi.Info.rate == 44100 && mi.Info.channels == 2 &&
              mi.Info.samples == 4 && mi.Info.block_size == 4);
        len = mkwav(f, 1, 1, 22050, 16, 2, pcm, 8, 1);
        CHECK("wav_chunks_extra", SndWav_Info(f, len, &mi) && mi.Info.data_ptr == f + 56 && mi.Info.data_len == 8);

        len = mkwav(f, 0x11, 1, 22050, 4, 8, ima, 8, 0);
        n = SndWav_Info(f, len, &mi) && mi.Info.format == 0x11 && mi.Info.samples == 9;
        memset(dec, 0x55, sizeof dec);
        i = SndAdpcm_Decode(ima, 8, 8, 1, dec, 20);
        CHECK("wav_adpcm", n && i == 9 && dec[0] == 1000 && dec[1] == 1007 && dec[2] == 1008 && dec[9] == 0x5555);
        memset(dec, 0x55, sizeof dec);
        i = SndAdpcm_Decode(ima2, 16, 16, 2, dec, 20);   /* stereo: L pred 1000, R pred -1000 (0xfc18) */
        CHECK("wav_adpcm_stereo", i == 9 && dec[0] == 1000 && dec[1] == -1000 && dec[2] == 1007 && dec[3] == -1000);
        memset(dec, 0x55, sizeof dec);
        i = SndAdpcm_Decode(ima, 8, 8, 1, dec, 3);
        CHECK("wav_adpcm_cap", i == 3 && dec[2] == 1008 && dec[3] == 0x5555);
        CHECK("wav_adpcm_bad", SndAdpcm_Decode(ima, 8, 0, 1, dec, 20) == 0 && SndAdpcm_Decode(ima, 8, 6, 1, dec, 20) == 0 &&
              SndAdpcm_Decode(ima, 3, 8, 1, dec, 20) == 0);

        len = mkwav(f, 1, 1, 44100, 16, 2, pcm, 40, 0);
        SndWav_Info(f, len, &mi);
        size = SndWav_ConvertedSize(22050, 1, &mi);
        n = size == 20 && SndWav_Convert(cv, 20, 22050, 1, &mi) == 20 && SndWav_Convert(cv, 19, 22050, 1, &mi) == 0;
        for (i = 0; i < 10; i++) n = n && ((short *)cv)[i] == pcm[i * 2];
        CHECK("convert_downsample", n && SndWav_ConvertedSize(44100, 0, &mi) == 20 && SndWav_ConvertedSize(0, 1, &mi) == 0);

        len = mkwav(f, 1, 2, 44100, 16, 4, st44, 16, 0);
        SndWav_Info(f, len, &mi);
        n = SndWav_ConvertedSize(44100, 1, &mi) == 8 && SndWav_Convert(cv, 8, 44100, 1, &mi) == 8;
        CHECK("convert_stereo_to_mono", n && ((short *)cv)[0] == 0 && ((short *)cv)[1] == 0 && ((short *)cv)[3] == 0);
        n = SndWav_ConvertedSize(44100, 3, &mi) == 16 && SndWav_Convert(cv, 16, 44100, 3, &mi) == 16 && !memcmp(cv, st44, 16);
        CHECK("convert_identity_stereo16", n);

        len = mkwav(f, 1, 1, 22050, 16, 2, pcm, 8, 1);
        for (n = 1, i = 0; i <= 64 && i < len; i++) {
            exact = malloc(i ? i : 1);   /* exact size: ASan sees any read past the prefix */
            memcpy(exact, f, i);
            n = n && SndWav_Info(exact, i, &mi) == 0;
            free(exact);
        }
        CHECK("wav_truncated", n);

        len = mkwav(f, 1, 1, 22050, 16, 2, pcm, 8, 0);
        put32(f + 40, 9);
        n = SndWav_Info(f, len, &mi) == 0;
        put32(f + 40, 0xffffffffUL);
        CHECK("wav_data_len_lies", n && SndWav_Info(f, len, &mi) == 0);

        len = mkwav(f, 1, 1, 22050, 16, 0, pcm, 8, 0);
        n = SndWav_Info(f, len, &mi) == 0;
        len = mkwav(f, 0x11, 1, 22050, 4, 0, ima, 8, 0);
        CHECK("wav_block_zero", n && SndWav_Info(f, len, &mi) == 0);

        memcpy(f, "ID3\3\0\0\0\0\0\0\xff\xfb\x90\x64", 14);
        n = SndWav_Info(f, 14, &mi) == 0;
        f[0] = 0xff; f[1] = 0xfb;
        CHECK("wav_mp3_rejected", n && SndWav_Info(f, 14, &mi) == 0);
        (void)mo;
    }
    /* Expected values worked out by hand from msssoft.m3d 0x22401000 and mss32.dll 0x2112a930:
       gain = volume^(5/3), pan = acos(x/dist)/pi, left = pan*gain, right = (1-pan)*gain. */
    CHECK("spatial_center", levels(0, 0, 10, 1, 100, 0, 1, .5f, .5f)
          && levels(0, 0, 10, 1, 100, 0, .5f, .15749013f, .15749013f));   /* .5^(5/3) = .31498026 */
    CHECK("spatial_full_left", levels(-10, 0, 0, 1, 100, 0, 1, 1, 0)
          && levels(10, 0, 0, 1, 100, 0, 1, 0, 1)
          && levels(-10, 0, 10, 1, 100, 0, 1, .75f, .25f)                 /* 135 degrees from +X */
          && levels(0, 10, 0, 1, 100, 0, 1, .5f, .5f));                   /* above: X = 0 */
    CHECK("spatial_behind", levels(0, 0, -10, 1, 100, 0, 1, .375f, .375f)  /* * .75 */
          && levels(-10, 0, -10, 1, 100, 0, 1, .5625f, .1875f));
    CHECK("spatial_inside_min", levels(0, 0, 5, 10, 100, 1, 1, .5f, .5f));
    CHECK("spatial_beyond_max", levels(0, 0, 100.5f, 1, 100, 0, 1, 0, 0)
          && levels(0, 0, 100, 1, 100, 0, 1, .5f, .5f)                    /* at max it still sounds */
          && levels(0, 0, 50, 100, 1, 0, 1, .5f, .5f));                   /* min > max are swapped */
    CHECK("spatial_rolloff", levels(0, 0, 40, 10, 100, 1, 1, .125f, .125f)     /* 10/(10+1*30) */
          && levels(0, 0, 40, 10, 100, .5f, 1, .2f, .2f)                       /* 10/(10+.5*30) */
          && levels(0, 0, 40, 10, 100, 0, 1, .5f, .5f)                         /* the game's rolloff */
          && levels(0, 0, 1, 0, 100, 1, 1, .00005f, .00005f));                 /* min clamps to 1e-4 */
    CHECK("spatial_zero_distance", levels(0, 0, 0, 0, 0, 1, 1, .5f, .5f)
          && levels(0, 0, -.00001f, 1, 100, 0, 1, .375f, .375f));         /* centered, yet still behind */
    CHECK("volume_pan_extremes", vol_pan(1, 0, 1, 0) && vol_pan(1, 1, 0, 1) && vol_pan(0, .5f, 0, 0)
          && vol_pan(1, .5f, .8122524f, .8122524f)                        /* .5^.3 */
          && vol_pan(.5f, 0, .31498026f, 0));
    reverb_tests();
#ifdef SND_TEST_STREAMS
    stream_tests();
#endif
#ifdef SND_TEST_AIL
    ail_tests();
#endif
    return failed;
}

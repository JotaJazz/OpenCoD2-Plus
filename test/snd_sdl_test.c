#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snd_sdl.h"

static int failed;
#define CHECK(name, cond) do { int ok_ = (cond); printf("%s %s\n", name, ok_ ? "PASS" : "FAIL"); if (!ok_) failed = 1; } while (0)

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
        CHECK("mix_wet", wet[0] == 16383.5f);
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
#ifdef SND_TEST_STREAMS
    stream_tests();
#endif
    return failed;
}

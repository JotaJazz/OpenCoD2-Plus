/* Streams: a WAV or MP3 decoded ahead by one reader thread into a ring per stream, which the audio
   callback drains without locks. Everything from the file is untrusted: iwds come from servers.
   Like the stream thread in mss32.dll (0x21110210), one thread serves all the streams and calls the
   read and seek callbacks itself, loops included (0x21110e40). */
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
/* Layer III is all the game has, so a hostile file reaches no more of the decoder than that. No SIMD:
   the reader is a thread entry, and on MinGW i686 its stack is not 16-byte aligned for -msse code. */
#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "snd_sdl.h"

enum {
    MAX_STREAMS = 13,              /* the game's stream handles, fsh[61..73] */
    IN_SIZE = 16384,               /* minimp3 wants 16 KB to sync; bounds an ADPCM block too */
    PCM_SHORTS = 32768,            /* an ADPCM block of IN_SIZE bytes */
    PCM_CHUNK = 1152,              /* frames per decode from a PCM WAV */
    FORMAT_MP3 = 0,                /* else the WAVE tag */
    FORMAT_PCM = 1,
    FORMAT_ADPCM = 0x11,
    MAX_RATE = 192000,             /* bounds the ring */
    MP3_SCAN = 256 * 1024,         /* how far Open looks for a first frame, past an ID3 tag */
    IDLE_MS = 10                   /* far below the ring's second */
};

struct SndStream {
    /* fixed after Open */
    SndFileCallbacks cb;
    unsigned long handle;
    int format, rate, channels, bits, block;
    long dataStart;                /* of the data chunk or the first MP3 frame */
    unsigned long dataLen;
    long fileLen;                  /* MP3 only, without a tag at the end */
    long long totalFrames;

    /* ring: frames are counted, not wrapped, and cap is a power of two. Lock free: the reader moves
       head, the callback tail, and head - tail <= cap. After a seek the reader leaves in drop the
       frame to go on from and raises dropped; the callback takes it there, as only it moves tail. */
    short *ring;
    unsigned cap, head, tail, drop, dropped;
    unsigned paused, seeking, eof;

    /* under lock */
    int loops;
    long seekMs;                   /* -1: none */
    long long baseFrame;           /* the frame at drop */

    /* the reader's, and Open's before the reader can see the stream */
    mp3dec_t mp3;
    unsigned char in[IN_SIZE];
    int inPos, inLen, srcEnd, kbps;
    unsigned long remaining;       /* bytes the file may still give */
    short pcm[PCM_SHORTS];
    int pcmPos, pcmLen;            /* frames */
    long skip;                     /* frames a seek inside an ADPCM block drops */
    int plays, playFrames, atEnd;  /* plays finished; whether this one gave sound; past the last frame */
};

static SDL_mutex *lock;            /* never held across a callback */
static SDL_cond *wake, *idle;      /* wake: the reader has news; idle: busy changed */
static SDL_Thread *thread;
static SndStream *streams[MAX_STREAMS];   /* written by the game thread under lock */
static SndStream *busy;            /* the stream the reader is working on, with lock released */
static int live, quit;             /* live: the game thread's */

static unsigned load(const unsigned *v) { return __atomic_load_n(v, __ATOMIC_ACQUIRE); }
static void store(unsigned *v, unsigned x) { __atomic_store_n(v, x, __ATOMIC_RELEASE); }

/* Everything the read callback can return is here: short, 0, or an error above want. */
static void refill(SndStream *s)
{
    unsigned long want = IN_SIZE - (s->inLen - s->inPos), got;

    memmove(s->in, s->in + s->inPos, s->inLen - s->inPos);
    s->inLen -= s->inPos;
    s->inPos = 0;
    if (want > s->remaining) want = s->remaining;
    got = want ? s->cb.read(s->handle, s->in + s->inLen, want) : 0;
    if (!got || got > want) { s->srcEnd = 1; return; }
    s->inLen += (int)got;
    s->remaining -= got;
}

/* Back to pos, or to the end of the source if the file refuses. */
static void reposition(SndStream *s, long pos, unsigned long left)
{
    s->srcEnd = s->cb.seek(s->handle, pos, 0) != pos;
    s->inPos = s->inLen = s->pcmPos = s->pcmLen = 0;
    s->remaining = left;
    s->skip = s->atEnd = 0;
    mp3dec_init(&s->mp3);
}

/* One mp3dec call, which may yield no sound (junk, a frame missing its reservoir): pcmLen 0 and more
   to come. Returns 0 at the end. One at a time, so megabytes of junk cannot keep Close waiting. */
static int decode_mp3(SndStream *s)
{
    mp3dec_frame_info_t info;
    int have = s->inLen - s->inPos, n;

    if (have < IN_SIZE / 2 && !s->srcEnd) { refill(s); have = s->inLen - s->inPos; }
    n = mp3dec_decode_frame(&s->mp3, s->in + s->inPos, have, s->pcm, &info);
    s->inPos += info.frame_bytes;
    if (n) {
        if (!s->rate) { s->rate = info.hz; s->channels = info.channels; s->kbps = info.bitrate_kbps; }
        /* the ring has the layout of the first frame */
        if (info.hz != s->rate || info.channels != s->channels) return 0;
        s->pcmLen = n;
    } else if (!info.frame_bytes) {
        if (s->srcEnd) return 0;
        if (have == IN_SIZE) return 0;   /* minimp3 took nothing from a full buffer */
        if (have >= IN_SIZE / 2) refill(s);
    }
    return 1;
}

/* A block or a run of frames from a WAV into pcm. Returns 0 at the end. As SndWav_Info counts them, a
   short last ADPCM block plays and a part frame of PCM is dropped. */
static int decode_wav(SndStream *s)
{
    const int need = s->block;
    const unsigned char *p;
    int have, i, n;

    while (s->inLen - s->inPos < need && !s->srcEnd) refill(s);
    have = s->inLen - s->inPos;
    if (have < (s->format == FORMAT_ADPCM ? 4 * s->channels : need)) return 0;
    p = s->in + s->inPos;
    if (s->format == FORMAT_ADPCM) {
        if (have > need) have = need;
        n = SndAdpcm_Decode(p, have, need, s->channels, s->pcm, PCM_SHORTS / s->channels);
        s->inPos += have;
    } else {
        n = have / need > PCM_CHUNK ? PCM_CHUNK : have / need;
        for (i = 0; i < n * s->channels; i++)
            s->pcm[i] = s->bits == 8 ? (short)((p[i] - 128) * 256) : (short)(p[i * 2] | p[i * 2 + 1] << 8);
        s->inPos += n * need;
    }
    s->pcmLen = n;
    return 1;
}

static int decode(SndStream *s) { return s->format == FORMAT_MP3 ? decode_mp3(s) : decode_wav(s); }

/* The source ran out: play again or end. A play with no sound ends it, or a looping stream of
   nothing would spin here forever. eof is set under lock so that SetLoop cannot slip past it. */
static void end_of_play(SndStream *s)
{
    int again;

    if (!s->atEnd) { s->atEnd = 1; s->plays++; }
    SDL_LockMutex(lock);
    again = s->playFrames && (!s->loops || s->plays < s->loops);
    if (!again) store(&s->eof, 1);
    SDL_UnlockMutex(lock);
    if (again) {
        reposition(s, s->dataStart, s->dataLen);
        s->playFrames = 0;
    }
}

/* One decode's worth into the ring. */
static void fill(SndStream *s)
{
    unsigned head = s->head, space = s->cap - (head - load(&s->tail)), at, n, first;
    int skip;

    if (s->pcmPos == s->pcmLen) {
        s->pcmPos = s->pcmLen = 0;
        if (!decode(s)) { end_of_play(s); return; }
        skip = s->skip < s->pcmLen ? (int)s->skip : s->pcmLen;
        s->pcmPos = skip;
        s->skip -= skip;
    }
    n = (unsigned)(s->pcmLen - s->pcmPos);
    if (n > space) n = space;
    if (!n) return;
    at = head & (s->cap - 1);
    first = s->cap - at < n ? s->cap - at : n;
    memcpy(s->ring + at * s->channels, s->pcm + s->pcmPos * s->channels, first * s->channels * sizeof(short));
    memcpy(s->ring, s->pcm + (s->pcmPos + first) * s->channels, (n - first) * s->channels * sizeof(short));
    s->pcmPos += n;
    s->playFrames = 1;
    store(&s->head, head + n);
}

/* shortcut: an MP3's seek is proportional to bytes and its length comes from the first frame, which
   is right for CBR and off for VBR; a Xing header would fix the length, if a game file ever needs it. */
static long long seek_to(SndStream *s, long ms)
{
    long long frame = (long long)ms * s->rate / 1000, pos;
    long skip = 0;

    if (frame > s->totalFrames) frame = s->totalFrames;
    if (s->format == FORMAT_MP3) {
        pos = s->totalFrames ? (long long)((double)frame / s->totalFrames * (s->fileLen - s->dataStart)) : 0;
    } else if (s->format == FORMAT_ADPCM) {
        long hdr = 4L * s->channels, perBlock = (s->block - hdr) / hdr * 8 + 1;
        pos = frame / perBlock * s->block;
        skip = (long)(frame % perBlock);
    } else {
        pos = frame * s->block;
    }
    reposition(s, s->dataStart + (long)pos, s->dataLen - (unsigned long)pos);
    s->skip = skip;
    s->playFrames = 1;   /* a seek to the end still loops */
    return frame;
}

static int wants_reader(const SndStream *s)
{
    return s->seekMs >= 0 || (!load(&s->eof) && s->head - load(&s->tail) < s->cap);
}

static int reader(void *unused)
{
    (void)unused;
    SDL_LockMutex(lock);
    while (!quit) {
        int i, worked = 0;
        for (i = 0; i < MAX_STREAMS; i++) {
            SndStream *s = streams[i];
            long ms;
            long long frame = 0;
            if (!s || !wants_reader(s)) continue;
            ms = s->seekMs;
            s->seekMs = -1;
            busy = s;
            SDL_UnlockMutex(lock);
            if (ms >= 0) frame = seek_to(s, ms); else fill(s);
            SDL_LockMutex(lock);
            if (ms >= 0) {
                s->baseFrame = frame;
                store(&s->drop, s->head);
                store(&s->dropped, 1);
                store(&s->eof, 0);     /* before seeking: Status trusts eof once seeking is down */
                if (s->seekMs < 0) store(&s->seeking, 0);
            }
            busy = NULL;
            SDL_CondBroadcast(idle);
            worked = 1;
        }
        if (!worked) SDL_CondWaitTimeout(wake, lock, IDLE_MS);
    }
    SDL_UnlockMutex(lock);
    return 0;
}

static void stop_reader(void)
{
    if (thread) {
        SDL_LockMutex(lock);
        quit = 1;
        SDL_CondSignal(wake);
        SDL_UnlockMutex(lock);
        SDL_WaitThread(thread, NULL);
    }
    SDL_DestroyCond(wake);
    SDL_DestroyCond(idle);
    SDL_DestroyMutex(lock);
    thread = NULL; wake = idle = NULL; lock = NULL;
}

/* It lives from the first stream to the last, so that nothing is left to shut down. */
static int start_reader(void)
{
    if (thread) return 1;
    quit = 0;
    lock = SDL_CreateMutex();
    wake = SDL_CreateCond();
    idle = SDL_CreateCond();
    if (lock && wake && idle) thread = SDL_CreateThread(reader, "snd stream", NULL);
    if (!thread) stop_reader();
    return thread != NULL;
}

static int probe_wav(SndStream *s)
{
    SndMixInfo info;

    if (!SndWav_Header(s->in, s->inLen, &info)) return 0;
    if (info.Info.rate > MAX_RATE || info.Info.block_size > IN_SIZE) return 0;
    s->format = (int)info.Info.format;
    s->rate = (int)info.Info.rate;
    s->channels = (int)info.Info.channels;
    s->bits = (int)info.Info.bits;
    s->block = (int)info.Info.block_size;
    s->totalFrames = (long long)info.Info.samples;
    s->dataStart = (long)((const unsigned char *)info.Info.data_ptr - s->in);
    s->dataLen = info.Info.data_len;
    return 1;
}

/* The file's end, past an ID3v1 tag: minimp3 trusts a sync only on ten frames in a row or the end of
   its input, so a tag behind a short sound (sound/ambient/silentsong.mp3) would lose all of it. Seeks
   from the end as Miles does (0x21110436); inside an iwd that end is reached by decompressing up to it. */
static long mp3_end(SndStream *s)
{
    char tag[128];
    long end = s->inLen;           /* a shorter file is all in the buffer */

    if (end < (long)sizeof tag) return end;
    end = s->cb.seek(s->handle, -(long)sizeof tag, 2);
    if (end < 0 || s->cb.read(s->handle, tag, sizeof tag) != sizeof tag) return 0;
    return memcmp(tag, "TAG", 3) ? end + (long)sizeof tag : end;
}

/* Finds the first frame with sound, for the format and the length. */
static int probe_mp3(SndStream *s)
{
    s->format = FORMAT_MP3;
    s->fileLen = mp3_end(s);
    if (s->fileLen <= 0) return 0;
    s->dataLen = (unsigned long)s->fileLen;   /* dataStart 0: an ID3v2 tag is junk to minimp3 */
    reposition(s, 0, s->dataLen);
    while (!s->pcmLen)
        if (!decode_mp3(s) || s->dataLen - s->remaining > MP3_SCAN) return 0;
    s->totalFrames = s->kbps ? (long long)s->fileLen * 8 * s->rate / (s->kbps * 1000) : 0;
    return 1;
}

static int probe(SndStream *s)
{
    s->remaining = (unsigned long)-1;
    mp3dec_init(&s->mp3);
    refill(s);
    if (s->inLen >= 12 && !memcmp(s->in, "RIFF", 4)) { if (!probe_wav(s)) return 0; }
    else if (!probe_mp3(s)) return 0;
    reposition(s, s->dataStart, s->dataLen);
    return !s->srcEnd;
}

SndStream *SndStream_Open(const SndFileCallbacks *cb, const char *name)
{
    SndStream *s;
    unsigned long handle;
    int slot = 0;

    while (slot < MAX_STREAMS && streams[slot]) slot++;
    if (slot == MAX_STREAMS) return NULL;
    if (!cb->open(name, &handle)) return NULL;   /* first: the game's open may Com_Error */
    s = calloc(1, sizeof *s);
    if (s) {
        s->cb = *cb;
        s->handle = handle;
    }
    if (!s || !probe(s)) goto fail;
    s->cap = 1;
    while ((int)s->cap < s->rate) s->cap *= 2;
    s->ring = malloc(s->cap * (size_t)s->channels * sizeof(short));
    s->loops = 1;
    s->seekMs = -1;
    if (!s->ring || !start_reader()) goto fail;
    SDL_LockMutex(lock);
    streams[slot] = s;
    live++;
    SDL_CondSignal(wake);
    SDL_UnlockMutex(lock);
    return s;

fail:
    if (s) free(s->ring);
    free(s);
    cb->close(handle);
    return NULL;
}

void SndStream_Close(SndStream *s)
{
    int i;

    if (!s) return;
    SDL_LockMutex(lock);
    for (i = 0; i < MAX_STREAMS; i++) if (streams[i] == s) streams[i] = NULL;
    while (busy == s) SDL_CondWait(idle, lock);
    SDL_UnlockMutex(lock);
    if (!--live) stop_reader();
    s->cb.close(s->handle);
    free(s->ring);
    free(s);
}

void SndStream_SetPaused(SndStream *s, int paused) { store(&s->paused, paused != 0); }

void SndStream_SetLoop(SndStream *s, int count)
{
    SDL_LockMutex(lock);
    s->loops = count;
    store(&s->eof, 0);      /* the reader looks at the end again */
    SDL_CondSignal(wake);
    SDL_UnlockMutex(lock);
}

void SndStream_SeekMs(SndStream *s, long ms)
{
    SDL_LockMutex(lock);
    s->seekMs = ms < 0 ? 0 : ms;
    store(&s->seeking, 1);
    SDL_CondSignal(wake);
    SDL_UnlockMutex(lock);
}

void SndStream_Info(SndStream *s, long *rate, long *channels, long *totalMs, long *curMs)
{
    long long frame;

    if (rate) *rate = s->rate;
    if (channels) *channels = s->channels;
    if (totalMs) {
        long long ms = s->totalFrames * 1000 / s->rate;
        *totalMs = ms > 0x7fffffffLL ? 0x7fffffffL : (long)ms;
    }
    if (!curMs) return;
    SDL_LockMutex(lock);
    if (s->seekMs >= 0) {
        frame = (long long)s->seekMs * s->rate / 1000;
    } else {
        frame = s->baseFrame + (load(&s->dropped) ? 0 : load(&s->tail) - load(&s->drop));
    }
    if (s->totalFrames && frame >= s->totalFrames) frame = s->loops == 1 ? s->totalFrames : frame % s->totalFrames;
    SDL_UnlockMutex(lock);
    *curMs = (long)(frame * 1000 / s->rate);
}

int SndStream_Read(SndStream *s, short *out, int frames)
{
    unsigned tail, n, at, first;

    if (frames <= 0 || load(&s->paused) || load(&s->seeking)) return 0;
    tail = s->tail;   /* only this thread writes it */
    if (__atomic_exchange_n(&s->dropped, 0, __ATOMIC_ACQ_REL)) tail = load(&s->drop);
    n = load(&s->head) - tail;
    if (n > (unsigned)frames) n = (unsigned)frames;
    at = tail & (s->cap - 1);
    first = s->cap - at < n ? s->cap - at : n;
    memcpy(out, s->ring + at * s->channels, first * s->channels * sizeof(short));
    memcpy(out + first * s->channels, s->ring, (n - first) * s->channels * sizeof(short));
    store(&s->tail, tail + n);
    return (int)n;
}

int SndStream_Status(SndStream *s)
{
    if (!load(&s->seeking) && load(&s->eof) && load(&s->head) == load(&s->tail)) return SND_SMP_DONE;
    return load(&s->paused) ? SND_SMP_STOPPED : SND_SMP_PLAYING;
}

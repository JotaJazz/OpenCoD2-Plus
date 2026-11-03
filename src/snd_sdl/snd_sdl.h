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
    unsigned long adpcm[10];       /* ADPCMDATA, unused here */
    unsigned short moresamples[16];
    unsigned long src_fract;
    long left_val, right_val;
} SndMixInfo;

/* Reads a WAV (PCM or IMA ADPCM) held in data[0..fileLen). Returns 1, or 0 if it is malformed. */
long SndWav_Info(const void *data, long fileLen, SndMixInfo *info);
/* The same over the first have bytes of a longer file; data_len is what the header says, capped at 1 GB. */
long SndWav_Header(const void *data, long have, SndMixInfo *info);
/* format: 0/1/2/3 = mono8/mono16/stereo8/stereo16 (PCM sources), 5/7 = ADPCM mono/stereo (copied as is).
   Both return 0 on anything unsupported or invalid. */
long SndWav_ConvertedSize(unsigned rate, unsigned format, const SndMixInfo *src);
long SndWav_Convert(void *dest, long size, unsigned rate, unsigned format, const SndMixInfo *src);
/* IMA ADPCM to interleaved PCM16; returns the frames written (at most maxFrames). */
int SndAdpcm_Decode(const unsigned char *src, int srcLen, int block, int channels, short *dst, int maxFrames);

/* pos is in listener space (X right, Y up, Z ahead; the listener never moves or turns). Gains for
   SndVoice left/right. distFactor is unused: in Miles it only scales Doppler. */
void SndSpatial_Levels(const float pos[3], float minDist, float maxDist,
                       float rolloff, float distFactor,
                       float volume, float *left, float *right);
/* AIL_set_sample_volume_pan: pan 0 = left, 1 = right. */
void SndSpatial_VolumePan(float volume, float pan, float *left, float *right);

/* The reverb bus, ours rather than Miles' (ADR 0012). room: an index of snd_roomStrings. SetRoom also
   sets the wet level to the room's own, as Miles' does; SetLevels overrides it. Their caller holds the
   device lock. Process takes the mono send, adds the tail to out at the device's rate and channels, and
   scales out by dry; it neither allocates nor waits. A rate of 0 forgets the tail. */
void SndReverb_SetRoom(int room);
void SndReverb_SetLevels(float dry, float wet);
void SndReverb_Process(const float *wetIn, short *out, int frames, int outChannels, int outRate);

/* Streams: one reader thread serves them all; it reads and seeks through the callbacks, and each
   stream keeps about a second of PCM, so a slow disk is heard as silence, never as a wait.
   seek type: 0 begin, 1 current, 2 end; it returns the new position. read returns the bytes read:
   0 at the end, and anything above the bytes asked for is an error (FS_Read's -1 reaches here so).
   The file handle is the reader's until Close: Open and Close call open and close on the game thread.

   Game thread: Open, Close, SetPaused, SetLoop, SeekMs, Info. Audio callback: Read, Status, which
   never block and never touch the file. Task 5: hold SDL_LockAudioDevice only to detach the stream
   from its voice, and call Open and Close with it released: Open reads the file and the game's open
   may Com_Error, Close waits for the reader, and the callback must be out of Read before Close. */
typedef struct SndFileCallbacks {
    unsigned long (*open)(const char *name, unsigned long *handle);   /* 0 on failure */
    void (*close)(unsigned long handle);
    long (*seek)(unsigned long handle, long offset, unsigned long type);
    unsigned long (*read)(unsigned long handle, void *buf, unsigned long bytes);
} SndFileCallbacks;
typedef struct SndStream SndStream;
/* A WAV (PCM or IMA ADPCM) or MP3, starting unpaused with one play. NULL if it cannot be opened or
   read, and past the 13 streams the game has file handles for. */
SndStream *SndStream_Open(const SndFileCallbacks *cb, const char *name);
void SndStream_Close(SndStream *s);            /* waits for the reader */
void SndStream_SetPaused(SndStream *s, int paused);
void SndStream_SetLoop(SndStream *s, int count);   /* 0 = forever, as Miles */
void SndStream_SeekMs(SndStream *s, long ms);
/* Any pointer may be NULL. An MP3's lengths are estimates from its first frame's bitrate. */
void SndStream_Info(SndStream *s, long *rate, long *channels, long *totalMs, long *curMs);
/* Interleaved PCM16 at the stream's rate and channels; returns the frames written, 0 if none are ready. */
int  SndStream_Read(SndStream *s, short *out, int frames);
/* SND_SMP_DONE once played out, SND_SMP_STOPPED while paused, else SND_SMP_PLAYING. */
int  SndStream_Status(SndStream *s);

#endif

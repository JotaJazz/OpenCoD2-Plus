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
    return failed;
}

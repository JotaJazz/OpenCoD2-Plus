#include <math.h>
#include <stdio.h>
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

    for (i = 0; i < 10; i++) pcm[i] = 1000;
    v = voice(pcm, 20, 16, 1, 44100);
    v.loopCount = 0;
    SndMix_Render(&v, 1, 44100, 1, out, NULL, 100);
    n = v.status == SND_SMP_PLAYING;
    w = voice(pcm, 20, 16, 1, 44100);
    w.loopCount = 2;
    SndMix_Render(&w, 1, 44100, 1, out, NULL, 19);
    n = n && w.status == SND_SMP_PLAYING;
    SndMix_Render(&w, 1, 44100, 1, out, NULL, 1);
    CHECK("mix_loop_wraps", n && w.status == SND_SMP_DONE);

    for (i = 0; i < 53; i++) many[i] = voice(big, 2, 16, 1, 44100);
    SndMix_Render(many, 53, 44100, 2, out, NULL, 1);
    CHECK("mix_saturates", out[0] == 32767 && out[1] == 32767);

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
    return failed;
}

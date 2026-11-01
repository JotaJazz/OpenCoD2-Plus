#include <string.h>
#include "snd_sdl.h"

static unsigned rd16(const unsigned char *p) { return p[0] | p[1] << 8; }
static unsigned long rd32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (unsigned long)p[3] << 24; }

/* Frames in the first len bytes of an ADPCM stream of the given block size (a short last block counts). */
static long adpcm_frames(long len, long block, long ch)
{
    long n = len / block * ((block - 4 * ch) / (4 * ch) * 8 + 1), rest = len % block;
    if (rest >= 4 * ch) n += (rest - 4 * ch) / (4 * ch) * 8 + 1;
    return n;
}

long SndWav_Info(const void *data, long fileLen, SndMixInfo *info)
{
    const unsigned char *p = data;
    long pos = 12, tag = 0, ch = 0, bits = 0, block = 0, haveFmt = 0;
    unsigned long rate = 0;

    memset(info, 0, sizeof *info);
    if (!p || fileLen < 12 || memcmp(p, "RIFF", 4) || memcmp(p + 8, "WAVE", 4)) return 0;
    while (fileLen - pos >= 8) {
        unsigned long size = rd32(p + pos + 4);
        long body = pos + 8, left = fileLen - body;
        if (!memcmp(p + pos, "fmt ", 4)) {
            if (size < 16 || (long long)size > left) return 0;
            tag = rd16(p + body); ch = rd16(p + body + 2); rate = rd32(p + body + 4);
            block = rd16(p + body + 12); bits = rd16(p + body + 14);
            haveFmt = 1;
        } else if (!memcmp(p + pos, "data", 4)) {
            long frames;
            if (!haveFmt || (long long)size > left) return 0;
            if (ch < 1 || ch > 2 || !rate || rate > 1000000 || !block) return 0;
            if (tag == 1 && (bits == 8 || bits == 16) && block == ch * bits / 8) frames = size / block;
            else if (tag == 0x11 && bits == 4 && block >= 4 * ch && (block - 4 * ch) % (4 * ch) == 0)
                frames = adpcm_frames(size, block, ch);
            else return 0;
            info->Info.format = tag; info->Info.data_ptr = info->Info.initial_ptr = p + body;
            info->Info.data_len = size; info->Info.rate = rate; info->Info.bits = bits;
            info->Info.channels = ch; info->Info.samples = frames; info->Info.block_size = block;
            return 1;
        }
        if ((long long)size > left) return 0;
        pos = body + (long)size + (size & 1);   /* chunks are word aligned */
    }
    return 0;
}

/* PCM sources only: ADPCM is decoded by the caller first. Returns frames out, or 0. */
static long out_frames(unsigned rate, unsigned format, const SndMixInfo *s, long *outBytes)
{
    const SndSoundInfo *i = &s->Info;
    long ob, frames;
    if (format > 3 || i->format != 1 || !i->data_ptr || !rate || !i->rate) return 0;
    if ((i->bits != 8 && i->bits != 16) || (i->channels != 1 && i->channels != 2)) return 0;
    frames = (long)((unsigned long long)(i->data_len / (i->bits / 8 * i->channels)) * rate / i->rate);
    ob = (format & 1 ? 2 : 1) * (format & 2 ? 2 : 1);
    if (frames > 0x7fffffffL / ob) return 0;
    *outBytes = ob;
    return frames;
}

long SndWav_ConvertedSize(unsigned rate, unsigned format, const SndMixInfo *src)
{
    long ob, f;
    if (format == 5 || format == 7) return src->Info.format == 0x11 ? (long)src->Info.data_len : 0;
    f = out_frames(rate, format, src, &ob);
    return f * ob;
}

static int sample(const unsigned char *p, long bits)   /* as signed 16 */
{
    return bits == 8 ? (*p - 128) * 256 : (short)rd16(p);
}

long SndWav_Convert(void *dest, long size, unsigned rate, unsigned format, const SndMixInfo *src)
{
    const SndSoundInfo *i = &src->Info;
    const unsigned char *in = i->data_ptr;
    unsigned char *out = dest;
    long n = SndWav_ConvertedSize(rate, format, src), ob, f, k;
    int sbytes, oc = format & 2 ? 2 : 1;

    if (!n || !dest || size < n) return 0;
    if (format == 5 || format == 7) { memcpy(dest, in, n); return n; }
    f = out_frames(rate, format, src, &ob);
    sbytes = i->bits / 8;
    for (k = 0; k < f; k++) {
        /* nearest source frame; < srcFrames because k < f = srcFrames * rate / srcRate */
        const unsigned char *fr = in + (long)((unsigned long long)k * i->rate / rate) * sbytes * i->channels;
        int l = sample(fr, i->bits), r = i->channels == 2 ? sample(fr + sbytes, i->bits) : l, c;
        for (c = 0; c < oc; c++) {
            int v = oc == 1 ? (l + r) / 2 : c ? r : l;
            if (format & 1) { out[0] = v & 255; out[1] = (v >> 8) & 255; out += 2; }
            else *out++ = (unsigned char)((v >> 8) + 128);
        }
    }
    return n;
}

static const signed char idxAdj[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };
static const short steps[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767 };

int SndAdpcm_Decode(const unsigned char *src, int srcLen, int block, int channels, short *dst, int maxFrames)
{
    long done = 0, off, hdr = 4L * channels;
    if (!src || !dst || channels < 1 || channels > 2 || block < hdr || (block - hdr) % hdr || maxFrames <= 0) return 0;
    for (off = 0; off < srcLen && done < maxFrames; off += block) {
        long len = srcLen - off < block ? srcLen - off : block, g, groups, c, k;
        int pred[2], idx[2];
        if (len < hdr) break;
        groups = (len - hdr) / hdr;
        for (c = 0; c < channels; c++) {
            const unsigned char *h = src + off + c * 4;
            pred[c] = (short)rd16(h); idx[c] = h[2] > 88 ? 88 : h[2];
            if (done < maxFrames) dst[done * channels + c] = (short)pred[c];
        }
        for (g = 0; g < groups; g++)
            for (c = 0; c < channels; c++) {
                const unsigned char *b = src + off + hdr + (g * channels + c) * 4;
                for (k = 0; k < 8; k++) {
                    int nib = b[k / 2] >> (k & 1 ? 4 : 0) & 15, st = steps[idx[c]], d = st >> 3;
                    long fr = done + 1 + g * 8 + k;
                    if (nib & 4) d += st;
                    if (nib & 2) d += st >> 1;
                    if (nib & 1) d += st >> 2;
                    pred[c] += nib & 8 ? -d : d;
                    if (pred[c] > 32767) pred[c] = 32767;
                    if (pred[c] < -32768) pred[c] = -32768;
                    idx[c] += idxAdj[nib & 7];
                    if (idx[c] < 0) idx[c] = 0;
                    if (idx[c] > 88) idx[c] = 88;
                    if (fr < maxFrames) dst[fr * channels + c] = (short)pred[c];
                }
            }
        done += 1 + groups * 8;
    }
    return (int)(done < maxFrames ? done : maxFrames);
}

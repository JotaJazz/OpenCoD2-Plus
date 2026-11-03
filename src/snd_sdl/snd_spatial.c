#include <math.h>
#include "snd_sdl.h"

/* Volume curve, volume^(5/3): msssoft.m3d 0x22401094, mss32.dll 0x2112a934 */
static double Curve(float volume)
{
    return volume > 0 ? pow(volume, 5.0 / 3.0) : 0;
}

/* The "Miles Fast 2D Positional Audio" update, msssoft.m3d 0x22401000 (provider name at 0x224014ba).
   Listener at the origin, face +Z, up +Y, so its right is +X: the game opens no listener and
   msssoft sets that default (0x22401f4f, right vector at 0x224019f8). */
void SndSpatial_Levels(const float pos[3], float minDist, float maxDist, float rolloff, float distFactor,
                       float volume, float *left, float *right)
{
    double x = pos[0], y = pos[1], z = pos[2];
    double dist = sqrt(x * x + y * y + z * z);
    double gain = Curve(volume), pan = 0.5;

    (void)distFactor;   /* only the Doppler branch reads it (0x224012a7), and the game sets no velocity */
    if (minDist > maxDist) {   /* mss32.dll 0x2111f921, AIL_set_3D_sample_distances */
        float t = minDist;
        minDist = maxDist;
        maxDist = t;
    }
    /* "Mute at maximum" defaults to 1 (0x2241322c) and the game keeps it. Written so NaN mutes too. */
    if (!(dist <= maxDist)) {   /* 0x22401058 */
        *left = *right = 0;
        return;
    }
    if (minDist < 1e-4f)   /* 0x224010a6 */
        minDist = 1e-4f;
    if (dist > minDist + 1e-4f)   /* 0x224010be */
        gain *= minDist / (minDist + rolloff * (dist - minDist));
    if (dist > 1e-4f)   /* 0x22401141: closer stays centered */
        pan = acos(x / dist) * 0.31830987f;   /* 0x2240115a; pan runs from 0 at +X to 1 at -X */
    if (z < 0)   /* 0x22401187: forward dot negative; a zero pos normalizes to +X (0x224029ad) */
        gain *= 0.75f;
    *left = (float)(pan * gain);   /* 0x224011a2: AIL_set_sample_volume_levels(S, pan*g, (1-pan)*g) */
    *right = (float)((1 - pan) * gain);
}

/* mss32.dll 0x2112a930 */
void SndSpatial_VolumePan(float volume, float pan, float *left, float *right)
{
    double gain = Curve(volume);

    pan = pan < 0 ? 0 : pan > 1 ? 1 : pan;   /* pow of a negative is NaN */
    *left = (float)(gain * pow(1 - pan, 0.3f));
    *right = (float)(gain * pow(pan, 0.3f));
}

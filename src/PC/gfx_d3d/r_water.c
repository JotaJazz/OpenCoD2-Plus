#include "common_types.h"
#include <math.h>
extern dvar_t *r_rendererInUse;
#include "imports.h"
#include "bytematch.h"
extern GfxBackEndData *backEndData;


static struct WaterGlob waterGlob;

extern r_globals_t rg;
extern r_backEndGlobals_t backEnd;
extern void **g_unknown_195f22c;
extern void **g_unknown_195f230;

extern void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src);
extern void R_DownsampleMipMapBilinear(const byte *src, int srcBufferSize, int srcWidth, int srcHeight, int texelPitch, byte *dst, int dstBufferSize);
extern void FFT(complex_t *data, long unsigned int log2_count, long unsigned int stride, int *bitSwap, complex_t *trigTable);
extern float MacOpenGLUtils_SquareRootLowPrecision(float fp0);

void RB_UploadWaterTexture(GfxImage *image, water_t *water);

void RB_UploadWaterTexture(GfxImage *image, water_t *water)
{
    int currentFrame;
    float t;
    int N, M;
    int n, m, vecKIndex;
    complex_t *waterData;
    complex_t *H0;
    float *wTerm;
    int log2_n, log2_m;
    int totalSize;
    float dz_float;
    float dz_sqrd;
    int xymask, ymask;
    int ixm1y;
    byte *pixels;
    byte *uploadFlag;

    waterData = (complex_t *)&waterGlob;

    currentFrame = backEndData->frameCount;
    if (water->writable.frameCount == currentFrame) {
        return;
    }

    water->writable.frameCount = currentFrame;

    if ((*(dvar_t **)(g_unknown_195f230))->current.enabled == 0) {
        return;
    }

    t = backEnd.sceneDef.floatTime;

    N = water->N;
    M = water->M;

    vecKIndex = 0;
    for (n = -N / 2; n < N / 2; n++) {
        for (m = -M / 2; m < M / 2; m++) {
            float w = water->wTerm[vecKIndex];

            int phase;
            int cosPhase;
            float phaseFloat;

            phaseFloat = (float)((double)(w * t) * 162.97466172610083);
            phase = (int)floorf(phaseFloat + 0.5f) & 0x3ff;

            cosPhase = (phase + 255) & 0x3ff;
            waterData[vecKIndex].real = water->H0[vecKIndex].real * rg.sinTable[cosPhase];

            waterData[vecKIndex].imag = water->H0[vecKIndex].imag * rg.sinTable[phase];

            vecKIndex++;
            M = water->M;
        }
    }

    if (N == 1) {
        log2_n = 0;
    } else {
        log2_n = 0;
        do {
            log2_n++;
        } while ((1 << log2_n) != N);
    }

    M = water->M;
    if (M == 1) {
        log2_m = 0;
    } else {
        log2_m = 0;
        do {
            log2_m++;
        } while ((1 << log2_m) != M);
    }

    if (N > 0) {
        int waterIndex;
        for (waterIndex = 0; waterIndex < water->N; waterIndex++) {
            FFT(&waterData[waterIndex * water->M], log2_m, 1,
                rg.fftBitswap, rg.fftTrigTable);
        }
        M = water->M;
    }

    if (M > 0) {
        int fftIndex;
        complex_t *ptr = waterData;
        for (fftIndex = 0; fftIndex < water->M; fftIndex++) {
            FFT(ptr, log2_n, water->M,
                rg.fftBitswap, rg.fftTrigTable);
            ptr++;
        }
    }

    {
        int *capsPtr = (int *)r_rendererInUse;
        if (capsPtr[2] == 2) {
            goto normalmap_path;
        }
    }

    {
        float dz;
        totalSize = water->M * water->N;
        dz = (float)totalSize;

        if (totalSize > 0) {
            int i;
            byte *dst = waterGlob.pixels;
            complex_t *src = waterData;

            for (i = 0; i < water->N * water->M; i++) {
                float real = src->real;
                float imag = src->imag;
                float mag = real * real + imag * imag;
                float sqrtMag;
                float val;

                sqrtMag = __builtin_sqrtf(mag);
                val = sqrtMag / dz;

                if (val - 1.0f >= 0.0f) {
                    val = 255.0f;
                } else {
                    val = val * 255.0f;
                }

                *dst = (byte)(int)floorf(val + 0.5f);
                dst++;
                src++;
            }
        }

        goto upload_heightmap;
    }

normalmap_path:

    {
        int totalPixels;
        float dz_val;
        float dz_sqrd_val;

        totalPixels = water->M * water->N;
        dz_val = (float)totalPixels;
        dz_sqrd_val = dz_val * dz_val;

        if (totalPixels > 0) {
            int i;
            complex_t *ptr = waterData;
            for (i = 0; i < totalPixels; i++) {
                float real = ptr->real;
                float imag = ptr->imag;
                float mag = real * real + imag * imag;
                ptr->real = MacOpenGLUtils_SquareRootLowPrecision(mag);
                ptr++;
            }
        }
        M = water->M;

        xymask = totalPixels - 1;
        ymask = M - 1;

        if (water->N > 0) {
            int y;
            int ixy_base;
            int ixm1y_base;

            pixels = waterGlob.pixels;
            ixm1y = 0;
            ixy_base = totalPixels - M;

            for (y = 0; y < water->N; y++) {
                M = water->M;

                if (M > 0) {
                    int x;
                    int ixm1y_idx = ixm1y;
                    int ixy_idx = ixy_base;
                    int prevYm1 = ymask;

                    for (x = 0; x < water->M; x++) {

                        float h_ixm1y = waterData[ixm1y_idx].real;

                        float dx = h_ixm1y - waterData[prevYm1].real;

                        float dy = h_ixm1y - waterData[ixy_idx].real;

                        float lenSq = dx * dx + dy * dy + dz_sqrd_val;
                        float len = MacOpenGLUtils_SquareRootLowPrecision(lenSq);
                        float invLen = 127.5f / len;

                        pixels[0] = 0xff;
                        pixels[1] = (byte)(int)floorf(dx * invLen + 127.5f + 0.5f);
                        pixels[2] = (byte)(int)floorf(dy * invLen + 127.5f + 0.5f);
                        pixels[3] = (byte)(int)floorf(dz_val * invLen + 127.5f + 0.5f);

                        pixels += 4;

                        prevYm1 = ixm1y_idx;
                        ixm1y++;
                        ixm1y_idx = ixm1y;
                        ixy_idx += 1;
                        M = water->M;
                    }
                }

                ixy_base = (totalPixels - M + ixm1y) & xymask;
            }
        }

        uploadFlag = *(byte **)g_unknown_195f22c;
        *uploadFlag = 1;
        Image_UploadData(image, D3DFMT_A8R8G8B8, 0, 0, waterGlob.pixels);
        *uploadFlag = 0;
        return;
    }

upload_heightmap:

    {
        int srcWidth, srcHeight, mipIndex;
        byte *uploadFlagPtr;

        uploadFlagPtr = *(byte **)g_unknown_195f22c;
        *uploadFlagPtr = 1;

        Image_UploadData(image, D3DFMT_L8, 0, 0, waterGlob.pixels);

        srcWidth = water->M;
        srcHeight = water->N;
        mipIndex = 1;

        while (srcWidth > 1 || srcHeight > 1) {

            R_DownsampleMipMapBilinear(waterGlob.pixels, 0x4000, srcWidth, srcHeight, 1,
                                       waterGlob.pixels, 0x4000);

            Image_UploadData(image, D3DFMT_L8, 0, mipIndex, waterGlob.pixels);

            srcWidth >>= 1;
            if (srcWidth < 1)
                srcWidth = 1;
            srcHeight >>= 1;
            if (srcHeight < 1)
                srcHeight = 1;

            mipIndex++;
        }

        *uploadFlagPtr = 0;
    }
}

#include "common_types.h"
#include <math.h>
#include "imports.h"

UINT32 g_TotalFilterPasses = 0x0;

extern float expf(float);

extern bool g_LastGlowFilter;
extern UINT32 g_TotalFilterPasses;
extern bool g_ShowShadowCookies;
extern UINT32 g_NumBlurShadowPasses;

extern unsigned char rgp[];
extern unsigned char dx[];
extern unsigned char vidConfig[];
extern unsigned char backEnd[];

extern void RB_SetRenderTarget(GfxRenderTargetId newTargetId);
extern void RB_DrawStretchPic(const Material *material, float x, float y, float w, float h,
                              float s0, float t0, float s1, float t1,
                              unsigned int color, GfxPrimStatsTarget splitScreen);
extern void RB_EndSurface(void);

static int RB_GaussianFilterPoints1D(float pixels, int srcRes, int dstRes, int tapLimit, float *tapOffsets, float *tapWeights);
static int RB_GenerateGaussianFilter2D(float radius, int srcWidth, int srcHeight, int dstWidth, int dstHeight, GfxImageFilterPass *filterPass);
static int RB_GenerateGaussianFilterChain(float radiusX, float radiusY, int srcWidth, int srcHeight, int dstWidth, int dstHeight, int passLimit, GfxImageFilterPass *filterPass);
static GfxRenderTargetId RB_ApplyGlowFilter(GfxRenderTargetId srcRenderTarget, GfxRenderTargetId dstRenderTarget, float glowRadius);
void RB_GlowFilterImage(float *radius);
void RB_GaussianFilterImage(float radius, GfxRenderTargetId renderTargetId);

#define DX_RT_IMAGE(id) (((DxGlobals *)dx)->renderTargets[id].image)
#define DX_RT_WIDTH(id) (((DxGlobals *)dx)->renderTargets[id].width)
#define DX_RT_HEIGHT(id) (((DxGlobals *)dx)->renderTargets[id].height)

#define RGP_SYM_FILTER_MAT(i) (((r_global_permanent_t *)rgp)->symmetricFilterMaterial[i])
#define RGP_GLOW_SETUP_MAT() (((r_global_permanent_t *)rgp)->glowSetupMaterial)

static float nextPowerOfTwoF(unsigned int val)
{
    unsigned int d;
    int i;

    if (val <= 1)
        return 1.0f;

    d = 1;
    for (i = 32; i > 0; i--) {
        d += d;
        if (val <= d)
            return (float)d;
    }
    return (float)d;
}

static float absf(float x)
{
    return x < 0.0f ? -x : x;
}

#define GLOW_MAX_RADIUS 1.3895604610443115f
#define GLOW_MAX_RADIUS_SQ 1.9308782815933228f
#define FILTER_MIN_RADIUS 0.3295051157474518f
#define FILTER_MAX_SINGLE 6.497750282287598f
#define FILTER_MAX_SQ 42.220760345458984f

static int RB_GaussianFilterPoints1D(float pixels, int srcRes, int dstRes, int tapLimit, float *tapOffsets, float *tapWeights)
{
    float srcResF;
    float ratio;
    int ratioRounded;
    float offset;
    float gaussianExponent;
    float totalWeight;
    int tapIndex;
    float weight0, weight1;
    float sample0, sample1;
    float combinedWeight;
    int tapCount;

    srcResF = (float)srcRes;
    ratio = srcResF / (float)dstRes;
    ratioRounded = (int)floorf(ratio + 0.5f);

    if (ratioRounded & 1) {
        offset = 0.0f;
    } else {
        offset = 0.5f;
    }

    gaussianExponent = -0.5f / (pixels * pixels);

    if (tapLimit <= 0) {

        tapWeights[0] = 0.5f;
        return 1;
    }

    totalWeight = 0.0f;
    for (tapIndex = 0; tapIndex < tapLimit; tapIndex++) {

        sample0 = (float)(tapIndex * 2) + offset;
        sample1 = (float)(tapIndex * 2 + 1) + offset;

        weight0 = expf(gaussianExponent * sample0 * sample0);
        weight1 = expf(gaussianExponent * sample1 * sample1);

        if (tapIndex == 0 && offset == 0.0f) {
            weight0 *= 0.5f;
        }

        combinedWeight = weight1 + weight0;
        tapWeights[tapIndex] = combinedWeight;

        if (combinedWeight != 0.0f) {

            tapOffsets[tapIndex] = (weight0 * sample0 + weight1 * sample1) / (combinedWeight * srcResF);
        } else {

            tapOffsets[tapIndex] = (sample0 + sample1) * 0.5f / srcResF;
        }

        totalWeight += tapWeights[tapIndex];
    }

    if (totalWeight <= 0.001f) {
        tapWeights[0] = 0.5f;
        return 1;
    }

    {
        float normFactor = 0.5f / totalWeight;
        tapCount = tapLimit;

        for (tapIndex = tapLimit - 1; tapIndex >= 0; tapIndex--) {
            tapWeights[tapIndex] *= normFactor;
            if (0.01f > tapWeights[tapIndex]) {
                tapCount = tapIndex + 1;
            }
        }
    }

    return tapCount;
}

static int RB_GenerateGaussianFilter2D(float radius, int srcWidth, int srcHeight, int dstWidth, int dstHeight, GfxImageFilterPass *filterPass)
{
    float tapOffsetsX[4];
    float tapOffsetsY[4];
    float tapWeightsX[4];
    float tapWeightsY[4];
    int iy, ix;
    int tapEntryIdx;

    RB_GaussianFilterPoints1D(radius, srcWidth, dstWidth, 2, tapOffsetsX, tapWeightsX);
    RB_GaussianFilterPoints1D(radius, srcHeight, dstHeight, 2, tapOffsetsY, tapWeightsY);

    tapEntryIdx = 0;
    for (iy = 0; iy < 2; iy++) {
        for (ix = 0; ix < 2; ix++) {

            filterPass->tapOffsetsAndWeights[tapEntryIdx][0] = -tapOffsetsX[ix];
            filterPass->tapOffsetsAndWeights[tapEntryIdx][1] = tapOffsetsY[iy];
            filterPass->tapOffsetsAndWeights[tapEntryIdx][2] = 0.0f;
            filterPass->tapOffsetsAndWeights[tapEntryIdx][3] = tapWeightsX[ix] * tapWeightsY[iy];
            tapEntryIdx++;

            filterPass->tapOffsetsAndWeights[tapEntryIdx][0] = tapOffsetsX[ix];
            filterPass->tapOffsetsAndWeights[tapEntryIdx][1] = tapOffsetsY[iy];
            filterPass->tapOffsetsAndWeights[tapEntryIdx][2] = 0.0f;
            filterPass->tapOffsetsAndWeights[tapEntryIdx][3] = tapWeightsX[ix] * tapWeightsY[iy];
            tapEntryIdx++;
        }
    }

    filterPass->material = RGP_SYM_FILTER_MAT(7);
    filterPass->tapHalfCount = 8;

    filterPass->srcWidth = (float)srcWidth / nextPowerOfTwoF((unsigned int)srcWidth);
    filterPass->srcHeight = (float)srcHeight / nextPowerOfTwoF((unsigned int)srcHeight);

    filterPass->dstWidth = dstWidth;
    filterPass->dstHeight = dstHeight;

    return 0;
}

static int RB_GenerateGaussianFilterChain(float radiusX, float radiusY, int srcWidth, int srcHeight, int dstWidth, int dstHeight, int passLimit, GfxImageFilterPass *filterPass)
{
    float tapOffsets[8];
    float tapWeights[8];
    int passCount;
    GfxImageFilterPass *curPass;
    float minRadius;
    float maxRadiusSq;
    int halfTapCount;
    int passAxis;
    int i;
    int curDstWidth;
    int curDstHeight;

    curDstWidth = dstWidth;
    curDstHeight = dstHeight;

    if (dstWidth == srcWidth && dstHeight == srcHeight) {
        passCount = 0;
    } else {

        minRadius = radiusX < radiusY ? radiusX : radiusY;

        if (minRadius > GLOW_MAX_RADIUS) {
            minRadius = GLOW_MAX_RADIUS;
            maxRadiusSq = GLOW_MAX_RADIUS_SQ;
        } else {
            maxRadiusSq = minRadius * minRadius;
        }

        radiusX = sqrtf(radiusX * radiusX - maxRadiusSq) * (float)dstWidth / (float)srcWidth;
        radiusY = sqrtf(radiusY * radiusY - maxRadiusSq) * (float)dstHeight / (float)srcHeight;

        RB_GenerateGaussianFilter2D(minRadius, srcWidth, srcHeight, dstWidth, dstHeight, filterPass);
        passCount = 1;
    }

    curPass = &filterPass[passCount];

    while (passCount < 16) {
        float filterRadius;
        int dstRes;

        if (radiusX < FILTER_MIN_RADIUS && radiusY < FILTER_MIN_RADIUS) {
            break;
        }

        if (absf(radiusX - radiusY) < FILTER_MIN_RADIUS) {
            float avgRadius = (radiusX + radiusY) * 0.5f;

            if (avgRadius <= GLOW_MAX_RADIUS) {

                RB_GenerateGaussianFilter2D(avgRadius, curDstWidth, curDstHeight, curDstWidth, curDstHeight, curPass);
                passCount++;
                break;
            }
        }

        if (radiusX > radiusY) {

            if (FILTER_MAX_SINGLE >= radiusX) {
                filterRadius = radiusX;
                radiusX = 0.0f;
            } else {
                radiusX = sqrtf(radiusX * radiusX - FILTER_MAX_SQ);
                filterRadius = FILTER_MAX_SINGLE;
            }
            passAxis = 0;
            dstRes = curDstWidth;
        } else {

            if (FILTER_MAX_SINGLE >= radiusY) {
                filterRadius = radiusY;
                radiusY = 0.0f;
            } else {
                radiusY = sqrtf(radiusY * radiusY - FILTER_MAX_SQ);
                filterRadius = FILTER_MAX_SINGLE;
            }
            passAxis = 1;
            dstRes = curDstHeight;
        }

        halfTapCount = RB_GaussianFilterPoints1D(filterRadius, dstRes, dstRes, 8, tapOffsets, tapWeights);

        curPass->material = RGP_SYM_FILTER_MAT(halfTapCount - 1);
        curPass->tapHalfCount = halfTapCount;

        for (i = 0; i < 8; i++) {
            if (passAxis == 0) {
                curPass->tapOffsetsAndWeights[i][0] = tapOffsets[i];
                curPass->tapOffsetsAndWeights[i][1] = 0.0f;
            } else {
                curPass->tapOffsetsAndWeights[i][0] = 0.0f;
                curPass->tapOffsetsAndWeights[i][1] = tapOffsets[i];
            }
            curPass->tapOffsetsAndWeights[i][2] = 0.0f;
            curPass->tapOffsetsAndWeights[i][3] = tapWeights[i];
        }

        curPass->srcWidth = (float)curDstWidth / nextPowerOfTwoF((unsigned int)curDstWidth);
        curPass->srcHeight = (float)curDstHeight / nextPowerOfTwoF((unsigned int)curDstHeight);
        curPass->dstWidth = curDstWidth;
        curPass->dstHeight = curDstHeight;

        passCount++;
        curPass++;
    }

    return passCount;
}

static void RB_ApplyFilterPasses(GfxImageFilter *filter, int swapLastPass)
{
    int passIndex;
    int passCount = filter->passCount;

    for (passIndex = 0; passIndex < passCount; passIndex++) {
        GfxImageFilterPass *pass = &filter->passes[passIndex];
        int pingPong = passIndex & 1;
        GfxRenderTargetId setTarget;
        int constIndex;
        float dstW, dstH;
        r_backEndGlobals_t *be = (r_backEndGlobals_t *)backEnd;

        if (passIndex == 0) {

            be->currentFeedbackImage = filter->sourceImage;
        } else {

            int srcPP = 1 - pingPong;
            GfxRenderTargetId ppTarget = filter->pingPongTargets[srcPP];
            be->currentFeedbackImage = DX_RT_IMAGE(ppTarget);
        }

        if (passIndex == passCount - 1) {

            setTarget = filter->finalTarget;
        } else {

            setTarget = filter->pingPongTargets[pingPong];
        }

        RB_SetRenderTarget(setTarget);

        if (pass->tapHalfCount > 0) {
            for (constIndex = 0; constIndex < pass->tapHalfCount; constIndex++) {
                be->codeConsts[35 + constIndex][0] = pass->tapOffsetsAndWeights[constIndex][0];
                be->codeConsts[35 + constIndex][1] = pass->tapOffsetsAndWeights[constIndex][1];
                be->codeConsts[35 + constIndex][2] = pass->tapOffsetsAndWeights[constIndex][2];
                be->codeConsts[35 + constIndex][3] = pass->tapOffsetsAndWeights[constIndex][3];
            }
        }

        dstW = (float)pass->dstWidth;
        dstH = (float)pass->dstHeight;

        if (swapLastPass && passIndex == passCount - 1) {

            RB_DrawStretchPic(pass->material, 0.0f, 0.0f, dstW, dstH,
                              0.0f, pass->srcHeight, pass->srcWidth, 0.0f,
                              0xFFFFFFFF, (GfxPrimStatsTarget)0xa);
        } else {

            RB_DrawStretchPic(pass->material, 0.0f, 0.0f, dstW, dstH,
                              0.0f, 0.0f, pass->srcWidth, pass->srcHeight,
                              0xFFFFFFFF, (GfxPrimStatsTarget)0xa);
        }

        RB_EndSurface();
    }
}

static GfxRenderTargetId RB_ApplyGlowFilter(GfxRenderTargetId srcRenderTarget, GfxRenderTargetId dstRenderTarget, float glowRadius)
{
    GfxImageFilter filter;
    int passCount;
    int chainPassCount;
    int startPass;
    int dstWidth, dstHeight;
    float scaledRadius;

    if (glowRadius == 0.0f) {
        return dstRenderTarget;
    }

    {
        const r_backEndGlobals_t *backend = (const r_backEndGlobals_t *)backEnd;
        const GfxViewParms *viewParms = backend->viewParms;
        int viewportHeight = (int)viewParms->viewport.Height;
        float heightScaled = (float)viewportHeight * glowRadius / 480.0f;

        const vidConfig_t *vcfg = (const vidConfig_t *)vidConfig;
        scaledRadius = heightScaled * vcfg->aspectRatioPixel;
    }

    filter.pingPongTargets[0] = R_RENDERTARGET_PINGPONG_0;
    filter.pingPongTargets[1] = R_RENDERTARGET_PINGPONG_1;

    dstWidth = DX_RT_WIDTH(dstRenderTarget);
    dstHeight = DX_RT_HEIGHT(dstRenderTarget);

    filter.sourceImage = DX_RT_IMAGE(srcRenderTarget);
    filter.finalTarget = dstRenderTarget;
    filter.passCount = 0;

    if (srcRenderTarget == R_RENDERTARGET_RESOLVED_SCENE) {

        int sceneWidth = DX_RT_WIDTH(R_RENDERTARGET_RESOLVED_SCENE);
        int sceneHeight = DX_RT_HEIGHT(R_RENDERTARGET_RESOLVED_SCENE);

        filter.passes[0].srcWidth = (float)sceneWidth / nextPowerOfTwoF((unsigned int)sceneWidth);
        filter.passes[0].srcHeight = (float)sceneHeight / nextPowerOfTwoF((unsigned int)sceneHeight);
        filter.passes[0].dstWidth = dstWidth;
        filter.passes[0].dstHeight = dstHeight;
        filter.passes[0].tapHalfCount = 0;
        filter.passes[0].material = RGP_GLOW_SETUP_MAT();
        filter.passCount = 1;
    }

    startPass = filter.passCount;
    chainPassCount = RB_GenerateGaussianFilterChain(
        scaledRadius, scaledRadius,
        dstWidth, dstHeight,
        dstWidth, dstHeight,
        16 - startPass,
        &filter.passes[startPass]);
    filter.passCount = startPass + chainPassCount;
    passCount = filter.passCount;

    if (passCount == 0) {
        return dstRenderTarget;
    }

    g_TotalFilterPasses += passCount;

    {
        int swapLast = (g_LastGlowFilter && !(g_TotalFilterPasses & 1));
        RB_ApplyFilterPasses(&filter, swapLast);
    }

    return dstRenderTarget;
}

void RB_GlowFilterImage(float *radius)
{
    float radiusX, radiusY;
    float scaleRatio;
    GfxRenderTargetId result;
    r_backEndGlobals_t *be;

    radiusX = radius[0];
    if (radiusX == 0.0f) {
        return;
    }

    scaleRatio = (float)DX_RT_WIDTH(R_RENDERTARGET_GLOW_0) / (float)DX_RT_WIDTH(R_RENDERTARGET_RESOLVED_SCENE);

    radiusX *= scaleRatio;
    radius[0] = radiusX;
    radiusY = radius[1] * scaleRatio;
    radius[1] = radiusY;

    g_LastGlowFilter = (radiusY == 0.0f);

    if (radius[1] != 0.0f) {

        float rx = radius[0];
        float ry = radius[1];
        float orthoRadius = sqrtf(ry * ry - rx * rx);

        g_LastGlowFilter = (orthoRadius == 0.0f) ? 1 : 0;
    }

    be = (r_backEndGlobals_t *)backEnd;
    {
        GfxRenderTargetId srcTarget = be->resolvedSceneTarget;
        result = RB_ApplyGlowFilter(srcTarget, R_RENDERTARGET_GLOW_0, radius[0]);
    }

    {
        GfxImage *blurImage = DX_RT_IMAGE(result);
        be->glowImage[0] = blurImage;
    }

    if (radius[1] == 0.0f) {

        be->glowCount = 1;
        return;
    }

    {
        float rx = radius[0];
        float ry = radius[1];
        radius[1] = sqrtf(ry * ry - rx * rx);
    }

    g_LastGlowFilter = 1;

    {
        GfxRenderTargetId secondResult = RB_ApplyGlowFilter(result, R_RENDERTARGET_GLOW_0, radius[1]);

        be = (r_backEndGlobals_t *)backEnd;
        be->glowImage[1] = DX_RT_IMAGE(secondResult);

        be->glowCount = 2;
    }

    return;
}

void RB_GaussianFilterImage(float radius, GfxRenderTargetId renderTargetId)
{
    GfxImageFilter filter;
    int passCount;
    int passIndex;
    r_backEndGlobals_t *be;
    int width, height;
    float scaledRadius;
    int oddPassCount;

    be = (r_backEndGlobals_t *)backEnd;
    {
        const GfxViewParms *viewParms = be->viewParms;
        int viewportHeight = (int)viewParms->viewport.Height;
        float heightScaled = (float)viewportHeight * radius / 480.0f;

        const vidConfig_t *vcfg = (const vidConfig_t *)vidConfig;
        scaledRadius = heightScaled * vcfg->aspectRatioPixel;
    }

    filter.pingPongTargets[0] = R_RENDERTARGET_PINGPONG_0;
    filter.pingPongTargets[1] = R_RENDERTARGET_PINGPONG_1;

    width = DX_RT_WIDTH(renderTargetId);
    height = DX_RT_HEIGHT(renderTargetId);
    filter.sourceImage = DX_RT_IMAGE(R_RENDERTARGET_RESOLVED_SCENE);
    filter.finalTarget = renderTargetId;

    {
        const vidConfig_t *vcfg2 = (const vidConfig_t *)vidConfig;
        int vidWidth = vcfg2->width;
        int vidHeight = vcfg2->height;

        passCount = RB_GenerateGaussianFilterChain(
            scaledRadius, scaledRadius,
            vidWidth, vidHeight,
            width, height,
            16,
            &filter.passes[0]);
    }
    filter.passCount = passCount;

    oddPassCount = passCount & 1;

    for (passIndex = 0; passIndex < passCount; passIndex++) {
        GfxImageFilterPass *pass = &filter.passes[passIndex];
        int pingPong = passIndex & 1;
        GfxRenderTargetId setTarget;
        int constIndex;
        float dstW, dstH;

        be = (r_backEndGlobals_t *)backEnd;

        if (passIndex == 0) {
            be->currentFeedbackImage = filter.sourceImage;
        } else {
            int srcPP = 1 - pingPong;
            GfxRenderTargetId ppTarget = filter.pingPongTargets[srcPP];
            be->currentFeedbackImage = DX_RT_IMAGE(ppTarget);
        }

        if (passIndex == passCount - 1) {
            setTarget = filter.finalTarget;
        } else {
            setTarget = filter.pingPongTargets[pingPong];
        }

        RB_SetRenderTarget(setTarget);

        if (pass->tapHalfCount > 0) {
            for (constIndex = 0; constIndex < pass->tapHalfCount; constIndex++) {
                be->codeConsts[35 + constIndex][0] = pass->tapOffsetsAndWeights[constIndex][0];
                be->codeConsts[35 + constIndex][1] = pass->tapOffsetsAndWeights[constIndex][1];
                be->codeConsts[35 + constIndex][2] = pass->tapOffsetsAndWeights[constIndex][2];
                be->codeConsts[35 + constIndex][3] = pass->tapOffsetsAndWeights[constIndex][3];
            }
        }

        dstW = (float)pass->dstWidth;
        dstH = (float)pass->dstHeight;

        if (!oddPassCount && passIndex == passCount - 1) {

            RB_DrawStretchPic(pass->material, 0.0f, 0.0f, dstW, dstH,
                              0.0f, pass->srcHeight, pass->srcWidth, 0.0f,
                              0xFFFFFFFF, (GfxPrimStatsTarget)0xa);
        } else {

            RB_DrawStretchPic(pass->material, 0.0f, 0.0f, dstW, dstH,
                              0.0f, 0.0f, pass->srcWidth, pass->srcHeight,
                              0xFFFFFFFF, (GfxPrimStatsTarget)0xa);
        }

        RB_EndSurface();
    }

    return;
}

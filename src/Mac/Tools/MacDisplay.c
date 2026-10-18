#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <dlfcn.h>

void *__Znam(unsigned long size);

typedef struct {
    const char *value;
    UInt32 reserved[4];
} MacDisplayPreferenceKeyStorage;

__attribute__((used)) static MacDisplayPreferenceKeyStorage kFirstTimeKey = {
    "mac.display.first",
    { 0, 0, 0, 0 },
};
__attribute__((used)) static const char *kDisplayIndexKey = "mac.display.index";
__attribute__((used)) static const char *kDisplayRectKey = "mac.display.rect";
static Boolean sInitialized;
static CGDirectDisplayID sDisplayID;
size_t sDisplayIndex;
static CGRect sDisplayRect;
static SInt32 sDisplayDepth;
static SInt32 sDisplayRefreshRate;
Boolean sInWindowMode;
static ContextRef sScreenContext;
__attribute__((used)) static UInt32 sEnableSwap = 1;
static UInt32 sSwapCount;
static WindowRef sMainWindow;
static CGGammaValue *sSystemGammaRed;
static CGGammaValue *sSystemGammaGreen;
static CGGammaValue *sSystemGammaBlue;
void *sDisplayList[3];
static CGDirectDisplayID sMainDisplayID;
static Str255 sCaptureName;
static SInt16 sCaptureRefNum;
static Movie sCaptureMovie;
static Track sCaptureTrack;
static Media sCaptureMedia;
static CGDisplayFadeReservationToken sFadeToken;
static struct CRectList sRectList;
static MacRect sMainRect;

typedef struct {
    short top;
    short left;
    short bottom;
    short right;
} Rect;
typedef struct {
    short v;
    short h;
} RGBColor;

void *__Znam(unsigned long size);
void __ZdaPv(void *ptr);
void __ZdlPv(void *ptr);

void MacTools_CenterRect(void *ioRect, const void *inBounds);
int MacTools_GetDictionaryValue(int dict, int key);
int MacResources_GetProductFamily(void);

static inline __attribute__((always_inline)) char *MacDisplay_CurrentDisplayInfo(void)
{
    return *(char **)&sDisplayList + sDisplayIndex * 100;
}

Boolean MacDisplay_IsFullscreen(void);
Boolean MacDisplay_InWindowMode(void);
WindowRef MacDisplay_GetMainWindow(void);
CGrafPtr MacDisplay_GetMainPort(void);
GDHandle MacDisplay_GetDeviceHandle(void);
Boolean MacDisplay_IsWindowMode(void);
Boolean MacDisplay_PointInWindow(struct Point inPoint);
short unsigned int MacDisplay_CenterRectInDisplay(MacRect *ioRect);
static short unsigned int MacDisplay_FadeIn_orig(float inInterval);
static short unsigned int MacDisplay_FadeOut_orig(float inInterval);
short unsigned int MacDisplay_GetCurrentDimensions(int *outWidth, int *outHeight);
int MacDisplay_GetCurrentDepth(void);
short unsigned int MacDisplay_SwapContext(ContextRef inContextRef);
short unsigned int MacDisplay_StopCapture(void);
static short unsigned int CenterWindowOnDisplay(void);
OSStatus MacDisplay_SetupDisplay(int inWidth, int inHeight);
static Boolean BuilderCallback(WindowRef theWindow, UInt32 theCommandID);
short unsigned int MacDisplay_GetNthMode(int inIndex, int *outWidth, int *outHeight, int *outDepth, int *outRefreshRate);
int MacDisplay_GetCardType(void);
const char *MacDisplay_GetGLVendor(void);
const char *MacDisplay_GetGLRenderer(void);
const char *MacDisplay_GetGLExtensions(void);
Boolean MacDisplay_IsGLExtensionSupported(const char *inExtension);
void MacDisplay_GetVideoMemoryInfo(long int *outVideoMemory, long int *outTextureMemory);
long int MacDisplay_GetMaxTextureUnits(void);
long int MacDisplay_GetMaxTextureImageUnits(void);
short unsigned int MacDisplay_GetAntiAliasingMultiSampleInfo(int *outMaxSampleBuffers, int *outMaxSamples, Boolean *outDoesSuperSampling, Boolean *outDoesMultiSampling, Boolean *outDoesAlphaSampling);
UInt32 MacDisplay_GetPCPixelShaderVersion(void);
Boolean MacDisplay_GetSupportsSeparateBlendFunc(void);
Boolean MacDisplay_GetSupportsAnisotropicFiltering(void);
float MacDisplay_GetMaxSupportedAnisotropy(void);
short unsigned int MacDisplay_SetGammaRamp(const _D3DGAMMARAMP *inRamp);
short unsigned int MacDisplay_LocalToGlobal(Point *ioPoint);
short unsigned int MacDisplay_GlobalToLocal(Point *ioPoint);
int MacDisplay_GetNumModes(void);
short unsigned int UserPaneDrawProc(ControlRef theControl);
static ControlPartCode UserPaneTrackingProc(ControlRef theControl, Point theStartPt);
void ZN16OpaqueContextRefD1Ev(char *this_ptr);
short unsigned int MacDisplay_ReleaseContext(int *ioContextRef);
void MacDisplay_ReleaseDisplay(void);
static ContextRef MacDisplay_CreateScreenContext_orig(int inDepthSize, int inUseStencil, int inMultiSampleType, int inMultiSampleQuality, int inPresentationInterval, Boolean *outHasAuxBuffer);
short unsigned int MacDisplay_GetCurrentMode(int *outWidth, int *outHeight, int *outDepth, int *outRefreshRate);
short unsigned int MacDisplay_SetMode(int inWidth, int inHeight, int inDepth, int inRefreshRate);
void ZN12CDisplayInfoaSERKS_(void);
short unsigned int CDisplayInfo_CDisplayInfo(const CDisplayInfo *_this, CGDirectDisplayID inDisplayID);
OSStatus MacDisplay_Initialize(void);
static short unsigned int __static_initialization_and_destruction_0(void);
static void GLOBAL__D__ZN10MacDisplay10InitializeEv(void);
static void GLOBAL__I__ZN10MacDisplay10InitializeEv(void);
void ZSt11__push_heapIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEiS2_EvT_T0_S9_T1_(void);
void ZSt13__adjust_heapIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEiS2_EvT_T0_S9_T1_(void);
void ZSt25__unguarded_linear_insertIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEES2_EvT_T0_(void);
void ZSt9make_heapIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_(void);
void ZSt21__unguarded_partitionIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEES2_ET_S8_S8_T0_(void);
void ZSt16__insertion_sortIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_(void);
void ZSt22__final_insertion_sortIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_(void);
void ZSt12partial_sortIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_S8_(void);
void ZSt16__introsort_loopIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEiEvT_S8_T0_(void);
void ZNSt6vectorI7MacRectSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void);
void ZNSt6vectorI8CResInfoSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void);
void ZNSt6vectorI8CResInfoSaIS0_EEaSERKS2_(void);
void ZNSt6vectorI12CDisplayInfoSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void);

Boolean MacDisplay_IsFullscreen(void)
{
    if (sInWindowMode)
        return 0;
    return sScreenContext != 0;
}

Boolean MacDisplay_InWindowMode(void)
{
    return sInWindowMode;
}

WindowRef MacDisplay_GetMainWindow(void)
{
    return sMainWindow;
}

CGrafPtr MacDisplay_GetMainPort(void)
{
    if (sScreenContext)
        return sScreenContext->mDrawable;

    return (CGrafPtr)GetWindowPort(sMainWindow);
}

GDHandle MacDisplay_GetDeviceHandle(void)
{
    return 0;
}

Boolean MacDisplay_IsWindowMode(void)
{
    if (!sInitialized)
        return 1;
    return sInWindowMode;
}

Boolean MacDisplay_PointInWindow(struct Point inPoint)
{
    Rect bounds;
    GetWindowPortBounds(sMainWindow, &bounds);
    return PtInRect(inPoint, &bounds);
}

short unsigned int MacDisplay_CenterRectInDisplay(MacRect *ioRect)
{
    GDHandle device = MacDisplay_GetDeviceHandle();
    Rect gdRect;
    char *devData = *(char **)device;
    int a = *(int *)(devData + 0x22);
    *(int *)&gdRect = a;
    int b = *(int *)(devData + 0x26);
    *((int *)&gdRect + 1) = b;

    MacTools_CenterRect(ioRect, &gdRect);
    return 0;
}

static short unsigned int MacDisplay_FadeIn_orig(float inInterval)
{
    if (sInWindowMode)
        return 0;

    if (!sFadeToken)
        return 0;

    CGDisplayFade(sFadeToken, inInterval, 0.0f, 1.0f, 0.0f, 0.0f, 0);
    CGReleaseDisplayFadeReservation(sFadeToken);
    sFadeToken = 0;
}

static short unsigned int MacDisplay_FadeOut_orig(float inInterval)
{
    return 0;
}

short unsigned int MacDisplay_GetCurrentDimensions(int *outWidth, int *outHeight)
{
    if (sMainWindow) {
        Rect bounds;
        GetWindowPortBounds(sMainWindow, &bounds);

        int width = (short)bounds.right;
        if (width <= 639)
            width = 640;
        *outWidth = width;

        int height = (short)bounds.bottom;
        *outHeight = height;
        if (height <= 479)
            *outHeight = 480;
        return 0;
    }

    *outWidth = CGDisplayPixelsWide(sDisplayID);
    *outHeight = CGDisplayPixelsHigh(sDisplayID);
    return 0;
}

int MacDisplay_GetCurrentDepth(void)
{
    if (sDisplayDepth)
        return sDisplayDepth;

    sDisplayDepth = CGDisplayBitsPerPixel(sDisplayID);
    return sDisplayDepth;
}

short unsigned int MacDisplay_SwapContext(ContextRef inContextRef)
{
    if (*(unsigned char *)&sEnableSwap == 0)
        return 0;

    aglSwapBuffers(*(void **)inContextRef);
    sSwapCount++;
    return 0;
}

short unsigned int MacDisplay_StopCapture(void)
{
    if (!sCaptureMovie)
        return 0;

    OSErr error = EndMediaEdits(sCaptureMedia);
    if (error == 0) {
        TimeValue duration = GetMediaDuration(sCaptureMedia);
        error = InsertMediaIntoTrack(sCaptureTrack, 0, 0, duration, 0x10000);
        if (error == 0) {
            short resID = -1;
            AddMovieResource(sCaptureMovie, (short)sCaptureRefNum, &resID, sCaptureName);
        }
    }

    CloseMovieFile((short)sCaptureRefNum);
    sCaptureMovie = 0;
    return 0;
}

static short unsigned int CenterWindowOnDisplay(void)
{
    return 0;
}

OSStatus MacDisplay_SetupDisplay(int inWidth, int inHeight)
{
    return 0;
}

static Boolean BuilderCallback(WindowRef theWindow, UInt32 theCommandID)
{
    return 0;
}

short unsigned int MacDisplay_GetNthMode(int inIndex, int *outWidth, int *outHeight, int *outDepth, int *outRefreshRate)
{
    char *di = MacDisplay_CurrentDisplayInfo();

    char *mode = (char *)(*(int *)(di + 0x14)) + inIndex * 16;
    *outWidth = *(int *)(mode + 0);
    *outHeight = *(int *)(mode + 4);
    *outDepth = *(int *)(mode + 8);
    *outRefreshRate = *(int *)(mode + 12);
    return 0;
}

int MacDisplay_GetCardType(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(int *)(di + 0x2c);
}

const char *MacDisplay_GetGLVendor(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(const char **)(di + 0x30);
}

const char *MacDisplay_GetGLRenderer(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(const char **)(di + 0x34);
}

const char *MacDisplay_GetGLExtensions(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(const char **)(di + 0x38);
}

Boolean MacDisplay_IsGLExtensionSupported(const char *inExtension)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return gluCheckExtension(inExtension, *(const char **)(di + 0x38));
}

void MacDisplay_GetVideoMemoryInfo(long int *outVideoMemory, long int *outTextureMemory)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    *outVideoMemory = *(long *)(di + 0x3c);
    *outTextureMemory = *(long *)(di + 0x40);
}

long int MacDisplay_GetMaxTextureUnits(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(long *)(di + 0x44);
}

long int MacDisplay_GetMaxTextureImageUnits(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(long *)(di + 0x48);
}

short unsigned int MacDisplay_GetAntiAliasingMultiSampleInfo(int *outMaxSampleBuffers, int *outMaxSamples, Boolean *outDoesSuperSampling, Boolean *outDoesMultiSampling, Boolean *outDoesAlphaSampling)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    *outMaxSampleBuffers = *(int *)(di + 0x58);
    *outMaxSamples = *(int *)(di + 0x5c);
    *outDoesSuperSampling = *(unsigned char *)(di + 0x60);
    *outDoesMultiSampling = *(unsigned char *)(di + 0x61);
    *outDoesAlphaSampling = *(unsigned char *)(di + 0x62);
    return 0;
}

UInt32 MacDisplay_GetPCPixelShaderVersion(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(UInt32 *)(di + 0x4c);
}

Boolean MacDisplay_GetSupportsSeparateBlendFunc(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(unsigned char *)(di + 0x51);
}

Boolean MacDisplay_GetSupportsAnisotropicFiltering(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(unsigned char *)(di + 0x52);
}

float MacDisplay_GetMaxSupportedAnisotropy(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();
    return *(float *)(di + 0x54);
}

short unsigned int MacDisplay_SetGammaRamp(const _D3DGAMMARAMP *inRamp)
{
    if (sInWindowMode)
        return 0;

    if (!sSystemGammaRed) {
        sSystemGammaRed = (CGGammaValue *)__Znam(0x400);
        sSystemGammaGreen = (CGGammaValue *)__Znam(0x400);
        sSystemGammaBlue = (CGGammaValue *)__Znam(0x400);
        int count;
        CGGetDisplayTransferByTable(sDisplayID, 0x100,
                                    sSystemGammaRed, sSystemGammaGreen, sSystemGammaBlue, &count);
    }

    float *redTable = (float *)__Znam(0x400);
    float *greenTable = (float *)__Znam(0x400);
    float *blueTable = (float *)__Znam(0x400);

    const unsigned short *red = (const unsigned short *)inRamp;
    const unsigned short *green = (const unsigned short *)((char *)inRamp + 0x200);
    const unsigned short *blue = (const unsigned short *)((char *)inRamp + 0x400);

    for (int i = 0; i < 256; i++) {
        redTable[i] = (float)(red[i] >> 8) * 0.00390625f;
        greenTable[i] = (float)(green[i] >> 8) * 0.00390625f;
        blueTable[i] = (float)(blue[i] >> 8) * 0.00390625f;
    }

    CGSetDisplayTransferByTable(sDisplayID, 0x100, redTable, greenTable, blueTable);

    if (blueTable)
        __ZdaPv(blueTable);
    if (greenTable)
        __ZdaPv(greenTable);
    if (redTable)
        __ZdaPv(redTable);
    return 0;
}

short unsigned int MacDisplay_LocalToGlobal(Point *ioPoint)
{
    int port;

    if (sInitialized && !sInWindowMode && sScreenContext)
        port = (int)sScreenContext->mDrawable;
    else
        port = GetWindowPort(sMainWindow);

    QDLocalToGlobalPoint(port, ioPoint);
    return 0;
}

short unsigned int MacDisplay_GlobalToLocal(Point *ioPoint)
{
    int port;

    if (sInitialized && !sInWindowMode && sScreenContext)
        port = (int)sScreenContext->mDrawable;
    else
        port = GetWindowPort(sMainWindow);

    QDGlobalToLocalPoint(port, ioPoint);
    return 0;
}

int MacDisplay_GetNumModes(void)
{
    char *di = MacDisplay_CurrentDisplayInfo();

    return (*(int *)(di + 0x18) - *(int *)(di + 0x14)) >> 4;
}

short unsigned int UserPaneDrawProc(ControlRef theControl)
{
    return 0;
}

static ControlPartCode UserPaneTrackingProc(ControlRef theControl, Point theStartPt)
{
    return 0;
}

void ZN16OpaqueContextRefD1Ev(char *this_ptr)
{
    int ctx = *(int *)this_ptr;
    if (!ctx)
        return;

    if ((sSwapCount & 1) && *(char *)(this_ptr + 0xc) && sEnableSwap) {
        aglSwapBuffers(ctx);
        sSwapCount++;
    }

    aglSetCurrentContext(0);

    if (*(int *)(this_ptr + 4))
        aglSetDrawable(ctx, 0);

    aglDestroyContext(ctx);
    *(int *)this_ptr = 0;
}

short unsigned int MacDisplay_ReleaseContext(int *ioContextRef)
{
    int ctx = *ioContextRef;

    if (ctx == (int)(intptr_t)sScreenContext)
        sScreenContext = 0;

    if (ctx) {
        ZN16OpaqueContextRefD1Ev((char *)ctx);
        __ZdlPv((void *)ctx);
    }
    *ioContextRef = 0;
    return 0;
}

void MacDisplay_ReleaseDisplay(void)
{
    if (!sInitialized)
        return;

    MacDisplay_StopCapture();

    if (!sInWindowMode && sSystemGammaRed) {
        CGSetDisplayTransferByTable(sDisplayID, 0x100,
                                    sSystemGammaRed, sSystemGammaGreen, sSystemGammaBlue);

        if (sSystemGammaRed)
            __ZdaPv((void *)sSystemGammaRed);
        if (sSystemGammaGreen)
            __ZdaPv((void *)sSystemGammaGreen);
        if (sSystemGammaBlue)
            __ZdaPv((void *)sSystemGammaBlue);

        sSystemGammaRed = 0;
        sSystemGammaGreen = 0;
        sSystemGammaBlue = 0;
    }

    int wasInWindowMode = sInWindowMode;
    sInWindowMode = 0;

    if (sMainWindow) {
        DisposeWindow(sMainWindow);
        sMainWindow = 0;
    }

    if (!wasInWindowMode) {
        ShowMenuBar();
        SetSystemUIMode(0, 0);
    }
}

static ContextRef MacDisplay_CreateScreenContext_orig(int inDepthSize, int inUseStencil, int inMultiSampleType, int inMultiSampleQuality, int inPresentationInterval, Boolean *outHasAuxBuffer)
{
    return 0;
}

short unsigned int MacDisplay_GetCurrentMode(int *outWidth, int *outHeight, int *outDepth, int *outRefreshRate)
{
    if (sMainWindow) {
        Rect bounds;
        GetWindowPortBounds(sMainWindow, &bounds);

        int width = (short)bounds.right;
        if (width <= 639)
            width = 640;
        *outWidth = width;

        int height = (short)bounds.bottom;
        *outHeight = height;
        if (height <= 479)
            *outHeight = 480;
    } else {
        *outWidth = CGDisplayPixelsWide(sDisplayID);
        *outHeight = CGDisplayPixelsHigh(sDisplayID);
    }

    if (!sDisplayDepth)
        sDisplayDepth = CGDisplayBitsPerPixel(sDisplayID);
    *outDepth = sDisplayDepth;

    if (!sDisplayRefreshRate) {
        int modeDict = CGDisplayCurrentMode(sDisplayID);
        if (modeDict) {
            int rate = MacTools_GetDictionaryValue(modeDict, 0x32e624);
            if (rate == -1)
                rate = sDisplayRefreshRate;
            sDisplayRefreshRate = rate;
        }
    }
    *outRefreshRate = sDisplayRefreshRate;
    return 0;
}

short unsigned int MacDisplay_SetMode(int inWidth, int inHeight, int inDepth, int inRefreshRate)
{
    return 0;
}

void ZN12CDisplayInfoaSERKS_(void)
{
}

short unsigned int CDisplayInfo_CDisplayInfo(const CDisplayInfo *_this, CGDirectDisplayID inDisplayID)
{
    return 0;
}

OSStatus MacDisplay_Initialize(void)
{
    return 0;
}

static short unsigned int __static_initialization_and_destruction_0(void)
{
    return 0;
}

static void GLOBAL__D__ZN10MacDisplay10InitializeEv(void)
{
}

static void GLOBAL__I__ZN10MacDisplay10InitializeEv(void)
{
}

void ZSt11__push_heapIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEiS2_EvT_T0_S9_T1_(void)
{
}

void ZSt13__adjust_heapIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEiS2_EvT_T0_S9_T1_(void)
{
}

void ZSt25__unguarded_linear_insertIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEES2_EvT_T0_(void)
{
}

void ZSt9make_heapIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_(void)
{
}

void ZSt21__unguarded_partitionIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEES2_ET_S8_S8_T0_(void)
{
}

void ZSt16__insertion_sortIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_(void)
{
}

void ZSt22__final_insertion_sortIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_(void)
{
}

void ZSt12partial_sortIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEEvT_S8_S8_(void)
{
}

void ZSt16__introsort_loopIN9__gnu_cxx17__normal_iteratorIP8CResInfoSt6vectorIS2_SaIS2_EEEEiEvT_S8_T0_(void)
{
}

void ZNSt6vectorI7MacRectSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void)
{
}

void ZNSt6vectorI8CResInfoSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void)
{
}

void ZNSt6vectorI8CResInfoSaIS0_EEaSERKS2_(void)
{
}

void ZNSt6vectorI12CDisplayInfoSaIS0_EE13_M_insert_auxEN9__gnu_cxx17__normal_iteratorIPS0_S2_EERKS0_(void)
{
}

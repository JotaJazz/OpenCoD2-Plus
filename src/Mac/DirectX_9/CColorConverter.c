#include "common_types.h"
#include "imports.h"

extern unsigned char sStdConverterARGB[];
extern unsigned char sStdConverterABGR[];
extern unsigned char sATI4CompsConverterARGB[];
extern unsigned char sATI4CompsConverterABGR[];
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_StdConverterARGB[];
extern fnptr_t vtbl_StdConverterABGR[];
extern fnptr_t vtbl_ATI4CompsConverterARGB[];
extern fnptr_t vtbl_ATI4CompsConverterABGR[];
extern void *imp___ZN7COpenGL7sOpenGLE;
extern void *imp___ZTV15CColorConverter;

extern int MacDisplay_GetCardType(void);
extern Boolean MacFeatures_IsAltiVecAvailable(void);
void __ZdlPv(void *ptr);

static UINT32 CColorConverter_RotateRight32(UINT32 value, unsigned int shift)
{
    return (value >> shift) | (value << (32 - shift));
}

static UINT32 CColorConverter_ByteSwap32(UINT32 value)
{
    return ((value & 0xFF000000) >> 24) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x000000FF) << 24);
}

static float CColorConverter_ByteToFloat(UINT8 value)
{
    return (float)value * (1.0f / 255.0f);
}

static UINT32 CColorConverter_FloatToByte(float value)
{
    float scaled;

    if (0.0f > value) {
        scaled = 0.0f;
    } else if (1.0f < value) {
        scaled = 255.0f;
    } else {
        scaled = value * 255.0f;
    }

    return (UINT32)(scaled + 0.5f);
}

static UINT32 CColorConverter_PackStdARGB(float r, float g, float b, float a)
{
    UINT32 packed;

    packed = (CColorConverter_FloatToByte(b) << 24) | (CColorConverter_FloatToByte(g) << 16) | (CColorConverter_FloatToByte(r) << 8) | CColorConverter_FloatToByte(a);
    return CColorConverter_ByteSwap32(packed);
}

static UINT32 CColorConverter_PackATI4CompsARGB(float r, float g, float b, float a)
{
    UINT32 packed;

    packed = (CColorConverter_FloatToByte(b) << 24) | (CColorConverter_FloatToByte(g) << 16) | (CColorConverter_FloatToByte(r) << 8) | CColorConverter_FloatToByte(a);
    return CColorConverter_RotateRight32(packed, 24);
}

static void CColorConverter_SetVTable(void *object, void **vtable)
{
    *(void ***)object = vtable;
}

static void **CColorConverter_GetBaseVTable(void)
{
    return (void **)((char *)imp___ZTV15CColorConverter + 8);
}

void StdConverterARGB_Convert(const StdConverterARGB *_this, const void *pDst, const void *pSrc);
void StdConverterABGR_Convert4ub4f(const StdConverterABGR *_this, const void *pDst, const float *Src);
void StdConverterARGB_Convert4f4ubInverse(const StdConverterARGB *_this, float *Dest, const UINT8 *Src);
void StdConverterABGR_Convert4f4ubInverse(const StdConverterABGR *_this, float *Dest, const UINT8 *Src);
void StdConverterARGB_Convert3f4ubInverse(const StdConverterARGB *_this, float *Dest, const UINT8 *Src);
void StdConverterABGR_Convert3f4ubInverse(const StdConverterABGR *_this, float *Dest, const UINT8 *Src);
void StdConverterABGR_ArrayConvert4f4ub(const StdConverterABGR *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices);
void ATI4CompsConverterARGB_Convert(const ATI4CompsConverterARGB *_this, const void *pDst, const void *pSrc);
void ATI4CompsConverterABGR_Convert(const ATI4CompsConverterABGR *_this, const void *pDst, const void *pSrc);
void ATI4CompsConverterABGR_Convert4ub4f(const ATI4CompsConverterABGR *_this, const void *pDst, const float *Src);
void ATI4CompsConverterARGB_Convert4f4ubInverse(const ATI4CompsConverterARGB *_this, float *Dest, const UINT8 *Src);
void ATI4CompsConverterABGR_Convert4f4ubInverse(const ATI4CompsConverterABGR *_this, float *Dest, const UINT8 *Src);
void ATI4CompsConverterARGB_Convert3f4ubInverse(const ATI4CompsConverterARGB *_this, float *Dest, const UINT8 *Src);
void ATI4CompsConverterABGR_Convert3f4ubInverse(const ATI4CompsConverterABGR *_this, float *Dest, const UINT8 *Src);
void ATI4CompsConverterABGR_ArrayConvert4f4ub(const ATI4CompsConverterABGR *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices);
void StdConverterABGR_Convert(const StdConverterABGR *_this, const void *pDst, const void *pSrc);
void StdConverterARGB_Convert4ub4f(const StdConverterARGB *_this, const void *pDst, const float *Src);
void StdConverterARGB_ArrayConvert4f4ubG3(const StdConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices);
void ATI4CompsConverterARGB_Convert4ub4f(const ATI4CompsConverterARGB *_this, const void *pDst, const float *Src);
void ATI4CompsConverterARGB_ArrayConvert4f4ubG3(const ATI4CompsConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices);
void ATI4CompsConverterARGB_ArrayConvert4f4ub(const ATI4CompsConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices);
void StdConverterARGB_ArrayConvert4f4ub(const StdConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices);
const CColorConverter *CColorConverter_GetColorConverter(Format SrcFormat);
static void __static_initialization_and_destruction_0(int __initialize_p, int __priority);
static void GLOBAL__D__ZN15CColorConverter17GetColorConverterENS_6FormatE(void);
static void GLOBAL__I__ZN15CColorConverter17GetColorConverterENS_6FormatE(void);
void CColorConverter_Convert4ub4f(const CColorConverter *_this, const void *Dest, const float *Src);
void CColorConverter_Convert4f4ubInverse(const CColorConverter *_this, float *Dest, const UINT8 *Src);
void CColorConverter_Convert3f4ubInverse(const CColorConverter *_this, float *Dest, const UINT8 *Src);
void CColorConverter_ArrayConvert4f4ub(const CColorConverter *_this, const void *pDst, const float *pSrc, UINT32 NumVertices);
ConverterType CColorConverter_GetType(const CColorConverter *_this);
void ZN15CColorConverterD1Ev(CColorConverter *_this);
void ZN15CColorConverterD0Ev(CColorConverter *_this);
ConverterType ATI4CompsConverterARGB_GetType(const ATI4CompsConverterARGB *_this);
ConverterType ATI4CompsConverterABGR_GetType(const ATI4CompsConverterABGR *_this);
ConverterType StdConverterARGB_GetType(const StdConverterARGB *_this);
ConverterType StdConverterABGR_GetType(const StdConverterABGR *_this);
void ZN16StdConverterARGBD1Ev(StdConverterARGB *_this);
void ZN16StdConverterARGBD0Ev(StdConverterARGB *_this);
void ZN16StdConverterABGRD1Ev(StdConverterABGR *_this);
void ZN16StdConverterABGRD0Ev(StdConverterABGR *_this);
void ZN22ATI4CompsConverterARGBD1Ev(ATI4CompsConverterARGB *_this);
void ZN22ATI4CompsConverterARGBD0Ev(ATI4CompsConverterARGB *_this);
void ZN22ATI4CompsConverterABGRD1Ev(ATI4CompsConverterABGR *_this);
void ZN22ATI4CompsConverterABGRD0Ev(ATI4CompsConverterABGR *_this);

void StdConverterARGB_Convert(const StdConverterARGB *_this, const void *pDst, const void *pSrc)
{
    UINT32 value;

    (void)_this;

    memcpy(&value, pSrc, sizeof(value));
    value = CColorConverter_ByteSwap32(value);
    memcpy((void *)pDst, &value, sizeof(value));
}

void StdConverterABGR_Convert4ub4f(const StdConverterABGR *_this, const void *pDst, const float *Src)
{
    (void)_this;
    (void)pDst;
    (void)Src;
}

void StdConverterARGB_Convert4f4ubInverse(const StdConverterARGB *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;

    Dest[0] = CColorConverter_ByteToFloat(Src[0]);
    Dest[1] = CColorConverter_ByteToFloat(Src[1]);
    Dest[2] = CColorConverter_ByteToFloat(Src[2]);
    Dest[3] = CColorConverter_ByteToFloat(Src[3]);
}

void StdConverterABGR_Convert4f4ubInverse(const StdConverterABGR *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void StdConverterARGB_Convert3f4ubInverse(const StdConverterARGB *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;

    Dest[0] = CColorConverter_ByteToFloat(Src[0]);
    Dest[1] = CColorConverter_ByteToFloat(Src[1]);
    Dest[2] = CColorConverter_ByteToFloat(Src[2]);
}

void StdConverterABGR_Convert3f4ubInverse(const StdConverterABGR *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void StdConverterABGR_ArrayConvert4f4ub(const StdConverterABGR *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices)
{
    (void)_this;
    (void)pRawDst;
    (void)pSrc;
    (void)NumVertices;
}

void ATI4CompsConverterARGB_Convert(const ATI4CompsConverterARGB *_this, const void *pDst, const void *pSrc)
{
    UINT32 value;

    (void)_this;

    memcpy(&value, pSrc, sizeof(value));
    value = CColorConverter_RotateRight32(value, 24);
    memcpy((void *)pDst, &value, sizeof(value));
}

void ATI4CompsConverterABGR_Convert(const ATI4CompsConverterABGR *_this, const void *pDst, const void *pSrc)
{
    (void)_this;

    memcpy((void *)pDst, pSrc, sizeof(UINT32));
}

void ATI4CompsConverterABGR_Convert4ub4f(const ATI4CompsConverterABGR *_this, const void *pDst, const float *Src)
{
    (void)_this;
    (void)pDst;
    (void)Src;
}

void ATI4CompsConverterARGB_Convert4f4ubInverse(const ATI4CompsConverterARGB *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;

    Dest[0] = CColorConverter_ByteToFloat(Src[3]);
    Dest[1] = CColorConverter_ByteToFloat(Src[2]);
    Dest[2] = CColorConverter_ByteToFloat(Src[1]);
    Dest[3] = CColorConverter_ByteToFloat(Src[0]);
}

void ATI4CompsConverterABGR_Convert4f4ubInverse(const ATI4CompsConverterABGR *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void ATI4CompsConverterARGB_Convert3f4ubInverse(const ATI4CompsConverterARGB *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;

    Dest[0] = CColorConverter_ByteToFloat(Src[3]);
    Dest[1] = CColorConverter_ByteToFloat(Src[2]);
    Dest[2] = CColorConverter_ByteToFloat(Src[1]);
}

void ATI4CompsConverterABGR_Convert3f4ubInverse(const ATI4CompsConverterABGR *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void ATI4CompsConverterABGR_ArrayConvert4f4ub(const ATI4CompsConverterABGR *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices)
{
    (void)_this;
    (void)pRawDst;
    (void)pSrc;
    (void)NumVertices;
}

void StdConverterABGR_Convert(const StdConverterABGR *_this, const void *pDst, const void *pSrc)
{
    (void)_this;

    memcpy((void *)pDst, pSrc, sizeof(UINT32));
}

void StdConverterARGB_Convert4ub4f(const StdConverterARGB *_this, const void *pDst, const float *Src)
{
    UINT32 packed;

    (void)_this;

    packed = CColorConverter_PackStdARGB(Src[0], Src[1], Src[2], Src[3]);
    memcpy((void *)pDst, &packed, sizeof(packed));
}

void StdConverterARGB_ArrayConvert4f4ubG3(const StdConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices)
{
    UINT32 *pDst;
    UINT32 i;

    (void)_this;

    pDst = (UINT32 *)pRawDst;
    for (i = 0; i < NumVertices; ++i) {
        *pDst++ = CColorConverter_PackStdARGB(pSrc[0], pSrc[1], pSrc[2], pSrc[3]);
        pSrc += 4;
    }
}

void ATI4CompsConverterARGB_Convert4ub4f(const ATI4CompsConverterARGB *_this, const void *pDst, const float *Src)
{
    UINT32 packed;

    (void)_this;

    packed = CColorConverter_PackATI4CompsARGB(Src[0], Src[1], Src[2], Src[3]);
    memcpy((void *)pDst, &packed, sizeof(packed));
}

void ATI4CompsConverterARGB_ArrayConvert4f4ubG3(const ATI4CompsConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices)
{
    UINT32 *pDst;
    UINT32 i;

    (void)_this;

    pDst = (UINT32 *)pRawDst;
    for (i = 0; i < NumVertices; ++i) {
        *pDst++ = CColorConverter_PackATI4CompsARGB(pSrc[0], pSrc[1], pSrc[2], pSrc[3]);
        pSrc += 4;
    }
}

void ATI4CompsConverterARGB_ArrayConvert4f4ub(const ATI4CompsConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices)
{
    if (!MacFeatures_IsAltiVecAvailable()) {
        ATI4CompsConverterARGB_ArrayConvert4f4ubG3(_this, pRawDst, pSrc, NumVertices);
    }
}

void StdConverterARGB_ArrayConvert4f4ub(const StdConverterARGB *_this, const void *pRawDst, const float *pSrc, UINT32 NumVertices)
{
    if (!MacFeatures_IsAltiVecAvailable()) {
        StdConverterARGB_ArrayConvert4f4ubG3(_this, pRawDst, pSrc, NumVertices);
    }
}

const CColorConverter *CColorConverter_GetColorConverter(Format SrcFormat)
{
    const unsigned char *openGlState;

    openGlState = (const unsigned char *)imp___ZN7COpenGL7sOpenGLE;
    if (MacDisplay_GetCardType() == 2 && openGlState[0x800] != 0) {
        return (const CColorConverter *)(SrcFormat ? sATI4CompsConverterABGR : sATI4CompsConverterARGB);
    }

    return (const CColorConverter *)(SrcFormat ? sStdConverterABGR : sStdConverterARGB);
}

static void __static_initialization_and_destruction_0(int __initialize_p, int __priority)
{
    if (__priority != 0xffff) {
        return;
    }

    if (__initialize_p == 0) {
        void **baseVTable;

        baseVTable = CColorConverter_GetBaseVTable();
        CColorConverter_SetVTable(sATI4CompsConverterABGR, baseVTable);
        CColorConverter_SetVTable(sATI4CompsConverterARGB, baseVTable);
        CColorConverter_SetVTable(sStdConverterABGR, baseVTable);
        CColorConverter_SetVTable(sStdConverterARGB, baseVTable);
        return;
    }

    if (__initialize_p == 1) {
        CColorConverter_SetVTable(sStdConverterARGB, (void**)vtbl_StdConverterARGB);
        CColorConverter_SetVTable(sStdConverterABGR, (void**)vtbl_StdConverterABGR);
        CColorConverter_SetVTable(sATI4CompsConverterARGB, (void**)vtbl_ATI4CompsConverterARGB);
        CColorConverter_SetVTable(sATI4CompsConverterABGR, (void**)vtbl_ATI4CompsConverterABGR);
    }
}

static void GLOBAL__D__ZN15CColorConverter17GetColorConverterENS_6FormatE(void)
{
    __static_initialization_and_destruction_0(0, 0xffff);
}

static void GLOBAL__I__ZN15CColorConverter17GetColorConverterENS_6FormatE(void)
{
    __static_initialization_and_destruction_0(1, 0xffff);
}

void CColorConverter_Convert4ub4f(const CColorConverter *_this, const void *Dest, const float *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void CColorConverter_Convert4f4ubInverse(const CColorConverter *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void CColorConverter_Convert3f4ubInverse(const CColorConverter *_this, float *Dest, const UINT8 *Src)
{
    (void)_this;
    (void)Dest;
    (void)Src;
}

void CColorConverter_ArrayConvert4f4ub(const CColorConverter *_this, const void *pDst, const float *pSrc, UINT32 NumVertices)
{
    (void)_this;
    (void)pDst;
    (void)pSrc;
    (void)NumVertices;
}

ConverterType CColorConverter_GetType(const CColorConverter *_this)
{
    (void)_this;
    return UNKNOWN_CONVERTER;
}

void ZN15CColorConverterD1Ev(CColorConverter *_this)
{
    CColorConverter_SetVTable(_this, CColorConverter_GetBaseVTable());
}

void ZN15CColorConverterD0Ev(CColorConverter *_this)
{
    ZN15CColorConverterD1Ev(_this);
    __ZdlPv(_this);
}

ConverterType ATI4CompsConverterARGB_GetType(const ATI4CompsConverterARGB *_this)
{
    (void)_this;
    return ATI_4_COMPS_CONVERTER_ARGB;
}

ConverterType ATI4CompsConverterABGR_GetType(const ATI4CompsConverterABGR *_this)
{
    (void)_this;
    return ATI_4_COMPS_CONVERTER_ABGR;
}

ConverterType StdConverterARGB_GetType(const StdConverterARGB *_this)
{
    (void)_this;
    return STD_CONVERTER_ARGB;
}

ConverterType StdConverterABGR_GetType(const StdConverterABGR *_this)
{
    (void)_this;
    return STD_CONVERTER_ABGR;
}

void ZN16StdConverterARGBD1Ev(StdConverterARGB *_this)
{
    CColorConverter_SetVTable(_this, (void**)vtbl_StdConverterARGB);
    CColorConverter_SetVTable(_this, CColorConverter_GetBaseVTable());
}

void ZN16StdConverterARGBD0Ev(StdConverterARGB *_this)
{
    ZN16StdConverterARGBD1Ev(_this);
    __ZdlPv(_this);
}

void ZN16StdConverterABGRD1Ev(StdConverterABGR *_this)
{
    CColorConverter_SetVTable(_this, (void**)vtbl_StdConverterABGR);
    CColorConverter_SetVTable(_this, CColorConverter_GetBaseVTable());
}

void ZN16StdConverterABGRD0Ev(StdConverterABGR *_this)
{
    ZN16StdConverterABGRD1Ev(_this);
    __ZdlPv(_this);
}

void ZN22ATI4CompsConverterARGBD1Ev(ATI4CompsConverterARGB *_this)
{
    CColorConverter_SetVTable(_this, (void**)vtbl_ATI4CompsConverterARGB);
    CColorConverter_SetVTable(_this, CColorConverter_GetBaseVTable());
}

void ZN22ATI4CompsConverterARGBD0Ev(ATI4CompsConverterARGB *_this)
{
    ZN22ATI4CompsConverterARGBD1Ev(_this);
    __ZdlPv(_this);
}

void ZN22ATI4CompsConverterABGRD1Ev(ATI4CompsConverterABGR *_this)
{
    CColorConverter_SetVTable(_this, (void**)vtbl_ATI4CompsConverterABGR);
    CColorConverter_SetVTable(_this, CColorConverter_GetBaseVTable());
}

void ZN22ATI4CompsConverterABGRD0Ev(ATI4CompsConverterABGR *_this)
{
    ZN22ATI4CompsConverterABGRD1Ev(_this);
    __ZdlPv(_this);
}

#ifdef _MSC_VER
COD2_CONSTRUCTOR(_ctor_CColorConverter)
#else
__attribute__((constructor)) static void _ctor_CColorConverter(void)
#endif
{
    GLOBAL__I__ZN15CColorConverter17GetColorConverterENS_6FormatE();
}

fnptr_t vtbl_StdConverterARGB[] = { (fnptr_t)StdConverterARGB_Convert, (fnptr_t)StdConverterARGB_Convert4ub4f, (fnptr_t)StdConverterARGB_Convert4f4ubInverse, (fnptr_t)StdConverterARGB_Convert3f4ubInverse, (fnptr_t)StdConverterARGB_ArrayConvert4f4ub, (fnptr_t)StdConverterARGB_GetType, (fnptr_t)ZN16StdConverterARGBD1Ev, (fnptr_t)ZN16StdConverterARGBD0Ev };
fnptr_t vtbl_StdConverterABGR[] = { (fnptr_t)StdConverterABGR_Convert, (fnptr_t)StdConverterABGR_Convert4ub4f, (fnptr_t)StdConverterABGR_Convert4f4ubInverse, (fnptr_t)StdConverterABGR_Convert3f4ubInverse, (fnptr_t)StdConverterABGR_ArrayConvert4f4ub, (fnptr_t)StdConverterABGR_GetType, (fnptr_t)ZN16StdConverterABGRD1Ev, (fnptr_t)ZN16StdConverterABGRD0Ev };
fnptr_t vtbl_ATI4CompsConverterARGB[] = { (fnptr_t)ATI4CompsConverterARGB_Convert, (fnptr_t)ATI4CompsConverterARGB_Convert4ub4f, (fnptr_t)ATI4CompsConverterARGB_Convert4f4ubInverse, (fnptr_t)ATI4CompsConverterARGB_Convert3f4ubInverse, (fnptr_t)ATI4CompsConverterARGB_ArrayConvert4f4ub, (fnptr_t)ATI4CompsConverterARGB_GetType, (fnptr_t)ZN22ATI4CompsConverterARGBD1Ev, (fnptr_t)ZN22ATI4CompsConverterARGBD0Ev };
fnptr_t vtbl_ATI4CompsConverterABGR[] = { (fnptr_t)ATI4CompsConverterABGR_Convert, (fnptr_t)ATI4CompsConverterABGR_Convert4ub4f, (fnptr_t)ATI4CompsConverterABGR_Convert4f4ubInverse, (fnptr_t)ATI4CompsConverterABGR_Convert3f4ubInverse, (fnptr_t)ATI4CompsConverterABGR_ArrayConvert4f4ub, (fnptr_t)ATI4CompsConverterABGR_GetType, (fnptr_t)ZN22ATI4CompsConverterABGRD1Ev, (fnptr_t)ZN22ATI4CompsConverterABGRD0Ev };

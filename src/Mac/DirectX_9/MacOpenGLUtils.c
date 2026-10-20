#include "common_types.h"
#include "imports.h"

static UINT32 sD3DTextureOpToOpenGL[16] = {
    0x00008577,
    0x00008578,
    0x00001702,
    0x00008576,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
};
static UINT8 FastTranslateTbl[32] = {
    0x10,
    0x20,
    0x00,
    0x00,
    0x30,
    0x12,
    0x12,
    0x40,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};
static UINT32 sDeclarationTable[8] = {
    0x11406040,
    0x21406040,
    0x31406040,
    0x41406040,
    0x41401011,
    0x41401010,
    0x21402020,
    0x41402020,
};

bool MacOpenGLUtils_IsGLExtensionSupported(const char *pExtension);
bool MacOpenGLUtils_AreMatricesDifferent4x4(const int *pA, const int *pB);
bool MacOpenGLUtils_IsCompressed(const D3DFORMAT *f);
UINT32 MacOpenGLUtils_GetFormatSizeInBits(const D3DFORMAT *f);
int MacOpenGLUtils_GetOpenGLTextureOp(GLenum *Source, GLenum *Params, UINT32 D3DTextureOp);
int MacOpenGLUtils_GetOpenGLTextureAlphaOp(GLenum *Source, GLenum *Params, UINT32 D3DTextureOp);
UINT32 MacOpenGLUtils_GetElementCount(GLenum Mode, UINT32 PrimitiveCount);
UINT32 MacOpenGLUtils_GetPCPixelShaderVersion(void);
UINT32 MacOpenGLUtils_GetNumTextureLevels(UINT32 Width, UINT32 Height, UINT32 Depth);
int MacOpenGLUtils_GetDeclarationInfo(GLint *VSize, GLenum *VType, UINT32 *ComponentSize, GLboolean *Normalized, UINT32 Type);
int MacOpenGLUtils_GetSubPixelOffset(float *XOffset, float *YOffset);
int MacOpenGLUtils_GetOpenGLTextureType(bool *CreateOpenGLResources, GLenum *OpenGLInternalFormat, GLenum *OpenGLFormat, GLenum *OpenGLElementType, D3DFORMAT mFormat);
UINT32 MacOpenGLUtils_GetLevelSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, const D3DFORMAT *f);
float MacOpenGLUtils_SquareRootLowPrecision(float fp0);
int MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(float *m, float ViewportWidth, float ViewportHeight);
UINT32 MacOpenGLUtils_GetImageSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, UINT32 LevelCount, const D3DFORMAT *f);
int MacDisplay_GetCardType(void);
Boolean MacDisplay_IsGLExtensionSupported(const char *ext);
UINT32 MacDisplay_GetPCPixelShaderVersion(void);

bool MacOpenGLUtils_IsGLExtensionSupported(const char *pExtension)
{
    return MacDisplay_IsGLExtensionSupported(pExtension) != 0;
}

bool MacOpenGLUtils_AreMatricesDifferent4x4(const int *pA, const int *pB)
{
    int code = 0;
    int count;

    for (count = 0; count < 4; count++) {
        code |= pA[count * 4 + 0] ^ pB[count * 4 + 0];
        code |= pA[count * 4 + 1] ^ pB[count * 4 + 1];
        code |= pA[count * 4 + 2] ^ pB[count * 4 + 2];
        code |= pA[count * 4 + 3] ^ pB[count * 4 + 3];
    }

    return code != 0;
}

bool MacOpenGLUtils_IsCompressed(const D3DFORMAT *f)
{
    D3DFORMAT fmt = *f;

    switch (fmt) {
    case D3DFMT_DXT1:
    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        return 1;
    default:
        return 0;
    }
}

UINT32 MacOpenGLUtils_GetFormatSizeInBits(const D3DFORMAT *f)
{
    D3DFORMAT fmt = *f;

    switch (fmt) {

    case D3DFMT_R8G8B8:
        return 24;

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_X8L8V8U8:
    case D3DFMT_Q8W8V8U8:
    case D3DFMT_V16U16:
    case D3DFMT_D32:
    case D3DFMT_D24S8:
    case D3DFMT_D24X8:
    case D3DFMT_D24X4S4:
    case D3DFMT_INDEX32:
        return 32;

    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_A4R4G4B4:
    case D3DFMT_A8R3G3B2:
    case D3DFMT_X4R4G4B4:
    case D3DFMT_A8P8:
    case D3DFMT_A8L8:
    case D3DFMT_V8U8:
    case D3DFMT_L6V5U5:
    case D3DFMT_D16_LOCKABLE:
    case D3DFMT_D15S1:
    case D3DFMT_D16:
    case D3DFMT_INDEX16:
        return 16;

    case D3DFMT_R3G3B2:
    case D3DFMT_A8:
    case D3DFMT_P8:
    case D3DFMT_L8:
    case D3DFMT_A4L4:
    case D3DFMT_DXT3:
    case D3DFMT_DXT2:
    case D3DFMT_DXT4:
    case D3DFMT_DXT5:
        return 8;

    case D3DFMT_DXT1:
        return 4;

    default:
        return 0;
    }
}

int MacOpenGLUtils_GetOpenGLTextureOp(GLenum *Source, GLenum *Params, UINT32 D3DTextureOp)
{
    UINT32 opIndex;
    UINT32 bit4;
    UINT32 bit5;

    opIndex = D3DTextureOp & 0xF;
    *Source = sD3DTextureOpToOpenGL[opIndex];

    bit4 = (D3DTextureOp >> 4) & 1;

    *Params = bit4 ? 0x301 : 0x300;

    bit5 = D3DTextureOp & 0x20;
    if (bit5) {

        *Params = bit4 ? 0x303 : 0x302;
    }

    return 0;
}

int MacOpenGLUtils_GetOpenGLTextureAlphaOp(GLenum *Source, GLenum *Params, UINT32 D3DTextureOp)
{
    UINT32 opIndex;
    UINT32 bit4;

    opIndex = D3DTextureOp & 0xF;
    *Source = sD3DTextureOpToOpenGL[opIndex];

    bit4 = D3DTextureOp & 0x10;

    *Params = bit4 ? 0x303 : 0x302;

    return 0;
}

UINT32 MacOpenGLUtils_GetElementCount(GLenum Mode, UINT32 PrimitiveCount)
{
    UINT8 MultAdd;
    UINT32 mult;
    UINT32 add;

    MultAdd = FastTranslateTbl[Mode];
    mult = (MultAdd >> 4);
    add = MultAdd & 0xF;

    return mult * PrimitiveCount + add;
}

UINT32 MacOpenGLUtils_GetPCPixelShaderVersion(void)
{
    return MacDisplay_GetPCPixelShaderVersion();
}

UINT32 MacOpenGLUtils_GetNumTextureLevels(UINT32 Width, UINT32 Height, UINT32 Depth)
{
    UINT32 ActualLevels = 0;
    UINT32 TempWidth = Width;
    UINT32 TempHeight = Height;
    UINT32 TempDepth = Depth;

    for (;;) {
        if (TempWidth) {
            if (TempHeight) {

                TempWidth >>= 1;
                TempHeight >>= 1;
            } else {

                TempWidth >>= 1;
                TempHeight = 0;
            }
        } else {
            if (TempHeight) {

                TempHeight >>= 1;
            } else if (TempDepth) {

                TempWidth = 1;
                TempWidth >>= 1;
                TempHeight = 0;
            } else {

                break;
            }
        }

        if (TempDepth) {
            TempDepth >>= 1;
        }

        ActualLevels++;
    }

    return ActualLevels;
}

int MacOpenGLUtils_GetDeclarationInfo(GLint *VSize, GLenum *VType, UINT32 *ComponentSize, GLboolean *Normalized, UINT32 Type)
{
    UINT32 DeclarationInfo;

    DeclarationInfo = sDeclarationTable[Type];
    *VSize = (DeclarationInfo >> 28);
    *VType = (DeclarationInfo >> 12) & 0xFFFF;
    *ComponentSize = (DeclarationInfo >> 4) & 0xF;
    *Normalized = (GLboolean)(DeclarationInfo & 1);

    return 0;
}

int MacOpenGLUtils_GetSubPixelOffset(float *XOffset, float *YOffset)
{
    if (MacDisplay_GetCardType() == 2) {

        *XOffset = -0.30000001192092896f;
        *YOffset = -0.30000001192092896f;
    } else {

        *XOffset = -0.15000000596046448f;

        *YOffset = -0.07500000298023224f;
    }

    return 0;
}

int MacOpenGLUtils_GetOpenGLTextureType(bool *CreateOpenGLResources, GLenum *OpenGLInternalFormat, GLenum *OpenGLFormat, GLenum *OpenGLElementType, D3DFORMAT mFormat)
{
    GLenum tmpInternal = 0, tmpFormat = 0, tmpElement = 0;
    if (CreateOpenGLResources)
        *CreateOpenGLResources = 1;
    if (!OpenGLInternalFormat)
        OpenGLInternalFormat = &tmpInternal;
    if (!OpenGLFormat)
        OpenGLFormat = &tmpFormat;
    if (!OpenGLElementType)
        OpenGLElementType = &tmpElement;

    switch (mFormat) {
    case D3DFMT_A1R5G5B5:
        *OpenGLInternalFormat = 0x8057;
        *OpenGLFormat = 0x80E1;
        *OpenGLElementType = 0x8366;
        break;

    case D3DFMT_R5G6B5:
        *OpenGLInternalFormat = 0x1907;
        *OpenGLFormat = 0x1907;
        *OpenGLElementType = 0x8363;
        break;

    case D3DFMT_A4R4G4B4:
        *OpenGLInternalFormat = 0x8056;
        *OpenGLFormat = 0x80E1;
        *OpenGLElementType = 0x8365;
        break;

    case D3DFMT_R8G8B8:
        *OpenGLInternalFormat = 0x1907;
        *OpenGLFormat = 0x1907;
        *OpenGLElementType = 0x1401;
        break;

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        *OpenGLInternalFormat = 0x1908;
        *OpenGLFormat = 0x80E1;
        *OpenGLElementType = 0x1401;
        break;

    case D3DFMT_A8:
        *OpenGLInternalFormat = 0x1906;
        *OpenGLFormat = 0x1906;
        *OpenGLElementType = 0x1401;
        break;

    case D3DFMT_L8:
        *OpenGLInternalFormat = 0x1909;
        *OpenGLFormat = 0x1909;
        *OpenGLElementType = 0x1401;
        break;

    case D3DFMT_A8L8:
        *OpenGLInternalFormat = 0x190A;
        *OpenGLFormat = 0x190A;
        *OpenGLElementType = 0x1401;
        break;

    case D3DFMT_DXT1:
        *OpenGLInternalFormat = 0x83F1;
        *OpenGLElementType = 0;
        *OpenGLFormat = 0;
        break;

    case D3DFMT_DXT2:
    case D3DFMT_DXT3:
        *OpenGLInternalFormat = 0x83F2;
        *OpenGLElementType = 0;
        *OpenGLFormat = 0;
        break;

    case D3DFMT_DXT5:
        *OpenGLInternalFormat = 0x83F3;
        *OpenGLElementType = 0;
        *OpenGLFormat = 0;
        break;

    case D3DFMT_D24S8:
        *OpenGLInternalFormat = 0x1902;
        *OpenGLFormat = 0x1902;
        *OpenGLElementType = 0x1401;
        break;

    case D3DFMT_D32:
    case D3DFMT_D15S1:
    case D3DFMT_D16:
        *CreateOpenGLResources = 0;
        break;

    default:
        break;
    }

    return 0;
}

UINT32 MacOpenGLUtils_GetLevelSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, const D3DFORMAT *f)
{
    UINT32 w, h, d;
    UINT32 bitsPerPixel;
    UINT32 blockSize;
    D3DFORMAT fmt;

    w = Width ? Width : 1;
    h = Height ? Height : 1;
    d = Depth ? Depth : 1;

    fmt = *f;

    if (MacOpenGLUtils_IsCompressed(f)) {
        if (fmt == D3DFMT_DXT1) {
            blockSize = 8;
        } else {
            blockSize = 16;
        }
        return ((w + 3) / 4) * ((h + 3) / 4) * d * blockSize;
    }

    bitsPerPixel = MacOpenGLUtils_GetFormatSizeInBits(f);
    return (w * h * d * bitsPerPixel) / 8;
}

float MacOpenGLUtils_SquareRootLowPrecision(float fp0)
{
    union {
        float f;
        int i;
    } conv;
    float xhalf;
    float y;

    conv.f = fp0;
    conv.i = 0x5f375a86 - (conv.i >> 1);
    y = conv.f;
    xhalf = fp0 * 0.5f;
    y = y * (1.5f - xhalf * y * y);
    return fp0 * y;
}

int MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(float *m, float ViewportWidth, float ViewportHeight)
{
    float x;
    float y;

    if (MacDisplay_GetCardType() == 2) {
        x = -0.30000001192092896f;
        y = -0.30000001192092896f;
    } else {
        x = -0.15000000596046448f;
        y = -0.07500000298023224f;
    }

    x /= ViewportWidth;
    y /= ViewportHeight;

    m[0] += x * m[3];
    m[1] += y * m[3];
    m[2] = 2.0f * m[2] - m[3];

    m[4] += x * m[7];
    m[5] += y * m[7];
    m[6] = 2.0f * m[6] - m[7];

    m[8] = -(m[8] + x * m[11]);
    m[9] = -(m[9] + y * m[11]);
    m[10] = m[11] - 2.0f * m[10];
    m[11] = -m[11];

    m[12] += x * m[15];
    m[13] += y * m[15];
    m[14] = 2.0f * m[14] - m[15];

    return 0;
}

UINT32 MacOpenGLUtils_GetImageSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, UINT32 LevelCount, const D3DFORMAT *f)
{
    UINT32 ImageSizeInBytes;
    UINT32 Level;
    UINT32 w, h, d;

    if (LevelCount == 0) {
        return 0;
    }

    ImageSizeInBytes = 0;
    w = Width;
    h = Height;
    d = Depth;

    for (Level = 0; Level < LevelCount; Level++) {
        D3DFORMAT fmt_val = *f;
        UINT32 ew, eh, ed;
        UINT32 levelSize;

        ew = w ? w : 1;
        eh = h ? h : 1;
        ed = d ? d : 1;

        if (fmt_val == D3DFMT_DXT1 || fmt_val == D3DFMT_DXT2 || fmt_val == D3DFMT_DXT3 ||
            fmt_val == D3DFMT_DXT4 || fmt_val == D3DFMT_DXT5) {
            UINT32 blockSize;
            if (fmt_val == D3DFMT_DXT1) {
                blockSize = 8;
            } else {
                blockSize = 16;
            }
            levelSize = ((ew + 3) / 4) * ((eh + 3) / 4) * ed * blockSize;
        } else {
            UINT32 bitsPerPixel = MacOpenGLUtils_GetFormatSizeInBits(f);
            levelSize = (ew * eh * ed * bitsPerPixel) / 8;
        }

        ImageSizeInBytes += levelSize;

        if (w)
            w >>= 1;
        if (h)
            h >>= 1;
        if (d)
            d >>= 1;
    }

    return ImageSizeInBytes;
}

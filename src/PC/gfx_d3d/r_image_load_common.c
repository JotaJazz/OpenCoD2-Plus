#include "common_types.h"
#include "imports.h"
extern int alwaysfails;
extern DxGlobals dx;

extern void Image_PicmipForSemantic(unsigned char semantic, Picmip *picmip);
extern void Image_Create2DTexture(GfxImage *image, int width, int height, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
extern void Image_Create3DTexture(GfxImage *image, int width, int height, int depth, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
extern void Image_CreateCubeTexture(GfxImage *image, int edgeLen, int mipmapCount, DWORD usage, D3DFORMAT imageFormat, D3DPOOL memPool);
extern void Image_TrackTexture(GfxImage *image, int imageFlags, D3DFORMAT format, int width, int height, int depth);

#define VTABLE_LOCKRECT 19
#define VTABLE_UNLOCKRECT 20

typedef int(D3DVTCC *LockRectFn)(void *texture, int level, D3DLOCKED_RECT *lockedRect, void *rect, int flags);
typedef int(D3DVTCC *LockRectCubeFn)(void *texture, int face, int level, D3DLOCKED_RECT *lockedRect, void *rect, int flags);
typedef int(D3DVTCC *LockBoxFn)(void *texture, int level, D3DLOCKED_BOX *lockedBox, void *box, int flags);
typedef int(D3DVTCC *UnlockRectFn)(void *texture, int level);
typedef int(D3DVTCC *UnlockRectCubeFn)(void *texture, int face, int level);
typedef int(D3DVTCC *UnlockBoxFn)(void *texture, int level);

static inline __attribute__((always_inline)) int Image_Max1(int val)
{
    return val > 1 ? val : 1;
}

static void Image_DebugUploadSample(const GfxImage *image, D3DFORMAT format, int mipLevel,
                                    const byte *src, int srcWidth, int srcHeight)
{
    (void)image;
    (void)format;
    (void)mipLevel;
    (void)src;
    (void)srcWidth;
    (void)srcHeight;
}

void Image_GetPicmip(const GfxImage *image, Picmip *picmip)
{
    if (!image->noPicmip) {
        Image_PicmipForSemantic(image->semantic, picmip);
    } else {
        *(short *)picmip = 0;
    }
}

D3DCUBEMAP_FACES Image_CubemapFace(int face)
{
    return (D3DCUBEMAP_FACES)face;
}

static inline __attribute__((always_inline)) int Image_FormatMemory(D3DFORMAT format, int width, int height, int depth)
{
    switch ((int)format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_D32:
    case D3DFMT_D24S8:
    case D3DFMT_R32F:
        return width * height * depth * 4;
    case D3DFMT_R5G6B5:
    case D3DFMT_A8L8:
    case D3DFMT_D16:
        return width * height * depth * 2;
    case D3DFMT_L8:
    case D3DFMT_A8:
        return width * height * depth;
    case D3DFMT_DXT1:
        return ((width + 3) >> 2) * ((height + 3) >> 2) * depth * 8;
    case D3DFMT_DXT3:
    case D3DFMT_DXT5:
        return ((width + 3) >> 2) * ((height + 3) >> 2) * depth * 16;
    default:
        return 0;
    }
}

int Image_GetCardMemoryAmount(int imageFlags, D3DFORMAT format, int width, int height, int depth)
{
    int memory;

    memory = Image_FormatMemory(format, width, height, depth);

    if (!(imageFlags & 2)) {
        while (width > 1 || height > 1 || depth > 1) {
            width >>= 1;
            width = Image_Max1(width);
            height >>= 1;
            height = Image_Max1(height);
            depth >>= 1;
            depth = Image_Max1(depth);
            memory += Image_FormatMemory(format, width, height, depth);
        }
    }

    if (imageFlags & 4) {
        memory = memory * 6;
    }

    return memory;
}

void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage, D3DFORMAT imageFormat)
{
    int minSize;
    int maxDim;
    int i;
    int mipLimit;
    int picmipLevel;
    int w, h, d;
    int mipmapCount;

    image->noPicmip = (imageFlags & 3) != 0;

    if (image->noPicmip) {
        *(short *)&image->picmip = 0;
    } else {
        Image_PicmipForSemantic(image->semantic, &image->picmip);
    }

    if (imageFormat == D3DFMT_DXT1 || imageFormat == D3DFMT_DXT3 || imageFormat == D3DFMT_DXT5) {
        minSize = 4;
    } else {
        minSize = 1;
    }

    maxDim = width;
    if (height < maxDim) {
        maxDim = height;
    }

    mipLimit = 3;
    for (i = 3; i > 0; i--) {
        if ((maxDim >> i) >= minSize) {
            break;
        }
        mipLimit--;
    }

    for (i = 0; i < 2; i++) {
        if (image->picmip.platform[i] > (byte)mipLimit) {
            image->picmip.platform[i] = (byte)mipLimit;
        }
    }

    picmipLevel = image->picmip.platform[0];
    w = Image_Max1(width >> picmipLevel);
    image->width = (unsigned short)w;
    h = Image_Max1(height >> picmipLevel);
    image->height = (unsigned short)h;
    d = Image_Max1(depth >> picmipLevel);
    image->depth = (unsigned short)d;

    mipmapCount = (imageFlags >> 1) & 1;

    if (imageFlags & 4) {

        Image_CreateCubeTexture(image, w, mipmapCount, usage, imageFormat, (D3DPOOL)(1));
    } else if (imageFlags & 8) {

        Image_Create3DTexture(image, w, h, d, mipmapCount, usage, imageFormat, (D3DPOOL)(1));
    } else {

        Image_Create2DTexture(image, w, h, mipmapCount, usage, imageFormat, (D3DPOOL)(1));
    }

    Image_TrackTexture(image, imageFlags, imageFormat, width, height, depth);
}

static void Image_GetSrcStrideAndDy(D3DFORMAT format, int srcWidth, int *outStride, int *outDy)
{
    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        *outStride = srcWidth * 4;
        *outDy = 1;
        return;
    case D3DFMT_L8:
    case D3DFMT_A8:
        *outStride = srcWidth;
        *outDy = 1;
        return;
    case D3DFMT_R5G6B5:
    case D3DFMT_A8L8:
    case D3DFMT_X8B8G8R8:
        *outStride = srcWidth * 2;
        *outDy = 1;
        return;
    case D3DFMT_DXT1:
        *outStride = ((srcWidth + 3) / 4) * 8;
        *outDy = 4;
        return;
    case D3DFMT_DXT3:
    case D3DFMT_DXT5:
        *outStride = ((srcWidth + 3) / 4) * 16;
        *outDy = 4;
        return;
    default:
        *outStride = 0;
        *outDy = 0;
        return;
    }
}

static void Image_CopyToLockedSurface(byte *dst, int dstPitch, const byte *src, int srcStride, int dy, int height)
{
    int y;

    if (srcStride == dstPitch) {

        int rows = ((height - 1) / dy) + 1;
        memcpy(dst, src, rows * srcStride);
    } else {

        if (height > 0) {
            for (y = 0; y < height; y += dy) {
                memcpy(dst, src, srcStride);
                dst += dstPitch;
                src += srcStride;
            }
        }
    }
}

static int Image_VolumeSlicePitch(D3DFORMAT format, int srcWidth, int srcHeight)
{
    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_D32:
    case D3DFMT_D24S8:
    case D3DFMT_R32F:
        return srcWidth * srcHeight * 4;
    case D3DFMT_R5G6B5:
    case D3DFMT_A8L8:
    case D3DFMT_D16:
        return srcWidth * srcHeight * 2;
    case D3DFMT_L8:
    case D3DFMT_A8:
        return srcWidth * srcHeight;
    case D3DFMT_DXT1:
        return ((srcWidth + 3) / 4) * ((srcHeight + 3) / 4) * 8;
    case D3DFMT_DXT3:
    case D3DFMT_DXT5:
        return ((srcWidth + 3) / 4) * ((srcHeight + 3) / 4) * 16;
    default:
        return 0;
    }
}

void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src)
{
    int srcWidth, srcHeight, srcDepth;
    int srcStride, dy;
    int dstPitch;
    byte *dst;
    D3DLOCKED_RECT lockedRect;
    D3DLOCKED_BOX lockedBox;
    int srcRowPitch;
    int sliceIndex;
    void *texture;
    void **vtable;

    {
        void *dev = dx.device;
        int devvt = dev ? *(int *)dev : 0;
        if (dev == NULL || devvt == 0) {
            return;
        }
    }

    if (image->mapType == 4) {
        /* a volume (the static models' lighting): every slice row by row */
        srcWidth = Image_Max1(image->width >> mipLevel);
        srcHeight = Image_Max1(image->height >> mipLevel);
        srcDepth = Image_Max1(image->depth >> mipLevel);
        Image_GetSrcStrideAndDy(format, srcWidth, &srcRowPitch, &dy);
        if (!srcRowPitch)
            return;
        texture = (void *)image->texture.volmap;
        if (!texture || !*(void **)texture)
            return;
        vtable = *(void ***)texture;
        /* the GL device's volumes cannot be locked (their table is empty) */
        if (!vtable[VTABLE_LOCKRECT] || ((LockBoxFn)vtable[VTABLE_LOCKRECT])(texture, mipLevel, &lockedBox, 0, 0) < 0)
            return;
        for (sliceIndex = 0; sliceIndex < srcDepth; sliceIndex++) {
            Image_CopyToLockedSurface((byte *)lockedBox.pBits + sliceIndex * lockedBox.SlicePitch, lockedBox.RowPitch,
                                      src + sliceIndex * Image_VolumeSlicePitch(format, srcWidth, srcHeight), srcRowPitch, dy, srcHeight);
        }
        ((UnlockBoxFn)vtable[VTABLE_UNLOCKRECT])(texture, mipLevel);
        return;
    }

    if (image->mapType == 5) {

        if (mipLevel != 0 && dx.canMipCubemaps == 0) {
            return;
        }

    }

    srcWidth = Image_Max1(image->width >> mipLevel);
    srcHeight = Image_Max1(image->height >> mipLevel);

    Image_DebugUploadSample(image, format, mipLevel, src, srcWidth, srcHeight);

    if (image->mapType == 3) {

        do {
            texture = (void *)image->texture.map;
            if (!texture || !*(void **)texture)
                return;
            vtable = *(void ***)texture;
            if (!vtable[VTABLE_LOCKRECT])
                return;
            ((LockRectFn)vtable[VTABLE_LOCKRECT])(texture, mipLevel, &lockedRect, 0, 0);
        } while (*(volatile int *)&alwaysfails);
    } else {

        do {
            texture = (void *)image->texture.cubemap;
            if (!texture || !*(void **)texture)
                return;
            vtable = *(void ***)texture;
            if (!vtable[VTABLE_LOCKRECT])
                return;
            ((LockRectCubeFn)vtable[VTABLE_LOCKRECT])(texture, face, mipLevel, &lockedRect, 0, 0);
        } while (*(volatile int *)&alwaysfails);
    }

    dst = (byte *)lockedRect.pBits;
    dstPitch = lockedRect.Pitch;

    switch (format) {
    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
        srcStride = srcWidth * 4;
        dy = 1;
        break;
    case D3DFMT_L8:
    case D3DFMT_A8:
        srcStride = srcWidth;
        dy = 1;
        break;
    case D3DFMT_R5G6B5:
    case D3DFMT_A8L8:
    case D3DFMT_X8B8G8R8:
        srcStride = srcWidth * 2;
        dy = 1;
        break;
    case D3DFMT_DXT1:
        srcStride = ((srcWidth + 3) / 4) * 8;
        dy = 4;
        break;
    case D3DFMT_DXT3:
    case D3DFMT_DXT5:
        srcStride = ((srcWidth + 3) / 4) * 16;
        dy = 4;
        break;
    default:
        goto unlock_2d;
    }

    Image_CopyToLockedSurface(dst, dstPitch, src, srcStride, dy, srcHeight);

unlock_2d:
    if (image->mapType == 3) {

        do {
            texture = (void *)image->texture.map;
            vtable = *(void ***)texture;
            ((UnlockRectFn)vtable[VTABLE_UNLOCKRECT])(texture, mipLevel);
        } while (*(volatile int *)&alwaysfails);
    } else {

        do {
            texture = (void *)image->texture.cubemap;
            vtable = *(void ***)texture;
            ((UnlockRectCubeFn)vtable[VTABLE_UNLOCKRECT])(texture, face, mipLevel);
        } while (*(volatile int *)&alwaysfails);
    }
}

#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

void *vtbl_CDirect3DVolumeTexture[30];

void __ZdlPv(void *ptr);
UINT32 MacOpenGLUtils_GetLevelSizeInBytes(UINT32 Width, UINT32 Height, UINT32 Depth, const D3DFORMAT *f);
UINT32 MacOpenGLUtils_GetNumTextureLevels(UINT32 Width, UINT32 Height, UINT32 Depth);
UINT32 MacOpenGLUtils_GetFormatSizeInBits(const D3DFORMAT *f);
int MacOpenGLUtils_GetOpenGLTextureType(bool *CreateOpenGLResources, GLenum *OpenGLInternalFormat, GLenum *OpenGLFormat, GLenum *OpenGLElementType, D3DFORMAT mFormat);
bool MacOpenGLUtils_IsCompressed(const D3DFORMAT *f);

#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_LINEAR 0x2601
#define GL_REPEAT 0x2901
#define GL_TEXTURE_BINDING_3D 0x806A
#define MAX_MIP_LEVELS 16

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 level;
    UINT32 width;
    UINT32 height;
    UINT32 depth;
    byte *volumeMemory;
    D3DFORMAT format;
    unsigned char isDirty;
    unsigned char pad[3];
    void *owner;
    GLenum glInternalFormat;
    GLenum glFormat;
    GLenum glType;
} CDirect3DVolumeClean;

ULONG CDirect3DVolume_AddRef(const void *_this)
{
    return ++((CDirect3DVolumeClean *)_this)->refCount;
}
ULONG CDirect3DVolume_Release(const void *_this)
{
    return --((CDirect3DVolumeClean *)_this)->refCount;
}
HRESULT CDirect3DVolume_QueryInterface(const void *_this, const IID *iid, void **ppv)
{
    (void)iid;
    *ppv = (void *)_this;
    CDirect3DVolume_AddRef(_this);
    return 0;
}
void ZN15CDirect3DVolumeD1Ev(const void *_this)
{
    (void)_this;
}
void ZN15CDirect3DVolumeD0Ev(const void *_this)
{
    __ZdlPv((void *)_this);
}

HRESULT CDirect3DVolume_GetDesc(const void *_this, D3DVOLUME_DESC *pDesc)
{
    CDirect3DVolumeClean *vol = (CDirect3DVolumeClean *)_this;
    pDesc->Format = vol->format;
    pDesc->Type = D3DRTYPE_VOLUME;
    pDesc->Usage = 0;
    pDesc->Pool = (D3DPOOL)0;
    pDesc->Width = vol->width;
    pDesc->Height = vol->height;
    pDesc->Depth = vol->depth;
    return 0;
}

HRESULT CDirect3DVolume_LockBox(const void *_this, D3DLOCKED_BOX *pLockedVolume, const D3DBOX *pBox, DWORD Flags)
{
    CDirect3DVolumeClean *vol = (CDirect3DVolumeClean *)_this;
    UINT32 bpp;
    (void)pBox;
    (void)Flags;

    if (MacOpenGLUtils_IsCompressed(&vol->format)) {
        UINT32 blockSize = (vol->format == D3DFMT_DXT1) ? 8 : 16;
        pLockedVolume->RowPitch = ((vol->width + 3) / 4) * blockSize;
        pLockedVolume->SlicePitch = pLockedVolume->RowPitch * ((vol->height + 3) / 4);
    } else {
        bpp = MacOpenGLUtils_GetFormatSizeInBits(&vol->format);
        pLockedVolume->RowPitch = (vol->width * bpp) >> 3;
        pLockedVolume->SlicePitch = pLockedVolume->RowPitch * vol->height;
    }
    pLockedVolume->pBits = vol->volumeMemory;
    vol->isDirty = 1;
    return 0;
}

HRESULT CDirect3DVolume_UnlockBox(const void *_this)
{
    (void)_this;
    return 0;
}
bool CDirect3DVolume_IsDirty(const void *_this)
{
    return ((CDirect3DVolumeClean *)_this)->isDirty;
}

void CDirect3DVolume_CreateOpenGLSurfaceObject(const void *_this)
{
    CDirect3DVolumeClean *vol = (CDirect3DVolumeClean *)_this;
    UINT32 w = vol->width ? vol->width : 1;
    UINT32 h = vol->height ? vol->height : 1;
    UINT32 d = vol->depth ? vol->depth : 1;

    if (!vol->volumeMemory)
        return;

    if (MacOpenGLUtils_IsCompressed(&vol->format)) {
        UINT32 dataSize = MacOpenGLUtils_GetLevelSizeInBytes(w, h, d, &vol->format);
        glCompressedTexImage3DARB(GL_TEXTURE_3D, vol->level,
                                  vol->glInternalFormat, w, h, d, 0, dataSize, vol->volumeMemory);
    } else {
        glTexImage3D(GL_TEXTURE_3D, vol->level,
                     vol->glInternalFormat, w, h, d, 0,
                     vol->glFormat, vol->glType, vol->volumeMemory);
    }
}

void CDirect3DVolume_UpdateOpenGLSurfaceObject(const void *_this, int bRecreate)
{
    CDirect3DVolumeClean *vol = (CDirect3DVolumeClean *)_this;
    UINT32 w = vol->width ? vol->width : 1;
    UINT32 h = vol->height ? vol->height : 1;
    UINT32 d = vol->depth ? vol->depth : 1;

    if (!vol->volumeMemory)
        return;

    if (bRecreate) {
        CDirect3DVolume_CreateOpenGLSurfaceObject(_this);
    } else if (MacOpenGLUtils_IsCompressed(&vol->format)) {
        UINT32 dataSize = MacOpenGLUtils_GetLevelSizeInBytes(w, h, d, &vol->format);
        glCompressedTexSubImage3D(GL_TEXTURE_3D, vol->level, 0, 0, 0,
                                  w, h, d, vol->glInternalFormat, dataSize, vol->volumeMemory);
    } else {
        glTexSubImage3D(GL_TEXTURE_3D, vol->level, 0, 0, 0,
                        w, h, d, vol->glFormat, vol->glType, vol->volumeMemory);
    }
    vol->isDirty = 0;
}

HRESULT CDirect3DVolume_GetDevice(const void *t, void *p)
{
    (void)t;
    (void)p;
    return 0;
}
HRESULT CDirect3DVolume_SetPrivateData(const void *t, const GUID *g, const void *d, DWORD s, DWORD f)
{
    (void)t;
    (void)g;
    (void)d;
    (void)s;
    (void)f;
    return 0;
}
HRESULT CDirect3DVolume_GetPrivateData(const void *t, const GUID *g, void *d, DWORD *s)
{
    (void)t;
    (void)g;
    (void)d;
    (void)s;
    return 0;
}
HRESULT CDirect3DVolume_FreePrivateData(const void *t, const GUID *g)
{
    (void)t;
    (void)g;
    return 0;
}
HRESULT CDirect3DVolume_GetContainer(const void *t, const IID *r, void **p)
{
    (void)t;
    (void)r;
    (void)p;
    return 0;
}

typedef struct {
    void **primaryVtable;
    void **secondaryVtable;
    GLuint *mpTexID;
    GLenum mTexWrapS;
    GLenum mTexWrapT;
    GLenum mTexWrapR;
    GLuint mTexBorderColor;
    GLenum mTexMinFilter;
    GLenum mTexMagFilter;
    GLfloat mTexAniso;
    UINT32 reserved;
    GLint glWidth;
    GLint glHeight;
    GLint glDepth;
    ULONG refCount;
    UINT32 width;
    UINT32 height;
    UINT32 levelCount;
    D3DFORMAT format;
    UINT32 depth;
    void **volumes;
    byte *pixelData;
    GLuint texIDStorage;
} CDirect3DVolumeTextureClean;

ULONG CDirect3DVolumeTexture_AddRef(const CDirect3DVolumeTexture *_this);
void ZN22CDirect3DVolumeTextureD0Ev(void *_this);
void ZN22CDirect3DVolumeTextureD1Ev(void *_this);

ULONG CDirect3DVolumeTexture_AddRef(const CDirect3DVolumeTexture *_this)
{
    return ++((CDirect3DVolumeTextureClean *)_this)->refCount;
}

HRESULT CDirect3DVolumeTexture_QueryInterface(const CDirect3DVolumeTexture *_this, const IID *iid, void **ppvObj)
{
    (void)iid;
    *ppvObj = (void *)_this;
    CDirect3DVolumeTexture_AddRef(_this);
    return 0;
}

ULONG CDirect3DVolumeTexture_Release(const CDirect3DVolumeTexture *_this)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    ULONG rc = --tex->refCount;
    if (!rc)
        ZN22CDirect3DVolumeTextureD0Ev((void *)_this);
    return rc;
}

HRESULT CDirect3DVolumeTexture_GetLevelDesc(const CDirect3DVolumeTexture *_this, UINT Level, D3DVOLUME_DESC *pDesc)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    if (Level >= tex->levelCount)
        return -1;
    return CDirect3DVolume_GetDesc(tex->volumes[Level], pDesc);
}

HRESULT CDirect3DVolumeTexture_GetVolumeLevel(const CDirect3DVolumeTexture *_this, UINT Level, void **ppVolumeLevel)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    if (Level >= tex->levelCount)
        return -1;
    CDirect3DVolume_AddRef(tex->volumes[Level]);
    *ppVolumeLevel = tex->volumes[Level];
    return 0;
}

HRESULT CDirect3DVolumeTexture_LockBox(const CDirect3DVolumeTexture *_this, UINT Level, D3DLOCKED_BOX *pLockedVolume, const D3DBOX *pBox, DWORD Flags)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    if (Level >= tex->levelCount)
        return -1;
    return CDirect3DVolume_LockBox(tex->volumes[Level], pLockedVolume, pBox, Flags);
}

HRESULT CDirect3DVolumeTexture_UnlockBox(const CDirect3DVolumeTexture *_this, UINT Level)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    CDirect3DVolumeClean *vol;
    if (Level >= tex->levelCount)
        return -1;
    vol = (CDirect3DVolumeClean *)tex->volumes[Level];

    if (vol->isDirty) {
        int prevTex = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_3D, &prevTex);
        glBindTexture(GL_TEXTURE_3D, tex->texIDStorage);
        CDirect3DVolume_UpdateOpenGLSurfaceObject(vol, 0);
        glBindTexture(GL_TEXTURE_3D, prevTex);
    }
    return 0;
}

HRESULT CDirect3DVolumeTexture_AddDirtyBox(const CDirect3DVolumeTexture *_this, const D3DBOX *pBox)
{
    (void)_this;
    (void)pBox;
    return 0;
}
DWORD CDirect3DVolumeTexture_SetLOD(const CDirect3DVolumeTexture *_this, DWORD LODNew)
{
    (void)_this;
    (void)LODNew;
    return 0;
}
DWORD CDirect3DVolumeTexture_GetLOD(const CDirect3DVolumeTexture *_this)
{
    (void)_this;
    return 0;
}
DWORD CDirect3DVolumeTexture_GetLevelCount(const CDirect3DVolumeTexture *_this)
{
    return ((CDirect3DVolumeTextureClean *)_this)->levelCount;
}
D3DRESOURCETYPE CDirect3DVolumeTexture_GetType(const CDirect3DVolumeTexture *_this)
{
    (void)_this;
    return D3DRTYPE_VOLUMETEXTURE;
}
HRESULT CDirect3DVolumeTexture_SetAutoGenFilterType(const CDirect3DVolumeTexture *_this, D3DTEXTUREFILTERTYPE f)
{
    (void)_this;
    (void)f;
    return 0;
}
D3DTEXTUREFILTERTYPE CDirect3DVolumeTexture_GetAutoGenFilterType(const CDirect3DVolumeTexture *_this)
{
    (void)_this;
    return (D3DTEXTUREFILTERTYPE)0;
}
void CDirect3DVolumeTexture_GenerateMipSubLevels(const CDirect3DVolumeTexture *_this)
{
    (void)_this;
}

void ZN22CDirect3DVolumeTextureD1Ev(void *_this)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    UINT32 i;
    if (tex->volumes) {
        for (i = 0; i < tex->levelCount; i++)
            if (tex->volumes[i])
                __ZdlPv(tex->volumes[i]);
        free(tex->volumes);
        tex->volumes = NULL;
    }
    if (tex->texIDStorage) {
        glDeleteTextures(1, (const unsigned int *)&tex->texIDStorage);
        tex->texIDStorage = 0;
    }
    free(tex->pixelData);
    tex->pixelData = NULL;
}

void ZN22CDirect3DVolumeTextureD0Ev(void *_this)
{
    ZN22CDirect3DVolumeTextureD1Ev(_this);
    free(_this);
}
void ZN22CDirect3DVolumeTextureD2Ev(void *_this)
{
    ZN22CDirect3DVolumeTextureD1Ev(_this);
}

void CDirect3DVolumeTexture_CDirect3DVolumeTexture(const CDirect3DVolumeTexture *_this, UINT32 Width, UINT32 Height, UINT32 Depth, UINT32 Levels, DWORD Usage, D3DFORMAT Format)
{
    CDirect3DVolumeTextureClean *tex = (CDirect3DVolumeTextureClean *)_this;
    UINT32 i, w, h, d, totalSize, offset;
    bool createGL = 0;
    GLenum internalFmt = 0, glFmt = 0, glType = 0;
    int prevTex = 0;
    (void)Usage;

    memset(tex, 0, sizeof(*tex));
    tex->primaryVtable = vtbl_CDirect3DVolumeTexture;
    tex->mpTexID = &tex->texIDStorage;
    tex->mTexWrapS = GL_REPEAT;
    tex->mTexWrapT = GL_REPEAT;
    tex->mTexMinFilter = GL_LINEAR;
    tex->mTexMagFilter = GL_LINEAR;
    tex->mTexAniso = 1.0f;
    tex->glWidth = Width;
    tex->glHeight = Height;
    tex->glDepth = Depth;
    tex->refCount = 1;
    tex->width = Width;
    tex->height = Height;
    tex->depth = Depth;
    tex->format = Format;

    if (Levels == 0)
        tex->levelCount = MacOpenGLUtils_GetNumTextureLevels(Width, Height, Depth);
    else
        tex->levelCount = Levels;
    if (tex->levelCount > MAX_MIP_LEVELS)
        tex->levelCount = MAX_MIP_LEVELS;

    MacOpenGLUtils_GetOpenGLTextureType(&createGL, &internalFmt, &glFmt, &glType, Format);

    glGenTextures(1, (unsigned int *)&tex->texIDStorage);
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &prevTex);
    glBindTexture(GL_TEXTURE_3D, tex->texIDStorage);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    totalSize = 0;
    w = Width;
    h = Height;
    d = Depth;
    for (i = 0; i < tex->levelCount; i++) {
        totalSize += MacOpenGLUtils_GetLevelSizeInBytes(w ? w : 1, h ? h : 1, d ? d : 1, &Format);
        w >>= 1;
        h >>= 1;
        d >>= 1;
    }
    tex->pixelData = (byte *)calloc(1, totalSize ? totalSize : 1);

    tex->volumes = (void **)calloc(tex->levelCount, sizeof(void *));
    offset = 0;
    w = Width;
    h = Height;
    d = Depth;
    for (i = 0; i < tex->levelCount; i++) {
        UINT32 lw = w ? w : 1, lh = h ? h : 1, ld = d ? d : 1;
        UINT32 levelSize = MacOpenGLUtils_GetLevelSizeInBytes(lw, lh, ld, &Format);
        CDirect3DVolumeClean *vol = (CDirect3DVolumeClean *)calloc(1, sizeof(CDirect3DVolumeClean));
        vol->refCount = 1;
        vol->level = i;
        vol->width = lw;
        vol->height = lh;
        vol->depth = ld;
        vol->volumeMemory = tex->pixelData + offset;
        vol->format = Format;
        vol->glInternalFormat = internalFmt;
        vol->glFormat = glFmt;
        vol->glType = glType;
        tex->volumes[i] = vol;

        if (createGL)
            CDirect3DVolume_CreateOpenGLSurfaceObject(vol);

        offset += levelSize;
        w >>= 1;
        h >>= 1;
        d >>= 1;
    }

    glBindTexture(GL_TEXTURE_3D, prevTex);
}

void CDirect3DVolumeTexture_UpdateOpenGLSurfaces(const CDirect3DVolumeTexture *_this)
{
    (void)_this;
}

HRESULT CDirect3DVolumeTexture_GetDevice(const CDirect3DVolumeTexture *t, void *p)
{
    (void)t;
    (void)p;
    return 0;
}
HRESULT CDirect3DVolumeTexture_SetPrivateData(const CDirect3DVolumeTexture *t, const GUID *g, const void *d, DWORD s, DWORD f)
{
    (void)t;
    (void)g;
    (void)d;
    (void)s;
    (void)f;
    return 0;
}
HRESULT CDirect3DVolumeTexture_GetPrivateData(const CDirect3DVolumeTexture *t, const GUID *g, void *d, DWORD *s)
{
    (void)t;
    (void)g;
    (void)d;
    (void)s;
    return 0;
}
HRESULT CDirect3DVolumeTexture_FreePrivateData(const CDirect3DVolumeTexture *t, const GUID *g)
{
    (void)t;
    (void)g;
    return 0;
}
DWORD CDirect3DVolumeTexture_SetPriority(const CDirect3DVolumeTexture *t, DWORD p)
{
    (void)t;
    (void)p;
    return 0;
}
DWORD CDirect3DVolumeTexture_GetPriority(const CDirect3DVolumeTexture *t)
{
    (void)t;
    return 0;
}
void CDirect3DVolumeTexture_PreLoad(const CDirect3DVolumeTexture *t)
{
    (void)t;
}

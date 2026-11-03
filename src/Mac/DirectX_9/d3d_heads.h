#ifndef D3D_HEADS_H
#define D3D_HEADS_H

/* The fields of the emulated Direct3D objects that other files read: the start of
   CDirect3DVertexBuffer.c's and CDirect3DIndexBuffer.c's buffers, and of
   CDirect3DTexture.c's and CDirect3DCubeTexture.c's textures up to their GL id. Those
   files check that their own structs agree. Pointers are 8 bytes on x64, so the offsets
   1.3 has (data +12, the texture id +0x54) are not x64's. */
typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 lengthBytes;
    byte *data;
} D3DBufferHead;

typedef struct {
    void **primaryVtable;
    void **secondaryVtable;
    GLuint *mpTexID;
    GLenum mTexWrapS;   /* the Mac's GL types: longs on x86, 32 bits on x64 */
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
    UINT32 size[3];
    D3DFORMAT format;
    void **surfaces;
    byte *pixelData;
    GLuint texIDStorage;
} D3DTextureHead;

#define D3D_HEAD_CHECK(T, H, f) typedef char d3d_head_check_##T##_##f[offsetof(T, f) == offsetof(H, f) ? 1 : -1]

#endif

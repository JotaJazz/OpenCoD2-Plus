#ifndef MAC_DIRECTX_9_CDIRECT3DVERTEXBUFFER_H_H
#define MAC_DIRECTX_9_CDIRECT3DVERTEXBUFFER_H_H

#ifdef __cplusplus
extern "C" {
#endif

struct CCacheInfoBlock;
struct CDirect3DVertexBuffer;
struct CFence;
struct CMemoryBuffer;
struct COpenGL;
struct COpenGLTexture;
struct CStaticCacheInfo;
struct ParameterType;
struct TextureInfoType;
struct _List_impl;
struct _List_iterator_CFenceptr;
struct _List_iterator_GLuintptr;
struct _List_node_CFenceptr;
struct _List_node_GLuintptr;
struct _List_node_base;
struct allocator_CCacheInfoBlock;
struct allocator_CFenceptr;
struct allocator_GLuintptr;
struct allocator_GLuint;
struct new_allocator_CCacheInfoBlock;
struct new_allocator_CFenceptr;
struct new_allocator_GLuintptr;
struct new_allocator_GLuint;

enum OwnershipType {
    IS_OWNER = 0,
    IS_NOT_OWNER = 1
};

struct CCacheInfoBlock {
    int _vptr$CCacheInfoBlock;
};

struct CDirect3DVertexBuffer {
    int _placeholder;
};

struct CFence {
    int _placeholder;
};

struct CMemoryBuffer {
    int _vptr$CMemoryBuffer;
};

struct COpenGL {
    int _vptr$COpenGL;
};

struct COpenGLTexture {
    int _vptr$COpenGLTexture;
};

struct CStaticCacheInfo {
    int _vptr$CStaticCacheInfo;
};

struct ParameterType {
    UINT32 NeedsValidation;
    UINT32 v[4];
};

struct TextureInfoType {
    const GLuint *mpTexID;
    GLenum mTexWrapS;
    GLenum mTexWrapT;
    GLenum mTexWrapR;
    GLuint mTexBorderColor;
    GLenum mTexMinFilter;
    GLenum mTexMagFilter;
    GLint mTexLastLevel;
    GLfloat mTexAnisotropicFilter;
};

struct _List_impl {
    struct _List_node_base _M_node;
};

struct _List_iterator_CFenceptr {
    _List_node_base *_M_node;
};

struct _List_iterator_GLuintptr {
    _List_node_base *_M_node;
};

struct _List_node_CFenceptr {
    CFence *_M_data;
};

struct _List_node_GLuintptr {
    GLuint *_M_data;
};

struct _List_node_base {
    int _M_next;
    int _M_prev;
};

struct allocator_CCacheInfoBlock {
    int _placeholder;
};

struct allocator_CFenceptr {
    int _placeholder;
};

struct allocator_GLuintptr {
    int _placeholder;
};

struct allocator_GLuint {
    int _placeholder;
};

struct new_allocator_CCacheInfoBlock {
    int _placeholder;
};

struct new_allocator_CFenceptr {
    int _placeholder;
};

struct new_allocator_GLuintptr {
    int _placeholder;
};

struct new_allocator_GLuint {
    int _placeholder;
};

#ifdef __cplusplus
}
#endif
#endif

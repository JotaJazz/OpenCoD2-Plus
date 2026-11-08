#include "common_types.h"
#include "imports.h"

typedef struct {
    void **vtable;
    unsigned char enabled;
    unsigned char needsValidation;
    unsigned short padding;
    GLint size;
    GLenum type;
    GLsizei stride;
    const void *pointer;
} CBaseVAImpl;

typedef struct {
    void *reserved0;
    void *reserved1;
    void (*enable)(const CBaseVA *self);
    void (*disable)(const CBaseVA *self);
} CBaseVAVTable;

extern void *imp___ZTV7CBaseVA;

static CBaseVAImpl *CBaseVA_GetImpl(const CBaseVA *self)
{
    return (CBaseVAImpl *)self;
}

static const CBaseVAVTable *CBaseVA_GetVTable(const CBaseVA *self)
{
    return (const CBaseVAVTable *)CBaseVA_GetImpl(self)->vtable;
}

static void CBaseVA_ResetState(CBaseVAImpl *base)
{
    base->enabled = 0;
    base->needsValidation = 1;
    base->size = 0;
    base->type = 0x1406;
    base->stride = 0;
    base->pointer = NULL;
}

static void **CBaseVA_GetBaseVTable(void)
{
    return (void **)((char *)imp___ZTV7CBaseVA + 2 * sizeof(void *));
}

void CBaseVA_CBaseVA(const CBaseVA *_this);
void CBaseVA_Reset(const CBaseVA *_this);
void CBaseVA_Enable(const CBaseVA *_this, GLenum VA);
void CBaseVA_Disable(const CBaseVA *_this, GLenum VA);
void CTexCoordArray_SetGuard(const CTexCoordArray *_this, int vEnable);
void CBaseVA_Force(const CBaseVA *_this);
void CColorArray_Set(const CColorArray *_this, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size);
void CColorArray_SetGuard(const CColorArray *_this, int vEnable, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size);
void CNormalArray_Set(const CNormalArray *_this, GLenum Type, GLsizei Stride, const void *Pointer);
void CNormalArray_SetGuard(const CNormalArray *_this, int vEnable, GLenum Type, GLsizei Stride, const void *Pointer);
void CVertexArray_Set(const CVertexArray *_this, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size);
void CVertexArray_SetGuard(const CVertexArray *_this, int vEnable, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size);
void CTexCoordArray_Set(const CTexCoordArray *_this, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size);

void CBaseVA_CBaseVA(const CBaseVA *_this)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl(_this);
    base->vtable = CBaseVA_GetBaseVTable();
    CBaseVA_ResetState(base);
}

void CBaseVA_Reset(const CBaseVA *_this)
{
    CBaseVA_ResetState(CBaseVA_GetImpl(_this));
}

void CBaseVA_Enable(const CBaseVA *_this, GLenum VA)
{
    CBaseVA_GetImpl(_this)->enabled = 1;
    glEnableClientState(VA);
}

void CBaseVA_Disable(const CBaseVA *_this, GLenum VA)
{
    CBaseVA_GetImpl(_this)->enabled = 0;
    glDisableClientState(VA);
}

void CTexCoordArray_SetGuard(const CTexCoordArray *_this, int vEnable)
{
    CBaseVA_GetImpl((const CBaseVA *)_this)->enabled = (unsigned char)vEnable;
}

void CBaseVA_Force(const CBaseVA *_this)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl(_this);
    if (base->enabled) {
        CBaseVA_GetVTable(_this)->enable(_this);
    } else {
        CBaseVA_GetVTable(_this)->disable(_this);
    }
    base->needsValidation = 0;
}

void CColorArray_Set(const CColorArray *_this, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = Size;
    glColorPointer(Size, Type, Stride, Pointer);
}

void CColorArray_SetGuard(const CColorArray *_this, int vEnable, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->enabled = (unsigned char)vEnable;
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = Size;
}

void CNormalArray_Set(const CNormalArray *_this, GLenum Type, GLsizei Stride, const void *Pointer)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = 3;
    glNormalPointer(Type, Stride, Pointer);
}

void CNormalArray_SetGuard(const CNormalArray *_this, int vEnable, GLenum Type, GLsizei Stride, const void *Pointer)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->enabled = (unsigned char)vEnable;
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = 3;
}

void CVertexArray_Set(const CVertexArray *_this, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = Size;
    glVertexPointer(Size, Type, Stride, Pointer);
}

void CVertexArray_SetGuard(const CVertexArray *_this, int vEnable, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->enabled = (unsigned char)vEnable;
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = Size;
}

void CTexCoordArray_Set(const CTexCoordArray *_this, GLenum Type, GLsizei Stride, const void *Pointer, GLint Size)
{
    CBaseVAImpl *base;

    base = CBaseVA_GetImpl((const CBaseVA *)_this);
    base->type = Type;
    base->stride = Stride;
    base->pointer = Pointer;
    base->size = Size;
    glTexCoordPointer(Size, Type, Stride, Pointer);
}

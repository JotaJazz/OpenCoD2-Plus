#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 elementCount;
    D3DVERTEXELEMENT9 *elements;
} CDirect3DVertexDeclarationImpl;
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DVertexDeclaration[];

static UINT32 CDirect3DVertexDeclaration_TypeToken(const D3DVERTEXELEMENT9 *element, UINT32 *currentOffset)
{
    switch (element->Type) {
    case 0:
        *currentOffset += 4;
        return 0x00000000;
    case 1:
        *currentOffset += 8;
        return 0x00010000;
    case 2:
        *currentOffset += 12;
        return 0x00020000;
    case 3:
        *currentOffset += 16;
        return 0x00030000;
    case 4:
        *currentOffset += 4;
        return 0x00040000;
    case 5:
        *currentOffset += 4;
        return 0x00050000;
    case 6:
        *currentOffset += 4;
        return 0x00060000;
    case 7:
        *currentOffset += 8;
        return 0x00070000;
    default:
        return 0;
    }
}

static UINT32 CDirect3DVertexDeclaration_UsageRegister(const D3DVERTEXELEMENT9 *element)
{
    switch (element->Usage) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 4;
    case 5:
        return (UINT32)element->UsageIndex + 8;
    case 6:
        return 7;
    case 9:
        return 0;
    case 10:
        return 5;
    default:
        return 0;
    }
}

ULONG CDirect3DVertexDeclaration_AddRef(const CDirect3DVertexDeclaration *_this);
void ZN26CDirect3DVertexDeclarationD1Ev(const CDirect3DVertexDeclaration *_this);
void ZN26CDirect3DVertexDeclarationD0Ev(const CDirect3DVertexDeclaration *_this);
HRESULT CDirect3DVertexDeclaration_QueryInterface(const CDirect3DVertexDeclaration *_this, const IID *iid, HRESULT (*ppvObj)());
ULONG CDirect3DVertexDeclaration_Release(const CDirect3DVertexDeclaration *_this);
HRESULT CDirect3DVertexDeclaration_GetDeclaration(const CDirect3DVertexDeclaration *_this, LONG (*pElement)(), UINT *pNumElements);
int CDirect3DVertexDeclaration_GetShaderDeclaration(const CDirect3DVertexDeclaration *_this, const UINT32 *pTokenStream);
int CDirect3DVertexDeclaration_CDirect3DVertexDeclaration(const CDirect3DVertexDeclaration *_this, const D3DVERTEXELEMENT9 *pVertexElements);
HRESULT CDirect3DVertexDeclaration_GetDevice(const CDirect3DVertexDeclaration *_this, IDirect3DDevice9 **ppDevice);

ULONG CDirect3DVertexDeclaration_AddRef(const CDirect3DVertexDeclaration *_this)
{
    CDirect3DVertexDeclarationImpl *declaration;

    declaration = (CDirect3DVertexDeclarationImpl *)_this;
    ++declaration->refCount;
    return declaration->refCount;
}

HRESULT CDirect3DVertexDeclaration_QueryInterface(const CDirect3DVertexDeclaration *_this, const IID *iid, HRESULT (*ppvObj)())
{
    (void)iid;

    *(void **)ppvObj = (void *)_this;
    CDirect3DVertexDeclaration_AddRef(_this);
    return 0;
}

ULONG CDirect3DVertexDeclaration_Release(const CDirect3DVertexDeclaration *_this)
{
    CDirect3DVertexDeclarationImpl *declaration;
    ULONG refCount;

    declaration = (CDirect3DVertexDeclarationImpl *)_this;
    refCount = --declaration->refCount;
    if (!refCount) {
        ZN26CDirect3DVertexDeclarationD0Ev(_this);
    }

    return refCount;
}

void ZN26CDirect3DVertexDeclarationD1Ev(const CDirect3DVertexDeclaration *_this)
{
    CDirect3DVertexDeclarationImpl *declaration;

    declaration = (CDirect3DVertexDeclarationImpl *)_this;
    declaration->vtable = (void**)vtbl_CDirect3DVertexDeclaration;
    if (declaration->elements) {
        free(declaration->elements);
    }
}

void ZN26CDirect3DVertexDeclarationD0Ev(const CDirect3DVertexDeclaration *_this)
{
    ZN26CDirect3DVertexDeclarationD1Ev(_this);
    free((void *)_this);
}

HRESULT CDirect3DVertexDeclaration_GetDeclaration(const CDirect3DVertexDeclaration *_this, LONG (*pElement)(), UINT *pNumElements)
{
    CDirect3DVertexDeclarationImpl *declaration;

    declaration = (CDirect3DVertexDeclarationImpl *)_this;
    if (pElement && declaration->elementCount) {
        memcpy((void*)pElement, declaration->elements,
               declaration->elementCount * sizeof(D3DVERTEXELEMENT9));
    }

    *pNumElements = declaration->elementCount;
    return 0;
}

int CDirect3DVertexDeclaration_GetShaderDeclaration(const CDirect3DVertexDeclaration *_this, const UINT32 *pTokenStream)
{
    CDirect3DVertexDeclarationImpl *declaration;
    UINT32 *tokenStream;
    unsigned char isUsed[16];
    UINT32 currentOffset;
    UINT32 lastStream;
    UINT32 tokenIndex;
    UINT32 i;

    declaration = (CDirect3DVertexDeclarationImpl *)_this;
    tokenStream = (UINT32 *)pTokenStream;
    memset(isUsed, 0, sizeof(isUsed));

    if (declaration->elementCount == 1) {
        tokenStream[0] = 0xffffffff;
        return 0;
    }

    currentOffset = 0;
    lastStream = 0xffffffff;
    tokenIndex = 0;

    for (i = 0; i + 1 < declaration->elementCount; ++i) {
        const D3DVERTEXELEMENT9 *element;
        UINT32 streamToken;
        UINT32 typeToken;
        UINT32 regIndex;

        element = &declaration->elements[i];
        streamToken = 0;

        if ((UINT32)element->Stream != lastStream) {
            lastStream = element->Stream;
            currentOffset = 0;
            streamToken = 1;
        }

        if ((UINT32)element->Offset != currentOffset) {
            UINT32 amountToSkip;

            amountToSkip = element->Offset - currentOffset;
            while (amountToSkip > 0xf) {
                tokenStream[tokenIndex++] = 0x500f0000;
                amountToSkip -= 0xf;
            }

            if (amountToSkip) {
                tokenStream[tokenIndex++] = 0x50000000 | (amountToSkip << 16);
            }

            currentOffset = element->Offset;
        }

        if (streamToken) {
            tokenStream[tokenIndex++] = 0x20000000 | (UINT32)element->Stream;
        }

        typeToken = CDirect3DVertexDeclaration_TypeToken(element, &currentOffset);
        regIndex = CDirect3DVertexDeclaration_UsageRegister(element);
        if (regIndex < 16) {
            isUsed[regIndex] = 1;
        }

        tokenStream[tokenIndex++] = 0x40000000 | typeToken | regIndex;
    }

    tokenStream[tokenIndex] = 0xffffffff;
    return 0;
}

int CDirect3DVertexDeclaration_CDirect3DVertexDeclaration(const CDirect3DVertexDeclaration *_this, const D3DVERTEXELEMENT9 *pVertexElements)
{
    CDirect3DVertexDeclarationImpl *declaration;
    UINT32 count;

    declaration = (CDirect3DVertexDeclarationImpl *)_this;
    declaration->vtable = (void**)vtbl_CDirect3DVertexDeclaration;
    declaration->elementCount = 0;
    declaration->elements = NULL;
    declaration->refCount = 1;

    count = 0;
    while (pVertexElements[count].Stream != 0xff ||
           pVertexElements[count].Offset != 0 ||
           pVertexElements[count].Type != 0x11 ||
           pVertexElements[count].Method != 0) {
        ++count;
    }
    ++count;

    declaration->elementCount = count;
    declaration->elements = (D3DVERTEXELEMENT9 *)malloc(count * sizeof(D3DVERTEXELEMENT9));
    if (declaration->elements) {
        memcpy(declaration->elements, pVertexElements, count * sizeof(D3DVERTEXELEMENT9));
    }

    return 0;
}

HRESULT CDirect3DVertexDeclaration_GetDevice(const CDirect3DVertexDeclaration *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    (void)ppDevice;
    return 0;
}

fnptr_t vtbl_CDirect3DVertexDeclaration[] = { (fnptr_t)CDirect3DVertexDeclaration_QueryInterface, (fnptr_t)CDirect3DVertexDeclaration_AddRef, (fnptr_t)CDirect3DVertexDeclaration_Release, (fnptr_t)CDirect3DVertexDeclaration_GetDevice, (fnptr_t)CDirect3DVertexDeclaration_GetDeclaration, (fnptr_t)ZN26CDirect3DVertexDeclarationD1Ev, (fnptr_t)ZN26CDirect3DVertexDeclarationD0Ev };

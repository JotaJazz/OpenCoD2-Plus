#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DIndexBuffer[];

typedef struct {
    void **vtable;
    ULONG refCount;
    UINT32 lengthBytes;
    byte *data;
    UINT32 indexSizeBytes;
    DWORD usage;
    byte *lockPtr;
    UINT32 lockSize;
    unsigned char isLocked;
} CDirect3DIndexBufferClean;

ULONG CDirect3DIndexBuffer_AddRef(const CDirect3DIndexBuffer *_this);
void ZN20CDirect3DIndexBufferD0Ev(const CDirect3DIndexBuffer *_this);
void ZN20CDirect3DIndexBufferD1Ev(const CDirect3DIndexBuffer *_this);

ULONG CDirect3DIndexBuffer_AddRef(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    return ++ib->refCount;
}

HRESULT CDirect3DIndexBuffer_QueryInterface(const CDirect3DIndexBuffer *_this, const IID *iid, LPVOID *ppvObj)
{
    (void)iid;
    *ppvObj = (LPVOID)_this;
    CDirect3DIndexBuffer_AddRef(_this);
    return 0;
}

ULONG CDirect3DIndexBuffer_Release(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    ULONG rc = --ib->refCount;
    if (!rc) {
        ZN20CDirect3DIndexBufferD0Ev(_this);
    }
    return rc;
}

void ZN20CDirect3DIndexBufferD1Ev(const CDirect3DIndexBuffer *_this)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    ib->vtable = (void**)vtbl_CDirect3DIndexBuffer;
    free(ib->data);
    ib->data = NULL;
}

void ZN20CDirect3DIndexBufferD0Ev(const CDirect3DIndexBuffer *_this)
{
    ZN20CDirect3DIndexBufferD1Ev(_this);
    free((void *)_this);
}

HRESULT CDirect3DIndexBuffer_Lock(const CDirect3DIndexBuffer *_this, UINT OffsetToLock, UINT SizeToLock, void **ppbData, DWORD Flags)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    (void)Flags;
    ib->isLocked = 1;
    ib->lockSize = SizeToLock ? SizeToLock : ib->lengthBytes;
    ib->lockPtr = ib->data + OffsetToLock;
    *ppbData = ib->lockPtr;
    return 0;
}

HRESULT CDirect3DIndexBuffer_Unlock(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
    return 0;
}

HRESULT CDirect3DIndexBuffer_GetDesc(const CDirect3DIndexBuffer *_this, D3DINDEXBUFFER_DESC *pDesc)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    pDesc->Format = ib->indexSizeBytes == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
    pDesc->Type = D3DRTYPE_INDEXBUFFER;
    pDesc->Usage = ib->usage;
    pDesc->Pool = (D3DPOOL)0;
    pDesc->Size = ib->lengthBytes;
    return 0;
}

void CDirect3DIndexBuffer_CDirect3DIndexBuffer(const CDirect3DIndexBuffer *_this, UINT32 Length, D3DFORMAT Format, DWORD Usage, D3DPOOL Pool)
{
    CDirect3DIndexBufferClean *ib = (CDirect3DIndexBufferClean *)_this;
    (void)Pool;
    ib->vtable = (void**)vtbl_CDirect3DIndexBuffer;
    ib->refCount = 1;
    ib->lengthBytes = (Length + 3) & ~3U;
    ib->data = (byte *)calloc(1, ib->lengthBytes);

    ib->indexSizeBytes = (Format == D3DFMT_INDEX16) ? 2 : 4;
    ib->usage = Usage;
    ib->lockPtr = NULL;
    ib->lockSize = 0;
    ib->isLocked = 0;
}

HRESULT CDirect3DIndexBuffer_GetDevice(const CDirect3DIndexBuffer *_this, IDirect3DDevice9 **ppDevice)
{
    (void)_this;
    (void)ppDevice;
    return 0;
}

HRESULT CDirect3DIndexBuffer_SetPrivateData(const CDirect3DIndexBuffer *_this, const GUID *refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)SizeOfData;
    (void)Flags;
    return 0;
}

HRESULT CDirect3DIndexBuffer_GetPrivateData(const CDirect3DIndexBuffer *_this, const GUID *refguid, void *pData, DWORD *pSizeOfData)
{
    (void)_this;
    (void)refguid;
    (void)pData;
    (void)pSizeOfData;
    return 0;
}

HRESULT CDirect3DIndexBuffer_FreePrivateData(const CDirect3DIndexBuffer *_this, const GUID *refguid)
{
    (void)_this;
    (void)refguid;
    return 0;
}

DWORD CDirect3DIndexBuffer_SetPriority(const CDirect3DIndexBuffer *_this, DWORD PriorityNew)
{
    (void)_this;
    (void)PriorityNew;
    return 0;
}

DWORD CDirect3DIndexBuffer_GetPriority(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
    return 0;
}

void CDirect3DIndexBuffer_PreLoad(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
}

D3DRESOURCETYPE CDirect3DIndexBuffer_GetType(const CDirect3DIndexBuffer *_this)
{
    (void)_this;
    return D3DRTYPE_INDEXBUFFER;
}

fnptr_t vtbl_CDirect3DIndexBuffer[] = { (fnptr_t)CDirect3DIndexBuffer_QueryInterface, (fnptr_t)CDirect3DIndexBuffer_AddRef, (fnptr_t)CDirect3DIndexBuffer_Release, (fnptr_t)CDirect3DIndexBuffer_GetDevice, (fnptr_t)CDirect3DIndexBuffer_SetPrivateData, (fnptr_t)CDirect3DIndexBuffer_GetPrivateData, (fnptr_t)CDirect3DIndexBuffer_FreePrivateData, (fnptr_t)CDirect3DIndexBuffer_SetPriority, (fnptr_t)CDirect3DIndexBuffer_GetPriority, (fnptr_t)CDirect3DIndexBuffer_PreLoad, (fnptr_t)CDirect3DIndexBuffer_GetType, (fnptr_t)CDirect3DIndexBuffer_Lock, (fnptr_t)CDirect3DIndexBuffer_Unlock, (fnptr_t)CDirect3DIndexBuffer_GetDesc, (fnptr_t)ZN20CDirect3DIndexBufferD1Ev, (fnptr_t)ZN20CDirect3DIndexBufferD0Ev };

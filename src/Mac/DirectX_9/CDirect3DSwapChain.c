#include "common_types.h"
#include "imports.h"
#include <stdlib.h>

typedef struct {
    void **vtable;
    ULONG refCount;
    IDirect3DSurface9 *backBuffer;
} CDirect3DSwapChainImpl;

typedef struct {
    void *QueryInterface;
    ULONG (*AddRef)(IDirect3DSurface9 *);
    ULONG (*Release)(IDirect3DSurface9 *);
} IDirect3DSurface9VTable;
typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CDirect3DSwapChain[];

static IDirect3DSurface9VTable *CDirect3DSwapChain_GetSurfaceVTable(IDirect3DSurface9 *surface)
{
    return *(IDirect3DSurface9VTable **)surface;
}

static ULONG CDirect3DSwapChain_SurfaceAddRef(IDirect3DSurface9 *surface)
{
    return CDirect3DSwapChain_GetSurfaceVTable(surface)->AddRef(surface);
}

static ULONG CDirect3DSwapChain_SurfaceRelease(IDirect3DSurface9 *surface)
{
    return CDirect3DSwapChain_GetSurfaceVTable(surface)->Release(surface);
}

static void CDirect3DSwapChain_Destroy(CDirect3DSwapChainImpl *swapChain)
{
    swapChain->vtable = (void**)vtbl_CDirect3DSwapChain;
    if (swapChain->backBuffer) {
        CDirect3DSwapChain_SurfaceRelease(swapChain->backBuffer);
    }
}

ULONG CDirect3DSwapChain_AddRef(const CDirect3DSwapChain *_this);
void ZN18CDirect3DSwapChainD1Ev(const CDirect3DSwapChain *_this);
void ZN18CDirect3DSwapChainD0Ev(const CDirect3DSwapChain *_this);
HRESULT CDirect3DSwapChain_QueryInterface(const CDirect3DSwapChain *_this, const IID *iid, LPVOID *ppvObj);
ULONG CDirect3DSwapChain_Release(const CDirect3DSwapChain *_this);
HRESULT CDirect3DSwapChain_GetBackBuffer(const CDirect3DSwapChain *_this, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer);
int CDirect3DSwapChain_SetBackBuffer(const CDirect3DSwapChain *_this, const IDirect3DSurface9 *pBackBuffer);
int CDirect3DSwapChain_CDirect3DSwapChain(const CDirect3DSwapChain *_this);
HRESULT CDirect3DSwapChain_Present(const CDirect3DSwapChain *_this, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags);
HRESULT CDirect3DSwapChain_GetFrontBufferData(const CDirect3DSwapChain *_this, IDirect3DSurface9 *pDestSurface);
HRESULT CDirect3DSwapChain_GetRasterStatus(const CDirect3DSwapChain *_this, D3DRASTER_STATUS *pRasterStatus);
HRESULT CDirect3DSwapChain_GetDisplayMode(const CDirect3DSwapChain *_this, D3DDISPLAYMODE *pMode);
HRESULT CDirect3DSwapChain_GetDevice(const CDirect3DSwapChain *_this, D3DDEVICE_CREATION_PARAMETERS *(*ppDevice)());
HRESULT CDirect3DSwapChain_GetPresentParameters(const CDirect3DSwapChain *_this, D3DPRESENT_PARAMETERS *pPresentationParameters);

ULONG CDirect3DSwapChain_AddRef(const CDirect3DSwapChain *_this)
{
    CDirect3DSwapChainImpl *swapChain;

    swapChain = (CDirect3DSwapChainImpl *)_this;
    ++swapChain->refCount;
    return swapChain->refCount;
}

HRESULT CDirect3DSwapChain_QueryInterface(const CDirect3DSwapChain *_this, const IID *iid, LPVOID *ppvObj)
{
    (void)iid;

    *ppvObj = (LPVOID)_this;
    CDirect3DSwapChain_AddRef(_this);
    return 0;
}

ULONG CDirect3DSwapChain_Release(const CDirect3DSwapChain *_this)
{
    CDirect3DSwapChainImpl *swapChain;
    ULONG refCount;

    swapChain = (CDirect3DSwapChainImpl *)_this;
    refCount = --swapChain->refCount;
    if (!refCount) {
        ZN18CDirect3DSwapChainD0Ev(_this);
    }

    return refCount;
}

void ZN18CDirect3DSwapChainD1Ev(const CDirect3DSwapChain *_this)
{
    CDirect3DSwapChain_Destroy((CDirect3DSwapChainImpl *)_this);
}

void ZN18CDirect3DSwapChainD0Ev(const CDirect3DSwapChain *_this)
{
    CDirect3DSwapChain_Destroy((CDirect3DSwapChainImpl *)_this);
    free((void *)_this);
}

HRESULT CDirect3DSwapChain_GetBackBuffer(const CDirect3DSwapChain *_this, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
    CDirect3DSwapChainImpl *swapChain;

    (void)BackBuffer;
    (void)Type;

    swapChain = (CDirect3DSwapChainImpl *)_this;
    *ppBackBuffer = swapChain->backBuffer;
    CDirect3DSwapChain_SurfaceAddRef(swapChain->backBuffer);
    return 0;
}

int CDirect3DSwapChain_SetBackBuffer(const CDirect3DSwapChain *_this, const IDirect3DSurface9 *pBackBuffer)
{
    CDirect3DSwapChainImpl *swapChain;

    swapChain = (CDirect3DSwapChainImpl *)_this;
    swapChain->backBuffer = (IDirect3DSurface9 *)pBackBuffer;
    return (int)CDirect3DSwapChain_SurfaceAddRef(swapChain->backBuffer);
}

int CDirect3DSwapChain_CDirect3DSwapChain(const CDirect3DSwapChain *_this)
{
    CDirect3DSwapChainImpl *swapChain;

    swapChain = (CDirect3DSwapChainImpl *)_this;
    swapChain->vtable = (void**)vtbl_CDirect3DSwapChain;
    swapChain->backBuffer = NULL;
    swapChain->refCount = 1;
    return (int)_this;
}

HRESULT CDirect3DSwapChain_Present(const CDirect3DSwapChain *_this, const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags)
{
    (void)_this;
    (void)pSourceRect;
    (void)pDestRect;
    (void)hDestWindowOverride;
    (void)pDirtyRegion;
    (void)dwFlags;
    return 0;
}

HRESULT CDirect3DSwapChain_GetFrontBufferData(const CDirect3DSwapChain *_this, IDirect3DSurface9 *pDestSurface)
{
    (void)_this;
    (void)pDestSurface;
    return 0;
}

HRESULT CDirect3DSwapChain_GetRasterStatus(const CDirect3DSwapChain *_this, D3DRASTER_STATUS *pRasterStatus)
{
    (void)_this;
    (void)pRasterStatus;
    return 0;
}

HRESULT CDirect3DSwapChain_GetDisplayMode(const CDirect3DSwapChain *_this, D3DDISPLAYMODE *pMode)
{
    (void)_this;
    (void)pMode;
    return 0;
}

HRESULT CDirect3DSwapChain_GetDevice(const CDirect3DSwapChain *_this, D3DDEVICE_CREATION_PARAMETERS *(*ppDevice)())
{
    (void)_this;
    (void)ppDevice;
    return 0;
}

HRESULT CDirect3DSwapChain_GetPresentParameters(const CDirect3DSwapChain *_this, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    (void)_this;
    (void)pPresentationParameters;
    return 0;
}

fnptr_t vtbl_CDirect3DSwapChain[] = { (fnptr_t)CDirect3DSwapChain_QueryInterface, (fnptr_t)CDirect3DSwapChain_AddRef, (fnptr_t)CDirect3DSwapChain_Release, (fnptr_t)CDirect3DSwapChain_Present, (fnptr_t)CDirect3DSwapChain_GetFrontBufferData, (fnptr_t)CDirect3DSwapChain_GetBackBuffer, (fnptr_t)CDirect3DSwapChain_GetRasterStatus, (fnptr_t)CDirect3DSwapChain_GetDisplayMode, (fnptr_t)CDirect3DSwapChain_GetDevice, (fnptr_t)CDirect3DSwapChain_GetPresentParameters, (fnptr_t)ZN18CDirect3DSwapChainD1Ev, (fnptr_t)ZN18CDirect3DSwapChainD0Ev };

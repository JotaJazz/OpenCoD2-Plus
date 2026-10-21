#include "common_types.h"
#include "imports.h"
extern refimport_t ri;
extern int alwaysfails;
extern DxGlobals dx;

#define DX_DEVICE_WRAP ((void *)imp_dx)
#define XMODEL_VTABLE ((char *)&ri)
#define GPU_FENCE_FLAG (*(volatile int *)&alwaysfails)

#define DEREF_PTR(ptr, off) (*(void **)((char *)(ptr) + (off)))

typedef int(__attribute__((cdecl)) * vtable_func_t)();
#define VTABLE(obj) (*(vtable_func_t **)((void *)(obj)))
/* VTABLE_CALL removed: it was a variadic macro (unsupported by VC7.1) and unused. */

typedef int(__attribute__((cdecl)) * flat_func_t)();
#if defined(_M_X64) || defined(__x86_64__)
/* &ri is a uniform pointer-slot table; x86 byte offsets double on x64 (8-byte slots) */
#define FLAT_SLOT(base, off) (*(flat_func_t *)((base) + (off) * 2))
#else
#define FLAT_SLOT(base, off) (*(flat_func_t *)((base) + (off)))
#endif
/* VC7.1 has no variadic macros -> fixed-arity wrappers. Cast the int(*)() slot to the
 * real call signature (byte-identical to the original variadic call). */
#define FLAT_CALL1(base, off, a) (((int(__cdecl *)(void *))FLAT_SLOT(base, off))(a))
#define FLAT_CALL4(base, off, a, b, c, d) (((int(__cdecl *)(void *, void *, int, void *))FLAT_SLOT(base, off))(a, b, c, d))

extern void R_FinishStaticVertexBuffer(IDirect3DVertexBuffer9 *vb);
extern dvar_t *r_rendererInUse;
extern void *R_AllocStaticIndexBuffer(IDirect3DIndexBuffer9 **ib, int sizeInBytes);
extern void R_FinishStaticIndexBuffer(IDirect3DIndexBuffer9 *ib);
extern void R_FreeStaticIndexBuffer(IDirect3DIndexBuffer9 *ib);
extern int XSurfaceGetBoneOffset(const XSurface *surf);
extern void XSurfaceTransfer(const XVertexBuffer *surfVerts, GfxVertex *verts, int vertCount);
extern void XSurfaceTransferDx7(const XVertexBuffer *surfVerts, GfxVertexDx7 *verts, int vertCount);
extern void Com_Memcpy(void *dest, const void *src, int count);

void XSurfaceOptimizeRigid(XModel *model, XSurface *surface, XVertexBuffer *surfVerts);
void XModelOptimize(XModel *model);
void XModelUnoptimize(XModel *model);

void XSurfaceOptimizeRigid(XModel *model, XSurface *surface, XVertexBuffer *surfVerts)
{
    IDirect3DVertexBuffer9 **surfRigid;
    void *vertexBuffer;
    int vertCount;
    int vertexStride;
    void *deviceWrapper;
    void *device;
    void **deviceVtable;
    void **vbVtable;
    int hr;

    surfRigid = &surface->surfRigid.vb;

    if (surface->surfRigid.vb != NULL)
        return;

    vertCount = (int)surface->vertCount;

    if (r_rendererInUse->current.integer == 2)
        vertexStride = 0x24;
    else
        vertexStride = 0x40;

    deviceWrapper = DX_DEVICE_WRAP;
    (void)deviceWrapper;
    device = dx.device;   /* was DEREF_PTR(imp_dx,8): x86 device offset; x64 dx.device is @16 */
    deviceVtable = *(void ***)device;

    hr = ((int(D3DVTCC *)(void *, int, int, int, int, void *, void *))deviceVtable[0x68 / 4])(device, vertCount * vertexStride, 8, 0, 0, surfRigid, 0);

    if (hr < 0) {

        surface->surfRigid.vb = NULL;
        return;
    }

    vbVtable = *(void ***)surface->surfRigid.vb;
    hr = ((int(D3DVTCC *)(void *, int, int, void *, int))vbVtable[0x2c / 4])(surface->surfRigid.vb, 0, 0, &vertexBuffer, 0);

    if (hr < 0) {

        vbVtable = *(void ***)surface->surfRigid.vb;
        ((void(D3DVTCC *)(void *))vbVtable[8 / 4])(surface->surfRigid.vb);
        surface->surfRigid.vb = NULL;
        return;
    }

    if (r_rendererInUse->current.integer == 2) {

        XSurfaceTransferDx7(surfVerts, (GfxVertexDx7 *)vertexBuffer, (int)surface->vertCount);
    } else {

        XSurfaceTransfer(surfVerts, (GfxVertex *)vertexBuffer, (int)surface->vertCount);
    }

    R_FinishStaticVertexBuffer(surface->surfRigid.vb);
}

void XModelOptimize(XModel *model)
{
    char *vtable;
    int lodCount;
    int lodIndex;
    int surfCount;
    int surfIndex;
    XSurface **surfaces;
    XPartBits partBits;
    XSurface *surf;
    int indexDataSize;
    int indexBytes;
    void *indexBuffer;

    vtable = XMODEL_VTABLE;

    lodCount = FLAT_CALL1(vtable, 0x174, model);
    if (lodCount <= 0)
        return;

    for (lodIndex = 0; lodIndex < lodCount; lodIndex++) {

        surfCount = FLAT_CALL4(vtable, 0x168, model, &surfaces, lodIndex, &partBits);
        if (surfCount <= 0)
            continue;

        for (surfIndex = 0; surfIndex < surfCount; surfIndex++) {
            surf = surfaces[surfIndex];

            if (surf->indexBuffer != NULL) {
                if (XSurfaceGetBoneOffset(surf) != -1) {
                    XSurfaceOptimizeRigid(model, surf, surf->verts);
                }
                continue;
            }

            indexDataSize = (int)surf->triCount * 6;

            indexBytes = (indexDataSize + 0x1f) & ~0x1f;

            indexBuffer = R_AllocStaticIndexBuffer(&surf->indexBuffer, indexBytes);
            if (indexBuffer == NULL)
                continue;

            Com_Memcpy(indexBuffer, surf->triIndices, indexDataSize);

            memset((char *)indexBuffer + indexDataSize, 0, indexBytes - indexDataSize);

            R_FinishStaticIndexBuffer(surf->indexBuffer);

            if (XSurfaceGetBoneOffset(surf) != -1) {
                XSurfaceOptimizeRigid(model, surf, surf->verts);
            }
        }
    }
}

void XModelUnoptimize(XModel *model)
{
    char *vtable;
    int lodCount;
    int lodIndex;
    int surfCount;
    int surfIndex;
    XSurface **surfaces;
    XPartBits partBits;
    XSurface *surf;
    void **vbVtable;

    vtable = XMODEL_VTABLE;

    lodCount = FLAT_CALL1(vtable, 0x174, model);
    if (lodCount <= 0)
        return;

    for (lodIndex = 0; lodIndex < lodCount; lodIndex++) {

        surfCount = FLAT_CALL4(vtable, 0x168, model, &surfaces, lodIndex, &partBits);
        if (surfCount <= 0)
            continue;

        for (surfIndex = 0; surfIndex < surfCount; surfIndex++) {
            surf = surfaces[surfIndex];

            if (surf->indexBuffer != NULL) {
                R_FreeStaticIndexBuffer(surf->indexBuffer);
                surf->indexBuffer = NULL;
            }

            if (surf->surfRigid.vb != NULL) {
                do {

                    vbVtable = *(void ***)surf->surfRigid.vb;
                    ((void(D3DVTCC *)(void *))vbVtable[8 / 4])(surf->surfRigid.vb);
                    surf->surfRigid.vb = NULL;
                } while (GPU_FENCE_FLAG != 0);
            }
        }
    }
}

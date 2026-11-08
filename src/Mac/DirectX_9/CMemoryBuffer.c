#include "common_types.h"
#include "imports.h"
#include <stdlib.h>
#include <string.h>

typedef void (*fnptr_t)(void);
extern fnptr_t vtbl_CMemoryBuffer[];

int CMemoryBuffer_sDelayedFreeRequests = 0;
UINT32 CMemoryBuffer_sMemoryDesignatedForDelayedFree = 0;

typedef struct {
    intptr_t vptr;  /* int on 32 bits, as 1.3; the vtable's address */
    byte *allocation;
    byte *data;
    UINT32 length;
    int freedLater;
} CMemoryBufferImpl;

typedef struct CMemoryBufferFreeRequestNode {
    struct CMemoryBufferFreeRequestNode *next;
    struct CMemoryBufferFreeRequestNode *prev;
    byte *allocation;
    UINT32 length;
    UINT32 frames;
} CMemoryBufferFreeRequestNode;

static byte *CMemoryBuffer_AlignAllocation(byte *allocation)
{
    return (byte *)(((uintptr_t)(allocation + 31)) & ~(uintptr_t)31);
}

static CMemoryBufferFreeRequestNode *CMemoryBuffer_GetDelayedFreeHead(void)
{
    return (CMemoryBufferFreeRequestNode *)&CMemoryBuffer_sDelayedFreeRequests;
}

static void CMemoryBuffer_EnsureDelayedFreeListInitialized(void)
{
    CMemoryBufferFreeRequestNode *head = CMemoryBuffer_GetDelayedFreeHead();

    if (head->next == NULL || head->prev == NULL) {
        head->next = head;
        head->prev = head;
    }
}

void CMemoryBuffer_CMemoryBuffer(const CMemoryBuffer *_this, UINT32 Length);
void CMemoryBuffer_Recreate(const CMemoryBuffer *_this);
void ZN13CMemoryBufferD1Ev(const CMemoryBuffer *_this);
void ZN13CMemoryBufferD0Ev(const CMemoryBuffer *_this);
void CMemoryBuffer_Resize(const CMemoryBuffer *_this, UINT32 Length);
void CMemoryBuffer_FreeLater(const CMemoryBuffer *_this, UINT32 Frames);
void CMemoryBuffer_Update(void);
void CMemoryBuffer_Reset(void);
void ZNSt10_List_baseIN13CMemoryBuffer11FreeRequestESaIS1_EE8_M_clearEv(CMemoryBufferFreeRequestNode *head);

void CMemoryBuffer_CMemoryBuffer(const CMemoryBuffer *_this, UINT32 Length)
{
    CMemoryBufferImpl *buffer;
    byte *allocation;

    buffer = (CMemoryBufferImpl *)_this;
    buffer->vptr = (intptr_t)vtbl_CMemoryBuffer;
    buffer->length = Length;
    buffer->freedLater = Length == 0;

    if (Length) {
        allocation = (byte *)malloc(((Length + 31) & ~31U) + 31);
        buffer->allocation = allocation;
        buffer->data = CMemoryBuffer_AlignAllocation(allocation);
    } else {
        buffer->allocation = NULL;
        buffer->data = NULL;
    }
}

void CMemoryBuffer_Recreate(const CMemoryBuffer *_this)
{
    CMemoryBufferImpl *buffer;

    buffer = (CMemoryBufferImpl *)_this;
    buffer->allocation = (byte *)malloc(((buffer->length + 31) & ~31U) + 31);
    buffer->data = CMemoryBuffer_AlignAllocation(buffer->allocation);
    buffer->freedLater = 0;
}

void ZN13CMemoryBufferD1Ev(const CMemoryBuffer *_this)
{
    CMemoryBufferImpl *buffer;

    buffer = (CMemoryBufferImpl *)_this;
    buffer->vptr = (intptr_t)vtbl_CMemoryBuffer;

    if (!buffer->freedLater && buffer->allocation) {
        free(buffer->allocation);
    }

    buffer->data = NULL;
    buffer->allocation = NULL;
}

void ZN13CMemoryBufferD0Ev(const CMemoryBuffer *_this)
{
    ZN13CMemoryBufferD1Ev(_this);
    free((void *)_this);
}

void CMemoryBuffer_Resize(const CMemoryBuffer *_this, UINT32 Length)
{
    CMemoryBufferImpl *buffer;

    buffer = (CMemoryBufferImpl *)_this;
    if (!buffer->freedLater && buffer->allocation) {
        free(buffer->allocation);
    }

    buffer->data = NULL;
    buffer->allocation = NULL;
    buffer->allocation = (byte *)malloc(((Length + 31) & ~31U) + 31);
    buffer->data = CMemoryBuffer_AlignAllocation(buffer->allocation);
    buffer->length = Length;
    buffer->freedLater = 0;
}

void CMemoryBuffer_FreeLater(const CMemoryBuffer *_this, UINT32 Frames)
{
    CMemoryBufferImpl *buffer;
    CMemoryBufferFreeRequestNode *head;
    CMemoryBufferFreeRequestNode *node;

    buffer = (CMemoryBufferImpl *)_this;
    if (buffer->freedLater) {
        return;
    }

    CMemoryBuffer_EnsureDelayedFreeListInitialized();
    head = CMemoryBuffer_GetDelayedFreeHead();

    node = (CMemoryBufferFreeRequestNode *)malloc(sizeof(*node));
    if (node) {
        node->allocation = buffer->allocation;
        node->length = buffer->length;
        node->frames = Frames;

        node->next = head;
        node->prev = head->prev;
        head->prev->next = node;
        head->prev = node;
    }

    CMemoryBuffer_sMemoryDesignatedForDelayedFree += buffer->length;
    buffer->data = NULL;
    buffer->allocation = NULL;
    buffer->freedLater = 1;
}

void CMemoryBuffer_Update(void)
{
    CMemoryBufferFreeRequestNode *head;
    CMemoryBufferFreeRequestNode *node;
    CMemoryBufferFreeRequestNode *next;

    CMemoryBuffer_EnsureDelayedFreeListInitialized();
    head = CMemoryBuffer_GetDelayedFreeHead();

    for (node = head->next; node != head; node = next) {
        next = node->next;

        if (node->frames) {
            --node->frames;
            continue;
        }

        if (node->allocation) {
            free(node->allocation);
        }

        CMemoryBuffer_sMemoryDesignatedForDelayedFree -= node->length;

        node->prev->next = node->next;
        node->next->prev = node->prev;
        free(node);
    }
}

void CMemoryBuffer_Reset(void)
{
    CMemoryBufferFreeRequestNode *head;
    CMemoryBufferFreeRequestNode *node;
    CMemoryBufferFreeRequestNode *next;

    CMemoryBuffer_EnsureDelayedFreeListInitialized();
    head = CMemoryBuffer_GetDelayedFreeHead();

    for (node = head->next; node != head; node = next) {
        next = node->next;
        CMemoryBuffer_sMemoryDesignatedForDelayedFree -= node->length;

        if (node->allocation) {
            free(node->allocation);
        }

        free(node);
    }

    head->next = head;
    head->prev = head;
}

static void __static_initialization_and_destruction_0(int __initialize_p, int __priority)
{
    CMemoryBufferFreeRequestNode *head;

    if (__priority != 0xffff) {
        return;
    }

    head = CMemoryBuffer_GetDelayedFreeHead();

    if (__initialize_p == 1) {
        head->next = head;
        head->prev = head;
        return;
    }

    if (__initialize_p == 0) {
        ZNSt10_List_baseIN13CMemoryBuffer11FreeRequestESaIS1_EE8_M_clearEv(head);
    }
}

void GLOBAL__D__ZN13CMemoryBuffer20sDelayedFreeRequestsE(void)
{
    __static_initialization_and_destruction_0(0, 0xffff);
}

void GLOBAL__I__ZN13CMemoryBuffer20sDelayedFreeRequestsE(void)
{
    __static_initialization_and_destruction_0(1, 0xffff);
}

void ZNSt10_List_baseIN13CMemoryBuffer11FreeRequestESaIS1_EE8_M_clearEv(CMemoryBufferFreeRequestNode *head)
{
    CMemoryBufferFreeRequestNode *node;
    CMemoryBufferFreeRequestNode *next;

    node = head->next;
    while (node != head) {
        next = node->next;
        free(node);
        node = next;
    }
}

fnptr_t vtbl_CMemoryBuffer[] = { (fnptr_t)ZN13CMemoryBufferD1Ev, (fnptr_t)ZN13CMemoryBufferD0Ev };

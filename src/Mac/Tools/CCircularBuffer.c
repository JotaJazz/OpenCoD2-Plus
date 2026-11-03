#include "common_types.h"
#include "imports.h"
#include <stdlib.h>

void CCircularBuffer_CCircularBuffer(CCircularBuffer *_this);
void CCircularBuffer_Reset(CCircularBuffer *_this);
UInt32 CCircularBuffer_ReadPtrSize(const CCircularBuffer *_this);
void *CCircularBuffer_ReadPtr(CCircularBuffer *_this, UInt32 *ioSize);
void ZN15CCircularBufferD1Ev(CCircularBuffer *_this);
void CCircularBuffer_Alloc(CCircularBuffer *_this, UInt32 inBufferSize);
void CCircularBuffer_Write(const CCircularBuffer *_this, const void *inBuffer, UInt32 *ioSize);

void CCircularBuffer_CCircularBuffer(CCircularBuffer *_this)
{
    char *p = (char *)_this;
    ((int *)p)[0] = 0;
    ((int *)p)[1] = 0;
    ((int *)p)[2] = 0;
    ((int *)p)[3] = 0;
    ((int *)p)[4] = 0;
}

void CCircularBuffer_Reset(CCircularBuffer *_this)
{
    char *p = (char *)_this;
    ((int *)p)[2] = 0;
    ((int *)p)[3] = 0;
    ((int *)p)[4] = 0;
}

UInt32 CCircularBuffer_ReadPtrSize(const CCircularBuffer *_this)
{
    const char *p = (const char *)_this;
    UInt32 bufferSize = ((const int *)p)[1];
    UInt32 readOffset = ((const int *)p)[2];
    UInt32 lastReadSize = ((const int *)p)[3];
    UInt32 writeOffset = ((const int *)p)[4];

    UInt32 readPosition = readOffset + lastReadSize;
    if (readPosition >= bufferSize)
        readPosition = 0;

    UInt32 endPosition;
    if (readPosition <= writeOffset)
        endPosition = writeOffset;
    else
        endPosition = bufferSize;

    return endPosition - readPosition;
}

void *CCircularBuffer_ReadPtr(CCircularBuffer *_this, UInt32 *ioSize)
{
    char *p = (char *)_this;
    void *buffer = *(void **)p;
    UInt32 bufferSize = ((int *)p)[1];
    UInt32 readOffset = ((int *)p)[2];
    UInt32 lastReadSize = ((int *)p)[3];
    UInt32 writeOffset = ((int *)p)[4];

    readOffset = readOffset + lastReadSize;
    ((int *)p)[2] = readOffset;
    if (readOffset >= bufferSize) {
        readOffset = 0;
        ((int *)p)[2] = 0;
    }
    ((int *)p)[3] = 0;

    UInt32 endPosition;
    if (readOffset <= writeOffset)
        endPosition = writeOffset;
    else
        endPosition = bufferSize;

    UInt32 size = endPosition - readOffset;
    UInt32 requested = *ioSize;
    if (size < requested) {
        *ioSize = size;
    } else {
        size = requested;
    }

    if (size == 0)
        return NULL;

    ((int *)p)[3] = size;
    return (char *)buffer + readOffset;
}

void ZN15CCircularBufferD1Ev(CCircularBuffer *_this)
{
    char *p = (char *)_this;
    void *buffer = *(void **)p;

    if (buffer) {
        free(buffer);
        ((int *)p)[0] = 0;
        ((int *)p)[1] = 0;
        ((int *)p)[2] = 0;
        ((int *)p)[3] = 0;
        ((int *)p)[4] = 0;
    }
}

void CCircularBuffer_Alloc(CCircularBuffer *_this, UInt32 inBufferSize)
{
    char *p = (char *)_this;
    void *buffer = *(void **)p;

    if (buffer) {
        free(buffer);
        ((int *)p)[0] = 0;
        ((int *)p)[1] = 0;
        ((int *)p)[2] = 0;
        ((int *)p)[3] = 0;
        ((int *)p)[4] = 0;
    }

    *(void **)p = malloc(inBufferSize);
    ((int *)p)[sizeof(void *) / 4] = inBufferSize;   /* after the pointer, as [1] on 32 bits */
}

void CCircularBuffer_Write(const CCircularBuffer *_this, const void *inBuffer, UInt32 *ioSize)
{
}

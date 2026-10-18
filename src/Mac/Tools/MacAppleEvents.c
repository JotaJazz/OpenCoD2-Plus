#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

void ZN8CAEEventD1Ev(CAEEvent *_this);
void ZN9CAETargetD1Ev(CAETarget *_this);
void CAETarget_CAETarget(CAETarget *_this, const ProcessSerialNumber *inPSN);
void CAEDesc_GetData(const CAEDesc *_this, DescType inTypeCode, void *outDataPtr, Size inDataSize);
void CAEDesc_PutData(CAEDesc *_this, DescType inTypeCode, const void *inDataPtr, Size inDataSize);
void CAEObject_CAEObject(CAEObject *_this, DescType inDesiredClass, const AEDesc *inContainer, DescType inKeyForm, const AEDesc *inKeyData);
void CAERecord_PutParam(CAERecord *_this, AEKeyword inKeyword, const AEDesc *inParam);
void CAEEvent_Create(CAEEvent *_this, CAETarget *inTarget, AEEventClass inEventClass, AEEventID inEventID);
void CAEEvent_CAEEvent(CAEEvent *_this);
void CAEEvent_Send(CAEEvent *_this);
double CAETarget_SetPropertyData(const CAETarget *_this, const AEDesc *inObjectSpecifier, DescType inPropertyID, const AEDesc *inData, const CAERecord *_this_4, AEKeyword inKeyword, SInt32 *outParam, const CAERecord *_this_7, AEKeyword inKeyword_8, StringPtr outParam_9);
double CAETarget_CountElements(const CAETarget *_this, const AEDesc *inObjectSpecifier, DescType inElementClass, SInt32 *outCount);
double CAETarget_GetPropertyData(const CAETarget *_this, const AEDesc *inObjectSpecifier, DescType inPropertyID, DescType inRequestedType, AEDesc *outData);

#define CAE_TYPE_NULL ((DescType)0x6e756c6c)
#define CAE_TYPE_PSN ((DescType)0x70736e20)
#define CAE_TYPE_SIGN ((DescType)0x7369676e)

extern void *__ZTIl;

static __attribute__((always_inline)) inline AEDesc *CAE_AsDesc(const void *desc)
{
    return (AEDesc *)(void *)desc;
}

static __attribute__((always_inline)) inline void CAE_InitNull(void *desc)
{
    AEDesc *aeDesc = CAE_AsDesc(desc);
    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;
}

static __attribute__((always_inline)) inline void CAE_DisposeThenNull(void *desc)
{
    AEDesc *aeDesc = CAE_AsDesc(desc);
    if (aeDesc->dataHandle) {
        AEDisposeDesc(aeDesc);
    }
    CAE_InitNull(desc);
}

static __attribute__((always_inline)) inline void CAE_ThrowStatus(SInt16 status)
{
    SInt32 *exception = (SInt32 *)cxa_allocate_exception(4);
    *exception = (SInt32)status;
    cxa_throw(exception, __ZTIl, 0);
}

static __attribute__((always_inline)) inline void CAE_ThrowIfError(SInt16 status)
{
    if (status) {
        CAE_ThrowStatus(status);
    }
}

void ZN8CAEEventD1Ev(CAEEvent *_this)
{
    AEDesc *aeDesc = (AEDesc *)(void *)_this;
    Handle dh = (Handle)aeDesc->dataHandle;
    if (dh) {
        AEDisposeDesc(aeDesc);
    }
    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;
}

void ZN9CAETargetD1Ev(CAETarget *_this)
{
    AEDesc *aeDesc = (AEDesc *)(void *)_this;
    Handle dh = (Handle)aeDesc->dataHandle;
    if (dh) {
        AEDisposeDesc(aeDesc);
    }
    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;
}

void CAETarget_CAETarget(CAETarget *_this, const ProcessSerialNumber *inPSN)
{

    AEDesc *aeDesc = (AEDesc *)(void *)_this;
    SInt16 status_raw;
    SInt32 status;

    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;

    status_raw = (SInt16)AECreateDesc(CAE_TYPE_PSN, inPSN, 8, aeDesc);
    status = (SInt32)status_raw;
    if (__builtin_expect(status_raw != 0, 0)) {
        SInt32 *exception = (SInt32 *)cxa_allocate_exception(4);
        *exception = status;
        cxa_throw(exception, __ZTIl, 0);
    }
}

void CAEDesc_GetData(const CAEDesc *_this, DescType inTypeCode, void *outDataPtr, Size inDataSize)
{
    AEDesc coercedDesc;
    SInt16 status;
    const AEDesc *sourceDesc = CAE_AsDesc(_this);

    if (sourceDesc->descriptorType == inTypeCode) {
        status = (SInt16)AEGetDescData(sourceDesc, outDataPtr, inDataSize);
        CAE_ThrowIfError(status);
        return;
    }

    CAE_InitNull(&coercedDesc);
    status = (SInt16)AECoerceDesc(sourceDesc, inTypeCode, &coercedDesc);
    if (!status) {
        status = (SInt16)AEGetDescData(&coercedDesc, outDataPtr, inDataSize);
    }
    if (status) {
        CAE_DisposeThenNull(&coercedDesc);
        CAE_ThrowStatus(status);
    }
    CAE_DisposeThenNull(&coercedDesc);
}

void CAEDesc_PutData(CAEDesc *_this, DescType inTypeCode, const void *inDataPtr, Size inDataSize)
{

    AEDesc *aeDesc = (AEDesc *)(void *)_this;
    SInt16 status_raw;
    SInt32 status;

    if (aeDesc->dataHandle) {
        AEDisposeDesc(aeDesc);
    }
    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;

    status_raw = (SInt16)AECreateDesc(inTypeCode, inDataPtr, inDataSize, aeDesc);
    status = (SInt32)status_raw;
    if (__builtin_expect(status_raw != 0, 0)) {
        SInt32 *exception = (SInt32 *)cxa_allocate_exception(4);
        *exception = status;
        cxa_throw(exception, __ZTIl, 0);
    }
}

void CAEObject_CAEObject(CAEObject *_this, DescType inDesiredClass, const AEDesc *inContainer, DescType inKeyForm, const AEDesc *inKeyData)
{
    AEDesc *aeDesc = (AEDesc *)(void *)_this;
    SInt16 status_raw;
    SInt32 status;

    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;

    status_raw = (SInt16)CreateObjSpecifier(inDesiredClass, inContainer, inKeyForm, inKeyData, 0, aeDesc);
    status = (SInt32)status_raw;
    if (__builtin_expect(status_raw != 0, 0)) {
        SInt32 *exception = (SInt32 *)cxa_allocate_exception(4);
        *exception = status;
        cxa_throw(exception, __ZTIl, 0);
    }
}

void CAERecord_PutParam(CAERecord *_this, AEKeyword inKeyword, const AEDesc *inParam)
{

    SInt16 status_raw = (SInt16)AEPutParamDesc(CAE_AsDesc(_this), inKeyword, inParam);
    SInt32 status = (SInt32)status_raw;
    if (__builtin_expect(status_raw != 0, 0)) {
        SInt32 *exception = (SInt32 *)cxa_allocate_exception(4);
        *exception = status;
        void *type_info = __ZTIl;
        cxa_throw(exception, type_info, 0);
    }
}

void CAEEvent_Create(CAEEvent *_this, CAETarget *inTarget, AEEventClass inEventClass, AEEventID inEventID)
{

    AEDesc *aeDesc = (AEDesc *)(void *)_this;
    Handle dh = (Handle)aeDesc->dataHandle;
    SInt16 status_raw;
    SInt32 status;

    if (dh) {
        AEDisposeDesc(aeDesc);
    }
    aeDesc->descriptorType = CAE_TYPE_NULL;
    aeDesc->dataHandle = 0;

    status_raw = (SInt16)AECreateAppleEvent(inEventClass, inEventID, CAE_AsDesc(inTarget), -1, 0, aeDesc);
    status = (SInt32)status_raw;
    if (__builtin_expect(status_raw != 0, 0)) {
        SInt32 *exception = (SInt32 *)cxa_allocate_exception(4);
        *exception = status;
        cxa_throw(exception, __ZTIl, 0);
    }
}

void CAEEvent_CAEEvent(CAEEvent *_this)
{
    CAE_InitNull(_this);
}

void CAEEvent_Send(CAEEvent *_this)
{
    AEDesc reply;
    SInt16 status;

    CAE_InitNull(&reply);
    status = (SInt16)AESend(CAE_AsDesc(_this), &reply, 0x11, 0, -1, 0, 0);
    if (status) {
        CAE_DisposeThenNull(&reply);
        CAE_ThrowStatus(status);
    }
    CAE_DisposeThenNull(&reply);
}

double CAETarget_SetPropertyData(const CAETarget *_this, const AEDesc *inObjectSpecifier, DescType inPropertyID, const AEDesc *inData, const CAERecord *_this_4, AEKeyword inKeyword, SInt32 *outParam, const CAERecord *_this_7, AEKeyword inKeyword_8, StringPtr outParam_9)
{
    return 0;
}

double CAETarget_CountElements(const CAETarget *_this, const AEDesc *inObjectSpecifier, DescType inElementClass, SInt32 *outCount)
{
    return 0;
}

double CAETarget_GetPropertyData(const CAETarget *_this, const AEDesc *inObjectSpecifier, DescType inPropertyID, DescType inRequestedType, AEDesc *outData)
{
    return 0;
}

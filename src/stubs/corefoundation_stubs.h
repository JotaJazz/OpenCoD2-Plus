#ifndef STUBS_COREFOUNDATION_H
#define STUBS_COREFOUNDATION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *CFTypeRef;
typedef void *CFAllocatorRef;
typedef void *CFStringRef;
typedef void *CFArrayRef;
typedef void *CFMutableArrayRef;
typedef void *CFDictionaryRef;
typedef void *CFMutableDictionaryRef;
typedef void *CFNumberRef;
typedef void *CFDataRef;
typedef void *CFBooleanRef;
typedef unsigned int CFIndex;
typedef unsigned int CFTypeID;
typedef double CFTimeInterval;

CFIndex CFArrayGetCount(CFArrayRef array);
const void *CFArrayGetValueAtIndex(CFArrayRef array, CFIndex idx);
CFNumberRef CFNumberCreate(CFAllocatorRef alloc, int theType, const void *valuePtr);
int CFNumberGetValue(CFNumberRef number, int theType, void *valuePtr);
const void *CFDictionaryGetValue(CFDictionaryRef dict, const void *key);
void CFRetain(CFTypeRef cf);

#ifdef __cplusplus
}
#endif
#endif

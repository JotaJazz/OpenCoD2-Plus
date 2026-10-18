#ifndef IMPORTS_COREFOUNDATION_H
#define IMPORTS_COREFOUNDATION_H

#ifdef __cplusplus
extern "C" {
#endif


int CFArrayGetCount();
int CFArrayGetValueAtIndex();
int CFBooleanGetValue();
int CFBundleCopyExecutableURL();
int CFBundleCopyLocalizedString(COD2_KR);
int CFBundleGetMainBundle();
int CFDataGetBytePtr();
int CFDictionaryGetValue();
int CFGetTypeID();
int CFNumberGetValue();
int CFPreferencesAppSynchronize(COD2_KR);
int CFPreferencesCopyAppValue();
int CFPreferencesSetAppValue();
int CFRelease(COD2_KR);
int CFStringCompare();
int CFStringCreateWithCString();
int CFStringCreateWithCharacters(COD2_KR);
int CFStringCreateWithFormat();
int CFStringGetCString(COD2_KR);
int CFStringGetPascalString(COD2_KR);
int CFStringGetTypeID();
int CFURLCreateFromFSRef();
int CFURLGetFSRef();

extern void **kCFAllocatorDefault;
extern void **kCFPreferencesCurrentApplication;


#ifdef __cplusplus
}
#endif
#endif

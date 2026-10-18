#ifndef IMPORTS_IOKIT_H
#define IMPORTS_IOKIT_H

#ifdef __cplusplus
extern "C" {
#endif


int IOBSDNameMatching();
int IOIteratorNext();
int IOMasterPort();
int IOObjectConformsTo();
int IOObjectGetClass();
int IOObjectRelease();
int IOObjectRetain();
int IORegistryEntryCreateCFProperty();
int IORegistryEntryCreateIterator();
int IORegistryEntryGetParentEntry();
int IOServiceGetMatchingServices();


#ifdef __cplusplus
}
#endif
#endif

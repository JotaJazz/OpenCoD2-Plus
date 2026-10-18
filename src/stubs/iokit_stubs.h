#ifndef STUBS_IOKIT_H
#define STUBS_IOKIT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *io_object_t;
typedef void *io_iterator_t;
typedef void *io_service_t;
typedef void *IOHIDDeviceInterface;
typedef void *IOHIDElementCookie;
typedef unsigned int IOReturn;
typedef unsigned int mach_port_t;
typedef void *CFMutableDictionaryRef;

CFMutableDictionaryRef IOServiceMatching(const char *name);
int IOServiceGetMatchingServices(mach_port_t masterPort, CFMutableDictionaryRef matching, io_iterator_t *existing);
io_object_t IOIteratorNext(io_iterator_t iterator);
int IOObjectRelease(io_object_t object);
int IORegistryEntryCreateCFProperties(io_object_t entry, void *properties, void *allocator, unsigned int options);
int IOCreatePlugInInterfaceForService(io_service_t service, void *pluginType, void *interfaceType, void **theInterface, int *theScore);
int IOHIDDeviceOpen(void *device, unsigned int options);
int IOHIDDeviceClose(void *device, unsigned int options);
mach_port_t IOMasterPort(unsigned int bootstrapPort, mach_port_t *masterPort);

#ifdef __cplusplus
}
#endif
#endif

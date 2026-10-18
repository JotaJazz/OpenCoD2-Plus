#ifndef STUBS_CARBON_H
#define STUBS_CARBON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *WindowRef;
typedef void *EventRef;
typedef void *EventHandlerRef;
typedef void *EventHandlerUPP;
typedef void *EventTargetRef;
typedef void *MenuRef;
typedef unsigned int EventClass;
typedef unsigned int EventKind;
typedef unsigned int OSStatus;
typedef unsigned int OSType;
typedef int OSErr;
typedef unsigned int UInt32;
typedef unsigned short UInt16;
typedef unsigned char UInt8;
typedef int SInt32;
typedef short SInt16;
typedef signed char SInt8;
typedef int Boolean;
typedef unsigned int FourCharCode;
typedef struct {
    short v;
    short h;
} Point;
typedef struct {
    short top;
    short left;
    short bottom;
    short right;
} Rect;
typedef void *GDHandle;
typedef void *CGrafPtr;
typedef void *DialogRef;
typedef void *ControlRef;
typedef void *CFStringRef;
typedef void *CFURLRef;
typedef void *CFBundleRef;
typedef void *CFDictionaryRef;
typedef const void *CFAllocatorRef;
typedef unsigned int CFStringEncoding;
typedef unsigned int CFTypeID;
typedef void *CFTypeRef;

void RunApplicationEventLoop(void);
void QuitApplicationEventLoop(void);
OSStatus InstallEventHandler(EventTargetRef target, EventHandlerUPP handler, unsigned int numTypes, const void *list, void *userData, EventHandlerRef *outRef);
OSStatus RemoveEventHandler(EventHandlerRef ref);
EventTargetRef GetApplicationEventTarget(void);
EventTargetRef GetWindowEventTarget(WindowRef window);
OSStatus GetEventParameter(EventRef event, OSType name, OSType type, void *outType, unsigned int bufSize, unsigned int *outSize, void *outData);
EventClass GetEventClass(EventRef event);
EventKind GetEventKind(EventRef event);
void GetMouse(Point *p);
Boolean Button(void);
unsigned int TickCount(void);
void GetKeys(void *keys);
int UpdateSystemActivity(int activity);
GDHandle GetMainDevice(void);
GDHandle GetDeviceList(void);
GDHandle GetNextDevice(GDHandle dev);
CGrafPtr GetWindowPort(WindowRef window);
void HideWindow(WindowRef window);
void ShowWindow(WindowRef window);
void DisposeWindow(WindowRef window);
void SetWindowBounds(WindowRef window, int regionCode, const Rect *bounds);
OSStatus SetWindowGroupLevel(void *group, int level);
void ShowCursor(void);
void HideCursor(void);
void ObscureCursor(void);
CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
int CFStringGetCString(CFStringRef str, char *buffer, int bufferSize, CFStringEncoding encoding);
CFBundleRef CFBundleGetMainBundle(void);
CFURLRef CFBundleCopyBundleURL(CFBundleRef bundle);
int CFURLGetFileSystemRepresentation(CFURLRef url, int resolveAgainstBase, unsigned char *buffer, int maxBufLen);
void CFRelease(CFTypeRef cf);

Boolean IsWindowVisible(WindowRef window);
void SizeWindow(WindowRef window, short w, short h, Boolean fUpdate);
CGrafPtr GetWindowPortBounds(WindowRef window, Rect *bounds);
void SetPortWindowPort(WindowRef window);
void ForeColor(int color);
void PaintRect(const Rect *r);
void ClipRect(const Rect *r);
void FrameRoundRect(const Rect *r, short ovalWidth, short ovalHeight);
void PaintRoundRect(const Rect *r, short ovalWidth, short ovalHeight);
void RGBForeColor(const void *color);
void OpenRgn(void);
void CloseRgn(void *rgn);
void SetClip(void *rgn);
void *NewRgn(void);
void DisposeRgn(void *rgn);
Boolean EqualRect(const Rect *a, const Rect *b);
void NormalizeThemeDrawingState(void);
int DrawThemeMenuBarBackground(const Rect *r, int style, int erase);
WindowRef GetControlOwner(ControlRef control);

#ifdef __cplusplus
}
#endif
#endif

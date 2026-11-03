#include "carbon_stubs.h"

extern void glOrtho(double left, double right, double bottom, double top, double zNear, double zFar);

void RunApplicationEventLoop(void)
{

}

void QuitApplicationEventLoop(void)
{
}

OSStatus InstallEventHandler(EventTargetRef target, EventHandlerUPP handler, unsigned int numTypes, const void *list, void *userData, EventHandlerRef *outRef)
{
    if (outRef)
        *outRef = 0;
    return 0;
}

OSStatus RemoveEventHandler(EventHandlerRef ref)
{
    return 0;
}

EventTargetRef GetApplicationEventTarget(void)
{
    return (EventTargetRef)0;
}

EventTargetRef GetWindowEventTarget(WindowRef window)
{
    return (EventTargetRef)0;
}

OSStatus GetEventParameter(EventRef event, OSType name, OSType type, void *outType, unsigned int bufSize, unsigned int *outSize, void *outData)
{
    return -1;
}

EventClass GetEventClass(EventRef event)
{
    return 0;
}

EventKind GetEventKind(EventRef event)
{
    return 0;
}

void GetMouse(Point *p)
{
    if (p) {
        p->h = 0;
        p->v = 0;
    }
}

Boolean Button(void)
{
    return 0;
}

unsigned int TickCount(void)
{
    return 0;
}

void GetKeys(void *keys)
{
}

GDHandle GetMainDevice(void)
{
    return (GDHandle)0;
}

GDHandle GetDeviceList(void)
{
    return (GDHandle)0;
}

GDHandle GetNextDevice(GDHandle dev)
{
    return (GDHandle)0;
}

CGrafPtr GetWindowPort(WindowRef window)
{
    return (CGrafPtr)0;
}

void HideWindow(WindowRef window)
{
}

#if !defined(_M_X64) || defined(__MINGW32__)  /* MSVC x64: real user32 ShowWindow (Mac Carbon stub collides by name); MinGW links these as 32 bits does */
void ShowWindow(WindowRef window)
{
}
#endif

void DisposeWindow(WindowRef window)
{
}

void SetWindowBounds(WindowRef window, int regionCode, const Rect *bounds)
{
}

OSStatus SetWindowGroupLevel(void *group, int level)
{
    return 0;
}

void ShowCursor(void)
{
}

void HideCursor(void)
{
}

void ObscureCursor(void)
{
}

CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding)
{
    return (CFStringRef)cStr;
}

int CFStringGetCString(CFStringRef str, char *buffer, int bufferSize, CFStringEncoding encoding)
{
    return 0;
}

CFBundleRef CFBundleGetMainBundle(void)
{
    return (CFBundleRef)0;
}

CFURLRef CFBundleCopyBundleURL(CFBundleRef bundle)
{
    return (CFURLRef)0;
}

int CFURLGetFileSystemRepresentation(CFURLRef url, int resolveAgainstBase, unsigned char *buffer, int maxBufLen)
{
    return 0;
}

void CFRelease(CFTypeRef cf)
{
}

Boolean IsWindowVisible(WindowRef window)
{
    return 0;
}

void SizeWindow(WindowRef window, short w, short h, Boolean fUpdate)
{
}

CGrafPtr GetWindowPortBounds(WindowRef window, Rect *bounds)
{
    if (bounds) {
        bounds->top = 0;
        bounds->left = 0;
        bounds->bottom = 480;
        bounds->right = 640;
    }
    return (CGrafPtr)0;
}

void SetPortWindowPort(WindowRef window)
{
}

void ForeColor(int color)
{
}

void PaintRect(const Rect *r)
{
}

void ClipRect(const Rect *r)
{
}

void FrameRoundRect(const Rect *r, short ovalWidth, short ovalHeight)
{
}

void PaintRoundRect(const Rect *r, short ovalWidth, short ovalHeight)
{
}

void RGBForeColor(const void *color)
{
}

void OpenRgn(void)
{
}

void CloseRgn(void *rgn)
{
}

void SetClip(void *rgn)
{
}

void *NewRgn(void)
{
    return (void *)0;
}

void DisposeRgn(void *rgn)
{
}

Boolean EqualRect(const Rect *a, const Rect *b)
{
    return 0;
}

void NormalizeThemeDrawingState(void)
{
}

int DrawThemeMenuBarBackground(const Rect *r, int style, int erase)
{
    return 0;
}

WindowRef GetControlOwner(ControlRef control)
{
    return (WindowRef)0;
}

GDHandle DMGetFirstScreenDevice(int activeOnly)
{
    return (GDHandle)0;
}

GDHandle DMGetNextScreenDevice(GDHandle dev, int activeOnly)
{
    return (GDHandle)0;
}

OSStatus CreateNewWindow(unsigned int wClass, unsigned int attrs, const Rect *bounds, WindowRef *outWindow)
{
    if (outWindow)
        *outWindow = (WindowRef)0;
    return 0;
}

#if !defined(_M_X64) || defined(__MINGW32__)  /* MSVC x64: real user32 MoveWindow (Mac Carbon stub collides by name); MinGW links these as 32 bits does */
void MoveWindow(WindowRef window, short h, short v, Boolean front)
{
}
#endif

void SetWindowContentColor(WindowRef window, const void *color)
{
}

void SetWindowTitleWithCFString(WindowRef window, CFStringRef title)
{
}

OSStatus GetAvailableWindowPositioningBounds(GDHandle device, Rect *bounds)
{
    if (bounds) {
        bounds->top = 0;
        bounds->left = 0;
        bounds->bottom = 1080;
        bounds->right = 1920;
    }
    return 0;
}

void SetSystemUIMode(unsigned int mode, unsigned int options)
{
}

void HideMenuBar(void)
{
}

void ShowMenuBar(void)
{
}

void InvalMenuBar(void)
{
}

MenuRef NewMenu(short menuID, const void *menuTitle)
{
    return (MenuRef)0;
}

void InsertMenu(MenuRef menu, short beforeID)
{
}

void InsetRect(Rect *r, short dh, short dv)
{
    if (r) {
        r->top += dv;
        r->left += dh;
        r->bottom -= dv;
        r->right -= dh;
    }
}

void OffsetRect(Rect *r, short dh, short dv)
{
    if (r) {
        r->top += dv;
        r->left += dh;
        r->bottom += dv;
        r->right += dh;
    }
}

void MapRect(Rect *r, const Rect *srcRect, const Rect *dstRect)
{
}

Boolean PtInRect(Point pt, const Rect *r)
{
    return 0;
}

void QDGlobalToLocalPoint(CGrafPtr port, Point *pt)
{
}

void QDLocalToGlobalPoint(CGrafPtr port, Point *pt)
{
}

OSStatus DrawThemeFocusRect(const Rect *r, Boolean hasFocus)
{
    return 0;
}

OSStatus GetIconRefFromFileInfo(const void *ref, int nameLen, const void *name, unsigned int inWhich, unsigned int inFlags, void *iconLabel, void **outIcon)
{
    if (outIcon)
        *outIcon = (void *)0;
    return 0;
}

void PlotIconRef(const Rect *r, short align, short transform, unsigned int usage, void *iconRef)
{
}

void ReleaseIconRef(void *iconRef)
{
}

unsigned int GetCurrentKeyModifiers(void)
{
    return 0;
}

int CloseMovieFile(short refNum)
{
    (void)refNum;
    return 0;
}

void EndMediaEdits(void *media)
{
}

long GetMediaDuration(void *media)
{
    return 0;
}

OSErr InsertMediaIntoTrack(void *track, int trackStart, int mediaTime, int mediaDuration, int mediaRate)
{
    return 0;
}

unsigned char gluCheckExtension(const unsigned char *extName, const unsigned char *extString)
{
    return 0;
}

void gluOrtho2D(double left, double right, double bottom, double top)
{
    glOrtho(left, right, bottom, top, -1.0, 1.0);
}

int IOObjectConformsTo(unsigned int object, const char *className)
{
    return 0;
}

int IORegistryEntryGetParentEntry(unsigned int entry, const char *plane, unsigned int *parent)
{
    if (parent)
        *parent = 0;
    return -1;
}

#include "common_types.h"
#include <ctype.h>
#include "imports.h"

extern void Sys_QueEvent(int time, sysEventType_t type, int value, int value2,
                         int ptrLength, void *ptr);
extern void CL_MouseEvent(int dx, int dy);
extern UInt8 TranslateKeyCodeToVK(UInt32 inKeyCode);
extern dvar_t *Dvar_RegisterBool(const char *name, int value, int flags);
extern void Dvar_ClearModified(dvar_t *var);
extern void Com_Printf(const char *fmt, ...);
#ifdef __EMSCRIPTEN__
#    include <ctype.h>
#else
#endif

#define K_BACKSPACE 0x08
#define K_MOUSE1 0xc8
#define K_MWHEELDOWN 0xcd
#define K_MWHEELUP 0xce

__attribute__((used)) static unsigned char extendedVirtualKeyConvert[64] = {
    0xb5,
    0x80,
    0xbf,
    0x81,
    0xdf,
    0x82,
    0xe0,
    0x83,
    0xe1,
    0x84,
    0xe4,
    0x85,
    0xe5,
    0x86,
    0xe6,
    0x87,
    0xe7,
    0x88,
    0xe8,
    0x89,
    0xe9,
    0x8a,
    0xec,
    0x8b,
    0xf1,
    0x8c,
    0xf2,
    0x8d,
    0xf3,
    0x8e,
    0xf6,
    0x8f,
    0xf8,
    0x90,
    0xf9,
    0x91,
    0xfa,
    0x92,
    0xfc,
    0x93,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

__attribute__((used)) static unsigned char virtualKeyConvert[320] = {
    0x00,
    0x00,
    0xc8,
    0xc8,
    0xc9,
    0xc9,
    0x00,
    0x00,
    0xca,
    0xca,
    0xcb,
    0xcb,
    0xcc,
    0xcc,
    0x00,
    0x00,
    0x7f,
    0x7f,
    0x09,
    0x09,
    0x00,
    0x00,
    0x00,
    0x00,
    0xba,
    0x00,
    0x0d,
    0xbf,
    0x00,
    0x00,
    0x00,
    0x00,
    0xa0,
    0xa0,
    0x9f,
    0x9f,
    0x9e,
    0x9e,
    0x99,
    0x99,
    0x97,
    0x97,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x1b,
    0x1b,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x20,
    0x20,
    0xb8,
    0xa4,
    0xbe,
    0xa3,
    0xbc,
    0xa6,
    0xb6,
    0xa5,
    0xb9,
    0x9c,
    0xb7,
    0x9a,
    0xbb,
    0x9d,
    0xbd,
    0x9b,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xc0,
    0xa1,
    0xc1,
    0xa2,
    0x00,
    0x00,
    0x30,
    0x30,
    0x31,
    0x31,
    0x32,
    0x32,
    0x33,
    0x33,
    0x34,
    0x34,
    0x35,
    0x35,
    0x36,
    0x36,
    0x37,
    0x37,
    0x38,
    0x38,
    0x39,
    0x39,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x61,
    0x61,
    0x62,
    0x62,
    0x63,
    0x63,
    0x64,
    0x64,
    0x65,
    0x65,
    0x66,
    0x66,
    0x67,
    0x67,
    0x68,
    0x68,
    0x69,
    0x69,
    0x6a,
    0x6a,
    0x6b,
    0x6b,
    0x6c,
    0x6c,
    0x6d,
    0x6d,
    0x6e,
    0x6e,
    0x6f,
    0x6f,
    0x70,
    0x70,
    0x71,
    0x71,
    0x72,
    0x72,
    0x73,
    0x73,
    0x74,
    0x74,
    0x75,
    0x75,
    0x76,
    0x76,
    0x77,
    0x77,
    0x78,
    0x78,
    0x79,
    0x79,
    0x7a,
    0x7a,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xc0,
    0xc0,
    0xbc,
    0xbc,
    0xbd,
    0xbd,
    0xbe,
    0xbe,
    0xb9,
    0xb9,
    0xba,
    0xba,
    0xbb,
    0xbb,
    0xb6,
    0xb6,
    0xb7,
    0xb7,
    0xb8,
    0xb8,
    0xc6,
    0xc6,
    0xc4,
    0xc4,
    0x00,
    0x00,
    0xc3,
    0xc3,
    0xc1,
    0xc1,
    0xc2,
    0xc2,
    0xa7,
    0xa7,
    0xa8,
    0xa8,
    0xa9,
    0xa9,
    0xaa,
    0xaa,
    0xab,
    0xab,
    0xac,
    0xac,
    0xad,
    0xad,
    0xae,
    0xae,
    0xaf,
    0xaf,
    0xb0,
    0xb0,
    0xb1,
    0xb1,
    0xb2,
    0xb2,
    0xb3,
    0xb3,
    0xb4,
    0xb4,
    0xb5,
    0xb5,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xc5,
    0xc5,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

dvar_t *in_mouse;

void IN_Init(void)
{
    dvar_t *p = Dvar_RegisterBool("in_mouse", 1, 0x1021);
    in_mouse = p;
    if (!p->current.enabled) {
        Com_Printf("Mouse control not active.\n");
        p = in_mouse;
    }
    Dvar_ClearModified(p);
}

void IN_Shutdown(void)
{
}

void CCallOfDutyEngine_DoMouseMoved(const CCallOfDutyEngine *_this,
                                    Point inDelta, Point inPoint,
                                    UInt32 inModifiers)
{

    CL_MouseEvent((SInt16)inDelta.h, (SInt16)inDelta.v);
}

void CCallOfDutyEngine_DoMouseWheel(const CCallOfDutyEngine *_this,
                                    SInt32 inDelta, Point inPoint,
                                    UInt32 inModifiers)
{
    if (inDelta > 0) {
        Sys_QueEvent(0, 1, K_MWHEELUP, 1, 0, 0);
        Sys_QueEvent(0, 1, K_MWHEELUP, 0, 0, 0);
    } else {
        Sys_QueEvent(0, 1, K_MWHEELDOWN, 1, 0, 0);
        Sys_QueEvent(0, 1, K_MWHEELDOWN, 0, 0, 0);
    }
}

void CCallOfDutyEngine_DoTextInput(const CCallOfDutyEngine *_this,
                                   UInt16 inUniChar)
{
    if (!isprint((UInt16)inUniChar) && inUniChar != 8) {
        return;
    }
    Sys_QueEvent(((WinVars_t *)imp_g_wv)->sysMsgTime, 2,
                 (UInt16)inUniChar, 0, 0, 0);
}

void CCallOfDutyEngine_DoKeyDown(const CCallOfDutyEngine *_this,
                                 int inCharCode, UInt32 inKeyCode)
{
    unsigned char vk;
    unsigned char keysym;

    if (inKeyCode <= 0x7f) {
        vk = TranslateKeyCodeToVK(inKeyCode);
        if (vk < 0x80) {
            keysym = virtualKeyConvert[vk * 2];
        } else {
            keysym = extendedVirtualKeyConvert[(vk - 0xc0) * 2];
        }
    } else {
        keysym = 0;
    }
    if (keysym) {
        Sys_QueEvent(0, 1, keysym, 1, 0, 0);
    }
}

void CCallOfDutyEngine_DoKeyUp(const CCallOfDutyEngine *_this,
                               int inCharCode, UInt32 inKeyCode)
{
    unsigned char vk;
    unsigned char keysym;

    if (inKeyCode <= 0x7f) {
        vk = TranslateKeyCodeToVK(inKeyCode);
        if (vk < 0x80) {
            keysym = virtualKeyConvert[vk * 2 + 1];
        } else {
            keysym = extendedVirtualKeyConvert[(vk - 0xc0) * 2 + 1];
        }
    } else {
        keysym = 0;
    }
    if (keysym) {
        Sys_QueEvent(0, 1, keysym, 0, 0, 0);
    }
}

void CCallOfDutyEngine_DoMouseDown(const CCallOfDutyEngine *_this,
                                   UInt16 inButton, Point inPoint,
                                   UInt32 inModifiers, UInt32 inClickCount)
{
    UInt32 *mousestate = (UInt32 *)((char *)_this + 0x20);
    UInt32 newmask;
    int bit;

    if (inButton > 0x20)
        return;
    newmask = (*mousestate) | (1u << (inButton - 1));
    *mousestate = newmask;
    for (bit = 0; bit < 5; bit++) {
        if ((newmask >> bit) & 1) {
            Sys_QueEvent(0, 1, K_MOUSE1 + bit, 1, 0, 0);
        }
    }
}

void CCallOfDutyEngine_DoMouseUp(const CCallOfDutyEngine *_this,
                                 UInt16 inButton, Point inPoint)
{
    UInt32 *mousestate = (UInt32 *)((char *)_this + 0x20);
    UInt32 newmask;
    int bit;

    if (inButton > 0x20)
        return;
    newmask = (*mousestate) & ~(1u << (inButton - 1));
    *mousestate = newmask;
    for (bit = 0; bit < 5; bit++) {
        if (((newmask >> bit) & 1) == 0) {
            Sys_QueEvent(0, 1, K_MOUSE1 + bit, 0, 0, 0);
        }
    }
}

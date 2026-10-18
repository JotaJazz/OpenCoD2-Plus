#include "common_types.h"
#include "imports.h"

typedef struct {
    short top;
    short left;
    short bottom;
    short right;
} Rect;

static ControlKeyFilterUPP sControlKeyFilterUPP;
static ControlEditTextValidationUPP sControlValidationUPP;
static MacBuilderProcPtr sBuilderProcPtr;

void StPortState_StPortState(StPortState *_this, WindowRef inWindow);
void ZN11StPortStateD1Ev(StPortState *_this);
ControlPartCode UserPaneHitTestProc(ControlRef theControl, struct Point theWherePt);
WindowRef MacBuilder_BuildWindow(CFStringRef inName, CFStringRef inNibName, int inStandardHandler, MacBuilderProcPtr inBuilderProc);
UInt32 MacBuilder_RunModalWindow(WindowRef inWindow);
void MacBuilder_ReleaseWindow(WindowRef inWindow);
ControlRef MacBuilder_GetControlRef(WindowRef inWindow, SInt32 inID);
static inflate_blocks_statef ValidationProc(ControlRef inControlRef);
static OSStatus HandleStandardEvents(EventRef inEvent, inflate_blocks_statef *inUserData);
static ControlKeyFilterResult KeyFilterProc(ControlRef inControlRef, SInt16 *ioCharCode, EventModifiers *ioModifiers);
inflate_blocks_statef MacBuilder_SetTextObjectFontStyle(WindowRef inWindow, SInt32 inID, int inFontFamilyID, int inFontSize);
void MacBuilder_GetControlText(WindowRef inWindow, SInt32 inID, int inBufferSize, char *outText);
void MacBuilder_SetControlText(WindowRef inWindow, SInt32 inID, const char *inText);
void MacBuilder_SetControlVisible(WindowRef inWindow, SInt32 inID, Bool inVisible);
void MacBuilder_SetControlEnabled(WindowRef inWindow, SInt32 inID, Bool inEnabled);
void MacBuilder_SetControlFontStyle(WindowRef inWindow, SInt32 inID, int inFontFamilyID, int inFontSize);
void MacBuilder_SetControlFocus(WindowRef inWindow, SInt32 inID);
void MacBuilder_SetupUserPaneControl(WindowRef inWindow, SInt32 inID, ControlUserPaneDrawProcPtr inDrawProc, ControlUserPaneTrackingProcPtr inTrackingProc);
inflate_blocks_statef MacBuilder_SetEditTextHook(WindowRef inWindow, SInt32 inID, MacBuilderEditHookPtr inHookProc, int inCharLimit);
TXNObject MacBuilder_GetTextObject(WindowRef inWindow, SInt32 inID);

void StPortState_StPortState(StPortState *_this, WindowRef inWindow)
{
    GetPort(_this);
    SetPortWindowPort(inWindow);
}

void ZN11StPortStateD1Ev(StPortState *_this)
{
    SetPort(*(void **)_this);
}

ControlPartCode UserPaneHitTestProc(ControlRef theControl, struct Point theWherePt)
{
    Rect bounds;
    GetControlBounds(theControl, &bounds);
    return PtInRect(theWherePt, &bounds) != 0;
}

WindowRef MacBuilder_BuildWindow(CFStringRef inName, CFStringRef inNibName, int inStandardHandler, MacBuilderProcPtr inBuilderProc)
{
    return 0;
}

UInt32 MacBuilder_RunModalWindow(WindowRef inWindow)
{
    InitCursor();
    ShowWindow(inWindow);
    RunAppModalLoopForWindow(inWindow);
    HideWindow(inWindow);
    return GetWRefCon(inWindow);
}

void MacBuilder_ReleaseWindow(WindowRef inWindow)
{
    DisposeWindow(inWindow);
    sBuilderProcPtr = 0;
}

ControlRef MacBuilder_GetControlRef(WindowRef inWindow, SInt32 inID)
{
    ControlID controlID;
    controlID.signature = 0x4974656d;
    controlID.id = inID;
    ControlRef theControl = NULL;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0) {
        return 0;
    }
    return theControl;
}

static inflate_blocks_statef ValidationProc(ControlRef inControlRef)
{
    {
        inflate_blocks_statef zero;
        memset(&zero, 0, sizeof(zero));
        return zero;
    }
}

static OSStatus HandleStandardEvents(EventRef inEvent, inflate_blocks_statef *inUserData)
{
    return 0;
}

static ControlKeyFilterResult KeyFilterProc(ControlRef inControlRef, SInt16 *ioCharCode, EventModifiers *ioModifiers)
{
    return 0;
}

inflate_blocks_statef MacBuilder_SetTextObjectFontStyle(WindowRef inWindow, SInt32 inID, int inFontFamilyID, int inFontSize)
{
    {
        inflate_blocks_statef zero;
        memset(&zero, 0, sizeof(zero));
        return zero;
    }
}

void MacBuilder_GetControlText(WindowRef inWindow, SInt32 inID, int inBufferSize, char *outText)
{
    ControlID controlID;
    ControlRef theControl = NULL;

    *outText = '\0';

    controlID.signature = 0x4974656d;
    controlID.id = inID;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    Size actualSize;
    error = GetControlData(theControl, 0, 0x74657874 , (short)inBufferSize - 1, outText, &actualSize);
    if (error == 0)
        outText[actualSize] = '\0';
}

void MacBuilder_SetControlText(WindowRef inWindow, SInt32 inID, const char *inText)
{
    ControlID controlID;
    controlID.signature = 0x4974656d;
    controlID.id = inID;
    ControlRef theControl = NULL;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    SetControlData(theControl, 0, 0x74657874 , strlen(inText), inText);
}

void MacBuilder_SetControlVisible(WindowRef inWindow, SInt32 inID, Bool inVisible)
{
    ControlID controlID;
    controlID.signature = 0x4974656d;
    controlID.id = inID;
    ControlRef theControl = NULL;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    if (inVisible)
        ShowControl(theControl);
    else
        HideControl(theControl);
}

void MacBuilder_SetControlEnabled(WindowRef inWindow, SInt32 inID, Bool inEnabled)
{
    ControlID controlID;
    controlID.signature = 0x4974656d;
    controlID.id = inID;
    ControlRef theControl = NULL;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    if (inEnabled)
        EnableControl(theControl);
    else
        DisableControl(theControl);
}

void MacBuilder_SetControlFontStyle(WindowRef inWindow, SInt32 inID, int inFontFamilyID, int inFontSize)
{
    ControlID controlID;
    ControlRef theControl = NULL;

    controlID.signature = 0x4974656d;
    controlID.id = inID;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    ControlFontStyleRec fontStyle;
    fontStyle.flags = 5;
    fontStyle.font = (short)inFontFamilyID;
    fontStyle.size = (short)inFontSize;
    SetControlFontStyle(theControl, &fontStyle);
}

void MacBuilder_SetControlFocus(WindowRef inWindow, SInt32 inID)
{
    ControlID controlID;
    ControlRef theControl = NULL;

    controlID.signature = 0x4974656d;
    controlID.id = inID;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    ControlKind kind;
    if (GetControlKind(theControl, &kind) != 0)
        return;

    if (kind.kind == 0x65747874  || kind.kind == 0x65757478 ) {
        SetKeyboardFocus(inWindow, theControl, 5);
    }
}

void MacBuilder_SetupUserPaneControl(WindowRef inWindow, SInt32 inID, ControlUserPaneDrawProcPtr inDrawProc, ControlUserPaneTrackingProcPtr inTrackingProc)
{
    ControlID controlID;
    ControlRef theControl = NULL;

    controlID.signature = 0x4974656d;
    controlID.id = inID;

    OSStatus error = GetControlByID(inWindow, &controlID, &theControl);
    if (error != 0)
        theControl = NULL;

    if (!theControl)
        return;

    ControlUserPaneDrawUPP drawUPP = NewControlUserPaneDrawUPP(inDrawProc);
    SetControlData(theControl, 0, 0x64726177 , 4, &drawUPP);

    if (!inTrackingProc)
        return;

    ControlUserPaneHitTestUPP testUPP = NewControlUserPaneHitTestUPP(UserPaneHitTestProc);
    SetControlData(theControl, 0, 0x68697474 , 4, &testUPP);

    ControlUserPaneTrackingUPP trackingUPP = NewControlUserPaneTrackingUPP(inTrackingProc);
    SetControlData(theControl, 0, 0x7472616b , 4, &trackingUPP);
}

inflate_blocks_statef MacBuilder_SetEditTextHook(WindowRef inWindow, SInt32 inID, MacBuilderEditHookPtr inHookProc, int inCharLimit)
{
    {
        inflate_blocks_statef zero;
        memset(&zero, 0, sizeof(zero));
        return zero;
    }
}

TXNObject MacBuilder_GetTextObject(WindowRef inWindow, SInt32 inID)
{
    return 0;
}

#ifndef SCR_DEBUGGER_UI_H
#define SCR_DEBUGGER_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cod2_feature_config.h"

#if COD2_FEATURE_SCRIPT_DEBUGGER

struct Material;
struct VariableValue_s;

struct Scr_AddFileInfo;
struct Scr_SelectedLineInfo;

typedef struct Scr_WatchElement_s Scr_WatchElement_s;
typedef struct Scr_Breakpoint Scr_Breakpoint;

typedef enum Scr_ConsoleOpenMode {
    SCR_CONSOLE_OPEN_INSERT = 0,
    SCR_CONSOLE_OPEN_REPLACE = 1
} Scr_ConsoleOpenMode;

struct Scr_Breakpoint {
     int codePos;
     int scriptFile;
     Scr_WatchElement_s *watchElement;
     int enabled;
     Scr_Breakpoint *next;
     int _pad14;
};

struct Scr_WatchElement_s {
     int header;
     int _pad004;
     char valueText[0x101];
     char name[129];
     char _pad18A[6];
     unsigned char kind;
     unsigned char _pad191;
     unsigned char expanded;
     unsigned char breakpointKind;
     unsigned char _pad194;
     unsigned char changed;
     unsigned char hasValueRef;
     unsigned char sortable;
     unsigned int value;
     int valueType;
     int _pad1A0;
     int childCount;
     int _pad1A8;
     int id;
     int readOnly;
     int sourcePos;
     char _pad1B8[0x0C];
     Scr_WatchElement_s *parent;
     Scr_WatchElement_s *childArray;
     Scr_WatchElement_s *firstChild;
     Scr_WatchElement_s *nextSibling;
};

#    define SCR_WATCHELEM_OBJECTID(e) (*(int *)((char *)(e) + 0x18C))

typedef struct Scr_ScriptCallStackEntry {
     int sourceBuffer;
     int sourcePos;
} Scr_ScriptCallStackEntry;

#    define SCR_CALLSTACK_MAX_FRAMES 64

typedef struct UI_Component UI_Component;
typedef struct UI_LinesComponent UI_LinesComponent;
typedef struct UI_ScrollPane UI_ScrollPane;
typedef struct UI_VerticalDivider UI_VerticalDivider;
typedef struct Scr_ScriptWindow Scr_ScriptWindow;
typedef struct Scr_AbstractScriptList Scr_AbstractScriptList;
typedef struct Scr_ScriptList Scr_ScriptList;
typedef struct Scr_OpenScriptList Scr_OpenScriptList;
typedef struct Scr_ScriptCallStack Scr_ScriptCallStack;
typedef struct Scr_ScriptWatch Scr_ScriptWatch;

typedef struct UI_Component_vtbl {
     void (*Init)(UI_Component *self);
     void (*Shutdown)(UI_Component *self);
     void (*dtor)(UI_Component *self, unsigned int flags);
     void (*Draw)(UI_Component *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(UI_Component *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(UI_Component *self, float *pos);
     void (*AddText)(UI_Component *self, const char *text);
} UI_Component_vtbl;

typedef struct UI_LinesComponent_vtbl {
     void (*Init)(UI_LinesComponent *self);
     void (*Shutdown)(UI_LinesComponent *self);
     void (*dtor)(UI_LinesComponent *self, unsigned int flags);
     void (*Draw)(UI_LinesComponent *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(UI_LinesComponent *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(UI_LinesComponent *self, float *pos);
     void (*AddText)(UI_LinesComponent *self, const char *text);
     int (*SetSelectedLineFocus)(UI_LinesComponent *self, int line, int fromMouse);
} UI_LinesComponent_vtbl;

typedef struct UI_ScrollPane_vtbl {
     void (*Init)(UI_ScrollPane *self);
     void (*Shutdown)(UI_ScrollPane *self);
     void (*dtor)(UI_ScrollPane *self, unsigned int flags);
     void (*Draw)(UI_ScrollPane *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(UI_ScrollPane *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(UI_ScrollPane *self, float *pos);
     void (*AddText)(UI_ScrollPane *self, const char *text);
} UI_ScrollPane_vtbl;

typedef struct UI_VerticalDivider_vtbl {
     void (*Init)(UI_VerticalDivider *self);
     void (*Shutdown)(UI_VerticalDivider *self);
     void (*dtor)(UI_VerticalDivider *self, unsigned int flags);
     void (*Draw)(UI_VerticalDivider *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(UI_VerticalDivider *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(UI_VerticalDivider *self, float *pos);
     void (*AddText)(UI_VerticalDivider *self, const char *text);
} UI_VerticalDivider_vtbl;

typedef struct Scr_ScriptWindow_vtbl {
     void (*Init)(Scr_ScriptWindow *self);
     void (*Shutdown)(Scr_ScriptWindow *self);
     void (*dtor)(Scr_ScriptWindow *self, unsigned int flags);
     void (*Draw)(Scr_ScriptWindow *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(Scr_ScriptWindow *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(Scr_ScriptWindow *self, float *pos);
     void (*AddText)(Scr_ScriptWindow *self, const char *text);
     int (*SetSelectedLineFocus)(Scr_ScriptWindow *self, int line, int fromMouse);
} Scr_ScriptWindow_vtbl;

typedef struct Scr_AbstractScriptList_vtbl {
     void (*Init)(Scr_AbstractScriptList *self);
     void (*Shutdown)(Scr_AbstractScriptList *self);
     void (*dtor)(Scr_AbstractScriptList *self, unsigned int flags);
     void (*Draw)(Scr_AbstractScriptList *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(Scr_AbstractScriptList *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(Scr_AbstractScriptList *self, float *pos);
     void (*AddText)(Scr_AbstractScriptList *self, const char *text);
     int (*SetSelectedLineFocus)(Scr_AbstractScriptList *self, int line, int fromMouse);
} Scr_AbstractScriptList_vtbl;

typedef struct Scr_ScriptList_vtbl {
     void (*Init)(Scr_ScriptList *self);
     void (*Shutdown)(Scr_ScriptList *self);
     void (*dtor)(Scr_ScriptList *self, unsigned int flags);
     void (*Draw)(Scr_ScriptList *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(Scr_ScriptList *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(Scr_ScriptList *self, float *pos);
     void (*AddText)(Scr_ScriptList *self, const char *text);
     int (*SetSelectedLineFocus)(Scr_ScriptList *self, int line, int fromMouse);
} Scr_ScriptList_vtbl;

typedef struct Scr_OpenScriptList_vtbl {
     void (*Init)(Scr_OpenScriptList *self);
     void (*Shutdown)(Scr_OpenScriptList *self);
     void (*dtor)(Scr_OpenScriptList *self, unsigned int flags);
     void (*Draw)(Scr_OpenScriptList *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(Scr_OpenScriptList *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(Scr_OpenScriptList *self, float *pos);
     void (*AddText)(Scr_OpenScriptList *self, const char *text);
     int (*SetSelectedLineFocus)(Scr_OpenScriptList *self, int line, int fromMouse);
} Scr_OpenScriptList_vtbl;

typedef struct Scr_ScriptCallStack_vtbl {
     void (*Init)(Scr_ScriptCallStack *self);
     void (*Shutdown)(Scr_ScriptCallStack *self);
     void (*dtor)(Scr_ScriptCallStack *self, unsigned int flags);
     void (*Draw)(Scr_ScriptCallStack *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(Scr_ScriptCallStack *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(Scr_ScriptCallStack *self, float *pos);
     void (*AddText)(Scr_ScriptCallStack *self, const char *text);
     int (*SetSelectedLineFocus)(Scr_ScriptCallStack *self, int line, int fromMouse);
} Scr_ScriptCallStack_vtbl;

typedef struct Scr_ScriptWatch_vtbl {
     void (*Init)(Scr_ScriptWatch *self);
     void (*Shutdown)(Scr_ScriptWatch *self);
     void (*dtor)(Scr_ScriptWatch *self, unsigned int flags);
     void (*Draw)(Scr_ScriptWatch *self, float x, float y, float w, float h, float a, float b);
     int (*KeyEvent)(Scr_ScriptWatch *self, float *pos, int key);
     UI_Component *(*GetCompAtLocation)(Scr_ScriptWatch *self, float *pos);
     void (*AddText)(Scr_ScriptWatch *self, const char *text);
     int (*SetSelectedLineFocus)(Scr_ScriptWatch *self, int line, int fromMouse);
} Scr_ScriptWatch_vtbl;

struct UI_Component {
     const UI_Component_vtbl *vtbl;
     float w;
     float h;
     int flags;
};

struct UI_LinesComponent {
     const UI_LinesComponent_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int lineCount;
     float scrollTop;
     float _f20;
};

struct Scr_ScriptWindow {
     const Scr_ScriptWindow_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int lineCount;
     float scrollTop;
     float _f20;
     int scriptFileIndex;
     int currentLine;
     char *currentLinePtr;
     Scr_Breakpoint *breakpoints;
     int maxLineWidth;
};

struct Scr_AbstractScriptList {
     const Scr_AbstractScriptList_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int entryCount;
     float scrollTop;
     float _f20;
     Scr_ScriptWindow **entries;
};

struct Scr_ScriptList {
     const Scr_ScriptList_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int entryCount;
     float scrollTop;
     float _f20;
     Scr_ScriptWindow **entries;
};

struct Scr_OpenScriptList {
     const Scr_OpenScriptList_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int entryCount;
     float scrollTop;
     float _f20;
     Scr_ScriptWindow **entries;
     void *savedFileList;
};

struct Scr_ScriptCallStack {
     const Scr_ScriptCallStack_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int frameCount;
     float scrollTop;
     float _f20;
     Scr_ScriptCallStackEntry frames[SCR_CALLSTACK_MAX_FRAMES];
};

struct Scr_ScriptWatch {
     const Scr_ScriptWatch_vtbl *vtbl;
     float w;
     float h;
     int flags;
     int selectedLine;
     unsigned char hasFocus;
     unsigned char focusFromMouse;
     unsigned char _pad16[2];
     int lineCount;
     float scrollTop;
     float _f20;
     Scr_WatchElement_s *root;
     int field28;
     int threadId;
     int field30;
};

struct Material *UI_Component_RegisterMaterialNoMip(const char *name, int mipLevel);
void UI_Component_DrawPic(float x, float y, float w, float h, const float *color, struct Material *mat);
void UI_Component_DrawPicRotate(float x, float y, float w, float h, const float *color, struct Material *mat);
void UI_Component_DrawText(float x, float y, int font, const float *color, const char *text);
void UI_Component_DrawCursor(void);
void UI_Component_MouseEvent(int x, int y);
void UI_Component_GetCursorPos(int *x, int *y);
void UI_Component_InitAssets(void);
void UI_Component_Init(UI_Component *self);
void UI_Component_Shutdown(UI_Component *self);
void UI_Component_dtor(UI_Component *self);
UI_Component *UI_Component_GetCompAtLocation(UI_Component *self, float *pos);
void UI_Component_AddText(UI_Component *self, const char *text);
void UI_Component_Init_Global(void);

void UI_LinesComponent_Init(UI_LinesComponent *self);
int UI_LinesComponent_SetSelectedLineFocus(UI_LinesComponent *self, int line, int fromMouse);
void UI_LinesComponent_IncSelectedLineFocus(UI_LinesComponent *self, int fromMouse);
void UI_LinesComponent_DecSelectedLineFocus(UI_LinesComponent *self, int fromMouse);
void UI_LinesComponent_UpdateHeight(UI_LinesComponent *self);
int UI_LinesComponent_KeyEvent(UI_LinesComponent *self, float *pos, int key);
void UI_LinesComponent_AddText(UI_LinesComponent *self, const char *text);
void UI_LinesComponent_ClearFocus(UI_LinesComponent *self);

void UI_ScrollPane_Init(UI_ScrollPane *self);
int UI_ScrollPane_GetFirstDisplayedLine(UI_ScrollPane *self);
int UI_ScrollPane_GetInnerSize(UI_ScrollPane *self, float *outSize);
int UI_ScrollPane_GetInnerLinesCount(UI_ScrollPane *self);
UI_Component *UI_ScrollPane_GetCompAtLocation(UI_ScrollPane *self, float *pos);
void UI_ScrollPane_AddText(UI_ScrollPane *self, const char *text);
void UI_ScrollPane_SetPos(UI_ScrollPane *self);
int UI_ScrollPane_GetLastDisplayedLine(UI_ScrollPane *self);
void UI_ScrollPane_DisplaySelectedLine(UI_ScrollPane *self);
int UI_ScrollPane_KeyEvent(UI_ScrollPane *self, float *pos, int key);
void UI_ScrollPane_CheckMouseScroll(UI_ScrollPane *self, int which, float *a, float *b, float c);
void UI_ScrollPane_Draw(UI_ScrollPane *self, float x, float y, float w, float h, float a, float b);

void UI_VerticalDivider_Init(UI_VerticalDivider *self);
void UI_VerticalDivider_DrawTop(UI_VerticalDivider *self, float x, float y, float w, float h);
void UI_VerticalDivider_Draw(UI_VerticalDivider *self, float x, float y, float w, float h, float a, float b);
int UI_VerticalDivider_KeyEvent(UI_VerticalDivider *self, float *pos, int key);
UI_Component *UI_VerticalDivider_GetCompAtLocation(UI_VerticalDivider *self, float *pos);

void Scr_ScriptWindow_SetScriptFile(Scr_ScriptWindow *self, const char *filename);
void Scr_ScriptWindow_SetCurrentLine(Scr_ScriptWindow *self, int line);
const char *Scr_ScriptWindow_GetFilename(Scr_ScriptWindow *self);
void Scr_ScriptWindow_Draw(Scr_ScriptWindow *self, float x, float y, float w, float h, float a, float b);
void Scr_ScriptWindow_AddBreakpoint(Scr_ScriptWindow *self, Scr_Breakpoint **list, char *codePos, Scr_WatchElement_s *elem, int enabled);
const char *Scr_ScriptWindow_GetBreakpointCodePos(Scr_ScriptWindow *self);
void Scr_ScriptWindow_RunToCursor(Scr_ScriptWindow *self);
void Scr_ScriptWindow_FindNext(Scr_ScriptWindow *self);
void Scr_ScriptWindow_FindPrev(Scr_ScriptWindow *self);
void Scr_ScriptWindow_CopySelectedText(Scr_ScriptWindow *self);
void Scr_ScriptWindow_Init(Scr_ScriptWindow *self);
void Scr_ScriptWindow_ToggleBreakpointInternal(Scr_ScriptWindow *self, Scr_WatchElement_s *elem, int a, int b, int c, int d);
void Scr_ScriptWindow_ToggleBreakpoint(Scr_ScriptWindow *self, Scr_WatchElement_s *elem, int a, int b, int c, int d);
void Scr_ScriptWindow_EnterCallInternal(Scr_ScriptWindow *self);
void Scr_ScriptWindow_EnterCall(Scr_ScriptWindow *self);
int Scr_ScriptWindow_KeyEvent(Scr_ScriptWindow *self, float *pos, int key);

void Scr_AbstractScriptList_Draw(Scr_AbstractScriptList *self, float x, float y, float w, float h, float a, float b);
void Scr_AbstractScriptList_Init(Scr_AbstractScriptList *self);
void Scr_AbstractScriptList_Shutdown(Scr_AbstractScriptList *self);
void Scr_AbstractScriptList_AddEntry(Scr_AbstractScriptList *self, Scr_ScriptWindow *win, int select);
int Scr_AbstractScriptList_AddEntryName(Scr_AbstractScriptList *self, const char *name, int select);
void Scr_AbstractScriptList_DeleteEntryInternal(Scr_AbstractScriptList *self);
void Scr_AbstractScriptList_DeleteEntry(Scr_AbstractScriptList *self);
void Scr_AbstractScriptList_BackspaceEntry(Scr_AbstractScriptList *self);
void Scr_AbstractScriptList_CopyEntry(Scr_AbstractScriptList *self);
void Scr_AbstractScriptList_PasteEntry(Scr_AbstractScriptList *self);

int Scr_ScriptList_KeyEvent(Scr_ScriptList *self, float *pos, int key);
void Scr_ScriptList_LoadScriptPos(Scr_ScriptList *self);
void Scr_ScriptList_Shutdown(Scr_ScriptList *self);
void Scr_ScriptList_AddFile(Scr_ScriptList *self, const char *filename, struct Scr_AddFileInfo *info);
void Scr_ScriptList_Init(Scr_ScriptList *self);

int Scr_OpenScriptList_SetSelectedLineFocus(Scr_OpenScriptList *self, int line, int fromMouse);
int Scr_OpenScriptList_KeyEvent(Scr_OpenScriptList *self, float *pos, int key);
int Scr_OpenScriptList_ReadFromFile(Scr_OpenScriptList *self);
void Scr_OpenScriptList_Init(Scr_OpenScriptList *self);
void Scr_OpenScriptList_Shutdown(Scr_OpenScriptList *self);

void Scr_ScriptCallStack_Draw(Scr_ScriptCallStack *self, float x, float y, float w, float h, float a, float b);
int Scr_ScriptCallStack_KeyEvent(Scr_ScriptCallStack *self, float *pos, int key);
void Scr_ScriptCallStack_Init(Scr_ScriptCallStack *self);
void Scr_ScriptCallStack_UpdateStack(Scr_ScriptCallStack *self);
int Scr_ScriptCallStack_SetSelectedLineFocus(Scr_ScriptCallStack *self, int line, int fromMouse);

void Scr_ScriptWatch_Draw_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, float a, float b, float c, float d, int g, int h, int i, int *j, float *k, float l, float m);
void Scr_ScriptWatch_Draw(Scr_ScriptWatch *self, float x, float y, float w, float h, float a, float b);
Scr_WatchElement_s *Scr_ScriptWatch_GetSelectedElement_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int *idx);
Scr_WatchElement_s *Scr_ScriptWatch_GetSelectedElement(Scr_ScriptWatch *self);
Scr_WatchElement_s *Scr_ScriptWatch_GetSelectedNonConditionalElement(Scr_ScriptWatch *self);
Scr_WatchElement_s *Scr_ScriptWatch_GetElementWithId_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int id);
Scr_WatchElement_s *Scr_ScriptWatch_GetElementWithId(Scr_ScriptWatch *self, int id);
Scr_WatchElement_s **Scr_ScriptWatch_GetElementRef(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
Scr_WatchElement_s *Scr_ScriptWatch_GetElementPrev(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
int Scr_ScriptWatch_SetSelectedElement_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, Scr_WatchElement_s *target, int *idx, int b);
void Scr_ScriptWatch_SetSelectedElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int focus);
void Scr_ScriptWatch_CopyElement(Scr_ScriptWatch *self);
void Scr_ScriptWatch_EditElement(Scr_ScriptWatch *self, Scr_ConsoleOpenMode mode);
void Scr_ScriptWatch_SaveSelectedLine(Scr_ScriptWatch *self, struct Scr_SelectedLineInfo *info);
void Scr_ScriptWatch_LoadSelectedLine(Scr_ScriptWatch *self, struct Scr_SelectedLineInfo *info);
int Scr_ScriptWatch_PostEvaluateWatchElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, struct VariableValue_s *v);
int Scr_ScriptWatch_EvaluateWatchChildElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, unsigned int idx, Scr_WatchElement_s *parent, int b);
void Scr_ScriptWatch_EvaluateWatchElementExpression(Scr_ScriptWatch *self, Scr_WatchElement_s *e, struct VariableValue_s *v);
void Scr_ScriptWatch_UpdateHeight(Scr_ScriptWatch *self);
Scr_WatchElement_s *Scr_ScriptWatch_CreateWatchElement(Scr_ScriptWatch *self, const char *name, Scr_WatchElement_s **list, Scr_WatchElement_s *parent);
Scr_WatchElement_s *Scr_ScriptWatch_CloneElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_UpdateBreakpoints(Scr_ScriptWatch *self, int announce);
Scr_WatchElement_s *Scr_ScriptWatch_AddBreakpoint(Scr_ScriptWatch *self, Scr_WatchElement_s *e, unsigned char kind);
Scr_WatchElement_s *Scr_ScriptWatch_RemoveBreakpoint(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
Scr_WatchElement_s *Scr_ScriptWatch_CreateBreakpointElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int line, unsigned int kind, int b);
void Scr_ScriptWatch_FreeWatchElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_EvaluateWatchChildren(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_EvaluateWatchElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_CloneSelectedElement(Scr_ScriptWatch *self);
void Scr_ScriptWatch_Evaluate(Scr_ScriptWatch *self);
void Scr_ScriptWatch_DisplayThreadPos(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
int Scr_ScriptWatch_SetSelectedLineFocus(Scr_ScriptWatch *self, int line, int fromMouse);
void Scr_ScriptWatch_ToggleWatchElementBreakpoint(Scr_ScriptWatch *self, Scr_WatchElement_s *e, unsigned char kind);
void Scr_ScriptWatch_SortHitBreakpointsTop(Scr_ScriptWatch *self);
void Scr_ScriptWatch_ToggleExpandElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_ExpandElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int expand);
void Scr_ScriptWatch_ExpandSelectedElement(Scr_ScriptWatch *self, int expand);
Scr_WatchElement_s *Scr_ScriptWatch_PasteNonBreakpointElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, const char *text, int b);
void Scr_ScriptWatch_InsertElement(Scr_ScriptWatch *self);
Scr_WatchElement_s *Scr_ScriptWatch_DeleteElementInternal(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_DeleteElement(Scr_ScriptWatch *self);
Scr_WatchElement_s *Scr_ScriptWatch_BackspaceElementInternal(Scr_ScriptWatch *self, Scr_WatchElement_s *e);
void Scr_ScriptWatch_BackspaceElement(Scr_ScriptWatch *self);
int Scr_ScriptWatch_LeftMouseEvent(Scr_ScriptWatch *self, float *pos);
void Scr_ScriptWatch_Shutdown(Scr_ScriptWatch *self);
void Scr_ScriptWatch_AddElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, const char *name);
void Scr_ScriptWatch_ToggleBreakpointInternal(Scr_ScriptWatch *self, Scr_WatchElement_s *e, unsigned char kind);
void Scr_ScriptWatch_ToggleBreakpoint(Scr_ScriptWatch *self, Scr_WatchElement_s *e, unsigned char kind);
void Scr_ScriptWatch_PasteBreakpointElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, const char *text, int b, int c, int d);
void Scr_ScriptWatch_PasteElementInternal(Scr_ScriptWatch *self, Scr_WatchElement_s *e, const char *text, int b);
void Scr_ScriptWatch_PasteElement(Scr_ScriptWatch *self);
int Scr_ScriptWatch_KeyEvent(Scr_ScriptWatch *self, float *pos, int key);
int Scr_ScriptWatch_ReadFromFile(Scr_ScriptWatch *self);
void Scr_ScriptWatch_Init(Scr_ScriptWatch *self);
void Scr_ScriptWatch_AddText(Scr_ScriptWatch *self, const char *text);

void Scr_WriteElement(Scr_WatchElement_s *e);
void Scr_ClearElementChanged(Scr_WatchElement_s *e);
int Scr_ElementChildrenExist(Scr_WatchElement_s *e);
Scr_WatchElement_s *Scr_GetElementRoot(Scr_WatchElement_s *e);
void Scr_PrintElementText(Scr_WatchElement_s *e, int a, int b, char *out);
int Scr_IsSortWatchElement(Scr_WatchElement_s *e);
const char *Scr_GetElementArchiveText(Scr_WatchElement_s *e);
int Scr_GetElementDepth(Scr_WatchElement_s *e);
Scr_WatchElement_s *Scr_CreateWatchElement(const char *name, Scr_WatchElement_s **list, Scr_WatchElement_s *parent);
void Scr_RemoveValue(Scr_WatchElement_s *e);
void Scr_FreeWatchElementChildren(Scr_WatchElement_s *e);
int Scr_GetWatchElementSize(Scr_WatchElement_s *e);
const char *Scr_GetElementThreadPos(Scr_WatchElement_s *e);
void Scr_PostSetText(Scr_WatchElement_s *e);
void Scr_DeltaElementValueText(Scr_WatchElement_s *e, char *text);
void Scr_SetNonFieldElementRefText(Scr_WatchElement_s *e);
void Scr_SetElementRefText(Scr_WatchElement_s *e, char *text);
void Scr_DeltaElementRefText(Scr_WatchElement_s *e, char *a, char *b);
void Scr_ConnectElementChildren(Scr_WatchElement_s *e);
void Scr_SortElementChildren(Scr_WatchElement_s *e);
void Scr_FreeWatchElementChildrenStrict(Scr_WatchElement_s *e);
void Scr_RemoveElementValue(Scr_WatchElement_s *e);
void Scr_WatchElementHitBreakpoint(Scr_WatchElement_s *e, int remote);
int Scr_ConditionalExpression(Scr_WatchElement_s *e, unsigned int idx);
int Scr_WatchElementHasSameValue(Scr_WatchElement_s *e, struct VariableValue_s *v);
void Scr_WatchElementHitBreakpointRemote(void);
Scr_WatchElement_s *Scr_ReadElement(void);
void Scr_AddDebugRefCountChildren(Scr_WatchElement_s *e, unsigned short *count);
void Scr_SetSelectionComp(UI_Component *comp);
void Scr_SetMiscScrollPaneComp(UI_LinesComponent *comp);

#endif

#ifdef __cplusplus
}
#endif
#endif

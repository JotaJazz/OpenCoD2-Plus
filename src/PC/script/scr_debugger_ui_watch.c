#include "cod2_feature_config.h"
#include "scr_debugger_ui.h"

#if COD2_FEATURE_SCRIPT_DEBUGGER

#    include <string.h>

typedef struct Material *MaterialHandle;
typedef struct Font *FontHandle;
typedef float vec_t;

typedef struct refexport_s {
    void *_slots0to36[37];
    void (*DrawStretchPic)(float x, float y, float w, float h,
                           float s0, float t0, float s1, float t1,
                           const vec_t *color, MaterialHandle material);
    void *_slots38to68[31];
    int (*TextWidth)(const char *text, int maxChars, FontHandle font);
    int (*TextHeight)(FontHandle font);
    void (*DrawText)(const char *text, int maxChars, FontHandle font,
                     float x, float y, float xScale, float yScale,
                     const vec_t *color, int style);
} refexport_t;

extern void *imp_re;
extern void *imp_cls;

extern void Com_Printf(const char *fmt, ...);
extern int Com_Milliseconds(void);
extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern void I_strncpyz(char *dst, const char *src, int len);
extern void I_strncat(char *dst, int dstSize, const char *src);
extern int I_stricmp(const char *a, const char *b);
extern int I_strnicmp(const char *a, const char *b, int n);
extern char *va(const char *fmt, ...);
extern void Com_sprintf(char *dst, int size, const char *fmt, ...);
extern void qsort(void *base, int n, int size,
                  int (*cmp)(const void *, const void *));

extern void UI_Component_DrawPic(float x, float y, float w, float h,
                                 const float *color, MaterialHandle mat);
extern void UI_Component_DrawText(float x, float y, int font,
                                  const float *color, const char *text);
extern void CL_LookupColor(int colorIndex, float *outColor);

extern int Key_IsDown(int key);

extern int FS_FOpenTextFileWrite(const char *filename);
extern void FS_Write(const void *data, int len, int handle);
extern void FS_FCloseFile(int handle);

extern char *Sys_GetClipboardData(void);
extern void Sys_SetClipboardData(const char *text);

extern void Con_OpenConsole(void);
extern void Con_OpenConsoleOutput(void);

extern int Sys_IsRemoteDebugServer(void);
extern void Sys_WriteDebugSocketMessageType(int type);
extern void Sys_WriteDebugSocketInt(int v);
extern void Sys_WriteDebugSocketString(const char *s);
extern void Sys_EndWriteDebugSocket(void);
extern int Sys_ReadDebugSocketInt(void);

struct VariableValue_s {
    unsigned int u;
    int type;
};
struct ScriptExpression_t;
struct Scr_StringNode_s {
    struct Scr_StringNode_s *next;
    char *str;
};

struct Scr_SelectedLineInfo {
    int id;
    int line;
    int focus;
};

extern int Scr_CompileText();
extern void Scr_FreeDebugExpr();
extern void Scr_RefScriptExpression();
extern int Scr_RefToVariable();
extern int Scr_EvalScriptExpression();
extern void Scr_EvalFieldVariable();
extern void Scr_EvalArrayVariable();
extern void Scr_EvalSizeValue();
extern void Scr_EvalEquality();
extern int Scr_CastBool();
extern int Scr_GetArrayIndexValue();
extern unsigned int Scr_GetThreadPos();

extern void AddRefToValue();
extern void RemoveRefToValue();
extern void AddRefToObject();
extern void RemoveRefToObject();
extern int GetVarType();
extern int Scr_GetValueString();
extern void Scr_ClearErrorMessage();

extern int Scr_FindAllVariableField();
extern int Scr_FindAllThreads();

extern int Scr_GetSourceBuffer();
extern int Scr_GetSourcePos();
extern int Scr_GetLineNum();
extern void Scr_SelectScriptLine();
extern const char *Scr_GetPrevSourcePos();
extern const char *Scr_GetClosestSourcePosOfType();
extern int Scr_IsEndonThread();
extern int Scr_GetBreakpointType();
extern void Scr_BreakOnAllAssignmentPos();
extern void Scr_RemoveAssignmentBreakpoint();
extern void Scr_UnbreakAllAssignmentPos();
extern void Scr_FreeLineBreakpoint();
extern void SL_ShutdownSystem();

extern void *Scr_GetStringList();

extern void Field_AdjustScroll();

extern struct {
    char *error_message;
    int debugMode;
    int eval_in_progress;
} scrVarPub;
extern struct {
    int debugFlags;
    int eval_in_progress;
} scrVmGlob;

extern int Scr_SortWatchElementCompare(const void *a, const void *b);

extern Scr_ScriptWatch *g_scriptWatch;

extern MaterialHandle Scr_GetBreakpointMarkerMaterial(int breakpointKind);
extern int Scr_ScriptWatch_MouseClickDepth(Scr_ScriptWatch *self, float *pos);
extern struct Material *g_debuggerWatchBackground;
extern float g_debuggerLineHeight;
extern float g_debuggerTextScale;
extern int debugMode;

extern float G_DEBUGGER_LINE_HEIGHT;
extern float G_DEBUGGER_TEXT_SCALE;
extern float G_DEBUGGER_WATCH_WIDTH;
extern float G_DEBUGGER_WATCH_HEIGHT;
extern int G_SCR_DEBUG_CONSOLE_MODE;
extern float G_DEBUGGER_FADE_INV;
extern struct Material *g_debuggerSelectBarMaterial;
extern float drawColorScaleH;
extern float drawColorScaleW;

static Scr_WatchElement_s s_clipboardElement;

extern int atoi(const char *s);
extern double floor(double x);

void Scr_WriteElement(Scr_WatchElement_s *e)
{
    int id;

    if (e == NULL)
        id = 0;
    else
        id = e->id;

    Sys_WriteDebugSocketInt(id);
}

void Scr_ClearElementChanged(Scr_WatchElement_s *e)
{
    Scr_WatchElement_s *child;

    if (e->readOnly != 0)
        return;

    child = e->firstChild;
    e->changed = 0;

    if (child == NULL)
        return;

    do {
        Scr_ClearElementChanged(child);
        child = child->nextSibling;
    } while (child != NULL);
}

int Scr_ElementChildrenExist(Scr_WatchElement_s *e)
{
    int result;

    if (e->firstChild == NULL)
        result = 0;
    else if (e->expanded == 0)
        result = 0;
    else if (e->kind == 0)
        result = 0;
    else
        result = (e->kind < 23) ? 1 : 0;

    return result & 0xFF;
}

Scr_WatchElement_s *Scr_GetElementRoot(Scr_WatchElement_s *e)
{

    while (e->parent != NULL)
        e = e->parent;

    return e;
}

int Scr_IsSortWatchElement(Scr_WatchElement_s *e)
{
    int result;

    result = 1;
    if (e->sortable == 0)
        result = 0;

    return result & 0xFF;
}

int Scr_GetElementDepth(Scr_WatchElement_s *e)
{
    int depth;
    Scr_WatchElement_s *p;

    if (e == NULL)
        return 0;

    p = e->parent;
    depth = 0;
    do {
        depth = depth + 1;
        p = p->parent;
    } while (p != NULL);

    return depth;
}

Scr_WatchElement_s *Scr_CreateWatchElement(const char *name, Scr_WatchElement_s **list, Scr_WatchElement_s *parent)
{
    Scr_WatchElement_s *e;

    e = (Scr_WatchElement_s *)Z_MallocInternal(468);
    memset(e, 0, 468);
    I_strncpyz(e->name, name, 129);

    e->nextSibling = *list;
    *list = e;

    (void)parent;
    return e;
}

void Scr_RemoveValue(Scr_WatchElement_s *e)
{

    if (e == NULL)
        return;

    e->hasValueRef = 0;
    RemoveRefToValue(e->valueType, e->value);
}

void Scr_FreeWatchElementChildren(Scr_WatchElement_s *e)
{
    if (e->hasValueRef != 0) {
        e->hasValueRef = 0;
        RemoveRefToValue(e->valueType, e->value);
    }
    Scr_FreeWatchElementChildrenStrict(e);
}

void Scr_FreeWatchElementChildrenStrict(Scr_WatchElement_s *e)
{
    Scr_WatchElement_s *child;
    Scr_WatchElement_s *next;

    child = e->firstChild;
    if (child == NULL)
        return;

    do {
        next = child->nextSibling;
        if (child->hasValueRef != 0) {
            child->hasValueRef = 0;
            RemoveRefToValue(child->valueType, child->value);
        }
        Scr_FreeWatchElementChildrenStrict(child);
        child = next;
    } while (next != NULL);

    Z_FreeInternal(e->childArray);
    e->childCount = 0;
    e->childArray = NULL;
    e->firstChild = NULL;
}

int Scr_GetWatchElementSize(Scr_WatchElement_s *e)
{
    int total;
    Scr_WatchElement_s *node;
    int expandable;

    if (e == NULL)
        return 0;

    total = 0;
    node = e;
    do {
        total = total + 1;

        expandable = 0;
        if (node->firstChild != NULL && node->expanded != 0 &&
            node->kind != 0 && node->kind < 23)
            expandable = 1;

        if (expandable)
            total = total + Scr_GetWatchElementSize(node->firstChild);

        node = node->nextSibling;
    } while (node != NULL);

    return total;
}

void Scr_ConnectElementChildren(Scr_WatchElement_s *e)
{
    int count;
    Scr_WatchElement_s *arr;
    int i;

    count = e->childCount;
    arr = e->childArray;
    if (count > 0) {

        for (i = 0; i < count; i++) {
            if (i >= count - 1)
                arr[i].nextSibling = NULL;
            else
                arr[i].nextSibling = &arr[i + 1];
        }
        e->firstChild = arr;
    }
}

void Scr_SortElementChildren(Scr_WatchElement_s *e)
{
    int count;
    Scr_WatchElement_s *arr;
    Scr_WatchElement_s **sorted;
    int i;

    count = e->childCount;
    arr = e->childArray;
    sorted = (Scr_WatchElement_s **)Z_MallocInternal(count * sizeof(Scr_WatchElement_s *));

    if (count > 0) {

        for (i = 0; i < count; i++)
            sorted[i] = &arr[count - 1 - i];
    } else {

        qsort(sorted, count, 4, Scr_SortWatchElementCompare);

        if (count > 0) {

            for (i = 0; i < count; i++) {
                if (i >= count - 1)
                    sorted[i]->nextSibling = NULL;
                else
                    sorted[i]->nextSibling = sorted[i + 1];
            }
        } else {
            e->firstChild = sorted[0];
            Z_FreeInternal(sorted);
            return;
        }
    }

}

const char *Scr_GetElementArchiveText(Scr_WatchElement_s *e)
{

    switch ((unsigned char)(e->breakpointKind - 1)) {
    case 0:
        return va("breakpoint");
    case 1:
        return va("read");
    case 2:
        return va("write");
    case 3:
        return va("readwrite");
    case 4:
        return va("condition");
    default:
        return va("");
    }
}

void Scr_RemoveElementValue(Scr_WatchElement_s *e)
{
    Scr_WatchElement_s *child;

    if (e->hasValueRef != 0) {
        e->hasValueRef = 0;
        RemoveRefToValue(e->valueType, e->value);
    }

    child = e->firstChild;
    if (child == NULL)
        return;

    do {
        Scr_RemoveElementValue(child);
        child = child->nextSibling;
    } while (child != NULL);
}

Scr_WatchElement_s *Scr_ReadElement(void)
{
    int id;

    id = Sys_ReadDebugSocketInt();
    if (id != 0) {

        return Scr_ScriptWatch_GetElementWithId(g_scriptWatch, id);
    }
    return NULL;
}

void Scr_WatchElementHitBreakpointRemote(void)
{
}

void Scr_PrintElementText(Scr_WatchElement_s *e, int a, int b, char *out)
{
    int len;

    if (e->readOnly != 0) {

        if (b != 0)
            I_strncpyz(out + 1, e->valueText, a - 1);
        return;
    }

    if (e->sortable != 0) {

        I_strncpyz(out + 1, e->name, a - 1);
        return;
    }

    if (SCR_WATCHELEM_OBJECTID(e) != 0) {
        char *nameStr = e->name;
        const char *p = nameStr;
        while (*p)
            p++;
        len = (int)(p - nameStr);

        if (a < 33 || (len + 1) > a) {

            int rem = a - b;

            Com_sprintf(out + b + 1, rem - 1, "%s : %s",
                        nameStr, e->valueText);
            return;
        }

        memset(out + 1, ' ', len);
        memcpy(out + 1, nameStr, len);

        Com_sprintf(out + 0x21, a - 33, "%s", e->valueText);
        return;
    }

    if (e->kind == 15 || e->kind == 23) {

        I_strncpyz(out + 1, e->name, a - 1);
        if (*(int *)((char *)e + 0x1B8) < 0)
            return;
        {
            const char *q = out + 1;
            int width;
            len = (int)strlen(q);
            width = (len >= 8) ? (len + 1) : 8;
            memset(out + len + 1, ' ', width - len);
            Scr_GetSourcePos(*(int *)((char *)e + 0x1B8),
                             *(int *)((char *)e + 0x1BC),
                             out + width + 1, a - width - 1);
            out[a - 1] = 0;
        }
        return;
    }

    if (e->kind == 21) {

        int width;
        I_strncpyz(out + 1, e->name, a - 1);
        len = (int)strlen(out + 1);
        width = (len >= 7) ? (len + 1) : 7;
        memset(out + len + 1, ' ', width - len);
        I_strncpyz(out + width + 1, e->valueText, a - width - 1);
        out[a - 1] = 0;
        return;
    }

    I_strncpyz(out + 1, e->name, a - 1);
}

const char *Scr_GetElementThreadPos(Scr_WatchElement_s *e)
{
    const char *pos;

    if (e->kind != 15)
        return (const char *)e->sourcePos;

    pos = (const char *)Scr_GetThreadPos(SCR_WATCHELEM_OBJECTID(e));
    if (pos == 0)
        return (const char *)e->sourcePos;
    return pos;
}

void Scr_PostSetText(Scr_WatchElement_s *e)
{
    unsigned char newKind;

    if (e->sortable != 0) {
        newKind = 14;
    } else {
        int type;
        if (SCR_WATCHELEM_OBJECTID(e) == 0)
            type = 0;
        else
            type = GetVarType(SCR_WATCHELEM_OBJECTID(e));

        newKind = (unsigned char)type;

        if ((unsigned char)(newKind - 15) <= 3) {
            if (Scr_IsEndonThread(SCR_WATCHELEM_OBJECTID(e)) & 255)
                newKind = 0;
            else
                newKind = 15;
        }
    }

    if ((unsigned char)(newKind - 15) <= 7) {
        char *val = e->valueText;
        const char *nm = e->name;

        while (*val) {
            if (*val != *nm) {
                if (*nm == 0)
                    e->valueText[0] = 0;
                break;
            }
            val++;
            nm++;
        }

    }

    if (e->kind != newKind) {
        e->kind = newKind;
        *(unsigned char *)((char *)e + 0x18A) = 0;

        if (Sys_IsRemoteDebugServer()) {
            Sys_WriteDebugSocketMessageType(30);
            Sys_WriteDebugSocketInt(e->id);
            Sys_WriteDebugSocketInt(newKind);
            Sys_WriteDebugSocketInt(0);
            Sys_EndWriteDebugSocket();
        }

        if (newKind == 15 || newKind == 23) {
            unsigned char prev = *(unsigned char *)((char *)e + 0x191);
            if (prev != 15 && prev != 23) {
                int pos = Scr_GetElementThreadPos(e);
                if (pos == 0) {
                    *(int *)((char *)e + 0x1BC) = 0;
                    *(int *)((char *)e + 0x1B8) = -1;
                } else {
                    int srcPos = pos - 1;
                    int buf = Scr_GetSourceBuffer(srcPos);
                    int prevPos = Scr_GetPrevSourcePos(srcPos, 0);
                    *(int *)((char *)e + 0x1B8) = buf;
                    *(int *)((char *)e + 0x1BC) =
                        Scr_GetClosestSourcePosOfType(buf, prevPos, 4);
                }

                if (Sys_IsRemoteDebugServer()) {
                    Sys_WriteDebugSocketMessageType(31);
                    Sys_WriteDebugSocketInt(e->id);
                    Sys_WriteDebugSocketInt(*(int *)((char *)e + 0x1B8));
                    Sys_WriteDebugSocketInt(*(int *)((char *)e + 0x1BC));
                    Sys_EndWriteDebugSocket();
                }
            }
        }
    }
}

void Scr_DeltaElementValueText(Scr_WatchElement_s *e, char *text)
{
    char *val = e->valueText;
    const char *src = text;

    while (*val) {
        if (*val != *src) {
            if (Sys_IsRemoteDebugServer()) {
                Sys_WriteDebugSocketMessageType(32);
                Sys_WriteDebugSocketInt(e ? e->id : 0);
                Sys_WriteDebugSocketString(e->valueText);
                Sys_EndWriteDebugSocket();
                return;
            }
            if (*text == 0)
                return;
            e->changed = 1;
            *(int *)((char *)e + 0x1C0) = 0;
            return;
        }
        val++;
        src++;
    }
}

void Scr_SetNonFieldElementRefText(Scr_WatchElement_s *e)
{
    int kind = *(int *)((char *)e + 0x1A0);

    if (kind > 3) {

        Com_sprintf(e->name, 129, "<expr>");
    } else {

        I_strncpyz(e->name, e->parent->valueText, 129);
    }
    Scr_PostSetText(e);
}

void Scr_SetElementRefText(Scr_WatchElement_s *e, char *text)
{
    Scr_WatchElement_s *parent = e->parent;

    if ((int)((unsigned char)parent->kind - 14) > 8) {
        Scr_PostSetText(e);
        return;
    }

    {
        const char *pv = parent->valueText;
        const char *tx = text;
        while (*pv) {
            if (*pv != *tx) {

                Com_sprintf(e->name, 129, "%s.%s", parent->valueText, text);
                Scr_PostSetText(e);
                return;
            }
            pv++;
            tx++;
        }
        I_strncpyz(e->name, text, 129);
    }
    Scr_PostSetText(e);
}

void Scr_DeltaElementRefText(Scr_WatchElement_s *e, char *a, char *b)
{
    char *nm = e->name;
    const char *src = a;

    while (*nm) {
        if (*nm != *src) {
            Sys_WriteDebugSocketMessageType(34);
            Sys_WriteDebugSocketInt(e ? e->id : 0);
            Sys_WriteDebugSocketString(b);
            Sys_EndWriteDebugSocket();
            return;
        }
        nm++;
        src++;
    }
}

void Scr_AddDebugRefCountChildren(Scr_WatchElement_s *e, unsigned short *count)
{
    Scr_WatchElement_s *child;

    if (e->hasValueRef != 0) {
        if (e->valueType == 1)
            count[e->value] += 1;
    }

    child = e->firstChild;
    if (child == 0)
        return;

    do {
        Scr_AddDebugRefCountChildren(child, count);
        child = child->nextSibling;
    } while (child != 0);
}

int Scr_ScriptWatch_PostEvaluateWatchElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, struct VariableValue_s *v)
{

    if (e->hasValueRef != 0) {
        e->hasValueRef = 0;
        RemoveRefToValue(e->valueType, e->value);
    }

    if (scrVarPub.error_message != 0)
    {
        Com_sprintf(e->valueText, 257, "<error>");
        SCR_WATCHELEM_OBJECTID(e) = 0;
        Scr_ClearErrorMessage();

        RemoveRefToValue(v->type, v->u);
        return 0;
    }

    Scr_GetValueString(self->threadId, v, 257, e->valueText);

    if (v->type != 1)
        SCR_WATCHELEM_OBJECTID(e) = 0;
    else
        SCR_WATCHELEM_OBJECTID(e) = v->u;

    if (SCR_WATCHELEM_OBJECTID(e) != 0) {

        e->hasValueRef = 1;
        e->value = v->u;
        e->valueType = v->type;
        return 1;
    }

    if (e->breakpointKind == 1 || e->breakpointKind == 3) {
        e->hasValueRef = 1;
        e->value = v->u;
        e->valueType = v->type;
        return 1;
    }

    RemoveRefToValue(v->type, v->u);
    return 1;
}

int Scr_ScriptWatch_EvaluateWatchChildElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, unsigned int idx, Scr_WatchElement_s *parent, int isField)
{

    struct VariableValue_s val;

    *(int *)((char *)parent + 0x1A0) = (int)idx;

    if ((isField & 0xFF) != 0) {

        if (idx > 3) {

            val.u = SCR_WATCHELEM_OBJECTID(e);
            val.type = 0;
        } else {
            AddRefToObject(SCR_WATCHELEM_OBJECTID(e));
            Scr_EvalSizeValue(&val);
        }
        AddRefToValue(val.type, val.u);
    } else if (e->kind == 14) {

        AddRefToObject((int)idx);
        val.u = (int)idx;
        val.type = 0;
    } else if (e->kind == 22) {

        val.u = Scr_GetArrayIndexValue((int)idx);
        AddRefToValue(val.type, val.u);
        Scr_EvalArrayVariable(SCR_WATCHELEM_OBJECTID(e), &val);
    } else {

        Scr_EvalFieldVariable(idx & 0xFFFF, &val, SCR_WATCHELEM_OBJECTID(e));
    }

    return Scr_ScriptWatch_PostEvaluateWatchElement(self, parent, &val);
}

void Scr_ScriptWatch_EvaluateWatchElementExpression(Scr_ScriptWatch *self, Scr_WatchElement_s *e, struct VariableValue_s *v)
{
    int condition;

    if (e->hasValueRef == 0)
        condition = 0;
    else if (e->breakpointKind == 1)
        condition = 1;
    else if (e->breakpointKind == 3)
        condition = 1;
    else
        condition = 0;

    {
        int savedEvalFlag = scrVmGlob.eval_in_progress;
        Scr_EvalScriptExpression(e, self->threadId, v, condition, 0);
        scrVmGlob.eval_in_progress = savedEvalFlag;
    }
}

void Scr_ScriptWatch_EvaluateWatchElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    struct VariableValue_s val;
    char oldText[257];
    char *src;
    char *dst;

    src = e->valueText;
    dst = oldText;
    do {
        *dst = *src;
        dst++;
    } while (*src++ != 0);

    Scr_ScriptWatch_EvaluateWatchElementExpression(self, e, &val);
    Scr_ScriptWatch_PostEvaluateWatchElement(self, e, &val);
    Scr_PostSetText(e);
    Scr_DeltaElementValueText(e, oldText);
    Scr_ScriptWatch_EvaluateWatchChildren(self, e);
}

void Scr_ScriptWatch_Evaluate(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *elem;
    int selectedId;
    struct Scr_SelectedLineInfo saved;

    if (Sys_IsRemoteDebugServer() == 0) {
        elem = Scr_ScriptWatch_GetSelectedElement(self);
        selectedId = (elem == NULL) ? 0 : elem->id;
        saved.id = selectedId;
        saved.focus = self->hasFocus;
        (void)selectedId;
    }

    if (self->root == NULL) {
        if (Sys_IsRemoteDebugServer() == 0)
            Scr_ScriptWatch_LoadSelectedLine(self, &saved);
        return;
    }

    {
        Scr_WatchElement_s *node = self->root;
        do {
            if (node->readOnly == 0)
                Scr_ScriptWatch_EvaluateWatchElement(self, node);
            node = node->nextSibling;
        } while (node != NULL);
    }
}

void Scr_ScriptWatch_EvaluateWatchChildren(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{

    Scr_WatchElement_s *parent = e;
    int names = 0;
    int stack[8];
    int kind;
    int *idList;
    Scr_WatchElement_s *newArr;
    int oldCount;
    int found;
    int comparator;

    parent->_pad191 = parent->kind;

    kind = parent->kind;

    if (kind != 14) {

        if (SCR_WATCHELEM_OBJECTID(parent) == 0 || parent->_pad18A != 0)
        {

            if (parent->kind == 15)
            {
                stack[names] = 2;
                names++;
            } else if (parent->kind == 22)
            {
                stack[names] = 0;
                names++;
            }

            if (names <= 4)
                found = Scr_FindAllVariableField(SCR_WATCHELEM_OBJECTID(parent), 0);
            else
                found = Scr_FindAllVariableField(SCR_WATCHELEM_OBJECTID(parent), 0);
        } else {
            found = Scr_FindAllVariableField(SCR_WATCHELEM_OBJECTID(parent), 0);
        }
    } else {

        Scr_WatchElement_s *p = parent->parent;
        found = Scr_FindAllThreads(SCR_WATCHELEM_OBJECTID(p), 0, self->threadId);
    }

    if (found == 0) {

        if (Sys_IsRemoteDebugServer() != 0) {
            Sys_WriteDebugSocketMessageType(29);
            Sys_WriteDebugSocketInt(parent->id);
            Sys_EndWriteDebugSocket();
        }
        Scr_FreeWatchElementChildren(parent);
        return;
    }

    idList = Z_MallocInternal(found * 4);
    memcpy(idList, stack, names * 4);

    if (parent->kind == 14) {
        Scr_FindAllThreads(SCR_WATCHELEM_OBJECTID(parent), idList + names, self->threadId);
        comparator = 0x82297028;
    } else {
        Scr_FindAllVariableField(SCR_WATCHELEM_OBJECTID(parent), idList + names);
        if (parent->kind == 22)
            comparator = 0x822A2370;
        else
            comparator = 0x82296EF8;
    }
    qsort(idList + names, found - names, 4, (void *)comparator);

    newArr = Z_MallocInternal(found * 468);
    memset(newArr, 0, found * 468);
    oldCount = parent->childCount;

    (void)newArr;
    (void)oldCount;
}

int Scr_ConditionalExpression(Scr_WatchElement_s *e, unsigned int idx)
{
    Scr_WatchElement_s *child;
    int hit;

    child = e->nextSibling;
    if (child == NULL) {

        hit = (e->breakpointKind == 1 || e->breakpointKind == 5) ? 1 : 0;

        if (Sys_IsRemoteDebugServer() != 0) {
            Sys_WriteDebugSocketMessageType(28);
            Sys_WriteDebugSocketInt(e->id);
            Sys_WriteDebugSocketInt(hit & 0xFF);
            Sys_EndWriteDebugSocket();
        }

        if ((hit & 0xFF) == 0) {
            e->changed = 1;
            *(int *)((char *)e + 0x1C0) = 0;
        } else {
            *(char *)((char *)e + 0x194) = 1;
        }

        for (child = e->nextSibling; child != NULL; child = child->nextSibling) {
            if (child->breakpointKind != 2)
                break;
            if (Sys_IsRemoteDebugServer() != 0) {
                Sys_WriteDebugSocketMessageType(28);
                Sys_WriteDebugSocketInt(child->id);
                Sys_WriteDebugSocketInt(hit & 0xFF);
                Sys_EndWriteDebugSocket();
            }
            if ((hit & 0xFF) == 0) {
                child->changed = 1;
                *(int *)((char *)child + 0x1C0) = 0;
            } else {
                *(char *)((char *)child + 0x194) = 1;
            }
        }
        return hit;
    }

    while (child->breakpointKind == 2) {
        struct VariableValue_s val;

        Scr_EvalScriptExpression(child, (int)idx, &val, 0, 1);

        if (val.type != 1) {
            Scr_CastBool(&val);
            if (scrVarPub.error_message != 0)
            {
                Scr_ClearErrorMessage();
                return 0;
            }

            if (val.u == 0)
                return 0;
        } else {

            RemoveRefToObject(val.u);
            if ((unsigned int)(GetVarType(val.u) - 15) > 6)
                return 0;
            if (val.u != (int)idx)
                return 0;
        }

        child = child->nextSibling;
        if (child == NULL)
            break;
    }
    return 0;
}

int Scr_WatchElementHasSameValue(Scr_WatchElement_s *e, struct VariableValue_s *v)
{
    struct VariableValue_s cur;

    if (e->hasValueRef == 0) {

        RemoveRefToValue(v->type, v->u);
        return 0;
    }

    cur.u = e->value;
    AddRefToValue(cur.type, cur.u);
    Scr_EvalEquality(&cur, v);

    if (scrVarPub.error_message != 0)
    {
        Scr_ClearErrorMessage();
        return 0;
    }

    if (cur.u == 0)
        return 0;

    if (e->valueType != 1)
        return 1;

    if (GetVarType(v->u) != e->kind)
        return 0;
    return 0;
}

void Scr_WatchElementHitBreakpoint(Scr_WatchElement_s *e, int remote)
{
    if (Sys_IsRemoteDebugServer() != 0) {
        Sys_WriteDebugSocketMessageType(28);
        Sys_WriteDebugSocketInt(e == NULL ? 0 : e->id);
        Sys_WriteDebugSocketInt(remote & 0xFF);
        Sys_EndWriteDebugSocket();
    }

    if ((remote & 0xFF) == 0) {
        e->changed = 1;
        *(int *)((char *)e + 0x1C0) = 0;
    } else {
        *(char *)((char *)e + 0x194) = 1;
    }
}

Scr_WatchElement_s *Scr_ScriptWatch_GetSelectedElement_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int *idx)
{
    Scr_WatchElement_s *found;

    while (e) {
        if (*idx == self->selectedLine)
            return e;
        ++(*idx);

        if (Scr_ElementChildrenExist(e)) {
            found = Scr_ScriptWatch_GetSelectedElement_r(self, e->firstChild, idx);
            if (found)
                return found;
        }

        e = e->nextSibling;
    }
    return 0;
}

Scr_WatchElement_s *Scr_ScriptWatch_GetSelectedElement(Scr_ScriptWatch *self)
{
    int idx = 0;
    return Scr_ScriptWatch_GetSelectedElement_r(self, self->root, &idx);
}

Scr_WatchElement_s *Scr_ScriptWatch_GetSelectedNonConditionalElement(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *e;

    e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e) {

        while (e->nextSibling && e->breakpointKind == 2)
            e = e->nextSibling;
        return e;
    }
    return 0;
}

Scr_WatchElement_s *Scr_ScriptWatch_GetElementWithId_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int id)
{
    Scr_WatchElement_s *found;

    while (e) {
        if (e->id == id)
            return e;

        if (e->firstChild) {
            found = Scr_ScriptWatch_GetElementWithId_r(self, e->firstChild, id);
            if (found)
                return found;
        }

        e = e->nextSibling;
    }
    return 0;
}

Scr_WatchElement_s *Scr_ScriptWatch_GetElementWithId(Scr_ScriptWatch *self, int id)
{
    return Scr_ScriptWatch_GetElementWithId_r(self, self->root, id);
}

Scr_WatchElement_s **Scr_ScriptWatch_GetElementRef(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    Scr_WatchElement_s **ref;

    ref = &self->root;
    if (*ref == e)
        return 0;

    do {
        ref = &(*ref)->nextSibling;
    } while (*ref != e);

    return ref;
}

Scr_WatchElement_s *Scr_ScriptWatch_GetElementPrev(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    Scr_WatchElement_s *prev;

    prev = self->root;
    if (prev == e)
        return 0;

    while (prev) {
        if (prev->nextSibling == e)
            return prev;
        prev = prev->nextSibling;
    }
    return 0;
}

int Scr_ScriptWatch_SetSelectedElement_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e, Scr_WatchElement_s *target, int *idx, int focus)
{
    while (e) {
        if (e == target) {
            self->vtbl->SetSelectedLineFocus(self, *idx, focus);
            return 1;
        }
        ++(*idx);

        if (Scr_ElementChildrenExist(e)) {
            if (Scr_ScriptWatch_SetSelectedElement_r(self, e->firstChild, target, idx, focus))
                return 1;
        }

        e = e->nextSibling;
    }
    return 0;
}

void Scr_ScriptWatch_SetSelectedElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int focus)
{
    int idx;

    if (e) {
        idx = 0;
        Scr_ScriptWatch_SetSelectedElement_r(self, self->root, e, &idx, focus);
    } else {
        self->selectedLine = -1;
    }
}

Scr_WatchElement_s *Scr_ScriptWatch_CreateWatchElement(Scr_ScriptWatch *self, const char *name, Scr_WatchElement_s **list, Scr_WatchElement_s *parent)
{
    Scr_WatchElement_s *e;

    e = (Scr_WatchElement_s *)Z_MallocInternal(468);
    memset(e, 0, 468);
    I_strncpyz(e->name, name, 129);

    e->nextSibling = *list;
    *list = e;

    self->field28 = self->field28 + 1;
    e->id = self->field28;

    e->parent = parent;

    self->lineCount = Scr_GetWatchElementSize(self->root);
    UI_LinesComponent_UpdateHeight((UI_LinesComponent *)self);
    return e;
}

Scr_WatchElement_s *Scr_ScriptWatch_CloneElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    Scr_WatchElement_s **list;
    Scr_WatchElement_s *clone;

    if (e->parent != 0)
        return 0;

    list = Scr_ScriptWatch_GetElementRef(self, e);
    clone = Scr_ScriptWatch_CreateWatchElement(self, e->name, list, e->parent);
    Scr_CompileText(e->name, clone);
    return clone;
}

void Scr_ScriptWatch_CloneSelectedElement(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *e;
    Scr_WatchElement_s *clone;

    e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e == 0)
        return;

    if (e->sortable != 0)
        return;

    if (e->readOnly == 0) {
        clone = Scr_ScriptWatch_CloneElement(self, e);
        Scr_ScriptWatch_EvaluateWatchElement(self, clone);
        Scr_ScriptWatch_SetSelectedElement(self, clone, 0);
    } else {

        Scr_WatchElement_s *ref = (Scr_WatchElement_s *)e->readOnly;
        Scr_SelectScriptLine(*(int *)((char *)ref + 0x4), *(int *)((char *)ref));
    }
}

void Scr_ScriptWatch_FreeWatchElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    Scr_WatchElement_s *bp;

    bp = (Scr_WatchElement_s *)e->readOnly;
    if (bp != 0) {
        if (*(int *)((char *)bp + 0xC) != 0)
            goto free_node;
        *(int *)((char *)bp + 0xC) = 0;
        Scr_FreeLineBreakpoint(bp);
        Z_FreeInternal(e);
        return;
    }

    if (e->hasValueRef != 0) {
        e->hasValueRef = 0;
        RemoveRefToValue(e->valueType, e->value);
    }

    Scr_FreeWatchElementChildrenStrict(e);

    if (e->breakpointKind != 0)
        Scr_ScriptWatch_RemoveBreakpoint(self, e);

    if (*(int *)((char *)e + 0x4) != 0)
        Scr_FreeDebugExpr(*(int *)((char *)e + 0x4));

free_node:
    Z_FreeInternal(e);
}

void Scr_ScriptWatch_AddElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, const char *name)
{
    int exprType;
    void *expr;
    int compiled;

    if (e->readOnly != 0)
        return;

    Scr_CompileText(name, &expr);
    exprType = *(int *)expr;
    compiled = *(int *)((char *)expr + 0x4);

    if (exprType == 82) {

        Scr_ExecCode(compiled, self->threadId);
        SL_ShutdownSystem(2);
        Scr_FreeDebugExpr(expr);
        Scr_ScriptWatch_Evaluate(self);
        return;
    }

    if ((unsigned int)(exprType - 83) <= 1) {

        Scr_FreeDebugExpr(expr);
        return;
    }

    if (*(int *)((char *)0x844101C4) == 0) {
        if (Sys_IsRemoteDebugServer()) {
            Sys_WriteDebugSocketMessageType(5);
            Sys_WriteDebugSocketInt(*(int *)((char *)0x844101AC));
            Sys_WriteDebugSocketString(name);
            Sys_EndWriteDebugSocket();
        }

        if (*(int *)((char *)0x84410004) != 0)
            Scr_FreeDebugExpr(*(int *)((char *)0x84410004));

        I_strncpyz((char *)0x84410109, name, 129);
        *(int *)((char *)0x84410000) = (int)expr;

        if (*(unsigned char *)((char *)0x84410196) != 0)
        {
            *(char *)((char *)0x84410196) = 0;
            RemoveRefToValue(*(int *)((char *)0x8441019C), *(int *)((char *)0x84410198));
        }

        Scr_ScriptWatch_EvaluateWatchElement(self, e);

        if (Sys_IsRemoteDebugServer()) {
            Sys_WriteDebugSocketMessageType(37);
            Sys_EndWriteDebugSocket();
        }

        self->lineCount = Scr_GetWatchElementSize(self->root);
        UI_LinesComponent_UpdateHeight((UI_LinesComponent *)self);
        return;
    }

    Com_Printf("expression already pending\n");
    Scr_FreeDebugExpr(expr);
}

static Scr_WatchElement_s s_clipboardElement;

void Scr_ScriptWatch_UpdateBreakpoints(Scr_ScriptWatch *self, int announce)
{
    Scr_WatchElement_s *e;

    e = self->root;

    if (e == NULL) {

        if ((announce & 0xFF) == 0)
            return;

        e = self->root;
        if (e == NULL) {

            if (*(unsigned char *)0x84411409 == 0)
                return;
            *(unsigned char *)0x84411409 = 0;
            {
                Scr_Breakpoint *node = *(Scr_Breakpoint **)0x84411400;
                if (node == NULL)
                    return;
                do {
                    Scr_RemoveAssignmentBreakpoint(*(int *)node);
                    node = *(Scr_Breakpoint **)((char *)node + 0x4);
                } while (node != NULL);
            }
        }

        do {
            if (e->breakpointKind == 1) {
                Scr_BreakOnAllAssignmentPos();
                return;
            }
            e = e->nextSibling;
        } while (e != NULL);
        return;
    }

    do {
        if (e->breakpointKind == 1) {

            *(Scr_WatchElement_s **)0x8441140C = e;
            *(int *)0x84411410 = 0;
            *(unsigned char *)0x84411409 = (unsigned char)announce;

            if (e->hasValueRef != 0) {
                if (e->valueType == 1)
                    Scr_RefToVariable(e->value);
            }
            Scr_RefScriptExpression(e);
        } else if (e->breakpointKind == 3) {

        }

        e = e->nextSibling;
    } while (e != NULL);
}

Scr_WatchElement_s *Scr_ScriptWatch_AddBreakpoint(Scr_ScriptWatch *self,
                                                  Scr_WatchElement_s *e,
                                                  unsigned char kind)
{
    Scr_WatchElement_s *elem;

    elem = e;
    if (e->parent != NULL)
        elem = Scr_ScriptWatch_CloneElement(self, e);

    elem->breakpointKind = kind;

    if (!Sys_IsRemoteDebugServer())
        Scr_ScriptWatch_SetSelectedElement(self, elem, 1);

    return elem;
}

Scr_WatchElement_s *Scr_ScriptWatch_RemoveBreakpoint(Scr_ScriptWatch *self,
                                                     Scr_WatchElement_s *e)
{
    (void)self;

    if (e->hasValueRef != 0) {
        e->breakpointKind = 0;
        e->hasValueRef = 0;
        RemoveRefToValue(e->valueType, e->value);
    } else {
        e->breakpointKind = 0;
    }
    return e;
}

Scr_WatchElement_s *Scr_ScriptWatch_CreateBreakpointElement(Scr_ScriptWatch *self,
                                                            Scr_WatchElement_s *e,
                                                            int line,
                                                            unsigned int kind,
                                                            int select)
{
    char name[129];
    char srcText[257];
    int lineNo;
    char *fileEntry;
    Scr_WatchElement_s *elem;

    if (e != NULL) {
        Scr_WatchElement_s *p = e->parent;
        while (p != NULL)
            p = p->parent;
    }

    {
        Scr_WatchElement_s **list = Scr_ScriptWatch_GetElementRef(self, e);

        lineNo = Scr_GetSourcePos(line, kind, srcText, 257);

        fileEntry = (char *)(line * 24) + *(int *)0x844B18E8;
        Com_sprintf(name, 129, "%s(%i)", *(int *)(fileEntry + 0x4), lineNo + 1);

        elem = Scr_ScriptWatch_CreateWatchElement(self, name, list,  NULL);
    }
    I_strncpyz(elem->valueText, srcText, 257);

    if (!Sys_IsRemoteDebugServer()) {
        if ((select & 0xFF) != 0)
            Scr_ScriptWatch_SetSelectedElement(self, elem, 1);
    }
    return elem;
}

void Scr_ScriptWatch_ToggleWatchElementBreakpoint(Scr_ScriptWatch *self,
                                                  Scr_WatchElement_s *e,
                                                  unsigned char kind)
{
    Scr_WatchElement_s *elem;

    if (e->breakpointKind == kind) {
        elem = Scr_ScriptWatch_RemoveBreakpoint(self, e);

        Scr_FreeDebugExpr(*(int *)((char *)elem + 0x4));
        Scr_CompileText(elem->name, elem);
    } else {
        elem = Scr_ScriptWatch_AddBreakpoint(self, e, kind);
    }

    if (elem->parent != NULL) {
        Scr_WatchElement_s *p = elem->parent;
        while (p->parent != NULL)
            p = p->parent;
        Scr_ScriptWatch_EvaluateWatchElement(self, elem);
    }
}

void Scr_ScriptWatch_SortHitBreakpointsTop(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s **head;
    Scr_WatchElement_s **pp;
    Scr_WatchElement_s **insertAt;
    Scr_WatchElement_s *cur;
    int moved;

    *(unsigned char *)0x82604F9C = 1;

    head = &self->root;

    if (self->root == NULL) {

        if (*head == NULL) {
            if (!Sys_IsRemoteDebugServer()) {

                cur = *head;
                if (cur != NULL)
                    (void)cur->readOnly;
                Scr_ScriptWatch_UpdateBreakpoints(self, 0);
                Scr_SetMiscScrollPaneComp((UI_LinesComponent *)self);
            } else {
                Sys_WriteDebugSocketMessageType(23);
                Sys_EndWriteDebugSocket();
            }
            return;
        }

        pp = head;
        insertAt = head;
        moved = 0;
        do {
            cur = *pp;

            if (*(unsigned char *)((char *)cur + 0x194) != 0) {

                *pp = cur->nextSibling;
                cur->nextSibling = *insertAt;
                *insertAt = cur;
                insertAt = &cur->nextSibling;
                moved = 1;
            } else {
                pp = &cur->nextSibling;
            }
        } while (*pp != NULL);
        (void)moved;
        return;
    }

    {
        Scr_WatchElement_s *e = self->root;
        while (*(unsigned char *)((char *)e + 0x194) != 0) {
            e = e->nextSibling;
            if (e == NULL)
                break;
        }
    }
}

void Scr_ScriptWatch_ToggleExpandElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    e->expanded = (unsigned char)(e->expanded == 0);

    if (e->expanded != 0) {
        if (e->kind == 0)
            e->expanded = 0;
        else
            Scr_ScriptWatch_EvaluateWatchChildren(self, e);
    }

    if (e->expanded == 0)
        Scr_FreeWatchElementChildrenStrict(e);

    if (Sys_IsRemoteDebugServer()) {
        Sys_WriteDebugSocketMessageType(37);
        Sys_EndWriteDebugSocket();
    }

    self->lineCount = Scr_GetWatchElementSize(self->root);
    UI_LinesComponent_UpdateHeight((UI_LinesComponent *)self);
}

void Scr_ScriptWatch_ExpandElement(Scr_ScriptWatch *self, Scr_WatchElement_s *e, int expand)
{
    if (e->expanded != (expand & 0xFF)) {
        if (Sys_IsRemoteDebugServer()) {
            Sys_WriteDebugSocketMessageType(18);
            Sys_WriteDebugSocketInt(e->id);
            Sys_EndWriteDebugSocket();
        }
        Scr_ScriptWatch_ToggleExpandElement(self, e);
    } else {
        Scr_WatchElement_s *target;
        if ((expand & 0xFF) == 0)
            target = e->parent;
        else
            target = e->firstChild;

        if (target != NULL) {
            if (!Sys_IsRemoteDebugServer()) {
                Scr_ScriptWatch_SetSelectedElement(self, target, 1);
            } else {
                Sys_WriteDebugSocketMessageType(17);
                Sys_WriteDebugSocketInt(target->id);
                Sys_EndWriteDebugSocket();
            }
        }
    }
}

void Scr_ScriptWatch_ExpandSelectedElement(Scr_ScriptWatch *self, int expand)
{
    Scr_WatchElement_s *e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e != NULL)
        Scr_ScriptWatch_ExpandElement(self, e, expand);
}

void Scr_ScriptWatch_ToggleBreakpointInternal(Scr_ScriptWatch *self,
                                              Scr_WatchElement_s *e,
                                              unsigned char kind)
{
    if (e->sortable == 0)
        return;

    if (Sys_IsRemoteDebugServer()) {
        Sys_WriteDebugSocketMessageType(27);
        Sys_WriteDebugSocketInt(e->id);
        Sys_WriteDebugSocketInt(kind & 0xFF);
        Sys_EndWriteDebugSocket();
    }

    if (e->readOnly == 0) {
        Scr_ScriptWatch_ToggleWatchElementBreakpoint(self, e, kind);
        return;
    }

    if (!Sys_IsRemoteDebugServer()) {

        int codePos = *(int *)(*(int *)((char *)e + 0x1B0));
        int fileIdx = *(int *)(*(int *)((char *)e + 0x1B0) + 0x4);
        Scr_SelectScriptLine(fileIdx, codePos);
    }

    if (e->breakpointKind != 4) {
        if ((kind & 0xFF) == 3) {
            e->breakpointKind = 4;
            return;
        }
        if ((kind & 0xFF) == 1)
            Scr_ScriptWatch_DeleteElementInternal(self, e);
        return;
    } else {
        if ((kind & 0xFF) == 1) {
            e->breakpointKind = 5;
            return;
        }
        if ((kind & 0xFF) == 3)
            Scr_ScriptWatch_DeleteElementInternal(self, e);
        return;
    }
}

void Scr_ScriptWatch_ToggleBreakpoint(Scr_ScriptWatch *self,
                                      Scr_WatchElement_s *e,
                                      unsigned char kind)
{

    (void)Sys_IsRemoteDebugServer();

    if (e != NULL)
        Scr_ScriptWatch_ToggleBreakpointInternal(self, e, kind);
}

Scr_WatchElement_s *Scr_ScriptWatch_PasteNonBreakpointElement(Scr_ScriptWatch *self,
                                                              Scr_WatchElement_s *e,
                                                              const char *text,
                                                              int select)
{
    Scr_WatchElement_s **ref;
    Scr_WatchElement_s *elem;

    if (e != NULL) {
        Scr_WatchElement_s *p = e->parent;
        while (p != NULL)
            p = p->parent;
    }

    ref = Scr_ScriptWatch_GetElementRef(self, e);
    elem = Scr_ScriptWatch_CreateWatchElement(self, text, ref,  NULL);

    Scr_CompileText(text, elem);
    Scr_ScriptWatch_EvaluateWatchElement(self, elem);

    if (!Sys_IsRemoteDebugServer()) {
        if ((select & 0xFF) != 0)
            Scr_ScriptWatch_SetSelectedElement(self, elem, 1);
    }
    return elem;
}

void Scr_ScriptWatch_PasteBreakpointElement(Scr_ScriptWatch *self,
                                            Scr_WatchElement_s *e,
                                            const char *text,
                                            int b, int c, int d)
{
    int line;
    const char *p;
    int i;
    int count;

    line = atoi(text);

    p = text;
    if ((signed char)*p != 0) {
        while ((signed char)*p != ' ') {
            p++;
            if ((signed char)*p == 0)
                return;
        }

    } else {
        return;
    }

    count = *(int *)0x84411AAC;
    if (count == 0)
        return;

    for (i = 0; i < count; i++) {
        char *entry = (char *)(i * 24) + *(int *)0x844B18E8;
        const char *fname = *(const char **)(entry + 0x4);
        if (fname == NULL)
            continue;

        if (stricmp(fname, p) == 0) {
            int winIdx = *(int *)(entry + 0x10);

            Scr_ScriptWindow *win =
                *(Scr_ScriptWindow **)((winIdx * 4) + *(int *)0x82710598);

            if (win->vtbl->SetSelectedLineFocus(win, line - 1, 0) == 0)
                return;
            Scr_ScriptWindow_ToggleBreakpoint(win, e, 1, b, c, d);
            return;
        }
    }
}

void Scr_ScriptWatch_PasteElementInternal(Scr_ScriptWatch *self,
                                          Scr_WatchElement_s *e,
                                          const char *text,
                                          int select)
{
    const char *p = text;
    unsigned char kind;

    (void)Sys_IsRemoteDebugServer();

    kind = (unsigned char)Scr_GetBreakpointType((int)&p);

    if (kind == 4) {
        Scr_ScriptWatch_PasteBreakpointElement(self, e, p, 0, 0, select);
        return;
    }
    if (kind == 5) {
        Scr_ScriptWatch_PasteBreakpointElement(self, e, p, 0, 1, select);
        return;
    }

    {
        Scr_WatchElement_s *elem =
            Scr_ScriptWatch_PasteNonBreakpointElement(self, e, p, select);
        if (kind != 0)
            Scr_ScriptWatch_ToggleBreakpoint(self, elem, kind);
    }
}

void Scr_ScriptWatch_PasteElement(Scr_ScriptWatch *self)
{
    char *clip;

    (void)Sys_IsRemoteDebugServer();

    clip = Sys_GetClipboardData();
    if (clip != NULL) {
        Scr_WatchElement_s *sel = Scr_ScriptWatch_GetSelectedElement(self);
        Scr_ScriptWatch_PasteElementInternal(self, sel, clip, 1);
        Z_FreeInternal(clip);
    }
}

void Scr_ScriptWatch_InsertElement(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *sel = Scr_ScriptWatch_GetSelectedElement(self);
    Scr_ScriptWatch_PasteNonBreakpointElement(self, sel, "", 1);
}

Scr_WatchElement_s *Scr_ScriptWatch_DeleteElementInternal(Scr_ScriptWatch *self,
                                                          Scr_WatchElement_s *e)
{
    Scr_WatchElement_s **ref;

    if (e->parent != NULL)
        return e;

    ref = Scr_ScriptWatch_GetElementRef(self, e);
    *ref = e->nextSibling;

    self->lineCount = Scr_GetWatchElementSize(self->root);
    UI_LinesComponent_UpdateHeight((UI_LinesComponent *)self);

    if (!Sys_IsRemoteDebugServer())
        Scr_ScriptWatch_SetSelectedElement(self, e->nextSibling, 1);

    Scr_ScriptWatch_FreeWatchElement(self, e);
    return *ref;
}

void Scr_ScriptWatch_DeleteElement(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e != NULL)
        Scr_ScriptWatch_DeleteElementInternal(self, e);
}

Scr_WatchElement_s *Scr_ScriptWatch_BackspaceElementInternal(Scr_ScriptWatch *self,
                                                             Scr_WatchElement_s *e)
{
    Scr_WatchElement_s **ref;
    Scr_WatchElement_s *prev;

    if (e->parent != NULL)
        return e;

    ref = Scr_ScriptWatch_GetElementRef(self, e);
    prev = Scr_ScriptWatch_GetElementPrev(self, e);

    if (!Sys_IsRemoteDebugServer())
        Scr_ScriptWatch_SetSelectedElement(self, prev, 1);

    *ref = e->nextSibling;

    self->lineCount = Scr_GetWatchElementSize(self->root);
    UI_LinesComponent_UpdateHeight((UI_LinesComponent *)self);

    Scr_ScriptWatch_FreeWatchElement(self, e);
    return prev;
}

void Scr_ScriptWatch_BackspaceElement(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e != NULL)
        Scr_ScriptWatch_BackspaceElementInternal(self, e);
}

void Scr_ScriptWatch_CopyElement(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e != NULL) {
        const char *text = Scr_GetElementArchiveText(e);
        Sys_SetClipboardData(text);
    }
}

void Scr_ScriptWatch_EditElement(Scr_ScriptWatch *self, Scr_ConsoleOpenMode mode)
{
    Scr_WatchElement_s *e;
    char *p;

    e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e == NULL)
        return;

    *(int *)0x847FD948 = 2;
    Con_OpenConsole();
    if (mode == SCR_CONSOLE_OPEN_REPLACE)
        Con_OpenConsoleOutput();

    I_strncpyz((char *)0x82F79228, e->name, 256);

    p = (char *)0x82F79228;
    while (*p != 0)
        p++;
    *(int *)0x82F79210 = (int)(p - (char *)0x82F79228);
    Field_AdjustScroll((void *)0x82F79210);
}

void Scr_ScriptWatch_DisplayThreadPos(Scr_ScriptWatch *self, Scr_WatchElement_s *e)
{
    int sourcePos;
    int sourceBuffer;
    int lineNum;

    (void)self;

    if (*(unsigned char *)0x84412264 == 0)
        return;

    sourcePos = 0;

    if (e->kind == 15) {
        int threadPos = Scr_GetThreadPos(SCR_WATCHELEM_OBJECTID(e));
        if (threadPos != 0)
            sourcePos = threadPos;
    }

    if (sourcePos == 0) {
        sourcePos = e->sourcePos;
        if (sourcePos == 0)
            return;
    }

    sourceBuffer = Scr_GetSourceBuffer(sourcePos - 1);
    lineNum = Scr_GetLineNum(sourceBuffer, Scr_GetPrevSourcePos(sourcePos - 1, 0));

    if (!Sys_IsRemoteDebugServer()) {
        Scr_SelectScriptLine(sourceBuffer, lineNum);
    } else {
        Sys_WriteDebugSocketMessageType(40);
        Sys_WriteDebugSocketInt(sourceBuffer);
        Sys_WriteDebugSocketInt(lineNum);
        Sys_EndWriteDebugSocket();
    }
}

void Scr_ScriptWatch_Draw_r(Scr_ScriptWatch *self, Scr_WatchElement_s *e,
                            float x, float y, float w, float h,
                            int depth, int hUnused, int iUnused,
                            int *lineNo, float *curY,
                            float a, float b)
{
    (void)hUnused;
    (void)iUnused;

    float lineColor[4];
    float dimColor[4];
    float markColor[4];
    char indent[390];
    float drawColor[4];

    float lineHeight;
    int selectedLine;
    Scr_WatchElement_s *child;
    int childrenExist;
    int len, indentLen;

    if (e == 0)
        return;

    lineHeight = G_DEBUGGER_LINE_HEIGHT;

    CL_LookupColor(55, lineColor);
    CL_LookupColor(49, dimColor);
    CL_LookupColor(51, markColor);

    for (; e != 0; e = e->nextSibling) {

        if (*curY < a) {

            ++(*lineNo);
            *curY += lineHeight;

            childrenExist = 0;
            if (e->firstChild != 0 && e->expanded != 0 &&
                e->kind != 0 && e->kind < 23)
                childrenExist = 1;

            if (childrenExist) {
                int isArray = (e->kind == 22) ? 1 : 0;
                (void)isArray;
                Scr_ScriptWatch_Draw_r(self, e->firstChild, x, y, w, h,
                                       depth + 1, 0, 0, lineNo, curY, a, b);
            }
            continue;
        }

        if (*curY > b)
            break;

        memset(indent, ' ', depth);

        if (e->kind == 0) {
            indent[depth] = ' ';
        } else {
            indent[depth] = (e->expanded == 0) ? '+' : '-';
        }

        selectedLine = self->selectedLine;
        if (*lineNo == selectedLine) {
            float boxY = *curY + y;
            float boxX = x;

            UI_Component_DrawPic(boxX, boxY, w, h, lineColor,
                                 g_debuggerSelectBarMaterial);
        }

        if ((unsigned char)(e->breakpointKind - 1) <= 4) {

            struct Material *bpMat =
                Scr_GetBreakpointMarkerMaterial(e->breakpointKind);
            float by = *curY + y;
            float bh = drawColorScaleH;
            float bw = drawColorScaleW;
            (void)bw;
            UI_Component_DrawPic(x, by, bh, bh, dimColor, bpMat);

            if (*(unsigned char *)((char *)e + 0x194) != 0)
            {
                float gy = *curY + y;

                UI_Component_DrawText(x, gy, 5, markColor,
                                      "*" );
            }
        }

        Scr_PrintElementText(e, 390, depth, indent);

        len = (int)strlen(indent);
        indentLen = len;

        if (indentLen <= depth) {
            ++(*lineNo);
            *curY += lineHeight;
            child = e->firstChild;
            if (child != 0 && e->expanded != 0 && e->kind != 0 && e->kind < 23)
                Scr_ScriptWatch_Draw_r(self, child, x, y, w, h,
                                       depth + 1, 0, 0, lineNo, curY, a, b);
            continue;
        }

        drawColor[0] = lineColor[0];
        drawColor[1] = lineColor[1];
        drawColor[2] = lineColor[2];
        drawColor[3] = lineColor[3];

        if (e->changed != 0) {
            int changeStamp = *(int *)((char *)e + 0x1C0);
            if (changeStamp == 0) {

                drawColor[0] = markColor[0];
                drawColor[1] = markColor[1];
                drawColor[2] = markColor[2];
                drawColor[3] = markColor[3];
            } else {
                int dt = Com_Milliseconds() - changeStamp;
                if (dt >= 1000) {
                    e->changed = 0;
                } else {

                    float t = (float)dt * G_DEBUGGER_FADE_INV;
                    drawColor[0] = (lineColor[0] - markColor[0]) * t + markColor[0];
                    drawColor[1] = (lineColor[1] - markColor[1]) * t + markColor[1];
                    drawColor[2] = (lineColor[2] - markColor[2]) * t + markColor[2];
                    drawColor[3] = (lineColor[3] - markColor[3]) * t + markColor[3];
                }
            }
        }

        {
            float ty = *curY + y;
            float tx = x;
            UI_Component_DrawText(tx, ty, 5, drawColor, indent);
        }

        ++(*lineNo);
        *curY += lineHeight;

        childrenExist = 0;
        if (e->firstChild != 0 && e->expanded != 0 &&
            e->kind != 0 && e->kind < 23)
            childrenExist = 1;

        if (childrenExist)
            Scr_ScriptWatch_Draw_r(self, e->firstChild, x, y, w, h,
                                   depth + 1, 0, 0, lineNo, curY, a, b);
    }
}

void Scr_ScriptWatch_Draw(Scr_ScriptWatch *self, float x, float y, float w, float h,
                          float a, float b)
{
    int lineNo;
    float curY;
    float lineHeight;

    UI_Component_DrawPic(x, y, w, h, 0, g_debuggerWatchBackground);

    lineHeight = G_DEBUGGER_LINE_HEIGHT;

    lineNo = 0;

    curY = -((float)(int)(b / lineHeight)) * lineHeight;

    Scr_ScriptWatch_Draw_r(self, self->root, x, y, w, h - lineHeight,
                           0, 0, 0, &lineNo, &curY, a, b);
}

int Scr_ScriptWatch_KeyEvent(Scr_ScriptWatch *self, float *pos, int key)
{
    int ctrl;

    ctrl = 0;
    if (Key_IsDown(158) != 0 || Key_IsDown(159) != 0 || Key_IsDown(160) != 0)
        ctrl = 1;

    if (ctrl == 0) {

        Scr_ScriptWatch_CloneSelectedElement(self);
        return 1;
    }

    Scr_ScriptWatch_PasteElement(self);
    return 1;
}

void Scr_ScriptWatch_Init(Scr_ScriptWatch *self)
{
    int i;
    const char *defaults[5];

    self->root = 0;
    self->threadId = 0;
    self->field28 = 0;

    defaults[0] = "self";
    defaults[1] = "level";
    defaults[2] = "game";
    defaults[3] = "anim";
    defaults[4] = "world";

    UI_LinesComponent_Init((UI_LinesComponent *)self);

    self->field30 = 0;

    self->w = G_DEBUGGER_TEXT_SCALE * G_DEBUGGER_WATCH_WIDTH;
    self->h = G_DEBUGGER_WATCH_HEIGHT;

    if (Sys_IsRemoteDebugServer() != 0)
        return;

    if (Scr_ScriptWatch_ReadFromFile(self) != 0)
        return;

    for (i = 0; i < 5; i++)
        Scr_ScriptWatch_PasteElementInternal(self, 0, defaults[i], 0);

}

void Scr_ScriptWatch_Shutdown(Scr_ScriptWatch *self)
{
    Scr_WatchElement_s *e;
    Scr_WatchElement_s *next;

    Scr_ScriptWatch_UpdateBreakpoints(self, 0);
    Scr_UnbreakAllAssignmentPos();

    if (Sys_IsRemoteDebugServer() == 0) {

        int fh = FS_FOpenTextFileWrite("watch.cfg");

        if (self->root == 0) {
            if (fh != 0)
                FS_FCloseFile(fh);
            return;
        }

        do {
            e = self->root;
            next = e->nextSibling;

            if (fh != 0) {
                const char *text = Scr_GetElementArchiveText(e);
                int len = (int)strlen(text);
                FS_Write(text, len, fh);
                FS_Write("\n", 1, fh);
            }

            Scr_ScriptWatch_FreeWatchElement(self, e);
            self->root = next;
        } while (next != 0);
    } else {

        if (self->root == 0)
            return;

        do {
            next = self->root->nextSibling;
            Scr_ScriptWatch_FreeWatchElement(self, self->root);
            self->root = next;
        } while (next != 0);
    }
}

void Scr_ScriptWatch_AddText(Scr_ScriptWatch *self, const char *text)
{
    Scr_WatchElement_s *e;
    int isCondition;

    if (G_SCR_DEBUG_CONSOLE_MODE != 2)
    {
        UI_LinesComponent_AddText((UI_LinesComponent *)self, text);
        return;
    }

    e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e == 0)
        return;

    if (e->readOnly == 0) {
        Scr_ScriptWatch_AddElement(self, e, text);
        return;
    }

    isCondition = (e->breakpointKind == 5) ? 1 : 0;
    Scr_ScriptWatch_PasteBreakpointElement(self, e, text, 1, isCondition, 0);

}

int Scr_ScriptWatch_SetSelectedLineFocus(Scr_ScriptWatch *self, int line, int fromMouse)
{
    Scr_WatchElement_s *e;

    if ((UI_LinesComponent_SetSelectedLineFocus((UI_LinesComponent *)self,
                                                line, fromMouse) &
         0xFF) == 0)
        return 0;

    e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e != 0) {

        if (e->kind == 15 || e->kind == 23)
            Scr_ScriptWatch_DisplayThreadPos(self, e);
    }
    return 1;
}

void Scr_ScriptWatch_UpdateHeight(Scr_ScriptWatch *self)
{
    self->lineCount = Scr_GetWatchElementSize(self->root);
    UI_LinesComponent_UpdateHeight((UI_LinesComponent *)self);
}

int Scr_ScriptWatch_LeftMouseEvent(Scr_ScriptWatch *self, float *pos)
{
    Scr_WatchElement_s *e;
    Scr_WatchElement_s *p;
    int depth;
    int clickDepth;

    clickDepth = Scr_ScriptWatch_MouseClickDepth(self, pos);

    e = Scr_ScriptWatch_GetSelectedElement(self);
    if (e == 0)
        return 0;

    if (e->parent == 0) {
        depth = 0;
    } else {
        depth = 0;
        p = e->parent;
        do {
            ++depth;
            p = p->parent;
        } while (p != 0);
    }

    if (depth != clickDepth)
        return 0;

    {
        Scr_WatchElement_s *sel = Scr_ScriptWatch_GetSelectedElement(self);
        if (sel != 0)
            Scr_ScriptWatch_ExpandElement(self, sel,
                                          (sel->expanded == 0) ? 1 : 0);
    }
    return 1;
}

void Scr_ScriptWatch_SaveSelectedLine(Scr_ScriptWatch *self, struct Scr_SelectedLineInfo *info)
{
    Scr_WatchElement_s *e;
    int id;

    e = Scr_ScriptWatch_GetSelectedElement(self);
    id = (e == 0) ? 0 : e->id;

    *(int *)((char *)info) = id;
    *(int *)((char *)info + 0x4) = self->selectedLine;
    *(unsigned char *)((char *)info + 0x8) = self->hasFocus;
}

void Scr_ScriptWatch_LoadSelectedLine(Scr_ScriptWatch *self, struct Scr_SelectedLineInfo *info)
{
    Scr_WatchElement_s *e;
    int savedLine;
    float lineHeight;

    e = Scr_ScriptWatch_GetElementWithId(self, *(int *)((char *)info));
    if (e == 0)
        return;

    Scr_ScriptWatch_SetSelectedElement(self, e, 0);

    lineHeight = G_DEBUGGER_LINE_HEIGHT;
    savedLine = self->selectedLine - *(int *)((char *)info + 0x4);
    self->scrollTop = (float)savedLine * lineHeight + self->scrollTop;

    self->hasFocus = *(unsigned char *)((char *)info + 0x8);
}

int Scr_ScriptWatch_ReadFromFile(Scr_ScriptWatch *self)
{
    int loaded;
    struct Scr_StringNode_s *list;
    struct Scr_StringNode_s *node;
    struct Scr_StringNode_s *next;

    list = Scr_GetStringList("watch.cfg", &loaded);

    if (list == 0) {

        if (loaded != 0)
            Z_FreeInternal(list);
        return 0;
    }

    node = list;
    do {
        next = node->next;
        Scr_ScriptWatch_PasteElementInternal(self, 0, node->str, 0);

        Z_FreeInternal(node);
        node = next;
    } while (next != 0);

    return 0;
}

void Scr_ScriptWatch_dtor(Scr_ScriptWatch *self, unsigned int flags)
{
    (void)flags;
    self->vtbl->Shutdown(self);
}

const Scr_ScriptWatch_vtbl Scr_ScriptWatch_vtable = {
     Scr_ScriptWatch_Init,
     Scr_ScriptWatch_Shutdown,
     Scr_ScriptWatch_dtor,
     Scr_ScriptWatch_Draw,
     Scr_ScriptWatch_KeyEvent,
     (UI_Component *(*)(Scr_ScriptWatch *, float *))UI_Component_GetCompAtLocation,
     Scr_ScriptWatch_AddText,
     Scr_ScriptWatch_SetSelectedLineFocus,
};

#endif

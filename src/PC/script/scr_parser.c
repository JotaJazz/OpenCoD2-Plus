#include "common_types.h"
#include "imports.h"
#include <stdarg.h>
extern scrCompilePub_t scrCompilePub;
extern scrVarPub_t scrVarPub;

extern struct scrParserPub_t scrParserPub;
extern struct scrParserGlob_t scrParserGlob;

extern void *Z_MallocInternal(int size);
extern void Z_FreeInternal(void *ptr);
extern int FS_ReadFile(const char *qpath, void **buffer);
extern void FS_FreeFile(void *buffer);
extern int FS_FOpenFileRead(const char *filename, fileHandle_t *file, qboolean uniqueFILE);
extern int FS_FOpenFileByMode(const char *filename, fileHandle_t *file, int mode);
extern int FS_Read(void *buffer, int len, fileHandle_t h);
extern void FS_FCloseFile(fileHandle_t h);
extern char * TempMalloc(int len);
extern void *Hunk_AllocateTempMemoryHighInternal(int size);

void Scr_InitOpcodeLookup(void);
void Scr_ShutdownOpcodeLookup(void);
void AddOpcodePos(unsigned int sourcePos, int type);
void RemoveOpcodePos(void);
void AddThreadStartOpcodePos(unsigned int sourcePos);
int Scr_GetSourceBuffer(const char *codePos);
static void Scr_AddSourceBufferInternal(const char *extFilename, const char *codePos, char *sourceBuf, int len, int doEolFixup, int archive);
static void Scr_StripSourceComments(char *sourceBuf);
char *Scr_ReadFile(const char *filename, const char *extFilename, const char *codePos, int archive);
char *Scr_AddSourceBuffer(const char *filename, const char *extFilename, const char *codePos, unsigned char archive);
static void Scr_PrintSourcePos(print_msg_type_t type, const char *filename, const char *sourceBuf, unsigned int sourcePos);
void CompileError(unsigned int sourcePos, const char *msg, ...);
void Scr_PrintPrevCodePos(print_msg_type_t type, const char *codePos, unsigned int index);
void RuntimeError(const char *codePos, unsigned int index, const char *msg, const char *dialogMessage);
void CompileError2(const char *codePos, const char *msg, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void Com_PrintMessage(int channel, const char *msg);
extern char *va(const char *format, ...);
extern int Scr_IsInOpcodeMemory(const char *pos);

void Scr_InitOpcodeLookup(void)
{
    if (!((struct scrVarPub_t *)imp_scrVarPub)->developer)
        return;

    scrParserGlob.delayedSourceIndex = -1;
    scrParserGlob.opcodeLookupMaxLen = 0x10000;
    scrParserGlob.opcodeLookupLen = 0;
    scrParserGlob.opcodeLookup = (OpcodeLookup *)(Z_MallocInternal(0x10000 * (int)COD2_SIZEOF_1_3(OpcodeLookup, 20)));
    memset(scrParserGlob.opcodeLookup, 0, scrParserGlob.opcodeLookupMaxLen * sizeof(OpcodeLookup));
    scrParserGlob.sourcePosLookupMaxLen = 0x10000;
    scrParserGlob.sourcePosLookupLen = 0;
    scrParserGlob.sourcePosLookup = (SourceLookup *)(Z_MallocInternal(0x80000));
    scrParserGlob.currentCodePos = 0;
    scrParserGlob.currentSourcePosCount = 0;
    scrParserGlob.sourceBufferLookupMaxLen = 0x10;
    scrParserPub.sourceBufferLookupLen = 0;
    /* 0x180 was 16 * 24 (the x86 struct size); on x64 the 3 pointers grow it, so size by sizeof. */
    scrParserPub.sourceBufferLookup = (SourceBufferInfo *)
        Z_MallocInternal((int)(scrParserGlob.sourceBufferLookupMaxLen * sizeof(*scrParserPub.sourceBufferLookup)));
}

void Scr_ShutdownOpcodeLookup(void)
{
    int i;

    if (scrParserGlob.opcodeLookup) {
        Z_FreeInternal(scrParserGlob.opcodeLookup);
        scrParserGlob.opcodeLookup = 0;
    }

    if (scrParserGlob.sourcePosLookup) {
        Z_FreeInternal(scrParserGlob.sourcePosLookup);
        scrParserGlob.sourcePosLookup = 0;
    }

    if (scrParserPub.sourceBufferLookup) {
        if (scrParserPub.sourceBufferLookupLen != 0) {
            for (i = 0; (unsigned int)i < scrParserPub.sourceBufferLookupLen; i++)
                Z_FreeInternal(scrParserPub.sourceBufferLookup[i].buf);
        }
        Z_FreeInternal(scrParserPub.sourceBufferLookup);
        scrParserPub.sourceBufferLookup = 0;
    }

    if (scrParserGlob.saveSourceBufferLookup) {
        if (scrParserGlob.saveSourceBufferLookupLen > 0) {
            for (i = 0; i < scrParserGlob.saveSourceBufferLookupLen; i++) {
                char *entry = scrParserGlob.saveSourceBufferLookup[i].sourceBuf;
                if (entry)
                    Z_FreeInternal(entry);
            }
        }
        Z_FreeInternal(scrParserGlob.saveSourceBufferLookup);
        scrParserGlob.saveSourceBufferLookup = 0;
    }
}

#define Scr_ShouldTrackOpcodePos() \
    (scrVarPub.developer && scrCompilePub.developer_statement != 2)

void AddOpcodePos(unsigned int sourcePos, int type)
{
    OpcodeLookup *opcodeLookup;
    SourceLookup *sourceLookup;
    scrCompilePub_t *compilePub;
    unsigned int sourceIndex;

    if (!Scr_ShouldTrackOpcodePos())
        return;

    compilePub = &scrCompilePub;
    if (!compilePub->allowedBreakpoint)
        type &= ~1;

    if (scrParserGlob.opcodeLookupLen >= scrParserGlob.opcodeLookupMaxLen) {
        unsigned int oldMaxLen = scrParserGlob.opcodeLookupMaxLen;
        OpcodeLookup *newOpcodeLookup;

        scrParserGlob.opcodeLookupMaxLen = oldMaxLen * 2;
        newOpcodeLookup = (OpcodeLookup *)(Z_MallocInternal((int)(scrParserGlob.opcodeLookupMaxLen * sizeof(*newOpcodeLookup))));
        memcpy(newOpcodeLookup, scrParserGlob.opcodeLookup, scrParserGlob.opcodeLookupLen * sizeof(*newOpcodeLookup));
        Z_FreeInternal(scrParserGlob.opcodeLookup);
        scrParserGlob.opcodeLookup = newOpcodeLookup;
    }

    if (scrParserGlob.sourcePosLookupLen >= scrParserGlob.sourcePosLookupMaxLen) {
        unsigned int oldMaxLen = scrParserGlob.sourcePosLookupMaxLen;
        SourceLookup *newSourcePosLookup;

        scrParserGlob.sourcePosLookupMaxLen = oldMaxLen * 2;
        newSourcePosLookup = (SourceLookup *)(Z_MallocInternal((int)(scrParserGlob.sourcePosLookupMaxLen * sizeof(*newSourcePosLookup))));
        memcpy(newSourcePosLookup, scrParserGlob.sourcePosLookup, scrParserGlob.sourcePosLookupLen * sizeof(*newSourcePosLookup));
        Z_FreeInternal(scrParserGlob.sourcePosLookup);
        scrParserGlob.sourcePosLookup = newSourcePosLookup;
    }

    if ((const byte *)compilePub->opcodePos == scrParserGlob.currentCodePos) {
        scrParserGlob.opcodeLookupLen--;
        opcodeLookup = &scrParserGlob.opcodeLookup[scrParserGlob.opcodeLookupLen];
    } else {
        scrParserGlob.currentSourcePosCount = 0;
        scrParserGlob.currentCodePos = compilePub->opcodePos;
        opcodeLookup = &scrParserGlob.opcodeLookup[scrParserGlob.opcodeLookupLen];
        opcodeLookup->sourcePosIndex = scrParserGlob.sourcePosLookupLen;
        opcodeLookup->codePos = (const char *)scrParserGlob.currentCodePos;
    }

    sourceIndex = opcodeLookup->sourcePosIndex + scrParserGlob.currentSourcePosCount;
    sourceLookup = &scrParserGlob.sourcePosLookup[sourceIndex];
    sourceLookup->sourcePos = sourcePos;

    if (sourcePos == (unsigned int)-1) {
        scrParserGlob.delayedSourceIndex = (int)sourceIndex;
    } else if (sourcePos == (unsigned int)-2) {
        scrParserGlob.threadStartSourceIndex = (int)sourceIndex;
    } else if (scrParserGlob.delayedSourceIndex >= 0 && (type & 1)) {
        scrParserGlob.sourcePosLookup[scrParserGlob.delayedSourceIndex].sourcePos = sourcePos;
        scrParserGlob.delayedSourceIndex = -1;
    }

    sourceLookup->type |= type;
    scrParserGlob.currentSourcePosCount++;
    opcodeLookup->sourcePosCount = scrParserGlob.currentSourcePosCount;
    scrParserGlob.opcodeLookupLen++;
    scrParserGlob.sourcePosLookupLen++;
}

void RemoveOpcodePos(void)
{
    OpcodeLookup *opcodeLookup;

    if (!Scr_ShouldTrackOpcodePos())
        return;

    scrParserGlob.sourcePosLookupLen--;
    scrParserGlob.opcodeLookupLen--;
    scrParserGlob.currentSourcePosCount--;

    opcodeLookup = &scrParserGlob.opcodeLookup[scrParserGlob.opcodeLookupLen];

    if (!scrParserGlob.currentSourcePosCount)
        scrParserGlob.currentCodePos = NULL;

    opcodeLookup->sourcePosCount = scrParserGlob.currentSourcePosCount;
}

void AddThreadStartOpcodePos(unsigned int sourcePos)
{
    SourceLookup *sourceLookup;

    if (!Scr_ShouldTrackOpcodePos())
        return;

    sourceLookup = &scrParserGlob.sourcePosLookup[scrParserGlob.threadStartSourceIndex];
    sourceLookup->sourcePos = sourcePos;
    sourceLookup->type = 4;
    scrParserGlob.threadStartSourceIndex = -1;
}

static inline __attribute__((always_inline)) int Scr_GetSourceBuffer_core(const char *codePos)
{
    int bufferIndex = (int)scrParserPub.sourceBufferLookupLen - 1;

    if (bufferIndex > 0) {
        SourceBufferInfo *sourceBuffer = &scrParserPub.sourceBufferLookup[bufferIndex];
        int count = bufferIndex;
        int i;

        for (i = 0; i != count; i++) {
            if (sourceBuffer->codePos && sourceBuffer->codePos <= codePos)
                break;

            bufferIndex--;
            sourceBuffer--;
        }
    }

    return bufferIndex;
}

int Scr_GetSourceBuffer(const char *codePos)
{
    return Scr_GetSourceBuffer_core(codePos);
}

static void Scr_AddSourceBufferInternal(const char *extFilename, const char *codePos, char *sourceBuf, int len, int doEolFixup, int archive)
{
    char *buf;
    char *sourceBuf2;
    size_t filenameLen;
    SourceBufferInfo *sourceBuffer;
    SourceBufferInfo *entry;

    if (!scrParserPub.sourceBufferLookup) {
        scrParserPub.sourceBuf = NULL;
        return;
    }

    filenameLen = strlen(extFilename) + 1;
    buf = (char *)(Z_MallocInternal((int)filenameLen + len + 1));
    strcpy(buf, extFilename);

    if (sourceBuf) {
        int i;

        sourceBuf2 = buf + filenameLen;
        if (len >= 0) {
            if (doEolFixup) {
                for (i = 0; i <= len; i++) {
                    char ch = sourceBuf[i];

                    if (ch == '\n' || (ch == '\r' && sourceBuf[i + 1] != '\n'))
                        sourceBuf2[i] = '\0';
                    else
                        sourceBuf2[i] = ch;
                }
            } else {
                memcpy(sourceBuf2, sourceBuf, len + 1);
            }
        }
    } else {
        sourceBuf2 = NULL;
    }

    if (scrParserPub.sourceBufferLookupLen >= scrParserGlob.sourceBufferLookupMaxLen) {
        unsigned int oldMaxLen = scrParserGlob.sourceBufferLookupMaxLen;

        scrParserGlob.sourceBufferLookupMaxLen = oldMaxLen * 2;
        sourceBuffer = (SourceBufferInfo *)(Z_MallocInternal(scrParserGlob.sourceBufferLookupMaxLen * sizeof(*sourceBuffer)));
        memcpy(sourceBuffer, scrParserPub.sourceBufferLookup, scrParserPub.sourceBufferLookupLen * sizeof(*sourceBuffer));
        Z_FreeInternal(scrParserPub.sourceBufferLookup);
        scrParserPub.sourceBufferLookup = sourceBuffer;
    }

    entry = &scrParserPub.sourceBufferLookup[scrParserPub.sourceBufferLookupLen++];
    entry->codePos = codePos;
    entry->buf = buf;
    entry->sourceBuf = sourceBuf2;
    entry->len = len;
    entry->sortedIndex = -1;
    entry->archive = (Bool)archive;

    if (sourceBuf2)
        scrParserPub.sourceBuf = sourceBuf2;
}

static void Scr_StripSourceComments(char *sourceBuf)
{
    char *p = sourceBuf;
    int quote = 0;

    while (*p) {
        if (quote) {
            if (*p == '\\' && p[1]) {
                p += 2;
                continue;
            }

            if (*p == quote)
                quote = 0;

            p++;
            continue;
        }

        if (*p == '"' || *p == '\'') {
            quote = *p;
            p++;
            continue;
        }

        if (p[0] == '/' && p[1] == '/') {
            *p++ = ' ';
            *p++ = ' ';
            while (*p && *p != '\n' && *p != '\r')
                *p++ = ' ';
            continue;
        }

        if (p[0] == '/' && p[1] == '*') {
            *p++ = ' ';
            *p++ = ' ';

            while (*p) {
                if (p[0] == '*' && p[1] == '/') {
                    *p++ = ' ';
                    *p++ = ' ';
                    break;
                }

                if (*p != '\n' && *p != '\r')
                    *p = ' ';

                p++;
            }

            continue;
        }

        p++;
    }
}

char *Scr_ReadFile(const char *filename, const char *extFilename, const char *codePos, int archive)
{
    fileHandle_t file;
    char *fileBuffer;
    int len;
    int bytesRead;

    (void)filename;

    len = FS_FOpenFileRead(extFilename, &file, 0);
    if (len < 0 || !file)
        return NULL;

    fileBuffer = (char *)TempMalloc(len + 1);
    bytesRead = FS_Read(fileBuffer, len, file);
    FS_FCloseFile(file);
    if (bytesRead != len)
        return NULL;

    fileBuffer[len] = '\0';

    if (scrParserPub.sourceBufferLookup) {
        Scr_AddSourceBufferInternal(extFilename, codePos, fileBuffer, len, 1, archive);
        Scr_StripSourceComments(fileBuffer);
    } else {
        Scr_StripSourceComments(fileBuffer);
        scrParserPub.sourceBuf = fileBuffer;
    }

    return fileBuffer;
}

char *Scr_AddSourceBuffer(const char *filename, const char *extFilename, const char *codePos, unsigned char archive)
{
    char *buf;
    int len;

    (void)filename;

    if (archive) {
        if (scrParserGlob.saveSourceBufferLookup) {
            SaveSourceBufferInfo *entry;

            scrParserGlob.saveSourceBufferLookupLen--;
            entry = &scrParserGlob.saveSourceBufferLookup[scrParserGlob.saveSourceBufferLookupLen];
            len = entry->len;

            if (len < 0) {
                buf = NULL;
            } else {
                int i;

                buf = (char *)Hunk_AllocateTempMemoryHighInternal(len + 1);
                for (i = 0; i < len; i++) {
                    char ch = entry->sourceBuf[i];
                    buf[i] = ch ? ch : '\n';
                }
                buf[len] = '\0';

                if (entry->sourceBuf)
                    Z_FreeInternal(scrParserGlob.saveSourceBufferLookup[scrParserGlob.saveSourceBufferLookupLen].sourceBuf);
            }

            Scr_AddSourceBufferInternal(extFilename, codePos, buf, len, 1, archive);
            return buf;
        }
    }

    {
        fileHandle_t file;

        len = FS_FOpenFileByMode(extFilename, &file, FS_READ);
        if (len < 0) {
            Scr_AddSourceBufferInternal(extFilename, codePos, NULL, -1, 1, archive);
            return NULL;
        }

        buf = (char *)Hunk_AllocateTempMemoryHighInternal(len + 1);
        FS_Read(buf, len, file);
        buf[len] = '\0';
        FS_FCloseFile(file);

        Scr_AddSourceBufferInternal(extFilename, codePos, buf, len, 1, archive);
        return buf;
    }
}

static void Scr_PrintSourcePos(print_msg_type_t type, const char *filename, const char *sourceBuf, unsigned int sourcePos)
{
    char line[1024];
    const char *startLine = sourceBuf;
    const char *pos = sourceBuf;
    int col = 0;
    int lineNumber = 0;
    int lineLen;
    int i;

    for (i = 0; i != (int)sourcePos; i++) {
        if (*pos) {
            pos++;
        } else {
            startLine = pos + 1;
            pos = startLine;
            lineNumber++;
        }
    }

    col = (int)(pos - startLine);
    lineLen = (int)strlen(startLine);
    if (lineLen > 1023)
        lineLen = 1023;

    for (i = 0; i <= lineLen; i++) {
        char ch = startLine[i];

        line[i] = (ch == '\t') ? ' ' : ch;
    }

    if (line[lineLen - 1] == '\r')
        line[lineLen - 1] = '\0';

    Com_PrintMessage(type, va("(file '%s'%s, line %d)\n",
                              filename,
                              scrParserGlob.saveSourceBufferLookup ? " (savegame)" : "",
                              lineNumber + 1));
    Com_PrintMessage(type, va("%s\n", line));

    for (i = 0; i < col; i++)
        Com_PrintMessage(type, " ");
    Com_PrintMessage(type, "*\n");
}

void CompileError(unsigned int sourcePos, const char *msg, ...)
{
    char text[1024];
    va_list argptr;
    scrVarPub_t *varPub;

    va_start(argptr, msg);
    vsnprintf(text, sizeof(text), msg, argptr);
    va_end(argptr);
    varPub = &scrVarPub;

    if (varPub->evaluate) {
        if (!varPub->error_message)
            varPub->error_message = va("%s", text);
        return;
    }

    Com_Printf("\n");
    Com_Printf("******* script compile error *******\n");

    if (!varPub->developer) {
        Com_Printf("%s\n", text);
    } else {
        Com_Printf("%s: ", text);
        Scr_PrintSourcePos( (print_msg_type_t)(0), scrParserPub.scriptfilename, scrParserPub.sourceBuf, sourcePos);
    }

    Com_Printf("************************************\n");
    Com_Error(5, "\x15script compile error\n(see console for details)");
}

static inline __attribute__((always_inline)) OpcodeLookup *Scr_FindOpcodeLookup(const char *codePos)
{
    int low = 0;
    int high = (int)scrParserGlob.opcodeLookupLen - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        OpcodeLookup *opcodeLookup = &scrParserGlob.opcodeLookup[mid];

        if (codePos >= opcodeLookup->codePos) {
            int next = mid + 1;

            if (next == (int)scrParserGlob.opcodeLookupLen || codePos < scrParserGlob.opcodeLookup[next].codePos)
                return opcodeLookup;

            low = next;
        } else {
            high = mid - 1;
        }
    }

    return NULL;
}

void Scr_PrintPrevCodePos(print_msg_type_t type, const char *codePos, unsigned int index)
{
    scrVarPub_t *varPub = &scrVarPub;

    if (!codePos) {
        Com_PrintMessage(type, "<frozen thread>\n");
        return;
    }

    if (codePos == (const char *)imp_g_EndPos) {
        Com_PrintMessage(type, "<removed thread>\n");
        return;
    }

#if defined(_M_X64) || defined(__x86_64__)
    {   /* x64: the link phase can hand CompileError2 a TRUNCATED codePos (high 32 bits
           zero) -> the va("%s") below would crash. Guard it so the real compile error
           surfaces instead of crashing the error printer. */
        if ((!varPub->programBuffer || !Scr_IsInOpcodeMemory(codePos)) &&
            ((unsigned long long)(uintptr_t)codePos >> 32) == 0) {
            Com_PrintMessage(type, "<bad codepos>\n\n");
            return;
        }
    }
#endif

    if (!varPub->developer) {
        if (Scr_IsInOpcodeMemory(codePos - 1))
            Com_PrintMessage(type, va("@ %d\n", (int)(codePos - varPub->programBuffer)));
        else
            Com_PrintMessage(type, va("%s\n\n", codePos));
        return;
    }

    if (varPub->programBuffer && Scr_IsInOpcodeMemory(codePos)) {
        const char *prevCodePos = codePos - 1;
        int bufferIndex = Scr_GetSourceBuffer_core(prevCodePos);
        OpcodeLookup *opcodeLookup = Scr_FindOpcodeLookup(prevCodePos);

        if (opcodeLookup) {
            SourceBufferInfo *sourceBuffer = &scrParserPub.sourceBufferLookup[bufferIndex];
            unsigned int sourceIndex = opcodeLookup->sourcePosIndex + index;

            Scr_PrintSourcePos(type, sourceBuffer->buf, sourceBuffer->sourceBuf,
                               scrParserGlob.sourcePosLookup[sourceIndex].sourcePos);
            return;
        }
    }

    Com_PrintMessage(type, va("%s\n\n", codePos));
}

#if COD2_FEATURE_SCRIPT_DEBUGGER

int Scr_Debugger_CodePosToLine(const char *codePos, const char **filenameOut)
{
    scrVarPub_t *varPub = &scrVarPub;
    int bufferIndex;
    OpcodeLookup *opcodeLookup;
    SourceBufferInfo *sourceBuffer;
    const char *sourceBuf;
    unsigned int sourcePos;
    int lineNumber = 0;
    unsigned int i;

    if (filenameOut)
        *filenameOut = NULL;
    if (!codePos || !varPub->programBuffer || !Scr_IsInOpcodeMemory(codePos))
        return 0;

    bufferIndex = Scr_GetSourceBuffer_core(codePos);
    opcodeLookup = Scr_FindOpcodeLookup(codePos);
    if (!opcodeLookup)
        return 0;

    sourceBuffer = &scrParserPub.sourceBufferLookup[bufferIndex];
    sourceBuf = sourceBuffer->sourceBuf;
    if (!sourceBuf)
        return 0;
    if (filenameOut)
        *filenameOut = sourceBuffer->buf;

    sourcePos = scrParserGlob.sourcePosLookup[opcodeLookup->sourcePosIndex].sourcePos;
    for (i = 0; i != sourcePos; i++) {
        if (!sourceBuf[i])
            lineNumber++;
    }
    return lineNumber + 1;
}

static int Scr_Debugger_FilenameMatch(const char *a, const char *b)
{
    size_t la, lb, n;

    if (!a || !b)
        return 0;
    if (I_stricmp(a, b) == 0)
        return 1;
    la = strlen(a);
    lb = strlen(b);
    n = (la < lb) ? la : lb;
    if (n == 0)
        return 0;
    if (I_stricmp(a + (la - n), b + (lb - n)) != 0)
        return 0;
    {
        char before = (la > n) ? a[la - n - 1] : ((lb > n) ? b[lb - n - 1] : '/');
        return (before == '/' || before == '\\');
    }
}

unsigned char *Scr_ResolveSourcePos(const char *filename, int line)
{
    unsigned int i;

    if (!filename || line <= 0)
        return NULL;

    for (i = 0; i < scrParserGlob.opcodeLookupLen; i++) {
        const char *cp = scrParserGlob.opcodeLookup[i].codePos;
        const char *fn = NULL;
        int l = Scr_Debugger_CodePosToLine(cp, &fn);

        if (l == line && Scr_Debugger_FilenameMatch(fn, filename))
            return (unsigned char *)cp;
    }
    return NULL;
}
#endif

void RuntimeError(const char *codePos, unsigned int index, const char *msg, const char *dialogMessage)
{
    scrVarPub_t *varPub = &scrVarPub;
    scrVmPub_t *vmPub = (scrVmPub_t *)imp_scrVmPub;
    Bool shouldAbort;
    print_msg_type_t type;

    if (!varPub->developer && !vmPub->terminal_error)
        return;

    if (vmPub->debugCode) {
        Com_Printf("%s\n", msg);
        if (!vmPub->terminal_error)
            return;
    } else {
        int i;
        int callerCount;

        shouldAbort = (vmPub->abort_on_error || vmPub->terminal_error);
        type = (print_msg_type_t)(shouldAbort ? 0 : 4);

        Com_PrintMessage(type, va("\n******* script runtime error *******\n%s: ", msg));
        Scr_PrintPrevCodePos(type, codePos, index);

        if (vmPub->function_count) {
            callerCount = vmPub->function_count - 1;
            for (i = 0; i < callerCount; i++) {
                function_frame_t *frame = &vmPub->function_frame_start[callerCount - i];

                Com_PrintMessage(type, "called from:\n");
                Scr_PrintPrevCodePos(type, frame->fs.pos, frame->fs.localId == 0);
            }

            Com_PrintMessage(type, "started from:\n");
            Scr_PrintPrevCodePos(type, vmPub->function_frame_start[0].fs.pos, 1);
        }

        Com_PrintMessage(type, "************************************\n");
        if (!shouldAbort)
            return;
    }

    if (!dialogMessage)
        dialogMessage = "";

    Com_Error(vmPub->terminal_error ? 5 : 4,
              "\x15script runtime error\n(see console for details)\n%s%s%s",
              msg,
              dialogMessage[0] ? "\n" : "",
              dialogMessage);
}

void CompileError2(const char *codePos, const char *msg, ...)
{
    char text[1024];
    va_list argptr;

    Com_Printf("\n");
    Com_Printf("******* script compile error *******\n");

    va_start(argptr, msg);
    vsnprintf(text, sizeof(text), msg, argptr);
    va_end(argptr);

    Com_Printf("%s: ", text);
    Scr_PrintPrevCodePos( (print_msg_type_t)(0), codePos, 0);
    Com_Printf("************************************\n");
    Com_Error(5, "\x15script compile error\n(see console for details)");
}

#ifndef SCR_DEBUGGER_H
#define SCR_DEBUGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cod2_feature_config.h"

#if COD2_FEATURE_SCRIPT_DEBUGGER

void Scr_InitDebuggerSystem(void);
void Scr_ShutdownDebuggerSystem(int restart);
void Scr_UpdateDebugger(void);
void Scr_RunDebugger(void);
void Scr_AddDebuggerRefs(void);
int Scr_HitBreakpoint(void *value, const char *file, unsigned int line, int column);
int Scr_HitAssignmentBreakpoint(void *value, const char *file, unsigned int line, int column);
void Scr_AddManualBreakpoint(const char *filename, int line);

void Scr_Debugger_OnInstruction(const char *codePos);

#else

static inline void Scr_InitDebuggerSystem(void) {}
static inline void Scr_ShutdownDebuggerSystem(int restart)
{
    (void)restart;
}
static inline void Scr_UpdateDebugger(void) {}
static inline void Scr_RunDebugger(void) {}
static inline void Scr_AddDebuggerRefs(void) {}
static inline int Scr_HitBreakpoint(void *value, const char *file, unsigned int line, int column)
{
    (void)value;
    (void)file;
    (void)line;
    (void)column;
    return 0;
}
static inline int Scr_HitAssignmentBreakpoint(void *value, const char *file, unsigned int line, int column)
{
    (void)value;
    (void)file;
    (void)line;
    (void)column;
    return 0;
}
static inline void Scr_AddManualBreakpoint(const char *filename, int line)
{
    (void)filename;
    (void)line;
}
static inline void Scr_Debugger_OnInstruction(const char *codePos)
{
    (void)codePos;
}

#endif

#ifdef __cplusplus
}
#endif
#endif

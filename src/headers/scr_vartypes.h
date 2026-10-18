#ifndef COD2_SCR_VARTYPES_H
#define COD2_SCR_VARTYPES_H

/* The object types Scr_GetPointerType returns, for the engine's game code.
 * CoD2rev-Plus's VM (COD2_PLUS_SCRIPT) adds VAR_INT64 after VAR_INTEGER, which
 * moves every type after it up by one; src/compat/script_missing.cpp checks
 * these against its enum. The types Scr_GetType returns for values, up to
 * VAR_INTEGER (6), are the same in both. */
#if COD2_PLUS_SCRIPT
#define COD2_VAR_DEAD_ENTITY 21
#define COD2_VAR_ENTITY      22
#define COD2_VAR_ARRAY       23
#define COD2_VAR_DEAD_THREAD 24
#else
#define COD2_VAR_DEAD_ENTITY 0x14
#define COD2_VAR_ENTITY      0x15
#define COD2_VAR_ARRAY       0x16
#define COD2_VAR_DEAD_THREAD 0x17
#endif

#endif

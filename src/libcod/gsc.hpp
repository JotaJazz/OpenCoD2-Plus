#ifndef _GSC_HPP_
#define _GSC_HPP_

#include "../qcommon/qcommon.h"
#include "../game/g_shared.h"

#define COD2_MAX_STRINGLENGTH MAX_STRING_CHARS

#include "libcod_client.h"

/* default stuff */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdarg.h> // va_args
#include <unistd.h> //link, unlink, usleep
#include <dirent.h> // dir stuff
#include <stddef.h> // offsetof
#include <sys/stat.h> // fsize
#include <time.h>  // getsystemtime
#include <sys/time.h> // milliseconds
#include <ctype.h> // isdigit

#if LIBCOD_COMPILE_BOTS == 1
#include "gsc_bots.hpp"
#endif

#if LIBCOD_COMPILE_ENTITY == 1
#include "gsc_entity.hpp"
#endif

#if LIBCOD_COMPILE_EXEC == 1
#include "gsc_exec.hpp"
#endif

#if LIBCOD_COMPILE_GRAPH == 1
#include "gsc_graph.hpp"
#endif

#if LIBCOD_COMPILE_HTTP == 1
#include "gsc_http.hpp"
#endif

#if LIBCOD_COMPILE_JSON == 1
#include "gsc_json.hpp"
#endif

#if LIBCOD_COMPILE_LEVEL == 1
#include "gsc_level.hpp"
#endif

#if LIBCOD_COMPILE_MEMORY == 1
#include "gsc_memory.hpp"
#endif

#if LIBCOD_COMPILE_MYSQL == 1
#include "gsc_mysql.hpp"
#endif

#if LIBCOD_COMPILE_PLAYER == 1
#include "gsc_player.hpp"
#endif

#if LIBCOD_COMPILE_SQLITE == 1
#include "gsc_sqlite.hpp"
#endif

#if LIBCOD_COMPILE_UTILS == 1
#include "gsc_utils.hpp"
#endif

#if LIBCOD_COMPILE_WEAPONS == 1
#include "gsc_weapons.hpp"
#endif

#ifdef EXTRA_INCLUDES_INC
#include "extra/includes.hpp"
#endif

#define STACK_UNDEFINED VAR_UNDEFINED
#define STACK_OBJECT VAR_POINTER
#define STACK_STRING VAR_STRING
#define STACK_LOCALIZED_STRING VAR_ISTRING
#define STACK_VECTOR VAR_VECTOR
#define STACK_FLOAT VAR_FLOAT
#define STACK_INT VAR_INTEGER
#define STACK_INT64 VAR_INT64
#define STACK_CODEPOS VAR_CODEPOS
#define STACK_PRECODEPOS VAR_PRECODEPOS
#define STACK_FUNCTION VAR_FUNCTION
#define STACK_STACK VAR_STACK
#define STACK_ANIMATION VAR_ANIMATION
#define STACK_DEVELOPER_CODEPOS VAR_DEVELOPER_CODEPOS
#define STACK_INCLUDE_CODEPOS VAR_INCLUDE_CODEPOS
#define STACK_THREAD_LIST VAR_THREAD_LIST
#define STACK_THREAD_1 VAR_THREAD
#define STACK_THREAD_2 VAR_NOTIFY_THREAD
#define STACK_THREAD_3 VAR_TIME_THREAD
#define STACK_THREAD_4 VAR_CHILD_THREAD
#define STACK_STRUCT VAR_OBJECT
#define STACK_REMOVED_ENTITY VAR_DEAD_ENTITY
#define STACK_ENTITY VAR_ENTITY
#define STACK_ARRAY VAR_ARRAY
#define STACK_REMOVED_THREAD VAR_DEAD_THREAD

#define stackPushUndefined Scr_AddUndefined
#define stackPushBool Scr_AddBool
#define stackPushInt Scr_AddInt
#define stackPushInt64 Scr_AddInt64
#define stackPushFloat Scr_AddFloat
#define stackPushString Scr_AddString
#define stackPushVector Scr_AddVector
#define stackPushEntity Scr_AddEntity
#define stackPushArray Scr_MakeArray
#define stackPushArrayLast Scr_AddArray
#define stackPushObject Scr_AddObject

#ifndef Q_vsnprintf
int Q_vsnprintf(char *s0, size_t size, const char *fmt, va_list args);
#endif

int stackGetParamType(int param);
const char *stackGetParamTypeAsString(int param);

int stackGetParams(const char *params, ...);
void stackError(const char *format, ...);

int stackGetParamInt(int param, int *value);
int stackGetParamInt64(int param, long int *value);
int stackGetParamFunction(int param, int *value);
int stackGetParamString(int param, const char **value);
int stackGetParamConstString(int param, unsigned int *value);
int stackGetParamVector(int param, vec3_t value);
int stackGetParamFloat(int param, float *value);
int stackGetParamObject(int param, unsigned int *value);

xfunction_t Scr_GetCustomFunction(const char **fname, qboolean *fdev);
xmethod_t Scr_GetCustomMethod(const char **fname, qboolean *fdev);

uint64_t Sys_Milliseconds64(void);

extern dvar_t *sv_master[MAX_MASTER_SERVERS];
extern dvar_t *fs_library;
extern dvar_t *g_playerCollision;
extern dvar_t *g_playerEject;
extern dvar_t *sv_allowRcon;
extern dvar_t *sv_downloadMessage;
extern dvar_t *sv_fastDownload;
extern dvar_t *sv_cracked;
extern dvar_t *sv_kickbots;
extern dvar_t *jump_bounceEnable;
extern dvar_t *g_mantleBlockEnable;
extern dvar_t *g_fixedWeaponSpreads;
extern dvar_t *g_dropGrenadeOnDeath;
extern dvar_t *g_safePrecache;
extern dvar_t *g_reservedModels;
extern dvar_t *logErrors;
extern dvar_t *g_corpseHit;
extern dvar_t *g_spectateBots;
extern dvar_t *g_logPickup;
extern dvar_t *sv_botKickMessages;
extern dvar_t *sv_noAuthorize;
extern dvar_t *sv_authorizeTimeout;
extern dvar_t *sv_limitLocalRcon;
extern bool libcod_precaching;
extern bool libcod_unsafeBuiltins;
extern char consolePrefix[64];
extern char testClientName[];

void RegisterLibcodDvars();
int hook_findMap(const char *qpath, void **buffer);
bool SV_IsBannedIp(netadr_t adr);
void hook_ClientCommand(int clientNum);
bool Scr_RemoteCommandCallback(netadr_t from);
bool Scr_ProcessRemoteCommand();
bool ContainsMapCommand(const char *text);
void SV_ExecuteRemoteCommand(netadr_t from, const char *command);
void InitLibcodCallbacks();
void Libcod_SpawnServerConfigstrings();
void Libcod_RegisterCallbackDvars();
void Libcod_LoadCallbacks();
void Libcod_PlayerEvent(const playerState_t *ps, int event, int eventParm);
void Libcod_EntityEvent(gentity_t *ent, int event, int eventParm);
void Libcod_Notify(gentity_t *ent, unsigned int stringValue, unsigned int paramcount);
void Libcod_QueueNotifyDebug(unsigned int objectId, unsigned int stringValue, VariableValue *top);
bool Libcod_MoverBlocked(gentity_t *check, gentity_t *pusher);
void Libcod_ClearMapItems();
bool Libcod_SkipMapEntity(const char *classname);
void Libcod_MapItemsCallbacks();
void Libcod_DPrintf(const char *message);
bool Libcod_SpamCallback(const char *type, netadr_t from);
void Libcod_RunCallbacksFrame();
void Libcod_ShutdownCallbacks();
void Libcod_InitLevel();
void Libcod_ShutdownLevel();
extern unsigned int libcodLevelGeneration;
void Scr_QueueErrorCallback(const char *message, const char *position);
void Scr_RunFrameCallbacks();
void Scr_ClientThinkCallbacks(client_t *cl, int oldButtons);
void SV_MasterHeartbeat_libcod(const char *hbname);
void SV_ResetPureClient_libcod(client_t *cl);
void SV_CalcPings_libcod( void );
void SV_CheckTimeouts_libcod( void );
void SV_BotUserMove_libcod(client_t *client);
void SV_ClientThink_libcod(client_t *cl, usercmd_t *cmd);
const char* NET_AdrToStringNoPort( netadr_t a );

void PM_ProjectVelocity(const float *velIn, const float *normal, float *velOut);
void SV_AddLibcodCommands();
const char *SV_ModifyConfigstringIwdChkSum( client_t *client, int index );
VariableValue* Scr_GetValue(unsigned int param);

#endif

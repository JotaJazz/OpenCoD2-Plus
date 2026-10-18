// Compiles the shared headers as C++: every COD2_ASSERT_FIELD/COD2_ASSERT_SIZE
// in them is evaluated with the C++ front end, and the C linkage of their
// declarations is checked below.
#include "common_types.h"
#include "imports.h"
#include "cod2_feature_config.h"
#include "pb_public.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "PC/qcommon/net_hardening.h"
#include "PC/qcommon/crash_handler.h"
#include "headers/PC/bgame/bg_funcs.h"
#include "headers/PC/cgame_mp/cg_funcs.h"
#include "headers/PC/client_mp/cl_funcs.h"
#include "headers/PC/EffectsCore/fx_funcs.h"
#include "headers/PC/game_mp/g_funcs.h"
#include "headers/PC/gfx_d3d/gfx_funcs.h"
#include "headers/PC/qcommon/qcommon_funcs.h"
#include "headers/PC/script/scr_funcs.h"
#include "headers/PC/server_mp/sv_funcs.h"
#include "headers/PC/ui_mp/ui_funcs.h"
#include "headers/PC/xanim/xanim_funcs.h"

#if !defined(COD2_X64)
static_assert(sizeof(void *) == 4, "32-bit");
#endif
static_assert(sizeof(qboolean) == 4, "qboolean");
static_assert(sizeof(Bool) == 1, "Bool");

// Declarations with C linkage after the headers must match theirs: had the
// headers declared these with C++ linkage, they would conflict.
extern "C" void Com_Printf(const char *fmt, ...);
extern "C" void *imp_Com_Error;

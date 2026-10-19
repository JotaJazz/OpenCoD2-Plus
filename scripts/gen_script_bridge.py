#!/usr/bin/env python3
"""Generates the C/C++ bridge between the engine and a CoD2rev-Plus module.

    python3 scripts/gen_script_bridge.py [--module script|server] [--build build/check-linux] [--names]

The engine calls the module by C names; CoD2rev-Plus's code (src/script, the
VM, or src/server) defines C++ functions (some overloaded) and calls engine
functions by C++ names. Reading the module's objects of a build configured
with it on (COD2_PLUS_SCRIPT, COD2_PLUS_SERVER), this writes:

- src/compat/<module>_exports.cpp: for each name in
  test/<module>_interface.txt, an extern "C" thunk whose symbol is the C name
  (an __asm__ label) and which calls the C++ function.
- src/compat/<module>_imports.cpp: for each C++ engine function the module
  needs and no other CoD2rev-Plus code defines, a C++ definition that calls
  the engine's C symbol. Variadic ones format into a 4096-byte buffer and pass
  it on as "%s".

Return types come from decltype on the real CoD2rev-Plus declarations, so the
compiler checks every thunk against them. When test/<module>_abi.txt exists
(scripts/engine_abi.py), each thunk also asserts that the return class and
size and the stack slot of every parameter match the engine's definition,
and widens an integer return the two sides give different sizes.
--names prints the imports and exports instead, one per line.
"""
import argparse, glob, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Overloaded names the engine calls: the parameter list it means.
OVERLOADS = {
    'AddRefToValue': ['int', 'VariableUnion'],
    'RemoveRefToValue': ['int', 'VariableUnion'],
}

# Engine functions that take more arguments here than CoD2rev-Plus passes:
# (extra C parameter types, the values passed). src/PC/script passed 10.
EXTRA_ARGS = {
    'FS_ListFiles': (['int'], ['10']),
    'FS_FreeFileList': (['int'], ['10']),
}

# Engine functions with another name in this repo.
RENAMED = {
    'Hunk_ClearTempMemoryHighInternal': 'Hunk_ClearTempMemoryHigh',
}

# Functions the VM needs that this engine does not have; they are written by
# hand in src/compat/script_missing.cpp.
PROVIDED = {'Hunk_AllocXAnimTreePrecache', 'Q_rint', 'Q_vsnprintf', 'Sys_UnimplementedFunctionInternal',
            'VectorCompare'}

# Exports the engine calls with the client binary's script system id first
# ('sys'), which CoD2rev-Plus's single-system VM does not take. The other
# such functions only gain an ignored trailing argument; these two change
# meaning, so their thunks are written out (as src/PC/script behaved).
EXPORT_CUSTOM = {
    'Scr_FreeScripts': '''extern "C" void cod2x_Scr_FreeScripts(int sys) __asm__(COD2_SYM("Scr_FreeScripts"));
void cod2x_Scr_FreeScripts(int sys)
{
    // src/PC/script freed the scripts only for system 0; G_InitGame passes 1
    // right after loading them, which only ends the loading.
    if (sys == 0) {
        Scr_FreeScripts();
        return;
    }
    if (scrCompilePub.script_loading) {
        scrCompilePub.script_loading = 0;
        Scr_EndLoadScripts();
    }
    if (scrAnimPub.animtree_loading) {
        scrAnimPub.animtree_loading = 0;
        Scr_EndLoadAnimTrees();
    }
}
''',
    # CoD2rev-Plus returns scr_anim_s by value, which GCC on Linux i386 returns
    # through a hidden pointer; the engine declares unsigned int and reads eax.
    'Scr_GetAnim': '''extern "C" unsigned int cod2x_Scr_GetAnim(unsigned int index, int tree) __asm__(COD2_SYM("Scr_GetAnim"));
unsigned int cod2x_Scr_GetAnim(unsigned int index, int tree)
{
    scr_anim_s anim = Scr_GetAnim(index, (XAnimTree_s *)(uintptr_t)tree);
    unsigned int value;
    static_assert(sizeof(anim) == sizeof(value), "scr_anim_s is 4 bytes");
    memcpy(&value, &anim, sizeof(value));
    return value;
}
''',
    'Scr_ShutdownSystem': '''extern "C" void cod2x_Scr_ShutdownSystem(int sys, int bComplete) __asm__(COD2_SYM("Scr_ShutdownSystem"));
void cod2x_Scr_ShutdownSystem(int sys, int bComplete)
{
    (void)sys;  // ignored by src/PC/script too
    Scr_ShutdownSystem(bComplete);
}
''',
}

# Imports written out.
SCRIPT_IMPORT_CUSTOM = {
    # CoD2rev-Plus declares Com_Error noreturn, so its callers do not go on
    # after it. The engine's returns for ERR_SCRIPT and ERR_LOCALIZATION once
    # the client's UI shows the error (it has cleared com_errorEntered then):
    # the error becomes a drop, which always unwinds.
    'Com_Error': '''extern "C" void cod2c_Com_Error(int code, const char *fmt, ...) __asm__(COD2_SYM("Com_Error"));
void Com_Error(errorParm_t code, const char *fmt, ...)
{
    char buf[4096];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    cod2c_Com_Error(code, "%s", buf);
    cod2c_Com_Error(ERR_DROP, "%s", buf);
    __builtin_unreachable();
}
''',
    # Formatted straight into dest, as CoD2rev-Plus's does: through the
    # 4096-byte buffer of the other variadic imports, a statusResponse or an
    # rcon reply longer than that (both up to 8192 there) was cut.
    'Com_sprintf': '''int Com_sprintf(char *dest, size_t size, const char *format, ...)
{
    int result;
    va_list ap;

    va_start(ap, format);
    result = vsnprintf(dest, size, format, ap);
    va_end(ap);
    dest[size - 1] = '\\0';
    return result;
}
''',
}

# Exports that return a struct by value on both sides (engine and VM agree on
# the hidden-pointer convention). Every other export must return a scalar or
# void: a struct returned to a caller that expects a scalar breaks the ABI.
STRUCT_RETURN_OK = {'Scr_GetEntityRef', 'Scr_FindAnimTree'}

SCRIPT_HEADER = '''// Generated by scripts/gen_script_bridge.py from CoD2rev-Plus's src/script; do not edit.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "script_bridge.h"
#include "script_decls.h"
'''

SERVER_HEADER = '''// Generated by scripts/gen_script_bridge.py from CoD2rev-Plus's src/server; do not edit.
#include "../qcommon/qcommon.h"
#include "../script/script_public.h"
#include "../server/server.h"
#include "../game/g_shared.h"
#include "script_bridge.h"
#include "server_decls.h"
'''

# The server's own tables. Engine functions with another name in this repo,
# functions written by hand (none left: CoD2rev-Plus's code defines them all),
# and exports written out.
SERVER_OVERLOADS = {}
SERVER_EXTRA_ARGS = {
    'FS_Shutdown': (['int'], ['1']),                     # closemfp, as src/PC/server_mp passes
    'MSG_WriteBigString': (['msg_t*', 'int', 'int', 'int'], ['0', '0', '0', '0']),  # unused here
    'DiffTrackAngle': (['float', 'float', 'float'], ['0.0f', '0.0f', '0.0f']),  # unused, as src/PC/bgame passes
}
# Imports CoD2rev-Plus calls with trailing arguments the engine does not take
# (cdecl: the caller pops them): how many. CoD2rev-Plus's own definitions do
# not use them either.
SERVER_IGNORED_ARGS = {'MSG_ReadDeltaPlayerstate': 1, 'MSG_WriteDeltaPlayerstate': 1}
# Imports written out.
SERVER_IMPORT_CUSTOM = {
    # The engine's version writes as many bytes as the input decodes to, up
    # to eight per input byte; CoD2rev-Plus's stops at toSizeBytes. Decode
    # into a buffer big enough for any input and copy what fits.
    'MSG_ReadBitsCompress': '''extern "C" int cod2c_MSG_ReadBitsCompress(const byte *from, byte *to, int size) __asm__(COD2_SYM("MSG_ReadBitsCompress"));
int MSG_ReadBitsCompress( const byte *from, int fromSizeBytes, byte *to, int toSizeBytes )
{
    static byte decoded[MAX_MSGLEN * 8];
    int count;

    if ( fromSizeBytes <= 0 || toSizeBytes <= 0 )
        return 0;
    if ( fromSizeBytes > MAX_MSGLEN )
        fromSizeBytes = MAX_MSGLEN;
    count = cod2c_MSG_ReadBitsCompress(from, decoded, fromSizeBytes);
    if ( count > toSizeBytes )
        count = toSizeBytes;
    memcpy(to, decoded, count);
    return count;
}
''',
}
# Hunk_ClearLow: CoD2rev-Plus's body is the engine's Hunk_ClearToMarkLow.
SERVER_RENAMED = {'Hunk_ClearLow': 'Hunk_ClearToMarkLow'}
SERVER_PROVIDED = set()
SERVER_EXPORT_CUSTOM = {
    # Static in CoD2rev-Plus: its SV_AddOperatorCommands registers say and
    # tell itself when com_dedicated is set, which it is from the start in the
    # dedicated server. The engine calls this only when a running listen
    # server turns dedicated.
    'SV_AddDedicatedCommands': '''extern "C" void cod2x_SV_AddDedicatedCommands(void) __asm__(COD2_SYM("SV_AddDedicatedCommands"));
void cod2x_SV_AddDedicatedCommands(void)
{
}
''',
    # The engine reads an int; CoD2rev-Plus returns nothing and leaves the
    # result in *hitNum, which is what src/PC/server_mp returned.
    'SV_SightTrace': '''extern "C" int cod2x_SV_SightTrace(int *hitNum, const float *start, const float *mins, const float *maxs, const float *end, int passEntityNum0, int passEntityNum1, int contentmask) __asm__(COD2_SYM("SV_SightTrace"));
int cod2x_SV_SightTrace(int *hitNum, const float *start, const float *mins, const float *maxs, const float *end, int passEntityNum0, int passEntityNum1, int contentmask)
{
    SV_SightTrace(hitNum, start, mins, maxs, end, passEntityNum0, passEntityNum1, contentmask);
    return *hitNum;
}
''',
    # The engine passes the entity; CoD2rev-Plus's is empty (a dedicated
    # server draws nothing).
    'SV_XModelDebugBoxes': '''extern "C" void cod2x_SV_XModelDebugBoxes(gentity_s *ent) __asm__(COD2_SYM("SV_XModelDebugBoxes"));
void cod2x_SV_XModelDebugBoxes(gentity_s *ent)
{
    (void)ent;
    SV_XModelDebugBoxes();
}
''',
    # src/PC/game_mp returned 0 (the blobs call it through imp_); CoD2rev-Plus's
    # returns nothing.
    'G_AnimScriptSound': '''extern "C" int cod2x_G_AnimScriptSound(int client, snd_alias_list_t *aliasList) __asm__(COD2_SYM("G_AnimScriptSound"));
int cod2x_G_AnimScriptSound(int client, snd_alias_list_t *aliasList)
{
    G_AnimScriptSound(client, aliasList);
    return 0;
}
''',
    # src/PC/game_mp's returned nothing (the engine takes its address in
    # stubs/link_stubs.c and imp_G_RegisterWeapon);
    # CoD2rev-Plus's returns the weapon index.
    'G_RegisterWeapon': '''extern "C" void cod2x_G_RegisterWeapon(int weapIndex) __asm__(COD2_SYM("G_RegisterWeapon"));
void cod2x_G_RegisterWeapon(int weapIndex)
{
    G_RegisterWeapon(weapIndex);
}
''',
    # Only cgame calls it (linked, never run, in the dedicated server) and
    # CoD2rev-Plus does not have it: src/PC/bgame/bg_weapons.c's body.
    'BG_IsPlayerWeaponAnAlt': '''extern "C" int cod2x_BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex) __asm__(COD2_SYM("BG_IsPlayerWeaponAnAlt"));
int cod2x_BG_IsPlayerWeaponAnAlt(int iWeaponIndex, int iAltIndex)
{
    int altIndex = BG_GetWeaponDef(iWeaponIndex)->altWeaponIndex;

    while (altIndex) {
        if (altIndex == iAltIndex)
            return 1;
        if (altIndex == iWeaponIndex)
            break;
        altIndex = BG_GetWeaponDef(altIndex)->altWeaponIndex;
    }

    return 0;
}
''',
}
# The engine's core functions CoD2rev-Plus lacks: src/PC's bodies on
# CoD2rev-Plus's file system and hunk. unzip.c opens the iwds through
# FS_FileOpen; com_sndalias.c is linked too.
SERVER_EXPORT_CUSTOM.update({
    'FS_FileOpen': '''extern "C" FILE *cod2x_FS_FileOpen(const char *filename, const char *mode) __asm__(COD2_SYM("FS_FileOpen"));
FILE *cod2x_FS_FileOpen(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}
''',
    'FS_Remove': '''extern "C" void cod2x_FS_Remove(const char *osPath) __asm__(COD2_SYM("FS_Remove"));
void cod2x_FS_Remove(const char *osPath)
{
    remove(osPath);
}
''',
    # The engine's checked the pure list first; CoD2rev-Plus's read does.
    'FS_TouchFile': '''extern "C" int cod2x_FS_TouchFile(const char *name) __asm__(COD2_SYM("FS_TouchFile"));
int cod2x_FS_TouchFile(const char *name)
{
    fileHandle_t f;

    FS_FOpenFileRead(name, &f, qfalse);
    if (f) {
        FS_FCloseFile(f);
        return 1;
    }
    return 0;
}
''',
    # The engine's opened fs_homepath/fs_gamedir/file, as FS_HomeFileLength does.
    'FS_FileExists': '''extern "C" int cod2x_FS_FileExists(const char *file) __asm__(COD2_SYM("FS_FileExists"));
int cod2x_FS_FileExists(const char *file)
{
    return FS_HomeFileLength(file) >= 0;
}
''',
    'Hunk_ClearTempMemory': '''extern "C" void cod2x_Hunk_ClearTempMemory(void) __asm__(COD2_SYM("Hunk_ClearTempMemory"));
void cod2x_Hunk_ClearTempMemory(void)
{
    Hunk_ClearTempMemoryInternal();
}
''',
    # stdio wrappers in the engine (com_fileaccess.c), with fread's and
    # fwrite's arguments; CoD2rev-Plus's take a length in bytes.
    'FS_FileRead': '''extern "C" size_t cod2x_FS_FileRead(void *ptr, size_t size, size_t nitems, FILE *stream) __asm__(COD2_SYM("FS_FileRead"));
size_t cod2x_FS_FileRead(void *ptr, size_t size, size_t nitems, FILE *stream)
{
    return fread(ptr, size, nitems, stream);
}
''',
    'FS_FileWrite': '''extern "C" size_t cod2x_FS_FileWrite(const void *ptr, size_t size, size_t nitems, FILE *stream) __asm__(COD2_SYM("FS_FileWrite"));
size_t cod2x_FS_FileWrite(const void *ptr, size_t size, size_t nitems, FILE *stream)
{
    return fwrite(ptr, size, nitems, stream);
}
''',
    'FS_FileClose': '''extern "C" int cod2x_FS_FileClose(FILE *stream) __asm__(COD2_SYM("FS_FileClose"));
int cod2x_FS_FileClose(FILE *stream)
{
    return fclose(stream);
}
''',
    # The engine passes an allocation tracking type CoD2rev-Plus does not take.
    'FS_ListFiles': '''extern "C" char **cod2x_FS_ListFiles(const char *path, const char *extension, FsListBehavior behavior, int *numfiles, int allocTrackType) __asm__(COD2_SYM("FS_ListFiles"));
char **cod2x_FS_ListFiles(const char *path, const char *extension, FsListBehavior behavior, int *numfiles, int allocTrackType)
{
    (void)allocTrackType;
    return FS_ListFiles(path, extension, behavior, numfiles);
}
''',
    'FS_FreeFileList': '''extern "C" void cod2x_FS_FreeFileList(char **list, int allocTrackType) __asm__(COD2_SYM("FS_FreeFileList"));
void cod2x_FS_FreeFileList(char **list, int allocTrackType)
{
    (void)allocTrackType;
    FS_FreeFileList(list);
}
''',
})
with open(os.path.join(ROOT, 'test', 'server_fatal.txt')) as f:
    SERVER_EXPORT_FATAL = {l.strip() for l in f if l.strip() and not l.startswith('#')}
# Exports CoD2rev-Plus defines under another name.
SERVER_EXPORT_RENAMED = {'SV_GetClientPositionAtTime': 'SV_GetClientPositionsAtTime'}
SERVER_STRUCT_RETURN_OK = set()

# objects: the module's objects (globs in CMakeFiles), whose imports the bridge
# defines; others: objects whose definitions the module may use directly. The
# module's own generated bridge is in neither, so regenerating it keeps every
# import. groups: groups of CoD2rev-Plus objects that some executables link
# and others do not; the imports a group defines go to
# src/compat/<module>_<group>_imports.cpp, which the executables without that
# group link (it calls src/PC's C functions instead). The client has the game
# group (CoD2rev-Plus's server, game and bgame) but not the core.
MODULES = {
    'script': dict(objects=['cod2_plus_script.dir/src/script/*.o'], others=[], header=SCRIPT_HEADER,
                   overloads=OVERLOADS, extra_args=EXTRA_ARGS, renamed=RENAMED, provided=PROVIDED,
                   export_custom=EXPORT_CUSTOM, struct_return_ok=STRUCT_RETURN_OK, import_custom=SCRIPT_IMPORT_CUSTOM,
                   groups={'game': ['cod2_plus_server.dir/src/' + d + '/*.o' for d in ('server', 'game', 'bgame')],
                           'core': ['cod2_plus_server.dir/src/' + d + '/*.o'
                                    for d in ('universal', 'qcommon', 'stringed', 'xanim', 'unix', 'win32')]}),
    'server': dict(objects=['cod2_plus_server.dir/src/server/*.o', 'cod2_plus_server.dir/src/game/*.o',
                            'cod2_plus_server.dir/src/bgame/*.o', 'cod2_plus_server.dir/src/universal/*.o',
                            'cod2_plus_server.dir/src/qcommon/*.o', 'cod2_plus_server.dir/src/stringed/*.o',
                            'cod2_plus_server.dir/src/xanim/*.o', 'cod2_plus_server.dir/src/unix/*.o',
                            'cod2_plus_server.dir/src/win32/*.o'],
                   others=['cod2_plus_script.dir/src/script/*.o', 'cod2_plus_script.dir/src/compat/*.o'],
                   header=SERVER_HEADER, overloads=SERVER_OVERLOADS, extra_args=SERVER_EXTRA_ARGS,
                   renamed=SERVER_RENAMED, provided=SERVER_PROVIDED, export_custom=SERVER_EXPORT_CUSTOM,
                   export_renamed=SERVER_EXPORT_RENAMED, struct_return_ok=SERVER_STRUCT_RETURN_OK,
                   ignored_args=SERVER_IGNORED_ARGS, import_custom=SERVER_IMPORT_CUSTOM,
                   export_fatal=SERVER_EXPORT_FATAL),
}

# The client runs CoD2rev-Plus's server, game and bgame on the engine's core
# and xanim: their imports come back, and the functions that core lacks are
# written by hand in src/compat/client_server_missing.cpp. The VM's core thunks
# (cod2_plus_script_core) already define the engine functions both call.
CLIENT_SERVER_PROVIDED = {'BigShort', 'Cmd_ReplaceControlChars', 'Com_BitCheck', 'Com_BitClear', 'Com_BitSet',
                          'DObjSetControlRotTransIndex', 'DObjSetRotTransIndex', 'FS_HomeFileLength',
                          'FS_RotateHomeFile', 'FS_VerifyIwd', 'I_strcmpConstantTime', 'MSG_ReadString',
                          'MSG_ReadStringLine', 'RoundFloatArray', 'SetConfigString', 'SetConfigString2',
                          'SnapAngles', 'Vec2Length', 'Vec2LengthSq', 'Vec3Lerp', 'VectorInverse', 'VectorLength',
                          'VectorLengthSquared', 'Com_SetWeaponInfoMemory', 'Com_FreeWeaponInfoMemory'}
MODULES['client_server'] = dict(
    MODULES['server'],
    objects=['cod2_plus_server_client.dir/src/' + d + '/*.o' for d in ('server', 'game', 'bgame')]
    + ['cod2_plus_server_client.dir/src/compat/client_server_missing.cpp.o'],
    others=['cod2_plus_script.dir/src/script/*.o', 'cod2_plus_script.dir/src/compat/*.o',
            'cod2_plus_script_core.dir/src/compat/*.o'],
    provided=CLIENT_SERVER_PROVIDED, export_fatal=set())

# C integer types of each size, for a return the two sides give different sizes.
WIDTH_TYPE = {1: 'unsigned char', 2: 'unsigned short', 4: 'unsigned int', 8: 'unsigned long long'}


def split_params(text):
    """The top-level comma-separated parameter types of a demangled signature."""
    out, depth, cur = [], 0, ''
    for ch in text:
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def named(ty, i):
    """'type aN', with the name inside a function pointer declarator."""
    if ty == '...':
        return '...'
    if '(*)' in ty:
        return ty.replace('(*)', f'(*a{i})', 1)
    return f'{ty} a{i}'


def fixed(params):
    return [p for p in params if p != '...']


def ret_type(name, params):
    return f'decltype({name}(' + ', '.join(f'std::declval<{p}>()' for p in fixed(params)) + '))'


def abi_asserts(name, params, abi, ret, side):
    """static_asserts that CoD2rev-Plus's declaration (params, ret) matches the
    engine's ABI line for name; side says whose definition the line comes from."""
    if abi is None:
        return ''
    cls, size, slots, variadic = abi
    plus = fixed(params)
    # A caller that expects nothing may ignore an integer in eax, but not a
    # float, which would stay on the x87 stack.
    void_ok = f' || cod2_abi_class<{ret}>() == \'v\'' if side == 'the engine' and cls == 'i' else ''
    out = [f'static_assert(cod2_abi_class<{ret}>() == \'{cls}\'{void_ok}, "{name}: return class differs from {side}");']
    if cls in 'fs':
        out.append(f'static_assert(cod2_abi_size<{ret}>() == {size}, "{name}: return size differs from {side}");')
    if len(plus) != len(slots) or ('...' in params) != variadic:
        out.append(f'static_assert(false && sizeof({ret}*), "{name}: {len(plus)} parameters, {side} has {len(slots)}'
                   f'{" and is variadic" if variadic else ""}");')
    else:
        out += [f'static_assert(cod2_abi_slot<{p}>() == {s}, "{name}: parameter {i} differs from {side}");'
                for i, (p, s) in enumerate(zip(plus, slots))]
    return '\n'.join(out) + '\n'


def widened(abi, ret):
    """The C type to declare for an integer return the engine gives another size, or None."""
    if abi is None or abi[0] != 'i':
        return None
    return f'cod2_abi_int<{ret}, {abi[1]}>'


def export_thunk(name, params, cfg=None, abi=None, target=None):
    """The C symbol name calling CoD2rev-Plus's target (name unless renamed)."""
    cfg = cfg or MODULES['script']
    target = target or name
    sig = ', '.join(named(p, i) for i, p in enumerate(params))
    args = ', '.join(f'a{i}' for i in range(len(fixed(params))))
    r = ret_type(target, params)
    guard = '' if name in cfg['struct_return_ok'] else \
        f'static_assert(cod2_bridge_returns_scalar<{r}>::value, "{name} returns a struct");\n'
    guard += abi_asserts(name, params, abi, r, 'src/PC/server_mp')
    w = widened(abi, r)
    if '...' in params:
        # the engine's arguments are formatted here and passed on as "%s", as
        # import_thunk does the other way
        n = len(fixed(params))
        fmt = f'a{n - 1}'
        before = ''.join(f'a{i}, ' for i in range(n - 1))
        return (guard + f'extern "C" {r} cod2x_{name}({sig}) __asm__(COD2_SYM("{name}"));\n'
                f'{r} cod2x_{name}({sig})\n{{\n'
                f'    char buf[4096];\n    va_list ap;\n    va_start(ap, {fmt});\n'
                f'    vsnprintf(buf, sizeof(buf), {fmt}, ap);\n    va_end(ap);\n'
                f'    return {target}({before}"%s", buf);\n}}\n')
    if w:
        return (guard + f'extern "C" {w} cod2x_{name}({sig}) __asm__(COD2_SYM("{name}"));\n'
                f'{w} cod2x_{name}({sig}) {{ return ({w}){target}({args}); }}\n')
    return (guard + f'extern "C" {r} cod2x_{name}({sig}) __asm__(COD2_SYM("{name}"));\n'
            f'{r} cod2x_{name}({sig}) {{ return {target}({args}); }}\n')


def export_names(interface, fatal):
    """The names to export: the interface's, then the fatal ones it lacks."""
    return list(interface) + sorted(set(fatal) - set(interface))


def fatal_export(name):
    """An export for an engine function the module lacks and whose callers do
    not run in the dedicated server: it stops the server with its name. cdecl
    leaves the arguments to the caller, so it takes none."""
    return (f'extern "C" void cod2x_{name}(void) __asm__(COD2_SYM("{name}"));\n'
            f'void cod2x_{name}(void)\n{{\n'
            f'    Com_Error( ERR_FATAL, "%s is not in the dedicated server", "{name}" );\n}}\n')


def import_thunk(name, params, cfg=None, abi=None):
    cfg = cfg or MODULES['script']
    if name in cfg.get('import_custom', {}):
        return cfg['import_custom'][name]
    sig = ', '.join(named(p, i) for i, p in enumerate(params))
    n = len(fixed(params))
    r = ret_type(name, params)
    extra_types, extra_values = cfg['extra_args'].get(name, ([], []))
    passed = params[:len(params) - cfg.get('ignored_args', {}).get(name, 0)]
    csig = ', '.join([named(p, i) for i, p in enumerate(passed)] +
                     [named(t, n + i) for i, t in enumerate(extra_types)])
    guard = abi_asserts(name, passed + extra_types if '...' not in params else params, abi, r, 'the engine')
    w = widened(abi, r)
    cr = w or r
    decl = guard + f'extern "C" {cr} cod2c_{name}({csig}) __asm__(COD2_SYM("{cfg["renamed"].get(name, name)}"));\n'
    cast = f'({r})' if w else ''
    if '...' not in params:
        args = ', '.join([f'a{i}' for i in range(len(fixed(passed)))] + extra_values)
        return decl + f'{r} {name}({sig}) {{ return {cast}cod2c_{name}({args}); }}\n'
    fmt = f'a{n - 1}'
    before = ''.join(f'a{i}, ' for i in range(n - 1))
    return decl + (f'{r} {name}({sig})\n{{\n'
                   f'    char buf[4096];\n    va_list ap;\n    va_start(ap, {fmt});\n'
                   f'    vsnprintf(buf, sizeof(buf), {fmt}, ap);\n    va_end(ap);\n'
                   f'    return {cast}cod2c_{name}({before}"%s", buf);\n}}\n')


def choose_overload(name, signatures, cfg=None):
    """The parameter types of the signature the engine means for name."""
    cfg = cfg or MODULES['script']
    candidates = [split_params(s[s.index('(') + 1:s.rindex(')')]) for s in signatures]
    if len(candidates) == 1:
        return candidates[0]
    want = cfg['overloads'].get(name)
    if want is None or want not in candidates:
        sys.exit(f'{name} is overloaded ({signatures}); add it to the module\'s overloads')
    return want


def demangle(symbols):
    out = subprocess.run(['c++filt'], input='\n'.join(symbols), capture_output=True, text=True).stdout
    return out.split('\n')[:len(symbols)]


def nm(args, objects):
    """The symbols nm lists, without the '_' PE/COFF objects (MinGW) put first."""
    if not objects:
        return []
    out = subprocess.run(['nm'] + args + objects, capture_output=True, text=True).stdout
    names = [line.split()[-1] for line in out.splitlines() if line.strip() and not line.endswith(':')]
    if all(o.endswith('.obj') for o in objects):
        names = [n[1:] if n.startswith('_') else n for n in names]
    return names


# The C library's names for FILE, which the demangler spells out.
FILE_TYPES = re.compile(r'\b(_IO_FILE|_iobuf)\b')


def signatures(mangled):
    """{name: [demangled signature, ...]} for the C++ function symbols, with
    FILE as the source spells it on every platform."""
    sigs = {}
    for m, d in zip(mangled, demangle(mangled)):
        d = FILE_TYPES.sub('FILE', d)
        if m.startswith('_Z') and '(' in d and '::' not in d.split('(')[0]:
            if d not in sigs.get(d.split('(')[0], []):
                sigs.setdefault(d.split('(')[0], []).append(d)
    return sigs


def read_module(module, builds):
    """(defined symbols, exports {name: sigs}, imports {name: sigs}, interface names) of a
    module, joined over builds: an unoptimized one (MinGW) keeps calls that -O2
    drops with the unused static functions making them."""
    cfg = MODULES[module]
    builds = [builds] if isinstance(builds, str) else builds
    defined, undefined = set(), set()
    for build in builds:
        files = [os.path.join(build, 'CMakeFiles', g) for g in cfg['objects']]
        objects = sorted(o for g in files for o in glob.glob(g) + glob.glob(g[:-2] + '.obj'))
        if not objects:
            continue  # a build without this module (the MinGW server has no client)
        mine = set(nm(['-g', '--defined-only'], objects))
        others = sorted(o for g in cfg['others'] for p in (g, g[:-2] + '.obj')
                        for o in glob.glob(os.path.join(build, 'CMakeFiles', p)))
        elsewhere = set(nm(['-g', '--defined-only'], others))
        defined |= mine
        undefined |= set(nm(['-u'], objects)) - mine - elsewhere
    if not defined:
        sys.exit(f'no objects of {cfg["objects"][0]} in {", ".join(builds)}: configure a build with the module on and build it')
    undefined = sorted(undefined - defined)
    exports = signatures(sorted(defined))
    imports = {k: v for k, v in signatures([u for u in undefined if u.startswith('_Z')]).items()
               if k not in cfg['provided']}
    with open(os.path.join(ROOT, 'test', f'{module}_interface.txt')) as f:
        interface = [l.strip() for l in f if l.strip() and not l.startswith('#')]
    return defined, exports, imports, interface


def split_imports(imports, groups):
    """(imports every executable needs, {group: imports only the executables
    without that group need}); groups maps a group to the names it defines."""
    parts = {group: {k: v for k, v in imports.items() if k in names} for group, names in groups.items()}
    grouped = set().union(*groups.values()) if groups else set()
    return {k: v for k, v in imports.items() if k not in grouped}, parts


def group_names(module, builds):
    """{group: the function names its objects define}, for the module's groups."""
    out = {}
    for group, globs in MODULES[module].get('groups', {}).items():
        names = set()
        for build in builds:
            objects = sorted(o for g in globs for p in (g, g[:-2] + '.obj')
                             for o in glob.glob(os.path.join(build, 'CMakeFiles', p)))
            names |= set(signatures([s for s in nm(['-g', '--defined-only'], objects) if s.startswith('_Z')]))
        out[group] = names
    return out


def crossing_names(module, build):
    """(import names, export names) of a module, for scripts/engine_abi.py."""
    _, exports, imports, interface = read_module(module, build)
    renamed = MODULES[module].get('export_renamed', {})
    return sorted(imports), [n for n in interface if renamed.get(n, n) in exports]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--module', default='script', choices=list(MODULES))
    ap.add_argument('--build', action='append', help='a build to read; repeat it to join builds '
                    '(default: build/check-linux and, when it exists, build/check-mingw)')
    ap.add_argument('--names', action='store_true', help='print the imports and exports and write nothing')
    args = ap.parse_args()
    cfg = MODULES[args.module]
    if not args.build:
        args.build = [os.path.join(ROOT, 'build', 'check-linux')]
        mingw = os.path.join(ROOT, 'build', 'check-mingw')
        if glob.glob(os.path.join(mingw, 'CMakeFiles', cfg['objects'][0][:-2] + '.obj')):
            args.build.append(mingw)

    defined, exports, imports, interface = read_module(args.module, args.build)
    if args.names:
        print('\n'.join(sorted(set(imports) | set(crossing_names(args.module, args.build)[1]))))
        return 0
    import engine_abi
    abi = engine_abi.load(args.module)

    out = [cfg['header']]
    missing = []
    for name in export_names(interface, cfg.get('export_fatal', ())):
        target = cfg.get('export_renamed', {}).get(name, name)
        if name in cfg['export_custom']:
            out.append(cfg['export_custom'][name])
        elif name in cfg.get('export_fatal', ()):
            out.append(fatal_export(name))
        elif target in exports:
            out.append(export_thunk(name, choose_overload(target, exports[target], cfg), cfg, abi.get(name), target))
        elif name not in defined:
            missing.append(name)
    with open(os.path.join(ROOT, 'src', 'compat', f'{args.module}_exports.cpp'), 'w') as f:
        f.write('\n'.join(out))

    shared, parts = split_imports(imports, group_names(args.module, args.build))
    files = [(f'{args.module}_imports.cpp', shared)] + [(f'{args.module}_{group}_imports.cpp', part)
                                                         for group, part in sorted(parts.items())]
    for path, part in files:
        out = [cfg['header']]
        for name in sorted(part):
            out.append(import_thunk(name, choose_overload(name, part[name], cfg), cfg, abi.get(name)))
        with open(os.path.join(ROOT, 'src', 'compat', path), 'w') as f:
            f.write('\n'.join(out))

    print(f'{len(interface) - len(missing)} exports, {len(imports)} imports')
    if missing:
        print(f'in test/{args.module}_interface.txt but not defined by the module: ' + ' '.join(missing))
    absent = sorted(n for n in set(imports) | set(interface) if abi and n in abi and abi[n] is None)
    if absent:
        print(f'not in the engine binary (test/{args.module}_abi.txt): ' + ' '.join(absent))
    return 0


if __name__ == '__main__':
    sys.exit(main())

# Changes in opencod2-plus

Everything this fork adds to [opencod2/opencod2](https://github.com/opencod2/opencod2), in detail: the functions, files and fixes behind each item that the [README](../README.md#changes-in-this-fork) summarizes.

## Building

- **The Linux and MinGW builds work again.** The upstream changes that removed the `imp_` indirection and the GCC assembler renames were only built with MSVC, which left 119 errors on Linux and 5 on MinGW:
  - `__cdecl` and `__stdcall` are defined for GCC outside Windows (`cod2_platform.h`).
  - The Windows-only diagnostics (`_ReturnAddress`, `GetModuleHandleA`, `VirtualProtect`, `timeGetTime`, `_set_invalid_parameter_handler`) are behind `_MSC_VER`/`_WIN32`.
  - `__forceinline` on `Com_FindSoundAlias` is MSVC-only, since with GCC it left no out-of-line copy.
  - The forward declarations of `SL_ConvertToString`, `R_FinishLoadingAabbTrees_r` and `CM_SightTraceThroughBrush` match their definitions.
  - The crash handler defines `_GNU_SOURCE` for the `REG_*` indexes, and `win_net.c` uses `FD_SET`.
  - The 13 dvar slots `bg_misc.c` now defines are dropped from `src/blobs/bss.c`.
  - MinGW keeps only the 48 `--defsym` aliases whose target still exists; the other 1,925 pointed at names gone since the assembler renames were removed. The headless server gets stubs for the two display functions that live in the client-only `macos_compat.c`.
- **`long long` keeps the Windows alignment on Linux.** `COD2_WIN_ALIGN8` gives `animation_s.movetype` 8-byte alignment, so `bgs_t` matches the original binary again (`anim_user` at 740328) and its `COD2_ASSERT_FIELD` pins hold.
- **`bool` is one byte in C too**, as in C++ and in the original binary. `COD2_ASSERT_SIZE` pins `VertexProgramStreamState` (24) and `CTexUnit` (284) in both languages.
- **`scripts/build_check.py`** configures and builds `linux` (1.3), `linux-10` (1.0) and `mingw` into `build/check-<variant>/` and reports the errors. On Linux it also fails when a binary needs a glibc newer than 2.34, or when `cod2_lnxded` needs one of the client's libraries.
- **The Linux dedicated server runs without the client's libraries.** It links the same objects as the client, whose renderer, Mac GL layer, SDL2 input, crash popup and HTTP download reference GL, X11, SDL2 and curl, but it never calls them: `src/unix/linux_ded_stubs.c` stands in for them in `cod2_lnxded`, as `src/win32/win32_stubs.c` does for the MinGW server, each function returning 0. `cod2_lnxded` needs only libc, libm, libz and libstdc++.

## C++

- **The engine compiles as C++ (gnu++17) on Linux and MinGW** (`COD2_CXX_ALL`, on by default). Each C source is built through a generated wrapper, `build/<dir>/cxx_wrap/<path>.c.cpp`, that holds `extern "C" { #include "<path>.c" }`, so everything it defines or declares, block-scope `extern`s included, keeps its C name for the data blobs, the `.S` files and the `--defsym` aliases. The data blobs and the third-party libraries (speex, jpeg-6, zlib) stay C. `COD2_CXX_ALL=OFF` builds C, and `COD2_CXX_DIRS` then picks single modules.
- **The shared headers are valid C++:**
  - 128 enums declared ahead of their definitions get an opaque C++ declaration (`COD2_ENUM_DECL`) and a matching `int` base (`COD2_ENUM_INT`); `surfaceType_t` gets `unsigned int`.
  - Every project header is wrapped in `extern "C"`.
  - `test/cxx/header_check.cpp` compiles them as C++, so every `COD2_ASSERT_FIELD`/`SIZE` is checked by the C++ front end too.
- **What C allowed and C++ does not, without changing behaviour:**
  - casts for constants out of range of their `int` fields and for implicit `void *` conversions;
  - declarations split from their initializers where a `goto` jumps over them;
  - C library redeclarations replaced by their headers (133, all with the real signature);
  - `const` tables that were never given values initialized to zero, as C left them.
  - K&R imports called with arguments are declared `f(COD2_KR)`, which is `...` in C++, so the calls pass the same arguments and promotions as in C.
  - Functions that can end without a `return` (undefined in C++, where GCC drops the code after the call) return 0; in CoD2rev-Plus they are `void`.
- **Checks:**
  - `scripts/cxx_errors.py` counts the C++ errors of each module; `--return-type` finds non-void functions that can end without a return.
  - `scripts/check_c_linkage.py` lists the C++-mangled names the objects define beyond `test/c_linkage_allowlist.txt`: the 337 names the C build already defines on purpose (the original binary's C++ symbols) and the inline functions of the C and C++ library headers.
- **The C++ Linux build stays on glibc 2.34.** g++ always defines `_GNU_SOURCE`, which on glibc 2.38+ sends `strtol`, `strtoul` and `sscanf` to their C23 versions. `src/headers/glibc_compat.h`, force-included in the Linux C++ build, turns that off.

## Script VM

- **The GSC virtual machine is CoD2rev-Plus's.** `src/script` and the 24 headers it includes come byte for byte from CoD2rev-Plus `ec884b8`, at the same paths, so its later commits apply with `git am` (`COD2_PLUS_SCRIPT`, on by default on Linux and MinGW; MSVC, the web build and x64 keep `src/PC/script`). With it the server holds two players, disconnects, reconnects and `clientkick`; the reconstructed VM crashed as soon as a second player joined.
- **The bridge (`src/compat`)** connects the engine's C names with the VM's C++ ones:
  - `scripts/gen_script_bridge.py` writes `script_exports.cpp`, 95 thunks whose `__asm__` symbol is the C name the engine calls, one per function in `test/script_interface.txt`.
  - It also writes `script_imports.cpp`: 44 C++ definitions of the engine functions the VM needs, which call the engine's C symbols. Variadic ones format into a 4096-byte buffer, the size of CoD2rev-Plus's own `MAXPRINTMSG`; `Com_sprintf` formats straight into its destination, as CoD2rev-Plus's does, since a `statusResponse` can take 8192 bytes.
  - `script_missing.cpp` checks that `PROTOCOL_VERSION`, which CMake derives from `COD2_FEATURE_CFLAGS`, and `COD2_PATCH_10`, which the engine reads wherever it is set, name the same game version.
  - Return types are `decltype` of a call to CoD2rev-Plus's own declarations, so the compiler checks every thunk. Exports must return a scalar or `void`, except the two that return a struct on both sides.
  - Thunks written by hand where the two sides differ:
    - `Scr_FreeScripts(sys)` and `Scr_ShutdownSystem(sys, bComplete)` take the client binary's script system id;
    - `Scr_GetAnim` returns `scr_anim_s` in `eax`;
    - `FS_ListFiles` and `FS_FreeFileList` take an `allocTrackType` here (10).
  - `script_missing.cpp` adds what this engine lacks, as CoD2rev-Plus writes it: `Hunk_AllocXAnimTreePrecache`, `Q_rint`, `Q_vsnprintf`, `VectorCompare`, `Sys_UnimplementedFunctionInternal`, `VM_Execute` under the original binary's name, and the `"int"` string of the MinGW blob's builtin table.
- **The VM globals** (`scrVarPub`, `scrVmPub`, the parser and yacc state, `var_typename`, …) are behind `#if !COD2_PLUS_SCRIPT` in `src/blobs/bss.c`, `build/native_gen/data32.c` and `build/win_gen/data.win.S`. MinGW links with `--allow-multiple-definition`, so its blob copies would otherwise have won silently.
- **Engine side:** `GScr_PostLoadScripts` (the class maps and entity fields) runs after `Scr_PostCompileScripts`, as in CoD2rev-Plus: earlier, `Scr_AddFields` wrote over the compiled program. `Hunk_ClearHigh` is added to `com_memory.c`.
- **The game code compares script object types with the VM's values.** CoD2rev-Plus's VM adds `VAR_INT64` after `VAR_INTEGER`, so its `VAR_ENTITY` is 22, not 0x15, and every object type after it is one more. `iprintln`, `isAlive`, the sight traces and the other builtins that take an entity did not see one, and `iprintln(&"MP_CONNECTED", self)` in `tdm.gsc` raised a script error for each player who connected (with `developer 1` it stopped the map). `src/headers/scr_vartypes.h` gives the types of the VM the build uses, and `src/compat/script_missing.cpp` checks them against CoD2rev-Plus's enum.
- **The copied files follow CoD2rev-Plus `ad7f726`.** The eight CoD2rev-Plus commits since `ec884b8` that touch them applied with `git am --keep-cr`, limited to their paths (some of CoD2rev-Plus's files have CRLF line endings, which `git am` strips without `--keep-cr`). `test/vendored_files.txt` names the revision.
- **Script VM fix in the reconstructed VM** (`src/PC/script`, still used by MSVC): the per-self wait lists were kept in `levelId` and the time buckets in `pauseArrayId`, one field off from the rest of the VM, so kicking or dropping a player with a `waittill` running crashed the server.

## Server

- **The dedicated server is CoD2rev-Plus's.** `src/server` comes byte for byte from CoD2rev-Plus `ad7f726` (`COD2_PLUS_SERVER`, on by default with `COD2_PLUS_SCRIPT`). `cod2_lnxded` and `cod2_win32_ded.exe` link it instead of `src/PC/server_mp`, which the client keeps for its local server: on Linux `server_mp` is its own object library, and `bss.c` and `data32.c` are compiled for each executable. In the dedicated server they leave to `src/server` the 48 globals it defines (`sv`, `svs`, the `sv_*` dvars, `rcon_password`, `nextmap`, `gameInitialized`, `actorLocationalMins`, `sv_serverId_value`…); `data.win.S` does the same and leaves out `server_mp`'s client command table.
- **The bridge (`src/compat/server_*.cpp`)** is generated like the script VM's (`scripts/gen_script_bridge.py --module server`): 66 exports, the `server_mp` functions the rest of the engine calls (`test/server_interface.txt`), and 205 imports. It joins the objects of the Linux and MinGW builds, since the unoptimized MinGW objects keep calls that `-O2` drops with unused static functions.
- **Every crossing is checked against the engine's ABI.** `scripts/engine_abi.py` reads with gdb the debug information of a binary built with `server_mp` and writes `test/server_abi.txt`: the class and size of each function's return and the stack slot of each parameter. Each thunk asserts its CoD2rev-Plus declaration against it. A void import of an integer is accepted, a void import of a float is not (it would stay on the x87 stack), and an integer return the two sides give different sizes travels as the engine's. No crossing function is `regparm`.
- **Thunks written out where the two sides differ:**
  - `SV_SightTrace` returns `*hitNum`, which the engine reads and CoD2rev-Plus leaves in its argument;
  - `SV_XModelDebugBoxes` takes the engine's entity and ignores it;
  - `SV_GetClientPositionAtTime` is `SV_GetClientPositionsAtTime` in CoD2rev-Plus;
  - `SV_AddDedicatedCommands` is empty: CoD2rev-Plus's `SV_AddOperatorCommands` registers `say` and `tell` itself when the server is dedicated;
  - `MSG_ReadBitsCompress` decodes with the engine's function, which has no bound on its output, into a buffer for any input, and copies at most `toSizeBytes`, keeping CoD2rev-Plus's bound on compressed client messages;
  - `FS_Shutdown` gets `closemfp` 1, as `server_mp` passed, and `MSG_WriteBigString` the engine's four unused arguments; `MSG_ReadDeltaPlayerstate` and `MSG_WriteDeltaPlayerstate` drop the one CoD2rev-Plus does not use.
- **`src/compat/server_missing.cpp`** has the CoD2rev-Plus functions this engine lacks, from its sources: `BigShort`, `I_strcmpConstantTime`, `SnapAngles`, `Cmd_ReplaceControlChars`, `DObjSetRotTransIndex`, `DObjSetControlRotTransIndex`, `FS_VerifyIwd`, `G_GetEntityToken`, and `MSG_ReadString` and `MSG_ReadStringLine`, which take a buffer and clean the characters they read where the engine's return a static buffer.
- **Structures.** In 1.3 the server's structures match the engine's. `dvar_s` is 72 bytes there (vectors in line) and 36 here, but the server reads only `current` and `modified`, at the same offsets. In `netchan_t`, `dropped` and `incomingSequence` are swapped; the server reads neither. In 1.0 CoD2rev-Plus keeps the 1.3 forms of `client_t`, `serverStatic_t` and `challenge_t`, which nothing outside the server touches.

## Fixes

- **Replies went to the wrong port.** `Sys_GetEvent` queued only the first 12 bytes of a packet's `netadr_t` and `Com_EventLoop` read the data from byte 12, so the port came from the packet itself: every reply to `getinfo` went to port 29801 ("ti" of "getinfo"). Both use `sizeof(netadr_t)`.
- **Game files on Windows drives under WSL.** Linux builds with `_FILE_OFFSET_BITS=64`: the 32-bit `readdir` failed with `EOVERFLOW` on file systems with large inode numbers, so no `.iwd` was found.
- **Protocol of the 1.0 build.** The server announced, registered and required protocol 118, and the client sent it, also in the 1.0 build; `COD2_PROTOCOL_VERSION` is 115 for 1.0 and 118 for 1.3.
- **The fourth or fifth map load crashed the server.** `BG_AnimParseAnimScript` appended the player animation script to `level_bgs.animScriptData` and nothing cleared it between maps: each `map` (not `map_restart`) added about 700 items, and the third one went over 2048 and raised `Com_Error`. `G_InitGame` now clears it, as CoD2rev-Plus does. The client was not affected: `CG_Init` clears all of `cg`.
- **A drop error raised by an rcon command corrupted the stack.** `SVC_RemoteCommand` redirects the output into a 16 KB buffer on its own stack, and `Com_Error` longjmps to `Com_Frame` without ending the redirect: `Com_ErrorCleanup` and the `Com_Printf` of the error message ran over that buffer, and appending the message overwrote their return addresses (`eip=0`). `Com_Error` now flushes the redirect while the buffer is valid, so the rcon client also gets the output up to the error. CoD2rev-Plus does not end it either.
- **The 1.0 server wrote over its configstrings.** `bss.c` reserved 41216 bytes for 1.0's `svs`, whose `serverStatic_t` is 0xc108: the end of `challenges`, `redirectAddress`, `authorizeAddress` and `tempBans` landed on `sv`'s first configstrings, and an rcon command made the next `SV_Shutdown` or `map_restart` free `0x4`. `sv` and `svs` have their types in every build.
- **Fragment offsets.** 1.0 sends them as shorts in both directions; 1.3 as longs in both, as `CoD2MP_s.exe` 1.3 writes the DWORD whatever the socket (`Netchan_TransmitNextFragment` at `0x447e61`). The reconstruction used longs only from the server socket, so no 1.0 client could put the gamestate together, and a fragmented 1.3 client message, such as a long userinfo, did not reach the server.
- **The port of `netadr_t` follows the IP.** The header kept it at 0x12, after the IPX address, as Quake III does; `NET_AdrToString` in `CoD2MP_s.exe` 1.3 (`0x44770a`) reads it at +8, as CoD2rev-Plus does. The engine reaches it by name, but a `netadr_t` crossing to CoD2rev-Plus's server had its port read and written over the IPX bytes.
- **The home folder of the game is searched too.** `FS_Startup` added `fs_homepath` only for `fs_basegame` and `fs_game`, so `.iwd` files in the server's own `main` folder were not in the search path; CoD2rev-Plus, from the Linux binary, and Quake III add it when it differs from `fs_basepath`. CoD2rev-Plus's server checks a download against the search path (`FS_VerifyIwd`, CVE-2006-2082).
- **The crash report lists the top of the stack** on 32-bit Linux, the only way back to the caller when `eip` is bad.
- **MinGW client:** it includes `<GL/gl.h>`, since calling the `__stdcall` GL functions through implicit `cdecl` declarations left the stack unbalanced. The `ioctl` stub returns -1 instead of whatever was in `eax`.

## Tests

- **Smoke test:** `scripts/smoke_test.py`, `cod2client.py` and `cod2snap.py` are copied unchanged from CoD2rev-Plus. `scripts/opencod2_smoke.py` runs the smoke test against this server and compares its failures with `test/smoke_expected_failures_<protocol>.txt`. It fails on a new failure and on a listed one that now passes, so the lists only shrink. `test/smoke_flaky.txt` holds the checks whose result depends on timing with this server.
- **Two players:** `scripts/two_players_test.py`, with `developer 1` so that a script error stops the map and fails the test.
- **Map loads:** `scripts/map_reload_test.py` loads maps by rcon, alternating `map_restart` and `map` between `mp_toujane` and `mp_carentan`. **Drop errors:** `scripts/rcon_drop_test.py` raises one with `rcon error drop` (`developer 1`).
- **Animation builtins:** `scripts/anim_builtins_test.py` runs `getAnimLength`, `animHasNotetrack`, `getMoveDelta` and `getAngleDelta` from a test mod.
- **Layout parity:** `scripts/layout_parity.py` compiles a program with this repo's headers and one with CoD2rev-Plus's and compares sizes and offsets of the types in `test/layout_parity_types.txt`: those the script VM and the server share with the engine, in 1.3 and 1.0.
- **Copied files:** `scripts/vendor_check.py` compares every file in `test/vendored_files.txt` with a CoD2rev-Plus checkout.
- **Unit tests** for the scripts: `python3 -m unittest discover -s test`.

## Known problems

- What CoD2rev-Plus has outside `src/server` is not here yet: console log rotation, timestamps and `logfileName` (and so the `sv_logRcon` check, which reads that log), `addip`, `net_strictPort`, the master server heartbeat, `games_mp.log` rotation and its crash report live in its `qcommon` and `game`.
- The client has not been checked connecting to a server: it needs a CD key, which the tests do not have.
- The MinGW dedicated server builds with CoD2rev-Plus's server but has not been run (no Wine here).
- The MSVC build is not checked by the scripts above and still compiles C with the reconstructed VM and server.

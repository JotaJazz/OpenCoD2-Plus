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
- **`scripts/build_check.py`** configures and builds `linux` (1.3), `linux-10` (1.0) and `mingw` into `build/check-<variant>/` and reports the errors. On Linux it also fails when a binary needs a glibc newer than 2.34.

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
  - It also writes `script_imports.cpp`: 44 C++ definitions of the engine functions the VM needs, which call the engine's C symbols. Variadic ones format into a 4096-byte buffer.
  - Return types are `decltype` of a call to CoD2rev-Plus's own declarations, so the compiler checks every thunk. Exports must return a scalar or `void`, except the two that return a struct on both sides.
  - Thunks written by hand where the two sides differ:
    - `Scr_FreeScripts(sys)` and `Scr_ShutdownSystem(sys, bComplete)` take the client binary's script system id;
    - `Scr_GetAnim` returns `scr_anim_s` in `eax`;
    - `FS_ListFiles` and `FS_FreeFileList` take an `allocTrackType` here (10).
  - `script_missing.cpp` adds what this engine lacks, as CoD2rev-Plus writes it: `Hunk_AllocXAnimTreePrecache`, `Q_rint`, `Q_vsnprintf`, `VectorCompare`, `Sys_UnimplementedFunctionInternal`, `VM_Execute` under the original binary's name, and the `"int"` string of the MinGW blob's builtin table.
- **The VM globals** (`scrVarPub`, `scrVmPub`, the parser and yacc state, `var_typename`, …) are behind `#if !COD2_PLUS_SCRIPT` in `src/blobs/bss.c`, `build/native_gen/data32.c` and `build/win_gen/data.win.S`. MinGW links with `--allow-multiple-definition`, so its blob copies would otherwise have won silently.
- **Engine side:** `GScr_PostLoadScripts` (the class maps and entity fields) runs after `Scr_PostCompileScripts`, as in CoD2rev-Plus: earlier, `Scr_AddFields` wrote over the compiled program. `Hunk_ClearHigh` is added to `com_memory.c`.
- **Script VM fix in the reconstructed VM** (`src/PC/script`, still used by MSVC): the per-self wait lists were kept in `levelId` and the time buckets in `pauseArrayId`, one field off from the rest of the VM, so kicking or dropping a player with a `waittill` running crashed the server.

## Fixes

- **Replies went to the wrong port.** `Sys_GetEvent` queued only the first 12 bytes of a packet's `netadr_t` and `Com_EventLoop` read the data from byte 12, so the port came from the packet itself: every reply to `getinfo` went to port 29801 ("ti" of "getinfo"). Both use `sizeof(netadr_t)`.
- **Game files on Windows drives under WSL.** Linux builds with `_FILE_OFFSET_BITS=64`: the 32-bit `readdir` failed with `EOVERFLOW` on file systems with large inode numbers, so no `.iwd` was found.
- **Protocol of the 1.0 build.** The server announced, registered and required protocol 118, and the client sent it, also in the 1.0 build; `COD2_PROTOCOL_VERSION` is 115 for 1.0 and 118 for 1.3.
- **MinGW client:** it includes `<GL/gl.h>`, since calling the `__stdcall` GL functions through implicit `cdecl` declarations left the stack unbalanced. The `ioctl` stub returns -1 instead of whatever was in `eax`.

## Tests

- **Smoke test:** `scripts/smoke_test.py`, `cod2client.py` and `cod2snap.py` are copied unchanged from CoD2rev-Plus. `scripts/opencod2_smoke.py` runs the smoke test against this server and compares its failures with `test/smoke_expected_failures_<protocol>.txt`. It fails on a new failure and on a listed one that now passes, so the lists only shrink. `test/smoke_flaky.txt` holds the checks whose result depends on timing with this server.
- **Two players:** `scripts/two_players_test.py`.
- **Animation builtins:** `scripts/anim_builtins_test.py` runs `getAnimLength`, `animHasNotetrack`, `getMoveDelta` and `getAngleDelta` from a test mod.
- **Layout parity:** `scripts/layout_parity.py` compiles a program with this repo's headers and one with CoD2rev-Plus's and compares sizes and offsets of the types in `test/layout_parity_types.txt`.
- **Copied files:** `scripts/vendor_check.py` compares every file in `test/vendored_files.txt` with a CoD2rev-Plus checkout.
- **Unit tests** for the scripts: `python3 -m unittest discover -s test`.

## Known problems

- The dedicated server crashes on the fourth map load (`map` or `map_restart`), with either VM.
- The 1.0 dedicated server crashes sending the gamestate.
- The MSVC build is not checked by the scripts above and still compiles C with the reconstructed VM.

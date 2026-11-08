# Building

Install commands and the CMake options are in the root
[README](../../README.md#building); this page is what a contributor needs on
top of it.

## Builds

Every x86 and x64 build computes float math on SSE2 (`-msse2 -mfpmath=sse`, [ADR 0016](../architecture/adr/0016-sse2-float-math.md)), as OpenCoD2-Plus Server: the 32-bit builds need an SSE2 CPU.

| Build | Configure | Outputs |
|---|---|---|
| Linux 1.3 | `scripts/build_linux_vk_deps.sh`, then `cmake -S . -B build/check-linux` | `cod2_linux` on DXVK, with `libdxvk_d3d9.so.0` and `libvkd3d-shader.so.1` next to it, and `cod2_lnxded`, speaking CoD2 1.3 Plus (`COD2_PLUS`, OpenCoD2-Plus Server's `make PLUS=1`): the servers take only the Plus client |
| Linux stock 1.3 | `+ -DCOD2_PLUS_PROTOCOL=OFF` | same, without 1.3 Plus: the servers take any 1.3 client, the client does not announce itself as Plus; every other improvement stays |
| Linux 1.0 | `+ -DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` | same, protocol 115 |
| Linux client on GL | `+ -DCOD2_LINUX_DXVK=OFF` (no `scripts/build_linux_vk_deps.sh` needed) | `cod2_linux` on the Mac port's GL device, stand-in shaders; the default for `-DCOD2_X64=ON`, as DXVK's libraries are 32-bit |
| MinGW dedicated | `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake` | `cod2_win32_ded.exe` |
| MinGW client | `+ -DCOD2_WIN32_CLIENT=ON` (static SDL2 in `src/win32/sdl2/lib`, not in git) | `cod2_win32.exe`, real Direct3D 9 |
| Windows client on Vulkan | `scripts/fetch_windows_dxvk.sh DIR_WITH_THE_EXE`, then Options > Graphics or `+set r_gfxApi vulkan` ([ADR 0011](../architecture/adr/0011-graphics-apis-behind-direct3d-9.md)) | `dxvk/d3d9.dll` |
| Windows client, MSVC ABI | from vcvars32: `cmake --preset msvc-client` (`cmake/toolchain-msvc-clang.cmake`: clang for `i686-pc-windows-msvc`, lld-link; SDL's VC package in `third_party/SDL2-<version>/`); from WSL `build_check.py msvc-client` ([ADR 0015](../architecture/adr/0015-msvc-abi-client-on-the-bridge.md)) | `cod2_win32.exe` on the MSVC runtime, same sources and bridge as MinGW's; needs `SDL2.dll` |
| Web | the Emscripten SDK in `third_party/emsdk` (`git clone emsdk`, `./emsdk install latest`, `./emsdk activate latest`), then `emcmake cmake -S . -B build/web`; `build_check.py web` | `cod2.html`, `cod2.js`, `cod2.wasm` ([src/web/README.md](../../src/web/README.md)); links with undefined symbols, only those `test/web_undefined.txt` lists; not run in a browser by the checks |
| Linux dedicated, 64-bit | `cmake -S cmake/lnxded64 -B build/check-linux-x64` (`-DCOD2_PROTOCOL=115` for 1.0) | `cod2_lnxded`, OpenCoD2-Plus Server's sources only, no multilib needed |
| Linux 64-bit client | `scripts/linux_x64_deps.sh` (amd64 SDL2 headers, no root), then `-DCOD2_X64=ON` | `cod2_linux` for x86_64, on OpenCoD2-Plus Server's VM and server like the 32-bit client; loads maps, not yet equal to it ([ADR 0010](../architecture/adr/0010-64-bit-dedicated-server-without-the-engine.md)) |
| Windows 64-bit client | `scripts/mingw64_deps.sh` (MinGW-w64 x86_64 and SDL2's x86_64 libraries, no root), then `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake -DCOD2_X64=ON -DCOD2_WIN32_CLIENT=ON` | `cod2_win32.exe` for x86_64, real Direct3D 9, on OpenCoD2-Plus Server's VM and server; loads maps under a 64-bit Wine (`scripts/portable_wine.sh DIR 64`), not yet equal to the 32-bit one; no libcod (it keeps pointers in `long`, 4 bytes on Win64) |

The client's sound (`src/snd_sdl/`, not in any glob of `src/PC`, `src/Mac` or
`src/stubs`) links into `cod2_linux` and `cod2_win32`, where
those files alone are `-O2`, as a mixer has a deadline. MP3 comes from
`src/snd_sdl/minimp3.h`, one unchanged header of `lieff/minimp3` `ea99364` (CC0).

## build_check.py

`python3 scripts/build_check.py [variant ...|all]` configures and builds each
variant into `build/check-<variant>/` and runs the static checks.

| Variant | Flags |
|---|---|
| `linux`, `linux-10` | native, 1.3 / 1.0, the client on DXVK (skipped without `scripts/build_linux_vk_deps.sh`'s libraries) |
| `linux-13` | native stock 1.3 (`-DCOD2_PLUS_PROTOCOL=OFF`), the client on DXVK |
| `linux-x64`, `linux-x64-10` | 64-bit dedicated server (`cmake/lnxded64`), 1.3 / 1.0; only the glibc and library checks apply |
| `mingw`, `mingw-10` | MinGW dedicated server |
| `mingw-client`, `mingw-client-10` | MinGW client, real Direct3D 9 (skipped without the static SDL2) |
| `mingw-client-gl` | MinGW client on the GL device (`-DCOD2_WIN32_REAL_D3D9=OFF`) |
| `mingw-client-x64` | 64-bit MinGW client (`cmake/toolchain-mingw64.cmake`, `-DCOD2_X64=ON`; skipped without `scripts/mingw64_deps.sh`'s SDL2); the three bridge checks with x86_64's `nm` and no `_` prefix |
| `linux-gl` | Linux client on the GL device (`-DCOD2_LINUX_DXVK=OFF`) |
| `linux-client-x64` | 64-bit Linux client, `cod2_linux` only (`-DCOD2_X64=ON`; skipped without `scripts/linux_x64_deps.sh`'s headers); glibc and the three bridge checks apply |

Checks it runs (each a script in `scripts/`):

| Check | Variants | Fails when |
|---|---|---|
| glibc floor | Linux | a binary needs glibc newer than 2.34, or `cod2_lnxded` needs a client library |
| `check_duplicates.py` | all executables | a C name comes from both OpenCoD2-Plus Server and the engine |
| `check_single_impl.py` | all executables | an engine object still defines a function the bridge exports |
| `check_bridge_symbols.py` | dedicated server, client | a bridge import calls a data symbol |
| `check_bgame_names.py` | Linux client | a plain bgame name is left where `eng_` should apply |
| `bgame_plus_consts.py --check` | Linux | `bg_plus_consts.h` differs from OpenCoD2-Plus Server's values |
| `check_c_decls.py` | Linux | a call's types differ from the definition's (DWARF); pointers by what they point to (float, integer, struct; `void *` matches any) |
| `check_var_sizes.py` | Linux | a global, array or not, is used as larger than the binary defines it |
| `check_c_linkage.py` | `linux`, `linux-10` | a C++-mangled name is not in `test/c_linkage_allowlist.txt` |
| `layout_parity.py` | `linux`, `linux-10`, `mingw` | a type in `test/layout_parity_types.txt` lays out differently from OpenCoD2-Plus Server's |
| `gen_script_bridge.py` | `all` only | `src/compat` differs from what the generator writes |

A variant skipped for lack of SDL2, DXVK, the amd64 SDL2 headers or the MinGW-w64 x86_64 SDL2 fails the run unless `--allow-skip`
is given.

Gotcha: an executable whose mtime is newer than its inputs is not relinked,
even if a previous link was cut short. If a binary is suspiciously small,
delete it and build again.

Other `build_check.py` flags: `--c` builds C (`-DCOD2_CXX_ALL=OFF`),
`--cxx-dirs "a;b"` then keeps those `src/` modules as C++.

## Dedicated server without the client's libraries

`cod2_lnxded` links the same engine objects as the client, whose renderer, GL
layer, SDL2 input and HTTP download reference GL, X11, SDL2 and curl but are
never called there: `src/unix/linux_ded_stubs.c` (Linux) and
`src/win32/win32_stubs.c` (MinGW) stand in for them, each returning 0. When the
glibc/client-library check fails, the missing stub goes there.

The MinGW client downloads redirected files (`sv_wwwBaseURL`) through WinINet:
`src/win32/win_wininet.c` implements the part of libcurl's API that
`src/PC/qcommon/dl_main.c` calls, so the download logic is the Linux client's.

Linux builds use `_FILE_OFFSET_BITS=64`: the 32-bit `readdir` fails with
`EOVERFLOW` on file systems with large inode numbers (Windows drives under WSL).

## How the engine compiles as C++

Each C source is built through `build/<dir>/cxx_wrap/<path>.c.cpp`, holding
`extern "C" { #include "<path>.c" }`, so names stay C for the blobs, the `.S`
files and the `--defsym` aliases ([ADR 0002](../architecture/adr/0002-compile-the-engine-as-cpp.md)).
`src/headers/glibc_compat.h` is force-included on Linux to keep the glibc 2.34
floor. `scripts/cxx_errors.py` counts C++ errors per module;
`scripts/check_c_linkage.py` lists mangled names beyond
`test/c_linkage_allowlist.txt`.

Rules for shared headers and engine sources:

| Rule | Why |
|---|---|
| enums declared ahead of their definition use `COD2_ENUM_DECL` (opaque C++ declaration) and `COD2_ENUM_INT` (`int` base) | C++ needs a fixed underlying type |
| every project header is wrapped in `extern "C"` | names stay C for blobs and aliases |
| K&R imports called with arguments are declared `f(COD2_KR)` (`...` in C++) | same arguments and promotions as in C |
| a non-void function that can end without `return` returns 0 | in C++ that is undefined and GCC drops the code after the call |
| C library functions come from their headers, not redeclarations | real signatures |

## Regenerating the bridge

The generator reads object files of existing builds: `build/check-linux`
and, when present, `build/check-mingw` and `build/check-mingw-client`
(MinGW compiles without optimizing and keeps calls GCC drops on Linux).
Build all three first, or the MinGW link may miss imports.

| Task | Command |
|---|---|
| Regenerate a bridge module | `python3 scripts/gen_script_bridge.py --module script\|server\|client_server` |
| Regenerate the `eng_` names header | `python3 scripts/bgame_names.py` |

## Windows client without Windows

`scripts/portable_wine.sh DIR` downloads a 32-bit Wine and its i386
dependencies with `apt-get download` (no root) into `DIR` and writes
`DIR/wine.sh`. `COD2_CONSOLE_STDOUT=1` copies the engine's console to stdout.

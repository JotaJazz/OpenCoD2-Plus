# Building

Install commands and the CMake options are in the root
[README](../../README.md#building); this page is what a contributor needs on
top of it.

## Builds

| Build | Configure | Outputs |
|---|---|---|
| Linux 1.3 | `cmake -S . -B build/check-linux` | `cod2_linux`, `cod2_lnxded` |
| Linux 1.0 | `+ -DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` | same, protocol 115 |
| MinGW dedicated | `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake` | `cod2_win32_ded.exe` |
| MinGW client | `+ -DCOD2_WIN32_CLIENT=ON` (static SDL2 in `src/win32/sdl2/lib`, not in git) | `cod2_win32.exe`, real Direct3D 9 |
| MSVC | `cmake --preset msvc-client` (`CMakePresets.json`, `cmake/win32-msvc.cmake`) | `cod2_win32.exe`, C, reconstructed VM/server |
| Web | `emcmake cmake -S . -B build/web` | see the top of `CMakeLists.txt` |
| x64 | `-DCOD2_X64=ON` | compiles, does not run |

## build_check.py

`python3 scripts/build_check.py [variant ...|all]` configures and builds each
variant into `build/check-<variant>/` and runs the static checks.

| Variant | Flags |
|---|---|
| `linux`, `linux-10` | native, 1.3 / 1.0 |
| `mingw`, `mingw-10` | MinGW dedicated server |
| `mingw-client`, `mingw-client-10` | MinGW client, real Direct3D 9 (skipped without the static SDL2) |
| `mingw-client-gl` | MinGW client on the GL device (`-DCOD2_WIN32_REAL_D3D9=OFF`) |

Checks it runs (each a script in `scripts/`):

| Check | Variants | Fails when |
|---|---|---|
| glibc floor | Linux | a binary needs glibc newer than 2.34, or `cod2_lnxded` needs a client library |
| `check_duplicates.py` | all executables | a C name comes from both CoD2rev-Plus and the engine |
| `check_single_impl.py` | all executables | an engine object still defines a function the bridge exports |
| `check_bridge_symbols.py` | dedicated server, client | a bridge import calls a data symbol |
| `check_bgame_names.py` | Linux client | a plain bgame name is left where `eng_` should apply |
| `bgame_plus_consts.py --check` | Linux | `bg_plus_consts.h` differs from CoD2rev-Plus's values |
| `check_c_decls.py` | Linux | a call's types differ from the definition's (DWARF) |
| `check_var_sizes.py` | Linux | a global is used as larger than the binary defines it |

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
| Record the engine's ABI (needs gdb and a build with the engine's server) | `python3 scripts/engine_abi.py --module server` |
| Regenerate the `eng_` names header | `python3 scripts/bgame_names.py` |

The client's ABI is read from a client built with
`-DCOD2_PLUS_CLIENT_SERVER=OFF`.

## Windows client without Windows

`scripts/portable_wine.sh DIR` downloads a 32-bit Wine and its i386
dependencies with `apt-get download` (no root) into `DIR` and writes
`DIR/wine.sh`. `COD2_CONSOLE_STDOUT=1` copies the engine's console to stdout.
Wine cannot load the game's real `mss32.dll`, so sound is not testable there.

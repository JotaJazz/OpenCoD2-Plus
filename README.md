# opencod2

> A reconstruction of the **Call of Duty 2** engine

> [!WARNING]
> **Work in progress — this does not fully work yet.** It is an early, incomplete
> reconstruction: it builds and boots, but expect crashes, missing functionality,
> and broken features. It is shared for the curious and for collaboration, not as
> a finished or playable port. No stability, no support, and the code may change
> shape at any time.

> [!IMPORTANT]
> **No game content is included — bring your own data.** This repository is
> *source code only*: no executables, archives, maps, models, textures, sounds,
> or scripts. To run anything you must supply data files from a copy of the game
> that **you legally own**.

## Status

The reconstruction is incomplete. It may crash, omit subsystems, or only
partially implement behavior that exists in the original game.

## Security

Older Call of Duty titles and game engines from this era have a history of
security-sensitive bugs, especially around networking, file parsing,
content-loading paths, and memory safety. This project should not currently be
treated as a hardened or production-safe engine.

One long-term goal of the reconstruction is to make those risks easier to audit
and fix: preserve compatibility where practical, but replace unsafe behavior and
close vulnerabilities as they are found. Until then, run it only with data you
trust and avoid exposing test servers to untrusted networks.

## Building

All targets are driven by **CMake (≥ 3.16)**, each an out-of-source build into
its own directory.

On Linux and MinGW the engine compiles as **C++** (gnu++17): each C source is
built through a generated wrapper, `extern "C" { #include "x.c" }`, so every
symbol keeps the C name the data blobs and `.S` files refer to. The data blobs
and the third-party libraries (speex, jpeg-6, zlib) stay C. Configure with
`-DCOD2_CXX_ALL=OFF` to build plain C again. The MSVC build below still
compiles C.

The script VM is [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus)'s,
copied unchanged into `src/script/` (with the headers it includes), so its
commits apply here; `src/compat/` bridges it to the engine's C names
(`scripts/gen_script_bridge.py` generates that bridge). `-DCOD2_PLUS_SCRIPT=OFF`
goes back to the reconstruction in `src/PC/script/`, which the MSVC, web and
x64 builds still use.

### Linux (primary)

The engine is a 32-bit x86 binary; you need a multilib toolchain and 32-bit libs
(Debian/Ubuntu names shown):

```sh
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install build-essential gcc-multilib g++-multilib cmake \
     libsdl2-dev:i386 libgl1-mesa-dev:i386 libx11-dev:i386 \
     libcurl4-openssl-dev:i386 zlib1g-dev:i386 libstdc++6:i386
```

```sh
cmake -S . -B build-native
cmake --build build-native -j
# -> build-native/cod2_linux    (client)
# -> build-native/cod2_lnxded   (dedicated server)
```

### Windows (MinGW cross-compile)

Needs `g++-mingw-w64-i686` and the static zlib for MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Add `-DCOD2_WIN32_CLIENT=ON` for the SDL2/GL client (supply SDL2 dev libs under
`src/win32/sdl2/lib/`; the dedicated server needs none).

#### Swap-in renderer DLL (optional, experimental)

The renderer can be built as a separate swap-in DLL — it exports `GetRefAPI`
and talks to the engine only through the `ri`/`re` tables. With the client
configured, run the `gfxdll` target:

```sh
cmake --build build-win32 --target gfxdll
# -> build-win32/gfx_d3d_mp_x86_s.dll   (renderer DLL)
# -> build-win32/cod2_win32_gfxdll.exe  (engine that loads it at runtime)
```

The renderer↔engine bridge is pre-generated and committed under `build/gfxdll/`;
the build is pure compile+link. Experimental — not exhaustively tested.

### Windows (MSVC, native — `cod2_win32.exe`)

A native Windows build of the full SDL2/GL client with the Microsoft C/C++
compiler (`cl.exe`) — no cross-compiler required. It is **additive**: it does not
touch the MinGW path above (everything MSVC-specific is gated behind the
`COD2_WIN_MSVC` CMake option).

Requires Visual Studio 2022 or newer with the **x86 MSVC toolset**. The binary is
32-bit, so configure and build from an **"x86 Native Tools for VS" command
prompt** (or run `call "<VS_INSTALL_DIR>\VC\Auxiliary\Build\vcvars32.bat"` to put the x86 `cl`, CMake, and Ninja on `PATH`):

**Debug build:**

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe   (full client, ~2.7 MB)
```

**Optimized Release build (`/O2`):**

```bat
cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (~1.9 MB)
```

> [!NOTE]
> Release applies `/O2` to decompiler-faithful C, which is less battle-tested
> than the Debug build — if something misbehaves only in Release, suspect the
> optimizer. (MSVC doesn't assume strict aliasing, so the code's heavy
> type-punning is comparatively safe.)

SDL2 is user-supplied (never committed). Drop the 32-bit MSVC SDL2 dev package
under `third_party/SDL2-<version>/` (or the legacy `src/win32/sdl2/`); the build
finds the headers and `lib/x86` automatically.

- **Default — dynamic.** Uses the import `SDL2.lib`; copy the 32-bit `SDL2.dll` next to the
  built exe in `build/msvc/` (or `build/msvc-release/`).
- **Optional — fully static / standalone** (`-DCOD2_SDL2_STATIC=ON`, on either
  preset). Produces a single self-contained exe that imports **only Windows
  system DLLs** — no `SDL2.dll`, no VC runtime DLLs. This one flag statically
  links **both** SDL2 **and** the CRT (`/MTd` Debug, `/MT` Release). You supply a
  static `SDL2-static.lib` you build yourself from the SDL2 source, in the
  **same config** as the engine:

  ```bat
  :: match the engine: -DCMAKE_BUILD_TYPE=Debug for msvc-client, =Release for -release
  cmake -S SDL2-2.32.10 -B sdl2-build -DCMAKE_BUILD_TYPE=Debug ^
        -DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON
  cmake --build sdl2-build
  :: copy the resulting SDL2-static*.lib -> third_party/SDL2-*/lib/x86/SDL2-static.lib
  ```

  - **Config must match** — a `/MTd` (Debug) SDL2 lib against a `/MT` (Release)
    engine, or vice-versa, fails with `LNK4098`. `SDL_FORCE_STATIC_VCRT=ON` gives
    the static CRT; the SDL2 build's `CMAKE_BUILD_TYPE` picks `/MTd` vs `/MT`.
  - `SDL_RENDER=OFF` — the engine uses SDL only for window/GL/input, and SDL's
    render backend exports a `MatrixMultiply` that otherwise collides with the
    engine's own.

  The extra system deps (`uuid`, `dinput8`) are linked for you. Most people
  won't need this.

With no SDL2 lib at all, the link falls back to a stub and no window opens.

This target compiles all TUs, links with no unresolved or duplicate symbols,
boots, renders the menu, and can load maps — but the same work-in-progress
caveats above apply.

### Checks

```sh
python3 scripts/build_check.py all        # linux (1.3), linux-10 (1.0) and mingw into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux   # no C++-mangled engine symbols
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118   # 115 for the 1.0 build
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118   # two players join, leave, get kicked
python3 scripts/layout_parity.py                  # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # copied files unchanged
python3 -m unittest discover -s test
```

`opencod2_smoke.py` runs `scripts/smoke_test.py` (unchanged from
[CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus)) against the dedicated
server and compares its failures with `test/smoke_expected_failures_<protocol>.txt`,
the checks this server does not pass yet; the lists may only shrink.
`scripts/cxx_errors.py` counts the C++ errors of a module compiled as C++
(`--return-type` finds non-void functions that can end without a return,
undefined behaviour in C++).

## Running

This reconstructs the engine, not the content. Point it at data from a copy of
the game you legally own:

```sh
./build-native/cod2_linux +set fs_basepath /path/to/your/game
```

On Windows (native MSVC client):

```bat
:: Debug
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\your\game"

:: Release
build\msvc-release\cod2_win32.exe +set fs_basepath "C:\path\to\your\game"
```

Without legally-obtained data the build runs but has nothing to load.

## Notice

This is an independent, source-level reconstruction of the Call of Duty
2 engine. It is not affiliated with, authorized by, sponsored by, or endorsed by
Activision Publishing, Inc., Infinity Ward, or any of their affiliates.

"Call of Duty" and "Call of Duty 2" are trademarks of Activision Publishing,
Inc. They are used in this repository only for identification and
interoperability, to describe what the code reconstructs. No claim is made to
those marks.

This project does not provide or help obtain copyrighted game data. To use it
with real game content, you must supply data files from a copy of Call of Duty 2
that you legally own.

The reconstructed engine source is a derivative work created for the purposes of
preservation, interoperability, research, and education. It is provided as-is,
without warranty of any kind, express or implied. The original port, build
system, and platform glue are separable original work.

If you are a rights holder and believe something here should not be distributed,
please open an issue or contact the maintainer and it will be addressed
promptly.

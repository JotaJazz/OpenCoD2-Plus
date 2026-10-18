# opencod2-plus

<p align="center"><img src="docs/banner.svg" alt="opencod2-plus" width="100%"></p>

![Status: work in progress](https://img.shields.io/badge/status-work%20in%20progress-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · **English** · [Português](README.pt.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Русский](README.ru.md)

Reverse-engineered source code of the **Call of Duty 2** engine: the multiplayer client and the dedicated server, for Linux and Windows.

This repository is a fork of [opencod2/opencod2](https://github.com/opencod2/opencod2), which is where the reconstruction of the engine was done. It also carries changes from [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). This fork only adds the changes described in [Changes in this fork](#changes-in-this-fork). Its goal is to become one repository with the client and the dedicated server, by bringing in the server of [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) module by module.

> [!WARNING]
> **Work in progress — this does not fully work yet.** The client boots to the main menu and the dedicated server loads a map and holds players, but expect crashes, missing functionality and broken features. It is shared for the curious and for collaboration, not as a finished or playable port.

> [!IMPORTANT]
> **No game content is included — bring your own data.** This repository is source code only: no executables, archives, maps, models, textures, sounds or scripts. To run anything you must supply data files from a copy of the game that **you legally own**.

## Transparency

- **Who maintains this:** I'm JotaJazz. I like old games, and that is why this fork exists: I want to keep playing CoD2 with my friends, and to keep the game alive for the community that still plays it, with an open project.
- **How the work is done:** I work with [Claude Code](https://claude.com/claude-code), Anthropic's coding assistant. The changes in this fork, including this README and its translations, are written with its help. Commits made this way carry a `Co-Authored-By: Claude` line in their message, so they can be identified in the history with `git log`.
- **Relationship with the original:** the reconstruction is the work of the opencod2 authors (riicchhaarrd) and of Sipex. This fork no longer pulls changes from them; it brings in code from CoD2rev-Plus instead, copied unchanged so that its commits apply here too.
- **License:** the original repository has no license, and neither does this one yet. CoD2rev-Plus, whose script VM is already here (`src/script`), is AGPL-3.0. The license of the whole is being sorted out with the original authors before more of that code is published here.
- **What it is not:** it is not an official project and is not affiliated with Activision, Infinity Ward or the authors of the original repository. It contains no game files.

## Changes in this fork

- **Builds that work again:** the Linux and MinGW builds, broken since the original was built only with MSVC, build again; `scripts/build_check.py` builds Linux 1.3, Linux 1.0 and MinGW in one go.
- **Compiled as C++:** on Linux and MinGW the whole engine compiles as C++ (gnu++17), with the shared headers made valid C++, without changing what it does; checks guard the C names the data blobs rely on and the structure layouts of the original binary.
- **CoD2rev-Plus's script VM:** the GSC virtual machine is CoD2rev-Plus's, copied unchanged into `src/script`, with a generated C/C++ bridge. The server now holds two players, disconnects, reconnects and `clientkick`, where the reconstructed VM crashed as soon as the second player joined.
- **Fixes:** the server answered on the wrong port, saw no `.iwd` on Windows drives under WSL, and crashed on any kick; it announced protocol 118 in the 1.0 build; the C++ Linux build stays on glibc 2.34; `getAnimLength` and three other animation builtins no longer crash the server.
- **Tests:** CoD2rev-Plus's smoke test, unchanged, against this server, with the list of checks it does not pass yet; a two-player test, an animation-builtins test, and checks for structure layouts, C names and the files copied from CoD2rev-Plus.

Every change in detail is in [docs/CHANGES.md](docs/CHANGES.md) (in English). The plan behind them is in [docs/superpowers](docs/superpowers).

## Game versions

- **The version is chosen when building.** By default the engine is built as 1.3 (protocol 118); with `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` it is built as 1.0 (protocol 115).
- **The 1.3 dedicated server works for testing:** it loads a map, answers `getinfo`, `getstatus` and rcon, and holds two players who join, leave, come back and get kicked. It still crashes on the fourth map load.
- **The 1.0 dedicated server does not work yet:** it loads a map and accepts connections, but crashes sending the gamestate. CoD2rev-Plus's server, which supports 1.0, 1.3 and the Game Pass version, is the next part to come in.

## Building

All targets are driven by **CMake (≥ 3.16)**, each an out-of-source build into its own directory.

### Linux

The engine is a 32-bit x86 binary. On a 64-bit Debian or Ubuntu system you need the 32-bit architecture and libraries:

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

### Windows (cross-compiled from Linux with MinGW)

Requires the 32-bit MinGW-w64 compilers (`g++-mingw-w64-i686`) and the static zlib for MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Add `-DCOD2_WIN32_CLIENT=ON` for the SDL2/GL client (supply the static SDL2 libraries under `src/win32/sdl2/lib/`; the dedicated server needs none).

#### Swap-in renderer DLL (optional, experimental)

The renderer can be built as a separate swap-in DLL: it exports `GetRefAPI` and talks to the engine only through the `ri`/`re` tables. With the client configured, run the `gfxdll` target:

```sh
cmake --build build-win32 --target gfxdll
# -> build-win32/gfx_d3d_mp_x86_s.dll   (renderer DLL)
# -> build-win32/cod2_win32_gfxdll.exe  (engine that loads it at runtime)
```

The renderer–engine bridge is pre-generated and committed under `build/gfxdll/`; the build is pure compile and link. Experimental and not exhaustively tested.

### Windows (MSVC, native)

A native build of the full SDL2/GL client with Visual Studio 2022 or newer and its **x86** toolset. Configure and build from an "x86 Native Tools for VS" command prompt:

```bat
cmake --preset msvc-client
cmake --build build/msvc --target cod2_win32
:: -> build/msvc/cod2_win32.exe

cmake --preset msvc-client-release
cmake --build build/msvc-release --target cod2_win32
:: -> build/msvc-release/cod2_win32.exe   (/O2)
```

This build still compiles C and uses the reconstructed script VM (`src/PC/script`). SDL2 is supplied by you: put the 32-bit MSVC SDL2 development package under `third_party/SDL2-<version>/` and copy `SDL2.dll` next to the executable. With `-DCOD2_SDL2_STATIC=ON` it links a static SDL2 you build yourself (`SDL2-static.lib`, same configuration as the engine) and the static C runtime, into one executable that needs no DLL; build SDL2 with `-DSDL_STATIC=ON -DSDL_SHARED=OFF -DSDL_RENDER=OFF -DSDL_FORCE_STATIC_VCRT=ON`. Release applies `/O2` to decompiler-faithful code that is less tested than the Debug build: if something misbehaves only in Release, suspect the optimizer.

### CMake options

| Option | Default | Description |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Compiles the engine as C++ on Linux and MinGW. `OFF` builds C. |
| `COD2_CXX_DIRS` | `""` | With `COD2_CXX_ALL=OFF`, the `src/` modules or files still compiled as C++. |
| `COD2_PLUS_SCRIPT` | `ON` | CoD2rev-Plus's script VM (`src/script`) on Linux and MinGW. `OFF` uses the reconstructed one (`src/PC/script`), as MSVC, the web build and x64 do. |
| `COD2_FEATURE_CFLAGS` | `""` | Feature switches; `-DCOD2_PATCH_10` builds the 1.0 version. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: the SDL2/GL client instead of the dedicated server. |
| `COD2_WIN_MSVC` | `OFF` | Native MSVC build (set by the `msvc-client` presets). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: one standalone executable with static SDL2 and C runtime. |
| `COD2_DEBUG` | `OFF` | `-O0` and no hardening, for debugging. |
| `COD2_X64` | `OFF` | x86_64 target; a port in progress that compiles but does not run yet. |

There is also a WebAssembly build (`emcmake cmake -S . -B build/web`, with the Emscripten SDK), described at the top of `CMakeLists.txt`.

## Running

The engine needs the original game files (the `main` folder with its `.iwd` files), which are not in this repository. You need your own copy of Call of Duty 2.

```sh
./build-native/cod2_linux +set fs_basepath /path/to/CoD2
./build-native/cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

On Windows:

```bat
build\msvc\cod2_win32.exe +set fs_basepath "C:\path\to\CoD2"
build-win32\cod2_win32_ded.exe +set fs_basepath "C:\path\to\CoD2" +set dedicated 1 +map mp_toujane
```

`dedicated 1` keeps the server on the local network; it is a test server, not one to run on the public list yet.

## Testing

The checks need the build and, for the ones that start the server, a game folder with the stock `.iwd` files. They start the server in LAN mode with a temporary home folder, so the game folder is only read.

```bash
python3 scripts/build_check.py all                      # linux (1.3), linux-10 (1.0) and mingw into build/check-*/
python3 scripts/check_c_linkage.py build/check-linux    # no unexpected C++ names in the engine
python3 scripts/layout_parity.py                        # types shared with CoD2rev-Plus's VM lay out alike
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118
python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol 118
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` runs CoD2rev-Plus's `scripts/smoke_test.py`, unchanged, and compares its failures with `test/smoke_expected_failures_<protocol>.txt`: the checks this server does not pass yet. It fails on a new failure and on a listed one that now passes, so the lists only shrink. `--protocol 115` tests the 1.0 build.
- `two_players_test.py` joins two clients on tdm, has them send commands, disconnects one and brings it back, and kicks the other; the server must answer after every step.
- `anim_builtins_test.py` runs `getAnimLength`, `animHasNotetrack`, `getMoveDelta` and `getAngleDelta` from a test mod's level script.
- `scripts/cxx_errors.py` counts the C++ errors of a module (`--return-type` finds non-void functions that can end without a return, which is undefined in C++); `scripts/gen_script_bridge.py` regenerates the bridge to the script VM.

## Repository layout

| Path | Contents |
| --- | --- |
| `src/PC` | The reconstructed engine: client (`client_mp`, `cgame_mp`, `ui_mp`), server (`server_mp`), game (`game_mp`, `bgame`), renderer (`gfx_d3d`), shared code (`qcommon`, `universal`), animations (`xanim`) and the reconstructed script VM (`script`). |
| `src/Mac` | The Mac port's platform layer that the reconstruction came from: OpenGL behind a Direct3D interface, input and sound. |
| `src/script` | CoD2rev-Plus's script VM, copied unchanged, with the headers it needs in `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` and `src/libcod`. |
| `src/compat` | The bridge between the engine's C names and CoD2rev-Plus's C++ VM; mostly generated. |
| `src/headers`, `src/imports` | Shared types, offsets checked against the original binary, and declarations of imported functions. |
| `src/blobs`, `build/native_gen`, `build/win_gen`, `build/gfxdll` | Data taken from the original binary, still used by the engine. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Platform code and stubs. |
| `scripts` | Build and test scripts, and the test client from CoD2rev-Plus. |
| `test` | Unit tests and the lists the checks read. |
| `docs` | [CHANGES.md](docs/CHANGES.md), the banner, and the design and plans in `docs/superpowers`. |

## Notice

This is an independent, source-level reconstruction of the Call of Duty 2 engine. It is not affiliated with, authorized by, sponsored by, or endorsed by Activision Publishing, Inc., Infinity Ward, or any of their affiliates.

"Call of Duty" and "Call of Duty 2" are trademarks of Activision Publishing, Inc. They are used in this repository only for identification and interoperability, to describe what the code reconstructs. No claim is made to those marks.

This project does not provide or help obtain copyrighted game data. To use it with real game content, you must supply data files from a copy of Call of Duty 2 that you legally own.

The reconstructed engine source is a derivative work created for the purposes of preservation, interoperability, research, and education. It is provided as-is, without warranty of any kind, express or implied.

If you are a rights holder and believe something here should not be distributed, please open an issue or contact the maintainer and it will be addressed promptly.

## Credits

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), the reconstruction of the engine
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), hardening and network fixes
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) and [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), the dedicated server and the script VM
- id Software
- Activision / Infinity Ward

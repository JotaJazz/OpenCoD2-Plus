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

This repository is a fork of [opencod2/opencod2](https://github.com/opencod2/opencod2), which is where the reconstruction of the engine was done. It also carries changes from [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web). This fork only adds the changes described in [Changes in this fork](#changes-in-this-fork). Its goal is to become one repository with the client and the dedicated server, by bringing in the server of [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) module by module. On Linux that is done: the dedicated server runs entirely on CoD2rev-Plus's code, and the client's local games run on its server.

> [!WARNING]
> **Work in progress — this does not fully work yet.** The client boots to the main menu, hosts local games that other clients join, and the dedicated server, now CoD2rev-Plus's, loads maps and holds players in 1.0 and 1.3, but expect crashes, missing functionality and broken features. It is shared for the curious and for collaboration, not as a finished or playable port.

> [!IMPORTANT]
> **No game content is included — bring your own data.** This repository is source code only: no executables, archives, maps, models, textures, sounds or scripts. To run anything you must supply data files from a copy of the game that **you legally own**.

## Transparency

- **Who maintains this:** I'm JotaJazz. I like old games, and that is why this fork exists: I want to keep playing CoD2 with my friends, and to keep the game alive for the community that still plays it, with an open project.
- **How the work is done:** I work with [Claude Code](https://claude.com/claude-code), Anthropic's coding assistant. The changes in this fork, including this README and its translations, are written with its help. Commits made this way carry a `Co-Authored-By: Claude` line in their message, so they can be identified in the history with `git log`.
- **Relationship with the original:** the reconstruction is the work of the opencod2 authors (riicchhaarrd) and of Sipex. This fork no longer pulls changes from them; it brings in code from CoD2rev-Plus instead, copied unchanged so that its commits apply here too.
- **License:** the original repository has no license, and neither does this one yet. CoD2rev-Plus, whose script VM, server, game and bgame are already here (`src/script`, `src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`), is AGPL-3.0. The license of the whole is being sorted out with the original authors before more of that code is published here.
- **What it is not:** it is not an official project and is not affiliated with Activision, Infinity Ward or the authors of the original repository. It contains no game files.

## Changes in this fork

- **Builds that work again:** the Linux and MinGW builds, broken since the original was built only with MSVC, build again; `scripts/build_check.py` builds Linux and MinGW (the dedicated server and, with SDL2, the client), as 1.3 and as 1.0, in one go.
- **Compiled as C++:** on Linux and MinGW the whole engine compiles as C++ (gnu++17), with the shared headers made valid C++, without changing what it does; checks guard the C names the data blobs rely on and the structure layouts of the original binary.
- **CoD2rev-Plus's script VM:** the GSC virtual machine is CoD2rev-Plus's, copied unchanged into `src/script`, with a generated C/C++ bridge. The server now holds two players, disconnects, reconnects and `clientkick`, where the reconstructed VM crashed as soon as the second player joined.
- **CoD2rev-Plus's server:** the dedicated server is built with CoD2rev-Plus's server, copied unchanged into `src/server`, with a generated bridge in which every call between the two is checked against the engine's ABI. The local games of the Linux and Windows (MinGW) clients run on it too, and the whole client traces with its collision: a client hosts a game and others join it. With it the server refuses to send the stock and server-only `.iwd` files. Its game and bgame (`src/game`, `src/bgame`) come with it, and with them `addip` and the rotation of `games_mp.log`; in the client, cgame keeps the engine's bgame (`src/PC/bgame`), whose globals take the `eng_` prefix, and which now predicts the player's moves, mantles and weapon with CoD2rev-Plus's code: in the same moves it now mispredicts (snaps back) about an eighth as often. Since then the dedicated server also runs on its core: `universal`, `qcommon` and the platform code (`src/universal`, `src/qcommon`, `src/unix`, `src/win32`), which bring the console log's rotation and timestamps, `net_strictPort` and its crash report. With its xanim (`src/xanim`) too, everything the dedicated server runs is CoD2rev-Plus's code.
- **A dedicated server without the client's libraries:** on Linux `cod2_lnxded` needs only libc, libm, libz and libstdc++, not GL, X11, SDL2 or curl.
- **Fixes:** the server answered on the wrong port, saw no `.iwd` on Windows drives under WSL, and crashed on any kick; it announced protocol 118 in the 1.0 build; the C++ Linux build stays on glibc 2.34; `getAnimLength` and three other animation builtins no longer crash the server. The fourth or fifth map load crashed the server: the animation script grew with each map, and the error it raised from rcon corrupted the stack. The 1.0 server wrote over its own configstrings and no 1.0 client could join. Fragmented messages used the wrong offsets in both versions, and the port of a network address was where Quake III keeps it, not where CoD2 does. The `.iwd` files in the server's own folder were not searched, and the builtins that take a player did not see one with CoD2rev-Plus's VM. Entities were linked into the world's sectors under the wrong number, so unlinking some of them crashed and traces tested the previous entity. In the client, every entity clipped as a box was left out of the world's sectors and the box test missed the segments that crossed it, so bullets went through the players of a local game; and since an upstream change for x64, no animation of more than one frame played on 32 bits, which left the players and the first-person weapon in a broken pose. The client crashed when it saw a player die, and its movement prediction could overrun its stack against walls; it sent a jump, a stance or a weapon change one command late and truncated the velocity it predicts; the MG42's anims, the UI's fill and text widths and a dozen other engine calls passed arguments or read results of another type than the function's, and effects played along a direction got a broken axis. No effect was drawn at all (impacts, explosions, the maps' smoke): the effects code read its flags, curve scales, colors and velocities otherwise than the original binary, scheduled delayed parts for a time that never came, left every particle at the world's origin with no second radius, and nothing drew them.
- **Tests:** CoD2rev-Plus's smoke test, unchanged, against this server, with the list of checks it does not pass yet; tests for two players, map loads, a drop error from rcon and the animation builtins; and checks for structure layouts, C names, the ABI of the bridge and the files copied from CoD2rev-Plus. Tests fire level and down at a player and check that each shot lands on the upper body or on the legs, and that a knife and an explosion reach a player, on the dedicated server and in a local game.

Every change in detail is in [docs/CHANGES.md](docs/CHANGES.md) (in English).

## Game versions

- **The version is chosen when building.** By default the engine is built as 1.3 (protocol 118); with `-DCOD2_FEATURE_CFLAGS=-DCOD2_PATCH_10` it is built as 1.0 (protocol 115).
- **Both dedicated servers work for testing:** in 1.3 and in 1.0 the server loads, changes and restarts maps, answers `getinfo`, `getstatus` and rcon, and holds two players who join, leave, come back and get kicked.
- **Local games work for testing:** in 1.3 and in 1.0 a client hosts a game on CoD2rev-Plus's server, another client joins it, and both stay through `map_restart` and a map change (`scripts/listen_test.py`, with your CD key). A shot lands on the body part it aims at (`scripts/hit_location_test.py`).
- **What CoD2rev-Plus has in libcod is not here yet:** its script functions, the bots and `sv_master1`…`sv_master5`, so the server sends its heartbeat only to the Activision master.

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

The client's libraries are needed to build both, but `cod2_lnxded` runs with only the 32-bit libc, libm, libz and libstdc++.

### Windows (cross-compiled from Linux with MinGW)

Requires the 32-bit MinGW-w64 compilers (`g++-mingw-w64-i686`) and the static zlib for MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Add `-DCOD2_WIN32_CLIENT=ON` for the SDL2/GL client (supply the static SDL2 libraries under `src/win32/sdl2/lib/`; the dedicated server needs none). Like the Linux client, it runs its local games on CoD2rev-Plus's server, game and bgame, and traces with its collision. Without Windows, `scripts/portable_wine.sh DIR` sets up a 32-bit Wine without root (Debian or Ubuntu with i386 enabled) and writes `DIR/wine.sh` to run it with; `COD2_CONSOLE_STDOUT=1` copies the console to stdout, and `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` has it host a game the Linux client joins.

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
| `COD2_PLUS_SERVER` | `ON` | CoD2rev-Plus's server, game, bgame and core (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` or `src/win32`) in the dedicated server, on Linux and MinGW; needs `COD2_PLUS_SCRIPT`. `OFF` uses the reconstructed ones (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | CoD2rev-Plus's server, game, bgame and collision for the client's local games (Linux, and MinGW with `COD2_WIN32_CLIENT`), on the engine's core; needs `COD2_PLUS_SERVER`. `OFF` keeps `src/PC/server_mp`, as MSVC, the web build and x64 still do. |
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
python3 scripts/check_c_decls.py build/check-linux     # every engine call matches its definition's types
python3 scripts/check_var_sizes.py build/check-linux    # no global is used as larger than it is defined
python3 scripts/opencod2_smoke.py /path/to/CoD2 --protocol 118
python3 scripts/two_players_test.py /path/to/CoD2 --protocol 118
python3 scripts/listen_test.py /path/to/CoD2 --protocol 118
python3 scripts/map_reload_test.py /path/to/CoD2 --protocol 118
python3 scripts/rcon_drop_test.py /path/to/CoD2 --protocol 118
python3 scripts/anim_builtins_test.py /path/to/CoD2 --protocol 118
python3 scripts/prediction_test.py /path/to/CoD2 --protocol 118
python3 scripts/fx_test.py /path/to/CoD2 --protocol 118
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` runs CoD2rev-Plus's `scripts/smoke_test.py`, unchanged, and compares its failures with `test/smoke_expected_failures_<protocol>.txt`: the checks this server does not pass yet. It fails on a new failure and on a listed one that now passes, so the lists only shrink. `--protocol 115` tests the 1.0 build. `--protocol 118 --binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh` runs it against the Windows dedicated server under Wine (`scripts/portable_wine.sh`), with its own list, `test/smoke_expected_failures_wine.txt`: under Wine the server prints only to its window, which also leaves the LAN heartbeat check nothing to read, and a SIGSEGV sent with `kill` does not reach it as a crash, so the crash report and the output's `net_strictPort` are not checked there (the wrapper checks `net_strictPort` in its console log).
- `two_players_test.py` joins two clients on tdm, has them send commands, disconnects one and brings it back, and kicks the other; the server, with `developer 1` so that a script error stops the map, must answer after every step.
- `listen_test.py` starts a client that hosts a game and a second one that joins it, and checks both after a `map_restart` and a map change; it needs a CD key in `~/.cod2/preferences`.
- `map_reload_test.py` loads maps by rcon, alternating `map_restart` and `map` between two maps, and checks every load; `rcon_drop_test.py` raises a drop error from rcon (`error drop`) and checks the server stays up.
- `anim_builtins_test.py` runs `getAnimLength`, `animHasNotetrack`, `getMoveDelta` and `getAngleDelta` from a test mod's level script.
- `prediction_test.py` counts the prediction misses of a real client (with a CD key) that walks, crouches, goes prone, jumps and fires; `--max-misses` sets how many it allows.
- `fx_test.py` has a real client (with a CD key) play a sprite, a bullet's impact and two explosions in front of it, and checks each changes the screenshot.
- `scripts/cxx_errors.py` counts the C++ errors of a module (`--return-type` finds non-void functions that can end without a return, which is undefined in C++); `scripts/gen_script_bridge.py --module script|server` regenerates the bridge to the script VM or to the server, and `scripts/engine_abi.py --module server` records, with gdb, the engine's ABI the bridge checks against (`test/server_abi.txt`).

## Repository layout

| Path | Contents |
| --- | --- |
| `src/PC` | The reconstructed engine: client (`client_mp`, `cgame_mp`, `ui_mp`), server (`server_mp`), game (`game_mp`, `bgame`), renderer (`gfx_d3d`), shared code (`qcommon`, `universal`), animations (`xanim`) and the reconstructed script VM (`script`). On Linux and in the MinGW dedicated server, `server_mp`, `game_mp`, `game` and the core CoD2rev-Plus replaces are left out (`test/plus_replaces.txt`); the MinGW client leaves out `server_mp`, `game_mp`, `game` and the collision. |
| `src/Mac` | The Mac port's platform layer that the reconstruction came from: OpenGL behind a Direct3D interface, input and sound. |
| `src/script` | CoD2rev-Plus's script VM, copied unchanged, with the headers it needs in `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` and `src/libcod`. |
| `src/server` | CoD2rev-Plus's server, copied unchanged; the dedicated server uses it. |
| `src/game`, `src/bgame` | CoD2rev-Plus's game and bgame, copied unchanged; the dedicated server uses them. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | CoD2rev-Plus's core and platform code, copied unchanged; the dedicated server runs on them. `test/plus_replaces.txt` lists the engine sources they replace there. |
| `src/compat` | The bridges between the engine's C names and CoD2rev-Plus's C++ VM, server and core; mostly generated. |
| `src/headers`, `src/imports` | Shared types, offsets checked against the original binary, and declarations of imported functions. |
| `src/blobs`, `build/native_gen`, `build/win_gen`, `build/gfxdll` | Data taken from the original binary, still used by the engine. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Platform code and stubs. |
| `scripts` | Build and test scripts, and the test client from CoD2rev-Plus. |
| `test` | Unit tests and the lists the checks read. |
| `docs` | [CHANGES.md](docs/CHANGES.md) and the banner. |

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

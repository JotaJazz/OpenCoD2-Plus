# OpenCoD2-Plus

<p align="center"><img src="docs/banner.svg" alt="OpenCoD2-Plus" width="100%"></p>

![Status: in development](https://img.shields.io/badge/status-in%20development-orange)
![License: none yet](https://img.shields.io/badge/license-none%20yet-lightgrey)
![Platforms: Linux | Windows](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-lightgrey)
![CoD2 1.3](https://img.shields.io/badge/CoD2-1.3-c9b37e)
[![Fork of opencod2/opencod2](https://img.shields.io/badge/fork%20of-opencod2%2Fopencod2-555)](https://github.com/opencod2/opencod2)
[![Last commit](https://img.shields.io/github/last-commit/JotaJazz/opencod2-plus)](https://github.com/JotaJazz/opencod2-plus/commits/main)

[Español](README.es.md) · **English** · [Português](README.pt.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Русский](README.ru.md)

**Call of Duty 2 is still alive.** OpenCoD2-Plus is the Call of Duty 2 multiplayer engine rebuilt from its source: the client and the dedicated server, for Windows and Linux, made by people who still play the game and keep improving it.

<table>
  <tr>
    <td><img src="docs/images/carentan.jpg" alt="Carentan: the piano in front of the Jeanne d'Arc café"></td>
    <td><img src="docs/images/toujane.jpg" alt="Toujane: the flak gun and the mosque tower"></td>
  </tr>
  <tr>
    <td colspan="2"><img src="docs/images/scoreboard.jpg" alt="The scoreboard of a local game with bots"></td>
  </tr>
</table>
<p align="center"><sub>Carentan, Toujane and a local game with bots, drawn by the Linux client with the game's own shaders.</sub></p>

## What works today

- **Online play on public 1.3 servers** with the Windows client, with the original lighting: Direct3D 9 and the game's own shaders.
- **Host a game from the client**, for friends to join, with bots if you like (libcod).
- **A dedicated server for 1.3 and 1.0**, built on [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus): on Linux it passes CoD2rev-Plus's whole smoke test.
- **A Linux client**, and one on Vulkan (DXVK) that draws with the game's own shaders too.
- **Checked against the original 1.3 binaries**: movement prediction, effects, sounds, recoil, menus and many fixes ([what changed](docs/changes.md)).

> [!NOTE]
> **Still in development.** Expect bugs and missing pieces, and please report them in the [issues](https://github.com/JotaJazz/opencod2-plus/issues).

> [!IMPORTANT]
> **No game content is included — bring your own data.** This repository is source code only: no executables, archives, maps, models, textures, sounds or scripts. To run anything you must supply data files from a copy of the game that **you legally own**.

## How to play

Ready-to-run builds will be published on the **Releases** page. Until then, build the client (see [Building](#building)) and point it at your Call of Duty 2 folder, the one with `main/`:

```bat
cod2_win32.exe +set fs_basepath "C:\path\to\CoD2" +connect IP:port
```

```sh
./cod2_linux +set fs_basepath /path/to/CoD2 +connect IP:port
./cod2_linux_vk +set fs_basepath /path/to/CoD2 +connect IP:port   # with the game's own shaders (Vulkan)
```

Without `+connect` the client opens the main menu, where you can also start your own game. A dedicated server for your local network:

```sh
./cod2_lnxded +set fs_basepath /path/to/CoD2 +set dedicated 1 +map mp_toujane
```

## For contributors

Every kind of help counts: playing and reporting bugs, testing on other systems, or code. The technical documentation, in English, starts at [docs/README.md](docs/README.md); [AGENTS.md](AGENTS.md) is the entry point for AI coding agents.

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

`cod2_linux` draws with stand-in shaders (no sun, a white sky). For the game's own shaders, build `cod2_linux_vk`: `scripts/build_linux_vk_deps.sh` builds DXVK native and vkd3d-shader (32-bit, from pinned releases; it also needs `meson`, `ninja` and `libvulkan1:i386`), then configure with `-DCOD2_LINUX_DXVK=ON` in its own build directory. It needs a Vulkan device.

### Windows (cross-compiled from Linux with MinGW)

Requires the 32-bit MinGW-w64 compilers (`g++-mingw-w64-i686`) and the static zlib for MinGW (`libz-mingw-w64-dev`).

```sh
cmake -S . -B build-win32 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake
cmake --build build-win32 -j
# -> build-win32/cod2_win32_ded.exe   (dedicated server)
```

Add `-DCOD2_WIN32_CLIENT=ON` for the client, which draws with Direct3D 9 and the game's shaders (`-DCOD2_WIN32_REAL_D3D9=OFF`: the GL device) (supply the static SDL2 libraries under `src/win32/sdl2/lib/`; the dedicated server needs none). Like the Linux client, it runs its local games on CoD2rev-Plus's server, game and bgame, and traces with its collision. Without Windows, `scripts/portable_wine.sh DIR` sets up a 32-bit Wine without root (Debian or Ubuntu with i386 enabled) and writes `DIR/wine.sh` to run it with; `COD2_CONSOLE_STDOUT=1` copies the console to stdout, and `scripts/listen_test.py --host-binary cod2_win32.exe --host-wine DIR/wine.sh` has it host a game the Linux client joins.

<details>
<summary><b>Windows (MSVC, native)</b></summary>

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

</details>

<details>
<summary><b>CMake options</b></summary>

| Option | Default | Description |
| --- | --- | --- |
| `COD2_CXX_ALL` | `ON` | Compiles the engine as C++ on Linux and MinGW. `OFF` builds C. |
| `COD2_CXX_DIRS` | `""` | With `COD2_CXX_ALL=OFF`, the `src/` modules or files still compiled as C++. |
| `COD2_PLUS_SCRIPT` | `ON` | CoD2rev-Plus's script VM (`src/script`) on Linux and MinGW. `OFF` uses the reconstructed one (`src/PC/script`), as MSVC, the web build and x64 do. |
| `COD2_PLUS_SERVER` | `ON` | CoD2rev-Plus's server, game, bgame and core (`src/server`, `src/game`, `src/bgame`, `src/universal`, `src/qcommon`, `src/xanim`, `src/unix` or `src/win32`) in the dedicated server, on Linux and MinGW; needs `COD2_PLUS_SCRIPT`. `OFF` uses the reconstructed ones (`src/PC/server_mp`, `src/PC/game_mp`, `src/PC/bgame`). |
| `COD2_PLUS_CLIENT_SERVER` | `ON` | CoD2rev-Plus's server, game, bgame and collision for the client's local games (Linux, and MinGW with `COD2_WIN32_CLIENT`), on the engine's core; needs `COD2_PLUS_SERVER`. `OFF` keeps `src/PC/server_mp`, as MSVC, the web build and x64 still do. |
| `COD2_PLUS_LIBCOD` | `ON` | CoD2rev-Plus's libcod (`src/libcod`) in the dedicated server and, with `COD2_PLUS_CLIENT_SERVER`, in the client's local games, with its modules but MySQL; needs `COD2_PLUS_SERVER`. |
| `COD2_FEATURE_CFLAGS` | `""` | Feature switches; `-DCOD2_PATCH_10` builds the 1.0 version. |
| `COD2_WIN32_CLIENT` | `OFF` | MinGW: the client instead of the dedicated server. |
| `COD2_WIN32_REAL_D3D9` | `ON` | MinGW client: real Direct3D 9 and the game's shaders (system d3dx9); `OFF` keeps the GL device. |
| `COD2_WIN_MSVC` | `OFF` | Native MSVC build (set by the `msvc-client` presets). |
| `COD2_SDL2_STATIC` | `OFF` | MSVC: one standalone executable with static SDL2 and C runtime. |
| `COD2_DEBUG` | `OFF` | `-O0` and no hardening, for debugging. |
| `COD2_X64` | `OFF` | x86_64 target; a port in progress that compiles but does not run yet. |

There is also a WebAssembly build (`emcmake cmake -S . -B build/web`, with the Emscripten SDK), described at the top of `CMakeLists.txt`.

</details>

## Testing

<details>
<summary><b>The test scripts</b></summary>

The checks need the build and, for the ones that start the server, a game folder with the stock `.iwd` files. They start the server in LAN mode with a temporary home folder, so the game folder is only read.

```bash
python3 scripts/build_check.py all                      # every variant (linux, linux-10, mingw, mingw-client...) into build/check-*/
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
python3 scripts/bots_test.py /path/to/CoD2 --binary build/check-linux/cod2_lnxded
python3 scripts/http_test.py /path/to/CoD2 --binary build/check-linux/cod2_lnxded
python3 scripts/vendor_check.py /path/to/CoD2rev-Plus   # files copied from CoD2rev-Plus are unchanged
python3 -m unittest discover -s test
```

- `opencod2_smoke.py` runs CoD2rev-Plus's `scripts/smoke_test.py`, unchanged, and compares its failures with `test/smoke_expected_failures_<protocol>.txt`: the checks this server does not pass yet. It fails on a new failure and on a listed one that now passes, so the lists only shrink. `--protocol 115` tests the 1.0 build. `--protocol 118 --binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh` runs it against the Windows dedicated server under Wine (`scripts/portable_wine.sh`), with its own list, `test/smoke_expected_failures_wine.txt`: under Wine the server prints only to its window and a signal does not reach it as one (a SIGSEGV sent with `kill` is ignored, SIGTERM ends it at once), so the crash report, the SIGTERM checks and those that read its output are not checked there (the wrapper checks `net_strictPort` in its console log). The Linux lists are empty: the dedicated server passes the whole smoke test.
- `two_players_test.py` joins two clients on tdm, has them send commands, disconnects one and brings it back, and kicks the other; the server, with `developer 1` so that a script error stops the map, must answer after every step.
- `listen_test.py` starts a client that hosts a game and a second one that joins it, and checks both after a `map_restart` and a map change; it needs a CD key in `~/.cod2/preferences`.
- `map_reload_test.py` loads maps by rcon, alternating `map_restart` and `map` between two maps, and checks every load; `rcon_drop_test.py` raises a drop error from rcon (`error drop`) and checks the server stays up.
- `anim_builtins_test.py` runs `getAnimLength`, `animHasNotetrack`, `getMoveDelta` and `getAngleDelta` from a test mod's level script.
- `prediction_test.py` counts the prediction misses of a real client (with a CD key) that walks, crouches, goes prone, jumps and fires; `--max-misses` sets how many it allows.
- `fx_test.py` has a real client (with a CD key) play a sprite, a bullet's impact and two explosions in front of it, and checks each changes the screenshot.
- `bots_test.py` and `http_test.py` are CoD2rev-Plus's, unchanged, for libcod: the bots of `mods/bots` must find their paths, save and reload their graph and fight on a real map; `httpFetch` and `webSocket*` must work against local HTTP, HTTPS, WS and WSS servers (they need `openssl`). Pass `--binary build/check-linux/cod2_lnxded`.
- `scripts/cxx_errors.py` counts the C++ errors of a module (`--return-type` finds non-void functions that can end without a return, which is undefined in C++); `scripts/gen_script_bridge.py --module script|server|client_server` regenerates the bridge to the script VM or to the server, and `scripts/engine_abi.py --module server` records, with gdb, the engine's ABI the bridge checks against (`test/server_abi.txt`).

</details>

## Repository layout

<details>
<summary><b>What is where</b></summary>

| Path | Contents |
| --- | --- |
| `src/PC` | The reconstructed engine: client (`client_mp`, `cgame_mp`, `ui_mp`), server (`server_mp`), game (`game_mp`, `bgame`), renderer (`gfx_d3d`), shared code (`qcommon`, `universal`), animations (`xanim`) and the reconstructed script VM (`script`). On Linux and in the MinGW dedicated server, `server_mp`, `game_mp`, `game` and the core CoD2rev-Plus replaces are left out (`test/plus_replaces.txt`); the MinGW client leaves out `server_mp`, `game_mp`, `game` and the collision. |
| `src/Mac` | The Mac port's platform layer that the reconstruction came from: OpenGL behind a Direct3D interface, input and sound. |
| `src/script` | CoD2rev-Plus's script VM, copied unchanged, with the headers it needs in `src/qcommon`, `src/universal`, `src/game`, `src/bgame`, `src/server`, `src/xanim`, `src/zlib`, `src/misc` and `src/libcod`. |
| `src/server` | CoD2rev-Plus's server, copied unchanged; the dedicated server uses it. |
| `src/game`, `src/bgame` | CoD2rev-Plus's game and bgame, copied unchanged; the dedicated server uses them. |
| `src/universal`, `src/qcommon`, `src/xanim`, `src/unix`, `src/win32` | CoD2rev-Plus's core and platform code, copied unchanged; the dedicated server runs on them. `test/plus_replaces.txt` lists the engine sources they replace there. |
| `src/libcod`, `mods/bots` | CoD2rev-Plus's libcod and its libraries (yyjson, SQLite, Mbed TLS; its Speex is not built, the engine's is used), and its bots mod, copied unchanged; the dedicated server and the client's local games use them. |
| `src/compat` | The bridges between the engine's C names and CoD2rev-Plus's C++ VM, server and core; mostly generated. |
| `src/headers`, `src/imports` | Shared types, offsets checked against the original binary, and declarations of imported functions. |
| `src/blobs`, `build/native_gen`, `build/win_gen` | Data taken from the original binary, still used by the engine. |
| `src/unix`, `src/win32`, `src/stubs`, `src/web` | Platform code and stubs. |
| `scripts` | Build and test scripts, and the test client from CoD2rev-Plus. |
| `test` | Unit tests and the lists the checks read. |
| `docs` | The [documentation](docs/README.md) and the banner. |

</details>

## Transparency

- **Who maintains this:** I'm JotaJazz. I like old games, and that is why this fork exists: I want to keep playing CoD2 with my friends, and to keep the game alive for the community that still plays it, with an open project.
- **How the work is done:** I work with [Claude Code](https://claude.com/claude-code), Anthropic's coding assistant. The changes in this fork, including this README and its translations, are written with its help. Commits made this way carry a `Co-Authored-By: Claude` line in their message, so they can be identified in the history with `git log`.
- **Relationship with the original:** the reconstruction is the work of the opencod2 authors (riicchhaarrd) and of Sipex. This fork no longer pulls changes from them; it brings in code from CoD2rev-Plus instead, copied unchanged so that its commits apply here too.
- **License:** none yet, as in the original. CoD2rev-Plus's code here (script VM, server, game, core, libcod and its bots mod) is AGPL-3.0, and the license of the whole is being sorted out with the original authors. Third-party libraries keep theirs: SQLite (public domain), Mbed TLS (Apache-2.0, `src/libcod/mbedtls/LICENSE`), yyjson (MIT) and Speex (BSD, not built).
- **What it is not:** it is not an official project and is not affiliated with Activision, Infinity Ward or the authors of the original repository. It contains no game files.

## Notice

This is an independent, source-level reconstruction of the Call of Duty 2 engine. It is not affiliated with, authorized by, sponsored by, or endorsed by Activision Publishing, Inc., Infinity Ward, or any of their affiliates.

"Call of Duty" and "Call of Duty 2" are trademarks of Activision Publishing, Inc. They are used in this repository only for identification and interoperability, to describe what the code reconstructs. No claim is made to those marks.

This project does not provide or help obtain copyrighted game data. To use it with real game content, you must supply data files from a copy of Call of Duty 2 that you legally own.

The reconstructed engine source is a derivative work created for the purposes of preservation, interoperability, research, and education. It is provided as-is, without warranty of any kind, express or implied.

If you are a rights holder and believe something here should not be distributed, please open an issue or contact the maintainer and it will be addressed promptly.

The screenshots show content of Call of Duty 2, property of Activision Publishing, Inc., for identification only.

## Credits

- [opencod2](https://github.com/opencod2/opencod2) (riicchhaarrd), the reconstruction of the engine
- [Sipex18/opencod2-web](https://github.com/Sipex18/opencod2-web) (Sipex), hardening and network fixes
- [CoD2rev-Plus](https://github.com/JotaJazz/CoD2rev-Plus) and [voron00/CoD2rev_Server](https://github.com/voron00/CoD2rev_Server), the dedicated server and the script VM
- id Software
- Activision / Infinity Ward

# System summary (live snapshot)

Last updated: 2026-11-07. Update in the same commit as the change that makes
a line wrong.

## Repository

```
AGENTS.md            entry point for agents
CMakeLists.txt       every build (Linux, MinGW, MSVC ABI, web, x64)
cmake/               MinGW and MSVC-ABI (clang) toolchains
docs/                documentation (this layer: docs/ai/)
mods/bots/           libcod bots mod (vendored)
scripts/             build checks, generators, tests (Python)
src/PC/              reconstructed engine (client, renderer, cgame, effects, ...)
src/compat/          generated bridge engine <-> OpenCoD2-Plus Server
src/{script,server,game,bgame,qcommon,universal,xanim,libcod,...}/
                     vendored OpenCoD2-Plus Server (src/unix, src/win32: its platform
                     code plus this fork's stubs and SDL2 libs)
src/blobs/           original binaries' data
src/headers/         shared headers, layout pins
src/snd_sdl/         our sound engine on SDL2, behind the AIL_* calls
src/Mac/, src/stubs/ GL device and platform glue
test/                interface/ABI lists, allowlists, expected failures, unit tests
third_party/         dependencies fetched by scripts, SDL2's VC package (user supplied)
cod2rad/             upstream tool (not built by default)
```

## Executables

| Executable | Build | State |
|---|---|---|
| `cod2_lnxded` | Linux 1.3/1.0 | works; passes OpenCoD2-Plus Server's smoke test whole; 1.3 is the Plus server unless built with `-DCOD2_PLUS_PROTOCOL=OFF` (stock 1.3) |
| `cod2_win32_ded.exe` | MinGW 1.3/1.0 | works; smoke test under Wine with `test/smoke_expected_failures_wine.txt` |
| `cod2_linux` | Linux 1.3/1.0 | works; real Direct3D 9 on DXVK native and vkd3d-shader (the game's shaders), the default ([ADR 0017](../architecture/adr/0017-dxvk-is-the-default-linux-client.md)) |
| `cod2_linux` (GL) | Linux, `-DCOD2_LINUX_DXVK=OFF` | works; GL device (stand-in shaders) |
| `cod2_linux` (x86_64) | Linux, `-DCOD2_X64=ON` | on DXVK like the 32-bit one ([ADR 0018](../architecture/adr/0018-the-x64-linux-client-on-dxvk.md)); loads and plays a map on OpenCoD2-Plus Server's VM and server; effects and world lighting equal the 32-bit client's (`fx_test.py`, screenshots) |
| `cod2_win32.exe` (x86_64) | MinGW-w64, `cmake/toolchain-mingw64.cmake` | plays on real Windows, online and in local games; real Direct3D 9; no libcod; not yet equal to the 32-bit client |
| `cod2_win32.exe` | MinGW | works; real Direct3D 9; plays on public 1.3 servers |
| `cod2_win32.exe` | MSVC ABI (clang, lld-link) | builds (`build_check.py msvc-client`); plays a local game on Windows (`msvc_run_test.py`); the bridge and libcod as MinGW's |
| `cod2` (web) | Emscripten (`third_party/emsdk`) | builds on the bridge (`build_check.py web`, no libcod); 65 symbols link undefined, `CG_Init` among them; not run in a browser |

## Subsystems

| Subsystem | State | Notes |
|---|---|---|
| Dedicated server | complete (OpenCoD2-Plus Server) | [../architecture/server.md](../architecture/server.md) |
| Client local games | OpenCoD2-Plus Server's server + collision | [../architecture/client.md](../architecture/client.md) |
| Prediction | OpenCoD2-Plus Server's movement and animation script | as many misses as 1.3's own client on the same test (`prediction_test.py --wine`) |
| Renderer | real D3D9 on Windows; GL elsewhere | [../architecture/renderer.md](../architecture/renderer.md) |
| Effects | all primitives per 1.3 | [../architecture/effects.md](../architecture/effects.md) |
| Sound | Our own SDL2 engine, `src/snd_sdl/`, behind the `AIL_*` calls, on Linux and Windows; `mss32.dll` is not used | [ADR 0012](../architecture/adr/0012-sound-engine-behind-the-ail-calls.md), [../architecture/client.md](../architecture/client.md#sound) |
| Online play | CD key auth, HTTP downloads (curl on Linux, WinINet on Windows) | 1.3 servers; prediction measured against 1.3 on a modded server; hostile input bounded ([../architecture/client.md](../architecture/client.md#what-a-server-cannot-do)) |

Open items: [../roadmap/debt.md](../roadmap/debt.md).

## Commands

| Task | Command |
|---|---|
| Build + static checks | `python3 scripts/build_check.py all` |
| Unit tests | `python3 -m unittest discover -s test` |
| Docs check | `python3 scripts/check_docs.py` |
| Runtime tests | [../development/testing.md](../development/testing.md); `scripts/hostile_server_test.py` for malformed server input |
| Regenerate bridge | `python3 scripts/gen_script_bridge.py --module <m>` |
| Wine for the Windows client | `scripts/portable_wine.sh DIR` |

## Contracts

| Contract | File |
|---|---|
| Vendored files and revision | `test/vendored_files.txt` |
| Bridge surfaces | `test/script_interface.txt`, `test/server_interface.txt`, `test/client_server_interface.txt` |
| Engine ABI | `test/server_abi.txt`, `test/client_server_abi.txt` |
| Sources OpenCoD2-Plus Server replaces | `test/plus_replaces.txt` |
| Expected smoke failures | `test/smoke_expected_failures_118.txt`, `_115.txt`, `_wine.txt` |

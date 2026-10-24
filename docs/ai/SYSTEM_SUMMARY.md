# System summary (live snapshot)

Last updated: 2026-10-21. Update in the same commit as the change that makes
a line wrong.

## Repository

```
AGENTS.md            entry point for agents
CMakeLists.txt       every build (Linux, MinGW, MSVC, web, x64)
cmake/               MinGW toolchain, MSVC settings
docs/                documentation (this layer: docs/ai/)
mods/bots/           libcod bots mod (vendored)
scripts/             build checks, generators, tests (Python)
src/PC/              reconstructed engine (client, renderer, cgame, effects, ...)
src/compat/          generated bridge engine <-> CoD2rev-Plus
src/{script,server,game,bgame,qcommon,universal,xanim,libcod,...}/
                     vendored CoD2rev-Plus (src/unix, src/win32: its platform
                     code plus this fork's stubs and SDL2 libs)
src/blobs/           original binaries' data
src/headers/         shared headers, layout pins
src/Mac/, src/stubs/ GL device and platform glue
test/                interface/ABI lists, allowlists, expected failures, unit tests
third_party/         SDL2 for MSVC (user supplied)
cod2rad/             upstream tool (not built by default)
```

## Executables

| Executable | Build | State |
|---|---|---|
| `cod2_lnxded` | Linux 1.3/1.0 | works; passes CoD2rev-Plus's smoke test whole |
| `cod2_win32_ded.exe` | MinGW 1.3/1.0 | works; smoke test under Wine with `test/smoke_expected_failures_wine.txt` |
| `cod2_linux` | Linux 1.3/1.0 | works; GL device (stand-in shaders) |
| `cod2_linux_vk` | Linux, `-DCOD2_LINUX_DXVK=ON` | works; real Direct3D 9 on DXVK native and vkd3d-shader (the game's shaders) |
| `cod2_win32.exe` | MinGW | works; real Direct3D 9; plays on public 1.3 servers |
| `cod2_win32.exe` | MSVC | builds; not checked by scripts |
| `cod2` (web) | Emscripten | upstream state |

## Subsystems

| Subsystem | State | Notes |
|---|---|---|
| Dedicated server | complete (CoD2rev-Plus) | [../architecture/server.md](../architecture/server.md) |
| Client local games | CoD2rev-Plus's server + collision | [../architecture/client.md](../architecture/client.md) |
| Prediction | CoD2rev-Plus's movement and animation script (parser and crouch mask as 1.3) | misses ~16–22 per test run |
| Renderer | real D3D9 on Windows; GL elsewhere | [../architecture/renderer.md](../architecture/renderer.md) |
| Effects | all primitives per 1.3 | [../architecture/effects.md](../architecture/effects.md) |
| Sound (Windows) | game's `mss32.dll` | not testable under Wine |
| Online play | CD key auth, HTTP downloads | 1.3 servers |

Open items: [../roadmap/debt.md](../roadmap/debt.md).

## Commands

| Task | Command |
|---|---|
| Build + static checks | `python3 scripts/build_check.py all` |
| Unit tests | `python3 -m unittest discover -s test` |
| Docs check | `python3 scripts/check_docs.py` |
| Runtime tests | [../development/testing.md](../development/testing.md) |
| Regenerate bridge | `python3 scripts/gen_script_bridge.py --module <m>` |
| Wine for the Windows client | `scripts/portable_wine.sh DIR` |

## Contracts

| Contract | File |
|---|---|
| Vendored files and revision | `test/vendored_files.txt` |
| Bridge surfaces | `test/script_interface.txt`, `test/server_interface.txt`, `test/client_server_interface.txt` |
| Engine ABI | `test/server_abi.txt`, `test/client_server_abi.txt` |
| Sources CoD2rev-Plus replaces | `test/plus_replaces.txt` |
| Expected smoke failures | `test/smoke_expected_failures_118.txt`, `_115.txt`, `_wine.txt` |

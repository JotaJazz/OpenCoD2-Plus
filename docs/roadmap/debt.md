# Debt, risks and deferred decisions

Live list. Remove an entry when it is fixed (and record the fix in
`docs/ai/PROJECT_MEMORY.md`); add one when a phase leaves something open.

## Open risks

| Risk | Impact | Notes |
|---|---|---|
| MSVC build unchecked | may not build or may diverge | not covered by `build_check.py`; compiles C with the reconstructed VM and server |
| Engine's server/game/collision still used by MSVC, web, x64 | bugs fixed in CoD2rev-Plus remain there | simpler collision: boxes not capsules, no 0.125 gap, sight traces against box entities a placeholder |
| x64 client | does not compile as C++, does not run | `COD2_X64`: about 100 pointer casts to `int`, 32-bit data blobs, the bridge's 4-byte slots, and `mss32.dll` on Windows ([ADR 0010](../architecture/adr/0010-64-bit-dedicated-server-without-the-engine.md)); the 64-bit dedicated server does not depend on it |
| Miles mixer crash with `snd_khz 22` | `mss32.dll` overflows an unsigned division (0xc0000095) on a negative volume delta at 2x rate | not known whether 1.3 does it too; `jugar.bat` sets `snd_khz 44`, not a code fix |
| Possible overruns in CoD2rev-Plus's copies | unconfirmed; fixes go there first | `src/qcommon/msg_mp.cpp` reads a playerstate's field count unchecked, `src/stringed/stringed_hooks.cpp` may copy past its buffer as the engine's did; `src/server`, `src/game` and `src/libcod` were not audited |
| A cut HTTP download on Windows | checked on Linux only | `CL_WWWDownload` compares the bytes received with the server's size; not run against WinINet |
| A Windows dedicated server on IPX loses a byte of its address | replies to the wrong node | `altPort` overlays `ipx[9]` to keep `netadr_t` at 20 bytes; `win_net.cpp` writes it for every protocol |
| Client and server movement differ by 1 ULP on some frames | none measurable | CoD2rev-Plus's `bgame` moved literals to float (`0.001f`, `0.4f`, `0.65f`), `src/PC/bgame` keeps doubles; far below `prediction_test.py`'s 0.1 miss |
| `r_gfxApi d3d12` never run on Direct3D 12 | may not start or may draw differently | Wine has no D3D9On12, so only its fallback to Direct3D 9 is tested |
| Real Direct3D 9 path tested mostly under Wine | a driver may differ | users test natively on Windows |

## Accepted debt

| Item | Why accepted |
|---|---|
| `cod2_linux` keeps the GL device (stand-in shaders) | `cod2_linux_vk` has the game's shaders but needs DXVK, vkd3d-shader and a Vulkan device |
| `fxRestart` takes hunk memory each time until the next map load | developer command; many restarts on one map end in a drop error |
| Server browser's find-player list indexes 1-based (`ui_main_mp.c`) | nothing starts that search, in the original either |
| Windows dedicated server prints only to its window, shows a message box on fatal errors | CoD2rev-Plus's `src/win32`; scripts read the console log |

## Known gaps

| Gap | Where | Notes |
|---|---|---|
| Snow's `particle_cloud_outdoor` shader does not compile under Wine | Wine's d3dx9 cannot flatten an `if` | Microsoft's d3dx9 compiles it |
| MSVC client has no HTTP download | `src/win32/win32_stubs.c` | its curl stubs answer 0, so a redirected download fails and the server drops the client; the MinGW client uses `win_wininet.c` |
| Particle clouds look faint at low resolutions | flakes are 1.5 units, the GPU samples the 1×1 mip of their 4×4 texture | matches the data; not compared with the original at runtime |

## Deferred decisions

| Decision | Waiting on |
|---|---|
| Move MSVC, web and x64 off `src/PC/server_mp` | porting the bridge to those toolchains ([ADR 0003](../architecture/adr/0003-keep-server-mp-for-msvc-web-x64.md)) |
| Make `cod2_linux_vk` the default Linux client | testing on native Linux with a GPU (under WSL Vulkan is llvmpipe); shipping DXVK and vkd3d-shader with it |

## Deliberate simplifications

| Simplification | Notes |
|---|---|
| Full screen on the Direct3D 9 path is a borderless desktop window with the frame stretched, not an exclusive mode change | resolution comes from `r_mode` as in the original |
| `COD2_FEATURE_GAMEPAD`, `COD2_FEATURE_RUMBLE` blocks remain | their sources were removed: nothing set the macros; turning one on no longer links |
| `ClickToContinue` not ported: 1.3's `IN_Frame` (`0x464b30`) posts a left click each frame while that dvar is set | nothing registers or sets it, in the game's data or the engine; porting it means emulating `IN_MouseEvent`'s button state |
| Shaders' `//` comments blanked before compiling | Wine's compiler rejects one after an `#include` |

# Debt, risks and deferred decisions

Live list. Remove an entry when it is fixed (and record the fix in
`docs/ai/PROJECT_MEMORY.md`); add one when a phase leaves something open.

## Open risks

| Risk | Impact | Notes |
|---|---|---|
| MSVC build unchecked | may not build or may diverge | not covered by `build_check.py`; compiles C with the reconstructed VM and server |
| Engine's server/game/collision still used by MSVC, web, x64 | bugs fixed in CoD2rev-Plus remain there | simpler collision: boxes not capsules, no 0.125 gap, sight traces against box entities a placeholder |
| x64 port | compiles, does not run | `COD2_X64` |
| Real Direct3D 9 path tested mostly under Wine | a driver may differ | users test natively on Windows |

## Accepted debt

| Item | Why accepted |
|---|---|
| Engine's bgame keeps its own animation script (`src/PC/bgame/bg_animation_mp.c`) | predicted legs/torso differ from the server's 4 times over the prediction test, against 20 origin misses |
| `cod2_linux` keeps the GL device (stand-in shaders) | `cod2_linux_vk` has the game's shaders but needs DXVK, vkd3d-shader and a Vulkan device |
| `fxRestart` takes hunk memory each time until the next map load | developer command; many restarts on one map end in a drop error |
| Server browser's find-player list indexes 1-based (`ui_main_mp.c`) | nothing starts that search, in the original either |
| Windows dedicated server prints only to its window, shows a message box on fatal errors | CoD2rev-Plus's `src/win32`; scripts read the console log |

## Known gaps

| Gap | Where | Notes |
|---|---|---|
| Snow's `particle_cloud_outdoor` shader does not compile under Wine | Wine's d3dx9 cannot flatten an `if` | Microsoft's d3dx9 compiles it |
| Particle clouds look faint at low resolutions | flakes are 1.5 units, the GPU samples the 1×1 mip of their 4×4 texture | matches the data; not compared with the original at runtime |
| The SDL pump ignores `in_mouse` (not registered there) and `ClickToContinue` (the original posts a click each frame) | `src/unix/linux_input.c` | 1.3's `IN_Frame` `0x464b30` |
| The Linux X11 fallback pump still sends absolute positions to menus | `src/unix/linux_input.c` | runs only when SDL has no events |
| `r_fullscreen` cannot be overridden by `+set` on the command line ("is read only") | dvar registration | profile value wins |

## Deferred decisions

| Decision | Waiting on |
|---|---|
| Move MSVC, web and x64 off `src/PC/server_mp` | porting the bridge to those toolchains ([ADR 0003](../architecture/adr/0003-keep-server-mp-for-msvc-web-x64.md)) |
| Port the engine's animation script to CoD2rev-Plus's | measurable benefit |
| Make `cod2_linux_vk` the default Linux client | testing on native Linux with a GPU (under WSL Vulkan is llvmpipe); shipping DXVK and vkd3d-shader with it |

## Deliberate simplifications

| Simplification | Notes |
|---|---|
| Full screen on the Direct3D 9 path is a borderless desktop window with the frame stretched, not an exclusive mode change | resolution comes from `r_mode` as in the original |
| Shaders' `//` comments blanked before compiling | Wine's compiler rejects one after an `#include` |

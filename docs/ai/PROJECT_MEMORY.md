# Project memory

Append-only operational memory. Newest stage first; at most 3 bullets per
stage; never rewrite a past stage (correct it with a new one). Not a
changelog: record what changed, why it mattered, what was rejected or left
open. Template in [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md#memory-entry).

## 2026-10-23 — view kicks, bob, camera animation, shellshock

- Several view effects of `CG_CalcViewValues` (1.3 `0x4cf900`) and its first-person offset (`0x4cee10`) were missing or wrong: the damage kick timed itself from `v_dmg_time`, the kick's end, instead of the hit (now `damageTime` less `deltaTime`, `0x4cee35`); the horizontal bob moved the eye forward and `bg_bobMax` was read as an integer (`0x4cef6c`); the view weapon's `tag_camera` bone never moved the camera (`0x4cf730`); `BG_GetSpeed` counted vertical speed, so jumps bobbed (`0x4cfa44`).
- A shellshock never shook the view (`cg_perturbations` empty; now the binary's table at `0x59e1e0`) and, once it did, mirrored it: `CG_PerturbCamera` passed three separate vectors as a 3×3 matrix (`0x4d2320`). It also crashed the Linux client: `SND_SetChannelVolumes` read `channelvol`, unset with no sound driver.
- Left open: CoD2rev-Plus's vendored `BG_GetSpeed` had the same 3D length (server side); fixed there in `a335fc6`, it comes here with the next vendor sync, which also brings upstream's xanim rewrite. `scripts/view_weapon_test.py` checks each fix with its own `COD2_TRACE_VIEW*` trace.

## 2026-10-23 — view weapon position

- The view weapon sat at the eye plus the sway and `cg_gun_*`: no lean offset, no move or stance offset, no dip after a landing, sway never scaled by shellshock. `CG_AddViewWeapon` now follows the 1.3 binary's `0x4d6390` and its helpers (`0x4d5970`, `0x4d53d0`, `0x4d57e0`, `0x4d52e0`, `0x4d5bc0`, `0x4d58c0`), including the ADS gun angles from the hand's axis.
- The binary's `cg_t` base is `0x14ee080`; up to `shellshock` its fields sit at the engine's `offsetof` (checked field by field), so the port reads `cg` by name.
- `scripts/view_weapon_test.py` drives a real client with a `vstr` loop (CoD2 has no `alias`; `wait` ahead of `+connect` would block it) and checks the `COD2_TRACE_VIEWWEAPON` trace: lean ±1.2 at `leanf` 0.5, crouch offset reached, landing dips up to `landChange` × 0.25.

## 2026-10-22 — Linux client on real Direct3D 9

- `cod2_linux_vk` (`-DCOD2_LINUX_DXVK=ON`): the Windows client's real Direct3D 9 path on DXVK native, with vkd3d-shader compiling the game's HLSL, so Linux gets the game's lighting (the GL device draws the sky white and the world flat). `cod2_linux` stays the GL client.
- Chosen over translating the shaders to GLSL: it reuses the Windows renderer path whole; only device, window and shader compiler differ. Found: vkd3d 1.19 rejects the game's overlapping `register(c)` reservations, but the include handler strips them already (as on Windows); DXVK native needs `DXVK_WSI_DRIVER` and ends the process when Vulkan has no device it can use, so the client tries it first in a forked child.
- Checked under WSL on llvmpipe: 152 shaders compile on mp_toujane, the map draws, `vid_restart` works, a missing library or Vulkan device is a clean error. Not checked on a GPU.

## 2026-10-22 — weapon motion, scoreboard banner, menu sounds

- After the first shot the view weapon shook until the next spawn: `CG_AddViewWeapon` dropped the motion state `BG_CalculateWeaponAngles` returned, so the gun kick restarted from 0 every frame with an ever-growing speed and followed the frame time. Written back as 1.3 (`0x4d66c4`), with its `deltaTime` time base; checked on the Linux client firing in bursts (idle frame-to-frame change 0.025° → 0.003°, the idle sway).
- The team banners on the scoreboard drew `$default` (a blue band): `Dvar_GetString` returned a `va()` copy, which the banner's own `va()` text overwrote. It returns the string itself, as `0x4375b0`.
- Menus were silent after the first remote connect: `Com_LoadSoundAliases` wiped the menus' aliases when cgame loaded its own. The 1.3 binary (`0x42e380`) wipes nothing there.

## 2026-10-21 — menu mouse

- In menus the SDL pump let the mouse go (system cursor shown) and passed the window's pixel position as the menus' 640×480 position: at 1920×1080 both cursors drifted apart. Now captured with deltas, as the 1.3 binary's `IN_Frame` (`0x464b30`).
- Checked on the Linux client (same pump) with XTest at 1024×768: captured in the menu, menu cursor follows the deltas.

## 2026-10-21 — view kick

- `CG_DrawActiveFrame` zeroed the kick angles every frame, so a shot's recoil never moved the view or the aim; added `CG_KickAngles` (`0x4cec80`), called while `pm_flags & 0x800000`, as the 1.3 binary.
- Checked at runtime: a rifle shot lifts the view about 3 degrees and returns it to center within a few frames; prediction misses unchanged (21 on main and on the branch).

## 2026-10-21 — documentation system

- The former CHANGES.md (one long narrative) split into `docs/architecture`, `docs/development`, `docs/roadmap`, ADRs and this memory layer; `AGENTS.md` added; English only (root README keeps 6 languages).
- `scripts/check_docs.py` keeps links, paths and personal data honest; run it with the unit tests.
- Rejected: a CLAUDE.md (AGENTS.md only), a security section (no surface yet).

## 2026-10-21 — client polish: scoreboard, grenade, snow

- `CL_GetServerCommand` had the server commands' letters wrong (`d` disconnected the client on any configstring); fixed to the 1.3 switch (`0x4017fa`), which also brought the scoreboard's scores, announcements and the equipped offhand.
- Added the per-frame entity loop (`CG_UpdateEntInfo`, `0x4cfbb0`): grenades hung in the air; `CG_Player`/`CG_Corpse` no longer advance their own animation.
- Outdoor map traced bottom-up by mistake (`0x10020790`): snow only fell indoors. Open: the `0x4cec80` kick-view call (see debt).

## 2026-10-20 — Windows client real lighting

- Real `IDirect3DDevice9` with the game's HLSL via the system's d3dx9; made the default for the Windows client the next day (ADR 0008).
- Filled renderer tables left zero (light grid basis, cube face axes, particle cloud geometry, outdoor size); static model lighting, picmip, `r_aspectRatio`, stdcall COM calls fixed.
- Reference harness: the original 1.3 under Wine (stub `mss32.dll`, `d3d9.dll` proxy for screenshots) drew nearly the same picture.

## 2026-10-20 — online play and Windows sound

- CD key authorization and HTTP downloads work on public 1.3 servers; `r_mode` lists display modes; turret view at `tag_player`.
- Windows client plays sound through the game's own `mss32.dll` (untestable under Wine).
- `.iwd` stream seeking and 2D sound handles fixed.

## 2026-10-20 — libcod

- CoD2rev-Plus's libcod (all modules but MySQL) in the dedicated server and the client's local games; Speex stays the engine's.
- CoD2rev-Plus's smoke test passes whole on Linux 1.3 and 1.0; bots and HTTP tests copied unchanged and pass.

## 2026-10-20 — MinGW client and effects

- The MinGW client runs its local games on CoD2rev-Plus's server and collision, like the Linux client; MSVC shims were leaking into MinGW builds (file names 8 bytes short).
- Effects drew nothing: flag tables, channel scales, scheduler helper indirection, particle origin/radius fixed against the 1.3 binary; all primitive updates follow it.
- Renderer DLL variant removed (ADR 0007).

## 2026-10-19 — client collision and prediction

- The client traces with CoD2rev-Plus's collision; the engine's bgame prediction follows CoD2rev-Plus line by line (misses 119–153 → 16 in the same moves).
- Fixed: usercmd long form sent one command late, `Sys_SnapVector` truncation, step-down, `PM_SlideMove` stack overrun, corpse info at raw offsets.
- Decided: keep `src/PC/server_mp` for MSVC, web, x64 (ADR 0003). Deferred: porting `bg_animation_mp.c`.

## 2026-10-18 — dedicated server on CoD2rev-Plus

- Script VM, server, game, bgame, core, xanim vendored behind a generated, ABI-checked bridge (ADR 0001); the reconstructed VM crashed with two players.
- Engine compiled as C++ (gnu++17) with C names kept (ADR 0002); glibc floor 2.34.
- Client local games moved to CoD2rev-Plus's server too (ADR 0004).

## 2026-10-17 — fork start

- Forked from opencod2/opencod2; the Linux and MinGW builds were broken by upstream changes only built with MSVC (119 and 5 errors).
- Goal set: one public repository with client and server on CoD2rev-Plus, validated against the original 1.3 binaries.

## 2026-09-15 — upstream opencod2 (since 2026-06-25)

- Initial reconstruction, MSVC build, data blobs, an x64 port in progress, the effects hierarchy; inherited as the engine (`src/PC`).

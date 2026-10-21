# Project memory

Append-only operational memory. Newest stage first; at most 3 bullets per
stage; never rewrite a past stage (correct it with a new one). Not a
changelog: record what changed, why it mattered, what was rejected or left
open. Template in [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md#memory-entry).

## 2026-10-21 — documentation system

- `docs/CHANGES.md` split into `docs/architecture`, `docs/development`, `docs/roadmap`, ADRs and this memory layer; `AGENTS.md` added; English only (root README keeps 6 languages).
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

## 2026-06-25 to 2026-09-15 — upstream opencod2

- Initial reconstruction, MSVC build, data blobs, an x64 port in progress, the effects hierarchy; inherited as the engine (`src/PC`).

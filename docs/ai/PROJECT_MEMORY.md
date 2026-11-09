# Project memory

Append-only operational memory. Newest stage first; at most 3 bullets per
stage; never rewrite a past stage (correct it with a new one). Not a
changelog: record what changed, why it mattered, what was rejected or left
open. Template in [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md#memory-entry).

## 2026-11-09 — the server browser's columns and clicks

- The PC `joinserver.menu` has 11 columns, the Mac build this follows 10: the ping went into the PunkBuster column and the sort keys were one short. Column and sort key 9 are now PunkBuster (empty, sorted by ping) and 10 the ping, the default sort.
- Three `UI_FeederCount` calls in `ui_shared_mp.c` passed `special`'s bits as an int, which C converts to a float of 1073741824 for feeder 2: every list counted 0 items in `Item_ListBox_HandleKey`, so a click never moved the selection.
- `xdotool` under WSLg cannot move the game's cursor (it stays at the corner), so the click was not driven here; `openmenu joinserver` leaves `main_text` open behind it, unlike the main menu's own Join Game.

## 2026-11-09 — the server browser lists servers

- Two bugs hid every internet server: `ui_main_mp.c` asked `globalservers` for protocol `0x73` (115) in the 1.3 build too, whose `CL_ServerInfoPacket` drops any other protocol, and `UI_BinaryInsertServer` (both copies) put the first server at 1 of an empty list, which `UI_InsertServerAtPosition` refuses; its loop also never ended at `hi == 1` with a server that sorts first. Now `COD2_PROTOCOL_VERSION` and a lower-bound search.
- Found by running the Linux client with `developer 1` and `openmenu joinserver` from a cfg (after `wait 600`), a run of about 90 s: 306 of 410 servers listed, the rest filtered (unpure ones by default).

## 2026-11-09 — d3d12 falls back when it shows nothing

- With `d3d12` in the menu, a GPU where it stays black would leave the player stuck: if none of the first 120 presents after the interface is made returns S_OK, the client sets `r_gfxApi d3d9` and runs `vid_restart` (`RealD3D9_D3D12ShowsNothing`).
- Only until the first S_OK: later, an alt-tabbed exclusive full screen returns S_PRESENT_OCCLUDED for as long as it likes. A black frame that returns S_OK is not caught.

## 2026-11-08 — d3d12 in the graphics menu

- Since `2659668` `d3d12` shows its frame in exclusive full screen, so the black screen that kept it out of the menu is gone: Options > Graphics now offers it with `d3d9` and `vulkan`.
- The patched list (80 bytes) still fits in place of the stock one (89 in `iw_06.iwd`'s `ui/options_graphics.menu`).
- Still open: windowed `d3d12`, and GPUs other than an RTX 5080.

## 2026-11-08 — OpenCoD2-Plus Server 274dca1: Plus ID review fixes

- What: vendored 274dca1 (dates past 2038 on 32 bits, a bans file that cannot be stat'ed kept, the address rule taking only the host's own private IPs and the port in `sv_publicAddress`, the seen file in memory under a lock). The engine got `Sys_IsLocalIP` (`src/PC/win32/win_net.c`, over its `localIP` list) and the `client_server` bridge its import and ABI rows, since the local server's `plusid.cpp` now calls it.
- The boundaries matrix said the engine never uses OpenCoD2-Plus Server, but `cl_plusid_mp.c` includes `plusid.hpp` and links its Mbed TLS: now an accepted exception, [ADR 0019](../architecture/adr/0019-the-client-signs-with-the-server-s-plusid-header.md).
- The key file is linked into place, which vfat, exFAT and some FUSE homes refuse (`EPERM`, `ENOTSUP`, `ENOSYS`, `EXDEV`): there it is renamed while no key exists; `plusid_client_test.py` checks it with an `LD_PRELOAD` shim.

## 2026-11-08 — Plus ID: the client signs

- What: with the server side vendored (OpenCoD2-Plus Server 48cf44c, its stage is at the end of this file), every non-web client keeps an ECDSA P-256 key in `fs_homepath/plusid.key` (64 hex and a newline; mode 0600 on POSIX; written once through `.tmp` and `link`/`rename`, never over a file) and adds `pk`, `ps` and `pa` to the `connect` userinfo (`src/PC/client_mp/cl_plusid_mp.c`, on the vendored `plusid.hpp` and `cod2_plus_mbedtls`); `plusid` prints the ID. Why: the Plus servers identify and ban a player by it, with no hardware data. A corrupt key file, or a userinfo without room for the keys, connects without an ID.
- Found on the way: the engine's `NET_AdrToString` prints a port over 32767 negative (`(short)`), so the client writes the signed `ip:port` as the server's `%hu` does; `Info_RemoveKey` moved overlapping strings with `strcpy`, which on x64 glibc garbled a long userinfo (`Dvar_InfoString` sets `name` again), so a 500-byte userinfo broke connecting: now `memmove`, as the server's `q_shared.cpp`.
- Left open: no run with a public address (`sv_publicAddress`; locally only LAN IDs exist); the Windows clients build but were not run with an ID; the key has no passphrase or OS keystore, backing the file up is the player's job.

## 2026-11-08 — OpenCoD2-Plus Server 0870352: error counts

- Brought the vendored files up to 0870352: the Plus build prints the count a repeated script runtime error left when it stopped, from a call in `SV_Frame` (`scr_parser.cpp`, `sv_main_mp.cpp`, `plus_rules_test.py`). Server code only; the Plus version stays 123.
- Checked: `vendor_check.py`, the dedicated variants, `linux-13` and `linux-client-x64` (with the main tree's `third_party` linked in). Not built: `mingw-client` and `msvc-client`.

## 2026-11-08 — the x64 Linux client on DXVK

- `COD2_LINUX_DXVK` is on for `-DCOD2_X64=ON` too ([ADR 0018](../architecture/adr/0018-the-x64-linux-client-on-dxvk.md)): `scripts/build_linux_vk_deps.sh x64` builds the pinned DXVK and vkd3d-shader with the native gcc into `third_party/linux-vk64/`, taking SDL2's headers from `scripts/linux_x64_deps.sh`'s unpacked package (`PKG_CONFIG_SYSROOT_DIR`, plus its multiarch include dir for `_real_SDL_config.h`); DXVK loads SDL2 at run time, so nothing links the absent amd64 `libSDL2.so`.
- Checked under WSL (llvmpipe): `build_check.py linux-client-x64` builds on DXVK and `client_view_test.py --binary` with it passes. Not checked: a native GPU, the other runtime tests on x64.

## 2026-11-08 — OpenCoD2-Plus Server 85f92f0: Plus batch 2

- Brought the vendored files up to 85f92f0: the bots graph cap rises to 32768 nodes with a warning when a map hits it (`gsc_graph.cpp`, `libcod.cpp`), and the Plus build prints a repeated script runtime error once and then as a count every 10 s (`scr_parser.cpp`, `plus_rules_test.py`). Server code only; the Plus version stays 123.
- Checked: `vendor_check.py`, the dedicated variants, `linux-13` and `linux-client-x64` (with the main tree's `third_party` linked in). Not built: `mingw-client` and `msvc-client`.

## 2026-11-08 — OpenCoD2-Plus Server 6219002: Plus batch

- Brought the vendored files up to 6219002 (upstream sync, Plus HTTP downloads, the antilag's anim rewind, votes across a reconnect, exact `wait`, bots floors) and added `src/server/sv_www_plus.cpp` and `.h` to `test/vendored_files.txt`. Server code only; the Plus version stays 123.
- The vendored code now calls four functions the engine lacks or keeps `static inline`: `Com_ParseRestOfLine` (upstream), `XAnimAllocInfo` and `XAnimFreeInfo` (the anim rewind, over the engine's `g_xAnimInfo` free list) and `Sys_StringToSockaddr` (the HTTP worker, Linux); `src/compat/client_server_missing.cpp` has them, with the server's bodies.
- Checked: `vendor_check.py`, the dedicated variants, `linux-13` and `linux-client-x64` (with the main tree's `third_party` linked in), the unit tests. Not built: `mingw-client` and `msvc-client` (no SDL2 here); not run: the anim rewind in the client's local server.

## 2026-11-08 — Plus downloads from the server itself

- A Plus server with `sv_wwwBaseURL auto` redirects to `plus:<name>`, as a home server does not know the address players reach it on. On a Plus connection (`cl_plusServer`, set from `connectResponse`) `CL_ParseWWWDownload` turns it into `http://<ip>:<port>/<name>` from `clc.serverAddress` (`NetHardening_PlusUrl`: the name passes `NetHardening_ValidDownloadName` as an `.iwd`, anything outside `[A-Za-z0-9._/-]` percent-encoded, a URL that does not fit `cls.downloadName` refused) and goes on through the existing redirect path; 1.3 uses the URL it reads (0x413ed2) as is. From a stock server `plus:` still fails the http(s) check.
- `scripts/www_download_test.py --plus-server PATH` runs a CoD2rev Plus server (protocol 118, Plus 123) with no local HTTP server: the client fetched the .iwd from the server's game port, byte for byte, plain and with `--disconnect`. The client without the change fell back to UDP, and the test failed on the server's missing `www:` line.
- Left open: IPv4 only, as the server; a player whose TCP leaves from another public IP than its UDP gets `404` and the UDP download (the server's debt).

## 2026-11-08 — review of the DXVK default

- From the review: `scripts/headless.sh` exits with the command's status, says why when KWin dies, and tolerates a missing `/tmp/.X11-unix`; `build_check.py` and CMake require all three files of `scripts/build_linux_vk_deps.sh`; the configure prints which client it set up, as an old build directory keeps `COD2_LINUX_DXVK=OFF` cached.
- Left as they are: two `headless.sh` runs started together may swap displays (noted in the script), and ADR 0009 stays superseded although its dependency script and `dlopen` still stand.

## 2026-11-08 — DXVK is the default Linux client

- `cod2_linux` is the DXVK client (`COD2_LINUX_DXVK` on by default, off for x64; [ADR 0017](../architecture/adr/0017-dxvk-is-the-default-linux-client.md)): `build_check.py`'s `linux`, `linux-10` and `linux-13` build it, `linux-gl` the GL one, so the runtime tests run it unchanged. Configuring without `scripts/build_linux_vk_deps.sh`'s libraries stops instead of falling back to GL. Renaming the GL client was dropped: the build directory tells them apart.
- Tested natively on Kubuntu with an RTX 5080: the whole runtime suite passes for 118 and 115, the `--listen` hit and contact tests after a rerun. `scripts/headless.sh` runs the clients in a virtual KWin, on the GPU, off the desktop.
- Left open, from `dev` before this change (same with the GL client and a GL-built server): `anim_script_test.py` (9 of 301 animations parsed differently) and the 1.0 smoke test's two flatline heartbeat checks.

## 2026-11-07 — cod2_linux full screen, settings kept

- The GL client (`cod2_linux`) always opened in a window: its device ignored `r_fullscreen`. It now switches the display to `r_mode` and covers it, as 1.3 (`r_fullscreen` read only, `0x2040`) and the real Direct3D 9 path do.
- Every start reset the profile's settings to the recommended ones (`r_mode` back to 640x480): the CPU speed came from `/proc/cpuinfo`'s current clock (0.8 and 1.6 GHz in two runs), past `Sys_HasInfoChanged`'s 10%. It is now cpufreq's `cpuinfo_max_freq`, steady as Windows' `~MHz` (`test/test_cpu_speed.py`).
- Rejected: `SDL_WINDOW_FULLSCREEN_DESKTOP` for the GL client, since a smaller `r_mode` would draw into a corner of the desktop-sized drawable.

## 2026-11-07 — a click outside a menu keeps the UI's keys; the controls menu shows its keys

- `Menus_HandleOOBClick` ended with `UI_Pause(0)` both when menus were left visible and when none were; 1.3 (0x53e786) lets go of the keys only when none is. A click whose action opens a menu from outside the focused menu's rect (zPAM's in-game bar, "Main Menu": close all, open main) left the fullscreen `main` open without `keyCatchers & 8`: nothing was drawn and the mouse did nothing until Escape. 32- and 64-bit alike, there since the reconstruction.
- Found with a diagnostic client that logged menu opens and closes and `Key_SetCatcher` (not kept). No runtime test: it needs a mod menu and a click.
- `Item_Bind_Paint` returned without drawing when the bind had no `maxPaintChars`, which the stock controls menus never set, so no key showed next to an action; 1.3 (0x5409c2) draws it whole. Left as it was: while waiting for a key ours draws `EXE_KEYWAIT` where 1.3 draws the bound keys in red.

## 2026-11-07 — x64: key bindings and enum lists in menus

- `Item_Bind_HandleKey` walked `g_bindings` at x86's 20-byte stride with `bind1`/`bind2` at +12/+16; on x64 `bind_t` is 24 bytes, so binding a key in the controls menu read command names mid-entry and wrote keys over other fields, and some binds did not take. It now indexes `g_bindings[i]`, as the other bind functions already did.
- The same leftover in `Item_HandleKey`'s `ITEM_TYPE_DVARENUM` (`dv + 0x14`, x86's `domain.enumeration.stringCount`) read inside `current` on x64: the graphics menu's mode and refresh lists did not cycle. The 32-bit clients were right. Checked by building only; no menu test exists.

## 2026-11-07 — right-aligned autowrapped menu text

- `Item_Text_AutoWrapped_Paint` ended each right-aligned line at `textalignx` plus `textRect.w`, the whole text's width, so zPAM's server info titles (`ITEM_ALIGN_RIGHT`, ending at their origin) ran over the values beside them. 1.3 (0x53f657) ends the line at `textalignx`. The term came from the initial reconstruction, so the 32-bit and 64-bit clients both had it.
- Checked against 1.3's disassembly only; no runtime test was added, as a menu check needs a mod menu and a screenshot like `hud_align_test.py`.

## 2026-11-07 — cl_vehicleInvertPitch

- For the vehicles mod (OCOD2PV), which sets it with `setClientCvar` while a player pilots a plane: 1 negates the mouse's pitch in `CL_MouseMove`. Setting `m_pitch` from the server was rejected: it is archived and would stay inverted after a disconnect in flight. The dvar has no flags, so `NetHardening_ServerMaySetCvar` lets a server set it.
- `CL_InitCGame` (new map, server or demo) and `CG_MapRestart` set it back to 0, since a map's end kills the script that would. Pitch keys are not inverted, as they are bound for walking; an already inverted `m_pitch` flies the other way.

## 2026-11-07 — Plus 123: a linked player's view lerped

- On a Plus server, cg_predict_mp.c lerps the predicted origin of a player linked in both `snap` and `nextSnap` (`PM_NORMAL_LINKED`/`PM_DEAD_LINKED`) with `frameInterpolation`, after Pmove and before `CG_TransitionPlayerState`, so weapon prediction and events stay. Pmove leaves a linked origin, so the view jumped by a snapshot's travel (a plane at 40 Hz). The lerped view is a snapshot behind `nextSnap` but matches the parent entity, which is lerped the same way; the old view ran ahead of it.
- The miss check is skipped while lerping: it would count the lerp as a miss each frame and add it to the view. `CG_InterpolatePlayerState(1)` was rejected, as it skips the weapon prediction. Client only; the Plus version stays 123, and the server's plus-protocol.md was left unchanged.
- Left open: the first frame of a link and of an unlink still step once (not linked in both snapshots). No automated test; a flight in the vehicles mod (sv_fps 40, 250 fps, x64 client) was smooth.

## 2026-11-07 — a language with no files of its own

- For zPAM Plus's translations (`localizedstrings/zpam.str`): on an English install, `loc_language` 1–4 (French to Spanish) now keeps the `localized_english` files in use instead of falling back to English. Sounds stay English, and the chosen language's `.str` lines apply; the stock `.str` files already carry Spanish. A real localized install is unchanged. `language_test.py` fails on the old client.
- Russian and Polish were left out, as the stock fonts only have Windows-1252 glyphs. No menu entry was added: the stock MP menus have no language list, and zPAM can offer one with `uiScript updateLanguage` (it sets `loc_language`, then `vid_restart`).

## 2026-11-07 — OpenCoD2-Plus Server 1551c01: the antilag rewinds the pose

- Brought the vendored files up to 1551c01: in `COD2_PLUS`, `G_PlusRecordPose` keeps each player's pose per frame and the antilag sets it with the origin (the lean, yaw, aim and stance flags of the hitboxes stayed in the present). Server code only; the Plus version stays 123.
- Done on a temporary branch in a separate worktree, as another session had `ui-bind-x64` checked out; `scripts/vendor_check.py` passes, the dedicated server builds and the unit tests were run.

## 2026-11-07 — OpenCoD2-Plus Server 53281a5: the archive at 62.5 Hz

- Brought the vendored files up to 53281a5: `SV_GetCachedSnapshot` counts whole frames, so the antilag and the killcam go back the time asked at `sv_fps` 60 (960 ms a second before). Server code only; the Plus version stays 123.
- Done on a temporary branch in a separate worktree, as `dev` was checked out in the main tree; `scripts/vendor_check.py` passes, the dedicated server builds and the unit tests were run.

## 2026-11-07 — OpenCoD2-Plus Server 6156d66: the server sets the rate

- Brought the vendored files up to 6156d66: `SV_PlusRate` gives Plus clients a snapshot every frame and 1,000 bytes a second per Hz, read per message, and `sv_lanForceRate` (Plus only) lets a LAN client count as one on the internet. No client change: its `rate` and `snaps` are ignored by a Plus server, and the Plus version stays 123.
- Done in a separate worktree on `dev`, as another session had `client-language-fallback` checked out; `scripts/vendor_check.py` passes, the builds but msvc-client (a shared folder) and the unit tests were run.

## 2026-11-07 — OpenCoD2-Plus Server 107f574: turrets at any sv_fps

- Brought the vendored files up to 107f574: in the Plus build, turrets and `trigger_hurt` step by `level.frameTime` (a manned MG fired twice as fast at 40 Hz), and `plus_rules_test.py --fps`/`bots_test.py --fps`. Server code only; the Plus version stays 123 and the client predicts nothing new.
- `scripts/vendor_check.py` passes; the build and unit tests were run.

## 2026-11-07 — OpenCoD2-Plus Server ebb8ee0: fixed shotgun spread

- Brought the vendored files up to ebb8ee0: `g_shotgun_spread_fix` (ae124d3) in `G_BulletFireSpread`, and `plus_rules_test.py`'s check for it. The cgame traces no pellets, so only the local server has something to run; the Plus version stays 123.
- `scripts/vendor_check.py` passes; the build and unit tests were run. The check itself ran against the dedicated server only.

## 2026-11-07 — Plus 123: the climbing sound for everyone

- The maintainer chose that everyone hears it: on a Plus server cg_event_mp.c plays `weap_raise` for an `EV_RAISE_WEAPON` of weapon 0, which is what the climber's own cgame plays with its old weapon. It is a client-only change, with no pmove or server change. `climb_sound_test.py` (with the new `COD2_TRACE_SOUND`) fails without the change. The silent climb that zPAM describes (weapon held down) was not checked.
- Review follow-ups (OpenCoD2-Plus Server 1aa8f15): the switch rule is limited to bolt-actions, the turret check uses a standing box, and `prediction_test.py` leaves out the spawn's miss by its shape, so it allows none.

## 2026-11-07 — Plus 123: zPAM's gameplay fixes in the engine

- zPAM Plus fixed fast reload, prone peek and MG clip from GSC, against the client's prediction or with a client dvar. OpenCoD2-Plus Server (b6c6816, 27c1045, d991a33) now applies the fixes in bgame and game code to Plus clients only, and the engine predicts the first two with `pmove_t.plusRules`. The client sets that field from the snapshot's message byte, so it predicts stock against stock servers and in demos. Plus version 123; zPAM turns its fixes off at `sv_plus >= 123`.
- `prediction_test.py` fails on predicted weapon state or lean that the server's disagrees with. Each check was run against a client with its rule turned off, to see it fail. `plus_rules_test.py` (vendored) fails on every build without the rules.
- Left open: the climbing sound. Stock weapon files leave `raiseSound` empty, so the cgame plays `weap_raise` for `EV_RAISE_WEAPON`. Going onto a ladder or a mantle raises `WP_NONE`: the climber's cgame plays that event with its old weapon and hears `weap_raise`, while others get weapon 0 and hear nothing. `anim_script_test.py` also fails on `dev` (9 animations 1 ms shorter in the engine since the SSE2 change).

## 2026-11-05 — SSE2 float math; OpenCoD2-Plus Server ae1c84a

- Every x86 and x64 build computes float math with `-msse2 -mfpmath=sse` (ADR 0016), as the server's ADR 0017: the 1.3 client ran x87 at 24 bits, which single-precision SSE matches, while our 32-bit builds ran it at 64 and the MSVC-ABI one already on SSE2. x87 is left in the i386 float return, `long double` code and the static libraries.
- The vendored server came up 40 commits (72c9f3e to ae1c84a: ADS for linked players, signed challenges, a faster VM, Huffman from ioquake3). Its bound on the Huffman output (e103ff1) gave `MSG_WriteBitsCompress` a fourth parameter: the engine's takes it too and its client drops a command or skips a demo message that would overrun its buffer.

## 2026-11-05 — the sight no longer shakes as it moves in and out

- Aiming down the sight shook every weapon, the Bren most: the engine's weapon loader timed the sight with fireTime and rechamberTime instead of adsTransInTime and adsTransOutTime (1.3 0x51b580, +0x2c0/+0x2c4), so the client predicted it faster than the server's bgame and was pulled back each snapshot. Found playing a local game with `COD2_TRACE_PREDDIFF=all`, a mode that prints any playerstate field predicted otherwise.
- `prediction_test.py` aims down the sight and back twice and fails on a sight fraction the server disagrees with; a 0.3 s transition is 300 ms in the engine and 299 in a server built for x87 (0.0006 a frame), which it tolerates.

## 2026-11-05 — review follow-ups of the MSVC-ABI work

- The MSVC pthread shim passed cdecl key destructors to FLS, which calls them stdcall (now an NTAPI trampoline), and kept unjoined threads' slots forever; zlib allocates through the engine as on MinGW (`MY_ZCALLOC`); the link aliases join only data.
- `build_check.py msvc-client` fails on a global defined both as C and as C++ (two copies the Itanium ABI would join); none today. The Windows scripts no longer prune a folder they did not make nor end the user's own game.
- Left open: 41 dead `!COD2_PLUS_SCRIPT` branches in the engine and no ABI asserts for the script module (`docs/roadmap/debt.md`).

## 2026-11-05 — MSVC ABI and web on the bridge; the engine's server is gone

- The MSVC client is now the MinGW branch built by clang for `i686-pc-windows-msvc` with lld-link (ADR 0015): cl.exe could not compile the bridge's `__asm__` labels. The MSVC ABI mangles C++ globals, so 127 `/alternatename` aliases are generated at link; it also exposed a reconstruction bug gcc's frames hid (`r_staticmodelcache.c` called the C color converter as stdcall). `msvc_run_test.py` plays a local game with it on Windows.
- The web build compiles and links again (Emscripten 6.0.13) and runs its local games on the bridge, without libcod; wasm traps calls through another signature, so three script exports take 1.3's trailing argument. 65 symbols still link undefined (`CG_Init` among them): no browser check runs it.
- With no build left on them, `src/PC/server_mp`, `game_mp`, `game`, `script` and `cm_*` were removed; the bridge's engine-side ABI files are kept by hand (1.3's ABI), and the generator refuses a crossing without its line.

## 2026-11-05 — the MSVC client builds again

- VS 2019 Build Tools (cl 19.29) stopped on two things: `COD2_ALIGNED` expanded to nothing under MSVC, so `GfxCodeMatrix` was 260 bytes instead of 272 and `backEnd` missed 1.3's 0x36e90 (the size assert fired); it is now `__declspec(align)`, placed between `struct` and the name, where gcc takes it too. The bgame files written for C++ used `true`/`false`, which MSVC's C build lacked.
- With the official SDL2 2.30.9 VC package in `third_party/`, `cod2_win32.exe` links and reaches the main menu on Windows (silent, as the Mac stubs). VS 2019's CMake 3.20 cannot read the v3 presets: configure with the preset's cache variables by hand. Still unchecked by the scripts.

## 2026-11-05 — JPEG loading per ABI

- `R_LoadJpg` passed libjpeg 0x1b4 as `jpeg_decompress_struct`'s size; the vendored jpeg-6 wants 464 on both 32-bit ABIs and 632 on 64-bit, so loading a raw `.jpg` material hit "JPEG parameter struct mismatch". Its field offsets, `FS_ReadFile` and the temp alloc were also read at x86 offsets; the dead allocator hooks at +0x18/+0x1c are gone.
- `test/test_jpeg_layout.py` compiles jpeg-6 for Linux i386/x64 and MinGW 32/64 and checks every offset `r_jpeg.c` uses, compress side included.

## 2026-11-04 — red hurt overlay, shellshock without sepia, water on x64

- The low health overlay returned unless `cg_drawHealth` (0 by default, as 1.3) was on; 1.3's overlay (0x4c5250) never reads it, only the health bar does. Only the breathing sound told the player they were hurt.
- The shellshock sepia came from the previous stage's fix: 1.3 keeps alpha in the vertex color's top byte, but this engine's `R_ConvertColorToBytes` packs it in byte 0, so the fade landed in blue at alpha 255; the fade is back in byte 0 and the view blurs and recovers.
- `Material_Marshal32To64` left TS_WATER_MAP water defs NULL, so the Windows x64 client crashed drawing water; the 32-byte file def becomes a water setup as on x86. Verified by the user on Windows.

## 2026-11-04 — Windows client polish: screenshots, chat, scoreboard, corpses, shellshock

- `cg.clientNum` held the gamestate's message sequence since the initial commit: `CL_InitCGame` passed `CG_Init`'s first and third arguments swapped (1.3 0x402c73), so the scoreboard highlighted another player. Chat drew at scale 1/3 over a 10-unit line step (1.3: 5/24, 0x4c7760); a new corpse cloned its player's anim tree on EF_PRONE instead of EF_BODY_START (0x4df227). `R_SaveJpg` passed Linux i386's `jpeg_compress_struct` size (372) on Win32 (376) and 64-bit (520), so `screenshot` exited the Windows clients.
- Shellshock's saved-screen blend and late blur put the fade in the blue byte with alpha 255 and used the Mac port's power-of-two, flipped texcoords (1.3 gfx_d3d 0x10031180, 0x10030b70: alpha byte, 0..1); the saved screen aliased rt1 instead of its own `$savedscreen` (0x100234a9), and the glow had the same texcoord error. Ghosting and streaks are gone.
- Left open: under shellshock the view turns sepia and darkens on Linux vulkan and on Windows alike (the 2D quad's vertex alpha reaches the blue channel somewhere); the red damage overlay does not show; frame-rate drops not investigated.

## 2026-11-03 — the 64-bit client draws effects and lights the world as 32-bit

- Effects were skipped on x64; they failed on 1.3's 4-byte types, not on the vtables the skip blamed: `Effect`'s vptr, `UInt32`/`SInt32`/`UINT4`/GL integers as `long` (8 bytes on LP64, so `irand` ran a 64-bit LCG), x86 offsets into `PrimitiveTemplate`, `Particle`, `GfxEntity`, `trace_t` and `ScheduledEffect`, `int` effect pointers in `cg_view_mp.c` and `g_missile_mp.c`, `fx_freeze` read at byte 8, and blobs sized for 4-byte pointers (`effectListArray*`, `s_backEndData`, `modelDObjBuf`). `fx_test.py` passes on x64 under ASan; each rename kept the 32-bit code identical (objdump) except where a type change reorders registers.
- Fixed on both builds: `CG_ImpactMark` passed one axis where `R_MarkFragments` reads three (decals took their texture axes from the stack), and `R_UpdateXModelBounds` tested the effect's XModel as a DObj instead of the shared one it had just set up.
- The yellow world: `R_InterpretSunLightParseParamsIntoLights` wrote `GfxLight` at x86 offsets, so the sun's blue was the ambient's; eye-level views now equal 32-bit's. Left: translucent effects sort by material address, so overlaps can blend in another order than on 32-bit; Win64 still untested on real Windows.

## 2026-11-03 — stock 1.3 as a build option

- `COD2_PLUS_PROTOCOL` (ON) decides whether the 1.3 builds define `COD2_PLUS`, as OpenCoD2-Plus Server's `make PLUS=1`: OFF gives a dedicated and local server any 1.3 player can join, and a client that does not announce itself as Plus (its connect key, notice and Plus messages are under `COD2_PLUS` too, since only that build's message code decodes them). `build_check.py linux-13` builds it; `opencod2_smoke.py --stock` passes it whole without `--plus`.
- Rejected for now: one server that speaks stock 1.3 and Plus to different players in a match; it would start in OpenCoD2-Plus Server, where `PLUS=1` is all or nothing.

## 2026-11-03 — two branches: main and dev

- `plus` was where development happened and `main` took critical fixes only; now `dev` (from `plus`) is the development branch and `main` the stable one, which takes `dev` when its changes are stable. `plus` is gone from GitHub; `main`'s two commits beyond it were already in `dev` by cherry-pick (the compass's objectives, the shellshock sounds), so the first `dev` into `main` is a merge.

## 2026-11-03 — the server is named OpenCoD2-Plus Server

- CoD2rev-Plus renamed itself OpenCoD2-Plus Server (its ADR 0016; repository `JotaJazz/OpenCoD2-Plus-Server`), and its ADR says this repository follows in its own documents: the living docs and ADRs use the new name, the READMEs say what it was called; past memory stages, commits and code comments keep the old one.
- The banner and `src/win32/opencod2.ico` (from `docs/icon.svg`) take the server's new emblem, a faceted star over "2+" in a roundel, so the two read as one family; the READMEs gained the server's architecture badge and "1.3 Plus".

## 2026-11-02 — the 64-bit Windows client loads a map

- `cod2_win32.exe` cross-builds for x86_64 with MinGW-w64 (`scripts/mingw64_deps.sh`, no root; `build_check.py mingw-client-x64`) on the same blobs and bridge as the Linux x64 client. Win64 is LLP64: casts through `unsigned long` and `HPROVIDER` truncated, PE x64 has no `_` on C names (link aliases, `CDirect3DDevice`'s flags), and MinGW defines `_M_X64`, so guards meant for MSVC x64 took the client off the Mac port's stubs and the real `MessageBoxA` waited for a click.
- Under a 64-bit Wine (`scripts/portable_wine.sh DIR 64`) the menu equals the 32-bit client's pixel for pixel and `mp_toujane` loads and quits. CoD2rev-Plus's VM needed four LLP64 fixes upstream (f33a05e, 72c9f3e: function and animation references left 4 bytes for a pointer); the renderer's code samplers read the world, lightmaps and dvars at 1.3's offsets.
- Open: the view is closer and bluer than 32-bit, no effect draws (as on Linux x64), and libcod is left out (`gsc_memory.cpp` keeps pointers in `long`).

## 2026-11-02 — CoD2rev-Plus up to 72c9f3e

- bdb0bfb brought the 2026-11-02 audit's server hardening; then CoD2rev-Plus made the Plus server a build variant (`make PLUS=1`, `COD2_PLUS`). The 1.3 builds here define `COD2_PLUS` (engine and 64-bit server) and `opencod2_smoke.py` passes `--plus` for 118; without it the client's local server would speak stock 1.3 again.
- 1.0 builds lose CoD2rev-Plus's `msg_plusSnapshotTime`; the client keeps its own there. Smoke 118 and 115 pass with their lists.

## 2026-11-02 — the console says "1.3 Plus"

- A Plus server is reported as "CoD2 1.3 Plus server, version 122" ([ADR 0014](../architecture/adr/0014-name-1-3-plus.md)); `hostile_server_test.py` expects the new line. The vendored server's update notice changes at the next sync (CoD2rev-Plus `bdb0bfb`).

## 2026-11-02 — the name "1.3 Plus"

- The maintainer wanted players to see the client as a patch after 1.3, as CoD2x does with 1.4. "1.4" was rejected: CoD2x uses it, and zk_libcod's `getProtocolString()` already returns it for Game Pass. Players read "1.3 Plus" ([ADR 0014](../architecture/adr/0014-name-1-3-plus.md)); `shortversion` "1.3" and the Plus version stay.
- Left open: renaming the console line "CoD2 Plus server, version".

## 2026-11-02 — the 64-bit Linux client loads a map

- `cod2_linux` builds for x86_64 (`-DCOD2_X64=ON`, `build_check.py linux-client-x64`) and runs CoD2rev-Plus's VM and server through the bridge, which now records each function's x64 ABI beside the x86 one; the engine's own VM kept values in 4 bytes and ran out of variables. The menu equals the 32-bit client's pixel for pixel; `mp_toujane` loads, plays and quits, and the 32-bit client reads the profile it writes.
- AddressSanitizer found the rest: 1.3's sizes for structs that hold pointers (FX media lists, `centity_t`, XAnim trees, occluders), `COD2_ALIGNED` off on x64 (it sat with the calling conventions), `DWORD` as an 8-byte `unsigned long`, and a viewport table one entry short on every build (`PINGPONG_1`).
- Open: the world draws yellower and `mp_toujane`'s smoke is missing; under Xvfb an X-root grab of an in-game frame shows a 2x zoom on both 32- and 64-bit builds while the engine's screenshot does not, so compare screenshots, not grabs.

## 2026-11-02 — review of the water branch

- Without a scene copy (antialiasing, no INTZ) `sceneColorSampler` read `renderTargets[R_RENDERTARGET_NONE]`, past the 13 targets: 1.3's `techFlags & 2` skip never fires for a sampler bound by name (only explicit arguments set it, `r_material_load_obj.c`). It now gets black; checked with `r_aaSamples 4` on `cod2_linux_vk` (water drawn, no crash).
- `floatZ` binds the INTZ depth only for water materials (the only ones that stop writing it); the copy runs only into the frame buffer and marks `resolvedScene` only when `StretchRect` succeeds.
- Rejected from the review: that the water technique would be skipped without the copy (the flag is not set, see above).

## 2026-11-02 — water compiles as ps_2_a

- On the user's RTX 5080 Microsoft's `d3dx9_43` refused the refracting water: 91 arithmetic slots, past `ps_2_0`'s 64, so `water_duhoc` loaded without a shader and the seabed's grass showed instead. `r_water`'s water pixel shaders now compile as `ps_2_a` (512 slots).
- The same `d3dx9_43.dll` and `d3dcompiler_43.dll`, copied from Windows and run under Wine, compiled it as `ps_2_0` without an error: that check does not prove the budget. Open why.

## 2026-11-02 — water refracts

- With `r_water` and INTZ, `RB_EndSurface` copies the scene drawn so far into `resolvedScene` (`renderTargets[2]`; 1.3 fills it only in `RB_ApplyLatePostEffectsCmd`) before the first water surface of each draw list, and the shader samples it as `sceneColorSampler`: the bottom shows under the water, bent by the waves, clear at the shore and the water's color 160 units down. The water is then opaque; the alpha fade stays for cards without INTZ, which get no copy.
- A bent sample that lands on something in front of the water (its depth nearer than the water's) takes the straight one, so nothing above the water is pulled below it.
- Checked on `cod2_linux_vk` in `sps_island` from the boat's deck: a debug colour showed depth on DXVK (clear only near the coast: that sea is deeper than 160), and the shore's seabed now shows through. Under Wine's own `d3d9` there is no INTZ (no foam either), so the Windows client can only be checked on the user's card.

## 2026-11-02 — script brush models draw as brush models

- `CG_ScriptMover` and `CG_EntityEffects` told a brush model by `constantLight == 0xffffff`; 1.3 tests `solid` (`centity_t` `+0x18c`, `0x4dea5b` and `0x4cc240`), where the server puts the marker. A `script_brushmodel` without a light fell into the xmodel branch and drew `gameModels[index]` at its origin: in `sps_island` the boats (`barco3`) became a huge stretched hill over the scene, on every renderer; their looping sounds played from the origin instead of the model's middle.
- Found under Wine on Xvfb at the user's spot: `r_drawEntities 0` and deleting `barco3` by script removed the hill, a trace showed no brush model added and an xmodel at `0 0 0`. After the fix the picture matches 1.3's (1.3 % of pixels differ, the trees' sway; 48 % before).
- No automated check: it needs the user's map. The water work was not the cause (70f3e91 shows it too).

## 2026-11-02 — water knows how deep it is

- With `r_water`, no antialiasing and INTZ, the frame buffer's depth is an INTZ texture bound to `floatZ` (`NULL` in 1.3 on PC). The water's shader turns it into the distance behind each pixel without the projection matrix: with the infinite far plane `d = 1 - near / z`, so `near` comes from the water's own depth. Shallow water fades and foams; water stops writing depth while it reads it.
- The first FOURCC was `INZT`, which every card refuses; a debug colour of the shader's values (`sceneD > 0`, `near`) showed it, and that `near` matched `r_znear` 4.
- Refraction is next on this base; the fade only shows where the material blends (`water_duhoc`, sort 27), not on 1.3's opaque water such as `mp_harbor`'s.

## 2026-11-02 — water reflects the map's sky

- The map loader marked the sky by the material's `surfaceFlags` where 1.3 reads `gameFlags` (`gfx_d3d_mp_x86_s.dll` `0x10002cb0`): no map had sky surfaces, `world->skyImage` stayed empty and the visibility pass never added the sky past the far plane. Fixed; `client_view_test.py` and `shader_compile_test.py` still pass.
- `r_water`'s reflection now samples that sky through `skySampler`, a default code sampler added to the table (bound by name in the HLSL, like `gameTime`), instead of the material's generic `case64blue` cube, whose alpha stays the reflectivity. `sampler.sky` without a sky now gives `$watercolor` instead of 1.3's error, which no stock material reaches.
- Shore fade and refraction wait for a depth texture: 1.3 on PC never makes one (`floatZ` binds `NULL`), and refraction alone would show the void under maps whose water was always opaque.

## 2026-11-02 — water that moves

- Nothing called `RB_UploadWaterTexture`, the FFT that animates a water material's height map, and it read `r_drawWater` and an upload flag through two empty stubs (`g_unknown_195f22c/230`, from the Mac port): every water surface was still and flat. It now runs where 1.3 binds the map (`gfx_d3d_mp_x86_s.dll` `0x1003a2db`); the flag, which nothing read, is gone.
- `r_water 1`, ours, replaces the game's `lib/water.hlsl` in the include handler of the real Direct3D 9 path (`D3DXShader.c`, not `r_material_load_obj.c`'s text cache, which that path does not read): two drifting layers, fresnel, sharper sun. 1.3's own shader samples fixed coordinates and blends the reflection by the cube map's alpha.
- Checked on `cod2_linux_vk` in `sps_island` (the user's map, `water_duhoc`): still before, waves with 1.3's shader after the fix, and ours. No automated check yet (`debt.md`): in `mp_harbor` its snow and a slightly different turn each run hide the waves' part of the motion.

## 2026-11-02 — cl_hideServerIp

- CoD2x's `cl_hideServerIp`, written here: every text the renderer draws passes through `R_AddCmdDrawTextWithCursor` (the console's lines too, through `R_DrawConsoleText`), which masks the digits of IPv4 addresses on its own copy (`r_hide_address.h`, `test/test_hide_address.py`). One place covers the scoreboard, console, menus and edit fields, and the server browser keeps joining with the real string, the reason the option had been deferred.
- Not done: CoD2x's server-side `sv_hideServerIp`, which would force it on clients (CoD2rev-Plus's), and masking host names. CoD2x's shotgun pattern, HWID and match system stay out: they are server-side, in CoD2rev-Plus; HWID is a hardware fingerprint a client can forge, and CoD2rev-Plus already bans by the CD key's GUID.
- Checked in a game on a local server: the scoreboard's footer drew `***.*.*.*:*****` for `127.0.0.1:28930`, fps and the clock untouched. `TEXTCMD_DBG` now picks texts by what was asked and prints what is drawn. The review found an address after a dot (`Connecting...`, a scrolled edit field) or a color code left visible; fixed.

## 2026-11-02 — prediction measured against 1.3's own client

- `prediction_test.py --wine` runs 1.3's `CoD2MP_s.exe` on the same moves, map and server (`main`'s dedicated: `plus`'s refuses stock clients). Misses, ours against 1.3: 5–11 against 10–11 at 20 fps (two runs gave the same 10 misses, 13.1 in total); 18, 18 against 18, 20 with 40 ms of ping (largest 4.1 in both); 0, 0 against 1, 0 at 60 fps. Prediction is 1.3's: the earlier online gap (53 against 26 on Titan) came from two different sessions; one session each on one server and map gave 1.3 8 misses (largest 13.8), ours 5 (largest 6.5).
- 1.3 under Wine needs its DLLs off DrvFs (`mss32.dll`'s entry faults when mapped from `/mnt/e`), a base dir without `main/players` (else it takes the user's profile, here with `com_hunkMegs 512`, out of memory) and a profile with `com_introPlayed 1`.
- Our session crashed at the end of a shellshock: `cg_shellshock.c` declared the listener origin as an `int` and passed its value, a NULL origin, which `SND_StartAliasStreamOnChannel` reads once a shellshock sound is a stream. 1.3 passes `vec3_origin` (`0x59bd2c`). `view_weapon_test.py`, whose mod shellshocks, crashed the same way; the runtime suite had not been run since sound went through SDL2.

## 2026-11-02 — 4096 dvars and a freeze report

- The client's dvar pool goes from 1280 to 4096, as CoD2x's does; the dedicated server keeps CoD2rev-Plus's limit. The command buffer goes from 64 KB to 512 KB: `exec` has to hold a whole file, and past 64 KB `config_mp.cfg` was dropped, then rewritten from defaults, which more dvars reach. The review of this change found that.
- `com_freezeWatch` is ours, in the crash handler, for Windows only: a thread that sees no frame for that many seconds writes `cod2_freeze_<pid>.txt`. It copies the main thread's context and lets it go before writing anything, as a thread suspended on the heap's lock would turn a pause into a hang.
- With many iwds the server's systeminfo drops one list of a pair, and the client ended the session with a count mismatch, a local game over Titan's iwds included. It now warns and takes the shorter list. CoD2rev-Plus's own `iwd sum/name mismatch` is untouched.

## 2026-11-02 — the compass shows objectives

- `CG_PLAYER_COMPASS_POINTERS` only turned the compass: 1.3's owner draw (`0x4c5620`) was never ported, so no player saw a bomb site. `CG_DrawCompassObjectives` follows it, and takes the owner draw's `rect`, so the compass offsets hold. Found by the review of CoD2x's options, confirmed by the user in Search and Destroy.
- The review against the binary caught two errors of this port that the first test passed: `0x15cf994` is `infoValid`, not the team, and the icons are `CS_SHADERS`, 1566. `hud_options_test.py` now plays without a team and on one, and demands the icon's white. `entNum` is bounded here, as it comes from the server.
- Left as it is: 1.3's friendlies (`0x4dafe0`) read `cg_hudObjectiveMaxRange` and `MinAlpha` too, and our port does not.

## 2026-11-02 — CoD2x's client options, in the engine

- `sensitivity_MG`, `cg_drawCompass`, the compass offsets, `cg_drawSpectatedPlayerName`, `com_writeConfig`, and `cl_maxpackets` up to 125. CoD2x's repository has no license, so nothing is copied; the names stay so that players' configs work.
- `m_rinput` was rejected: SDL 2.30 reads Raw Input in relative mode. `snaps` 40 too: the server caps it at 30, and `g_forceSnaps` decides at 40 Hz. `cl_hideServerIp` waits: the address is printed and shown in several places, and hiding only some would promise privacy it does not give.
- `sensitivity_MG` and the spectated name were not run: no test puts a player on a turret or in spectate.

## 2026-11-01 — Miles retired from the Windows client

- `src/snd_sdl/` is now the only sound engine on Windows: `mss32_dll.c`, `snd_backend` and the `__stdcall` table are gone, and `AIL_*` are plain functions, as on Linux. The user heard no difference in real games. Going back means reverting this commit; `plus` only, `main` keeps Miles.
- The crash with `snd_khz 22` went with `mss32.dll`. `sound_test.py --wine` runs without the DLL in the game's folder, and a client without this change is silent there.
- `src/PC/win32/mss32shim/`, a debug interposer nothing linked, went too. Reverb by ear stays open, now with nothing left to compare against but a build of `main`.

## 2026-11-01 — Plus snapshots from the client's own server

- A local game with moving players died on `Illegible server message 3`: the client announces `plus 122` to its own server, which marks the snapshot as Plus, but it writes through `src/PC/qcommon/msg_mp.c`, which had only the reader. The engine's writer now has the offsets too, which also covers a remote Plus client on a listen server.
- The reader now clears `msg_plusSnapshotTime` after each snapshot: client and local server share it, and a stale value would have written a gamestate with offsets. `sound_test.py`'s `callback_cost` found this, as the only local case with moving bots.
- The VM's `sv_fps` reads `Dvar_FindVar` from the server's bridge, so the script module takes it as provided.

## 2026-11-01 — reverb: a bus of our own, and 1.3's conditions

- `src/snd_sdl/snd_reverb.c` is a Freeverb-style bus with the public EAX presets, the one thing that only resembles Miles. Its input is at half of one over the combs, as the tail ran about 8 dB hot; underwater is doubled to keep `shock/default.shock`, the only stock multiplayer case, at the level it had. No value of Miles' table is copied but generic's zero. Tuning by ear stays open.
- The sound channels' reverb conditions in `snd_driver.c` are now 1.3's. That file is shared, so this changes the sound of the default Miles client too, not only of ours.

## 2026-11-01 — Windows sound: Miles or SDL2

- `snd_backend sdl` runs the Windows client on `src/snd_sdl/`, without loading `mss32.dll`; `miles` stays the default until the user has compared them in real games. It gives this client sound under Wine for the first time: every `sound_test.py` case passes there.
- The calls cross `__stdcall`, so a wrong size wrecks the caller's stack silently: `AIL_startup` is `@0` although the driver passes it flags. The test reads the sizes from a MinGW object rather than trusting the table.
- `snd_backend` is registered before the first `AIL_*` call, not beside `mss_3d_provider`, and read once. Only the command line sets it, against the archived flags first written, as a launcher without it silently inherited SDL from the config.

## 2026-11-01 — sound on Linux through SDL2

- The Linux client's `AIL_*` calls land in our own engine, `src/snd_sdl/` ([ADR 0012](../architecture/adr/0012-sound-engine-behind-the-ail-calls.md)); `scripts/sound_test.py` reads what reaches SDL's disk driver.
- `snd_driver.c` changed in two places: it hands over the file's length, and it closes a stream it does not start, which would have outlived `FS_Shutdown`. Miles' MP3 length is an estimate from the bitrate too, so ours stays.
- The provider refuses stereo and ADPCM on 3D channels, as read from `msssoft.m3d`; reverb is silent until its bus exists.

## 2026-11-01 — Plus snapshots, version 122

- CoD2rev-Plus sends `pos.trTime` as an offset from the snapshot's time, in a message of its own, `svc_snapshotPlus` (8), so the client reads it with no state from the handshake: demos carry the format, and neither stock servers nor the local game, which has its own server code, change. The version is 122.
- Vendored `fa4da34` with `src/qcommon/plus_format.h`. Against it: 10 prediction misses and 1 animation miss, as before, the view test passes, and the client joins at 40 Hz and a 1.3 server of `master`.
- Left open: a real match at 40 Hz, and the original 1.3 executable as a server with this build (the handshake build joined it).

## 2026-11-01 — the client speaks the Plus handshake

- CoD2rev-Plus's `plus` branch refuses a 1.3 client without the Plus version, so the connect packet carries `plus 121`, in that packet only, and a `connectResponse 121` is recognised. A bare answer stays a stock server; the format does not depend on it yet, so nothing is stored. Joined CoD2rev-Plus `0cf6e01` at 20 and 40 Hz.
- The vendored code stays on `642bc91`: the handshake is client code, and the `plus` branch was still moving. A client's local game does not pass through the check.
- At 40 Hz the client needs no change: `g_forceSnaps` and `g_forceRate` act on the server, 0 misses against 11 at 20 Hz, and its clock settles tighter.

## 2026-11-01 — final review of plus before main

- `r_gfxApi vulkan` died at every start on a device without Vulkan 1.3, since DXVK throws out of `Direct3DCreate9` and the menu saves the choice. The client now asks Vulkan first, and falls back when the layer cannot make a device.
- `R_RecoverLostDevice` handed a real device the fake device's layout: a bad window and flags on any lost device. It now resets with what the device was made with.
- A module named `d3d9.dll` stays loaded, so going back from DXVK reached DXVK again: after DXVK the system's is loaded by its path. `r_gfxApi` applies at `vid_restart`, not once per process as first recorded.

## 2026-10-31 — the console, the lagometer and cg_drawFPS against 1.3

- The console opened full because the key code read `keys['`'].binding`, always set; 1.3 (`0x40bc6e`) tests `keys[K_SHIFT].down`: only Shift gives the full console.
- The lagometer drew every sample at `x = -7`, one column over its black background; 1.3 draws `-7 - i`, clamps the snapshot bars to 24 and colours them by the snapshot's flag.
- `colorWhiteFaded` and its two siblings were zero, so `cg_drawFPS` and the other developer overlays drew with alpha 0: the values are in 1.3's data (`0x59e08c`, `0x59e09c`, `0x59e0ac`).

## 2026-10-31 — the console key, r_swapInterval and the API in the menu

- SDL gives a key by what it types, so a Spanish layout never sent `` ` `` and the console could not open: that key now goes by position, as 1.3's scancode table.
- The real device always asked for the default present interval, which waits for the display: `r_swapInterval` did nothing and `com_maxfps` stopped at the refresh rate.
- The menu's rendering method item picks `r_gfxApi`, patched when the file loads: a pure server refuses a menu in an iwd of ours. `d3d12` is not offered: a black screen cannot be left from the menu.

## 2026-10-31 — d3d12 is black on the screen, not in the back buffer

- A black menu screenshot first pointed at rendering. A readback through a temporary tracing client showed otherwise on an RTX 5080: no refused call, a real frame at frame 30, a clear and a triangle at 150. Frame 120 reads black on Wine too, so one sample proves little: read several.
- Replacing the device's vtable with a 119-entry copy crashes inside `d3d9.dll`; patch the table in place. `d3d12` stays experimental, `vulkan` covers the same machines.

## 2026-10-31 — r_gfxApi: Vulkan and Direct3D 12 behind Direct3D 9

- The renderer talks only to `IDirect3DDevice9`, so the Windows client picks what stands behind it: `d3d9`, `vulkan` (DXVK) or `d3d12` (D3D9On12), falling back to `d3d9`. A native backend per API was rejected: the renderer is 38,000 lines.
- Wine loads its builtin `d3d9` even from a full path: testing DXVK there needs `WINEDLLOVERRIDES=d3d9=n`. A log line saying "Vulkan" proved nothing until then.
- DXVK ran the 32-bit process out of its 2 GB on lavapipe, so the executable is large address aware.

## 2026-10-31 — vendor sync 9e26dd4, a 64-bit dedicated server, an icon

- CoD2rev-Plus moved 65 commits: its `netadr_t` grew with `altPort`, which the engine shares by value and inside `netchan_t`, so `altPort` now overlays `ipx[9]` and the struct stays 20 bytes. Growing the engine's was rejected: its layouts are pinned to 1.3. Its removed `Sys_BeginStreamedFile`/`Sys_StreamedRead` were an `FS_Read`; cinematics call that.
- `cmake/lnxded64` builds the dedicated server at 64 bits from CoD2rev-Plus's sources alone. The engine's `COD2_X64` no longer compiles as C++ and stays a known gap: ADR 0010.
- `docs/icon.svg` and `src/win32/opencod2.ico` go in both MinGW executables; SDL takes the client's window icon from there. The Linux client has none.

## 2026-10-31 — the client's clock stays in step

- `CL_SetCGameTime` marked `extrapolatedSnapshot` with its comparison inverted (1.3 `0x4031a5` is `>=`): every snapshot took 2 ms off the clock until it was 100 ms behind, then `CL_AdjustTimeDelta` averaged it forward, a visible jump about every 2.3 seconds, in every online game since the reconstruction. `hostile_server_test.py client_clock_steady` went from 13 jumps in 30 s to 2 at the start. The earlier online prediction gap against 1.3 may have come from this; it was not measured again.

## 2026-10-31 — sessions no longer end at cl_timeout

- `CL_CheckTimeout`'s path in `CL_Frame` measured from `connectTime`, not `lastPacketTime` as 1.3 (`0x40f480`): every online session ended "timed out" 200 seconds after connecting, however well the network ran, which also explains why every traced session lasted about 3 minutes. `hostile_server_test.py` has `timeout_keeps_playing` and `timeout_when_silent`.

## 2026-10-31 — the filter let the server's physics through again

- The filter refused every CHEAT dvar, so a server's `jump_height`, `friction`, `player_*` and `bg_*` never reached the client, whose prediction then ran other physics: seen as stutter on a public server, whose log listed 52 refused dvars. CODINFO and cheat dvars pass now; `hostile_server_test.py` has `codinfo_dvars_apply`.

## 2026-10-31 — audit: a server is not trusted; checks that fail; dead sources removed

- `COD2_FEATURE_NET_HARDENING` was off in every build. A server could run console commands through `activeAction`, write outside the game's folder over HTTP and crash the client in a dozen ways; `scripts/hostile_server_test.py` reproduced them on a real client before the fixes. The dvar rule lets SYSTEMINFO dvars through: blocking `fs_` and `sv_` outright, as the old code did, would have broken mod servers.
- `build_check.py` now fails on a skip, a crashed script and bridge drift, and runs `layout_parity` and `check_c_linkage`; `check_var_sizes.py` reads arrays, which found `cl_pinglist` and `cl_serverStatusList` short in 1.0 and `buf` declared larger than defined.
- `third_party/libjpeg-turbo`, four stale `src/blobs` files and the gamepad and rumble sources are gone; `cod2rad/` stays by decision. Rejected: reworking the gamepad `#if` blocks. Left open: the debt list's CoD2rev-Plus entries, and WinINet's cut transfers.

## 2026-10-28 — online prediction measured against 1.3; hud alignment

- Same server (Titan), `cg_showmiss 1`: the original client missed 26 times (median 1.6, max 9.6), ours 53 (median 1.8) plus bursts of up to 32 units while rubbing another player. Collision with players (`0x4de2f0`, interpolated origin, box one unit smaller), the `centity_t` layout and the frame's order (`0x4d002d`..`0x4d0047`) are 1.3's; the misses far from players have the server moving slower than predicted, likely the mod's speed scale, which the playerstate does not carry. No client bug found; `COD2_TRACE_PREDDIFF` (per-word playerstate diff) kept as a tool. The prediction messages are 1.3's (`dvar_developer` was an alias of `cg_showmiss`).
- The killcam's title and timer were drawn at the screen's edges, half way down: hud elements read `alignScreen`'s horizontal from bits 0-2 and vertical from 3-5, the other way round from the server (and 1.3, `0x4dd04b`); pairs like left/top look the same either way. Materials with `alignX` center or right went off screen too: without a label `labelWidth` was never set and a material's width adds it (1.3 zeroes it, `0x4dd715`). `scripts/hud_align_test.py` failed on both, passes now.

## 2026-10-28 — shell shock files and the download label, as 1.3

- After downloading the Titan mod the client dropped with "couldn't register shell shock 'medical'": the table of dvars a `.shock` file sets had `cg_shock_viewKickFadeTime` at index 4, so of the 29 read `cg_shock_mouse_fadeTime` was left out and a file without the former was refused; 1.3's table (`0x5d89c8`) has neither that dvar nor the gap. Fixed in `cg_shellshock.c` and `data.S`; a local game with such a file failed before and loads now.
- The download label still read `EXE_DOWNLOA...`: the builds take `dlText` from the generated blobs (`build/win_gen/data.win.S`, `build/native_gen/data32.c`, also `src/blobs/data.S` and `build/web_gen/data.c`), where the blob's `"ING\0"` had been taken for a pointer (`scrMemTreeGlob+517961`); only `src/blobs/data.c` was fixed before. `test/test_blob_tables.py` pins the label in the blobs and the shock table.
- Not changed: a local game over a `main` folder holding many downloaded `.iwd` drops with `FS_PureServerSetReferencedIwds: count mismatch`, because `systeminfo` passes 1024 bytes and loses `sv_referencedIwds`; 1.3's `Info_SetValueForKey_Big` has the same 1024 limit (`0x44b14b`).

## 2026-10-28 — HTTP downloads on Windows, SD stopwatch, loading screen names

- The Windows client never downloaded over HTTP: MinGW linked `win32_stubs.c`'s curl stubs (`curl_multi_init` answered 0), so every redirected download failed, the client sent `wwwdl fail` and `bbl8r` as 1.3 does (`0x414055`) and servers that ask for it dropped it; the slow downloads seen before were servers' UDP fallback. `src/win32/win_wininet.c` gives `dl_main.c` libcurl's calls on WinINet (1.3 itself uses libwww over WSOCK32); `scripts/www_download_test.py` (a local HTTP server) passes on Linux and under Wine, failed with the old exe. The bytes received now reach `cl_downloadCount`, as 1.3's read callback does (`0x4b58e0`): the bar stayed at 0% on both clients.
- Online screenshots: the download box showed `EXE_DOWNLOAD` over the file name (the key is `EXE_DOWNLOADING`, `0x5ddc44`); SD's stopwatch drew a checkered diamond, its hand looked up as `<face>_hand` instead of `<face>needle` (`0x4dda60`); the loading screen showed `sd` and `mp_carentan`: it compared the gametype with the display name and never took the map's `longname` (`0x539211` uses the menus' lookups, `0x532d90`, `0x532cd0`).
- Left open: the online view jumps (the traced session was too short; `COD2_TRACE_PREDDIFF` stays uncommitted); the MSVC client still has the curl stubs.

## 2026-10-25 — checks pass on Ubuntu 26.04 in Spanish

- glibc 2.43 gave i386's `acosf`, `asinf`, `atan2f`, `log10f` and `sqrtf` new versions, raising the Linux binaries' floor past 2.34: `glibc_compat.h` binds them to `GLIBC_2.0` (`.symver`) and is now force-included in C too (speex calls `sqrtf`); its `<features.h>` part stays C++-only, since included first in C it hid sqlite3.c's `mremap`.
- `readelf` translates its DWARF dump: in a Spanish locale `check_c_decls.py` and `check_var_sizes.py` found "no DWARF". The nine scripts that parse binutils' output set `LC_ALL=C` (`test/test_check_c_decls.py` reads a dump with `es_CL`).
- Ubuntu 26.04's MinGW is GCC 13, which rejects `-Wno-error=return-mismatch` (GCC 14's): CMake adds it only when the compiler knows the warning.

## 2026-10-25 — GL client loads color_channel_mixer

- The GL clients (no `GFX_REAL_D3D9`) logged `color_channel_mixer.tech, line 15: expected 'sampler' or 'material', found 'constant'` and loaded the material from partial data: their `D3DXGetShaderConstantTable` stub returns no constants, so `Material_SetPassShaderArguments` takes the reconstruction's fallback (1.3 always has d3dx9's table), which parsed every value as a sampler. It now skips, as unused, every constant but `colorMapSampler`, the only one it keeps; `constant.materialColor` is the stock data's only `constant.` in a shader block.
- `scripts/fx_test.py` also fails when a material does not load (`FinishLoadingInstance failed`). Checked here with the native client's log on the stock iwds (the test needs a CD key this box lacks): the two lines gone, no other warning changed.
- The material only draws `r_showFbColorDebug 2` and `sc_showDebug`; whether the GL backend draws it right was not looked at.

## 2026-10-25 — HUD and menus seen online, as 1.3

- Playing online on a CoD2rev-Plus server: no hip crosshair (its alpha came from `fWeaponPosFrac`, 0 at the hip; 1.3 uses `CG_DrawWeapReticle`'s return, `0x4c95b9`), a white square for the turret's reticle (`0x4c8da0` draws the turret weapon's own), the MG42's use hint showing the grenade icon (weapon icons stored at `weaponNum + 2`, not `+ 4`, `0x4d4dfe`), blue tracers (RGB24 images loaded with the PowerPC byte swap, `0x1000f6e0`) and an empty "Name" list in the in-game menu (`player_profile`, visible since parsed, never closed: `Menus_CloseAll` walks every loaded menu, `0x53adc0`).
- The crosshair's red-on-enemy color was not 1.3's: its color helper (`0x4c8d50`) is only green over a friendly, else white.
- Left open: the view still jumps now and then online (ping 35–45), not yet measured (`cg_showmiss 1` prints the misses); the scoreboard's columns differ from 1.3's table (`0x59e628`: status and talking icons first).

## 2026-10-24 — the server's animation script as 1.3, vendor sync 1094533

- The three CoD2rev-Plus differences from 1.3 left open by the previous stage are fixed there (`1094533`, user-approved, pushed with `0267dff`) and vendored: strafe flags from STRAFING only (`0x4f7f54`), crouch and prone masks as 1.3's and answering 0 or 1 (`0x4f9750`, `0x4f97b0`; a bit mask made two prone anims look like a stance change), 1.3's float constants. Found on the way: `BG_SetNewAnimation` divided the start time as integers, so every looping anim started at frame 0 (`0x4f9adb`).
- `scripts/anim_script_test.py` also compares the parsed animations (flags, movetype, duration, speed): 55 of 301 differed on `0267dff`, none now; the engine and the server parse and pick alike.
- Playing CTF with bots, the Windows client crashed whenever a grenade exploded in view: with a dynamic point light `R_DrawScene` clears alpha and stencil with a full-screen quad, which `r_scene.c` declared with color and material swapped (upstream code), so `colorWhite` was drawn as a material. Found by a new crash-report line (the last surface begun and its caller); `check_c_decls.py` now compares pointer parameters by what they point to, which caught it and ten harmless casts (allowlisted).

## 2026-10-24 — animation script from CoD2rev-Plus, vendor sync 0267dff

- The engine's animation script (cgame's prediction and other players' animation) follows CoD2rev-Plus, each function checked against the 1.3 client: torso-only anims do not count as set (`0x4f8f20`), UNDERHAND is the view's pitch (`0x4f94b0`), locomotion picks `clientNum % count`, not `rand()` (`0x4f9190`); remote players' turret test, idle yawing, torso clamp, stance transition time, death anims, the animation rate between snapshots (it went back to 1.0 every frame) and the prone back pitch now as 1.3. `scripts/anim_script_test.py` runs both bgames on the same playerstates in one client: 96 of 2.7 million cases differed, none now.
- Kept the engine's where CoD2rev-Plus differs from 1.3: the parser's strafe flags (CoD2rev-Plus sets 0x10/0x20 for every condition but STRAFING; 1.3 `0x4f7f54` only for it) and the crouch mask (`0x4f9750`). The 2–4 animation misses `prediction_test.py` still counts come from the spawn's weapon and the landing's MOVETYPE condition, which the playerstate does not carry, not from the script.
- Vendored files at CoD2rev-Plus `0267dff` (the user's zk_libcod port, `sv_libcodUnsafe`, wrap restarts, and the controllers' multipliers as 1.3's floats); the engine gained `Dvar_CanCreate` for the client's local games. Left open, for the user: fixing the strafe flags, the crouch mask and the double literals of `BG_PlayerAngles`/`BG_SwingAngles`/`BG_SetNewAnimation` in CoD2rev-Plus.

## 2026-10-24 — client mouse as 1.3's IN_Frame

- The SDL pump (Linux and Windows clients, `src/unix/linux_input.c`) now honours `in_mouse 0` as 1.3's `IN_Frame` (`0x464b30`) and `IN_MouseEvent` (`0x464940`): the mouse is never captured, does not move and its buttons do nothing (the wheel still counts, as in 1.3); the X11 fallback moves the menus' cursor by deltas, only when captured, instead of absolute positions. The decisions are `IN_WantMouse` and `IN_MouseButtons` (`src/unix/in_mouse.h`, `test/test_in_mouse.py`).
- Rejected (user's choice): porting `ClickToContinue`, a click per frame from a dvar nothing sets; it is listed under deliberate simplifications.

## 2026-10-24 — walkable normal, r_fullscreen

- `MIN_WALK_NORMAL` is the float `0.7`, as every site that uses it in 1.3 (`0x5c427c`): fixed in CoD2rev-Plus `b60ab35` and vendored; the engine's bgame follows through `bg_plus_consts.h`. Prediction misses stay within their run-to-run spread (13–21).
- `+set r_fullscreen 0` and a profile's `r_fullscreen 0` are both ignored, by the original too: run under Wine, 1.3 prints `r_fullscreen is read only` and opens full screen, because its renderer DLL registers the dvar read only (`0x2040`), which resets it to 1. Kept as 1.3 (the user's choice); the debt entry is gone and `validation.md` no longer claims the profile helps.

## 2026-10-23 — vendor sync with CoD2rev-Plus f77014a

- Vendored files moved from `dfb5099` to `f77014a`: upstream's reviewed xanim (`xanim_calc.cpp` folded into `xanim.cpp`), the walkable-normal and trace constants (`cm_mesh.cpp` now compares with `0.7f`; prediction misses unchanged, 19), and two server fixes made in CoD2rev-Plus first: `BG_GetSpeed` horizontal (`0x4fde06`) and `BG_GetBobCycle` divided as a float (`0x4fddb6`), whose integer division had kept the server's eye from ever bobbing. No bridge change.
- `scripts/server_eye_test.py` checks the server's eye through libcod's `getViewOrigin`; the stock `getEye` is only the origin plus 40, and a jump from a stop shows nothing because standing resets the bob cycle.
- Left open: the CoD2rev-Plus commits `a335fc6` and `f77014a` are local there, not pushed.

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

## 2026-11-08 — OpenCoD2-Plus Server 48cf44c: Plus ID, Mbed TLS split

- Brought the vendored files up to 48cf44c (branch `plus-id`, not yet on the server's master): the server's Plus ID (`src/libcod/plusid*.cpp/.hpp`, `scripts/plusid_test.py`, `banlist.py`, `test_banlist.py`, hooks in `gsc*`, `net_http`, `libcod.cpp`, `sv_client_mp.cpp`, `cod2client.py`). Its design is in the server's `docs/architecture/plus-id.md`. Plus version stays 123.
- CMake: Mbed TLS moved out of `cod2_plus_libcod` into the `cod2_plus_mbedtls` OBJECT library (same flags), built once in every non-web build and linked by the client and dedicated targets, Win64 included; the Windows clients link `bcrypt` always. Client-side signing is the next stage.

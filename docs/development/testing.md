# Testing

Runtime tests need the original game files (a folder with `main/*.iwd`), passed
as `GAME` below. Tests with a real client need a CD key in
`$HOME/.cod2/preferences` (with no `HOME`, as usual on Windows, `.cod2/preferences`
in the working directory) and open windows; without a key they are skipped
(exit 0). Servers run in
LAN mode with a temporary home, so `GAME` is only read.

## Runtime tests

| Script | Proves | Real client |
|---|---|---|
| `scripts/opencod2_smoke.py GAME --protocol P` | OpenCoD2-Plus Server's smoke test (`scripts/smoke_test.py`, unchanged) against this server; failures compared with `test/smoke_expected_failures_<P>.txt`, which may only shrink; flaky checks in `test/smoke_flaky.txt`. 118 runs it with `--plus`; `--stock` for a build with `-DCOD2_PLUS_PROTOCOL=OFF` (`--binary build/check-linux-13/cod2_lnxded`) | no |
| `scripts/two_players_test.py` | two clients join, send commands, disconnect, reconnect, kick; `developer 1` | no |
| `scripts/map_reload_test.py` | alternating `map_restart` and `map` loads | no |
| `scripts/rcon_drop_test.py` | `rcon error drop` leaves the server up | no |
| `scripts/anim_builtins_test.py` | `getAnimLength`, `animHasNotetrack`, `getMoveDelta`, `getAngleDelta` | no |
| `scripts/hit_location_test.py [--listen]` | shots land on the aimed body part (head, legs, turned torso) | `--listen` |
| `scripts/contact_test.py [--listen]` | body stop, trigger, melee, grenade, explosion, `radiusDamage` | `--listen` |
| `scripts/listen_test.py` | a client hosts, another joins, through `map_restart` and a map change; `--host-binary EXE --host-wine WINE` hosts on the Windows client | yes |
| `scripts/client_view_test.py` | a real client is shot, blasted, watches deaths, dies and respawns without crashing | yes |
| `scripts/view_weapon_test.py` | a real client leans, crouches and jumps; its view weapon (traced with `COD2_TRACE_VIEWWEAPON`) moves sideways, to the stance offset and dips on landing as 1.3's; hits every 4 s kick the view within `v_dmg_pitch`/`v_dmg_roll` (`COD2_TRACE_VIEWDAMAGE`); walking, the eye bobs along the view's right (`COD2_TRACE_VIEWBOB`); firing, reloading and melee move the camera by `tag_camera` (`COD2_TRACE_VIEWANIM`); a jump in place keeps `xyspeed` at 0; after 30 s shellshocks: the client keeps running and the shaken view stays a rotation (`COD2_TRACE_PERTURB`) | yes |
| `scripts/server_eye_test.py` | a walking player's eye on the server (libcod `getViewOrigin`, where shots start) bobs sideways; thrown straight up mid-walk with its horizontal velocity cleared, it stays over the origin, as 1.3 (horizontal speed 0) | no |
| `scripts/anim_script_test.py` | in a local game the engine's animation script (cgame's prediction) and OpenCoD2-Plus Server's (the server's) parse the same animations (flags, movetype, duration, speed) and pick the same legs and torso animations, timers and durations for every weapon, stance, aim, pitch, state, movetype and event (`COD2_CHECK_ANIMSCRIPT`) | yes |
| `scripts/prediction_test.py` | prediction misses while moving, mantling, firing stay under `--max-misses` (60); aiming down the sight and back, the sight's fraction (`COD2_TRACE_PREDDIFF=all`) matches the server's (`--max-ads-misses`, 0); through a shot cut by a reload and a switch away and back during the drop, the weapon's state matches it too (`--max-weapon-misses`, 0, leaving out the spawn's weapon), and so does the lean going prone while leaning (`--max-lean-misses`, 0; the Plus 123 rules); legs and torso that change when predicted again stay under `--max-anim-misses` (6; `COD2_TRACE_PREDANIM`: the spawn's weapon and the landing's MOVETYPE condition, which the playerstate does not carry); `--moves CFG`, `--spot`; `--delay MS` relays the packets through a UDP relay that holds each MS ms each way (online ping), `--maxfps N` sets the client's frame rate (default 20); `--wine CMD --binary EXE` runs a Windows client, also 1.3's `CoD2MP_s.exe` as a baseline (copied off DrvFs with its DLLs, against `main`'s dedicated server, which takes stock clients; a base dir without `main/players`, or 1.3 takes that profile) | yes |
| `scripts/www_download_test.py GAME` | a client joins a 1.3 server whose mod it lacks: it fetches the mod's `.iwd` over HTTP (`sv_wwwBaseURL`, a local HTTP server), keeps it byte for byte and joins; `--disconnect` (`sv_wwwDlDisconnected 1`: leave, download, reconnect); `--wine WINE --binary EXE` runs the Windows client (WinINet) | yes |
| `scripts/hud_align_test.py` | hud elements land where their `horzAlign`/`vertAlign` and `alignX`/`alignY` put them: three colored boxes (center_safearea/top centered, right/bottom, left/middle) found in a screenshot | yes |
| `scripts/hud_options_test.py` | `cg_drawCompass 0` takes the compass away, `cg_hudCompassOffsetX`/`Y` move it, found by comparing screenshots; `com_writeConfig 0` leaves no `config_mp.cfg`; an objective appears on its side of the compass, with its own icon | yes |
| `scripts/stability_test.py` | a client archives 3000 dvars and reads them back from a config over 64 KB, and past the pool's end `set` only prints; with `--wine CMD --binary cod2_win32.exe`, `com_freezeWatch` reports the `freeze` command's stall, the client goes on, and 0 turns it off | yes |
| `scripts/fx_test.py` | effects change the picture; none fails to load | yes |
| `scripts/msvc_run_test.py --exe EXE` | from WSL, a Windows client (MinGW or MSVC ABI) on Windows loads mp_toujane in a local game, its cgame starts, the gametype script talks to it, frames are drawn, and it keeps running `--linger` s with no crash report | yes |
| `scripts/shader_compile_test.py GAME` | `cod2_linux_vk` compiles the game's shaders (150 at least, none fails but `distortion.hlsl`) and draws mp_toujane | yes |
| `scripts/bots_test.py --binary B` | libcod bots (OpenCoD2-Plus Server's test, `mods/bots`) | no |
| `scripts/plus_rules_test.py GAME --binary B` | the Plus 123 rules (OpenCoD2-Plus Server's test, vendored) against a minimal Plus client: fast reload, prone peek, MG clip; 1.3 Plus builds only | no |
| `scripts/http_test.py --binary B` | libcod `httpFetch`, `webSocket*` (needs `openssl`) | no |
| `scripts/sound_test.py` | what the client sends to the device, through SDL's disk driver: the menu's MP3 stream, `playLocalSound`, left and right of a `script_origin`, `map_restart`, `vid_restart`, `disconnect` and `quit` with sounds playing, no device, `snd_khz 22`, the callback's cost with bots fighting; `--cases a,b`; `--wine CMD --binary cod2_win32.exe` runs the Windows client | yes |

`P` is 118 (1.3, `build/check-linux`) or 115 (1.0, `build/check-linux-10`);
the scripts pick the binary from `--protocol` (`--binary` overrides it).
`bots_test.py`, `http_test.py` and `plus_rules_test.py` need `--binary`: their default
(`bin/cod2rev_lnxded`, OpenCoD2-Plus Server's layout) does not exist here.

## Operational notes

| Note | Detail |
|---|---|
| Minimal client | `scripts/hit_location_test.py`'s client sends pitch, move keys and buttons every command: a field left unchanged keeps the server's previous command's value |
| Prediction misses | `cg_showmiss 1` prints them, with 1.3's messages (`prediction error`, `Prediction miss`); running into another player always misses (the client clips players at their interpolated origin with a box one unit smaller per side, as the original). `COD2_TRACE_PREDDIFF=1` prints, for each snapshot whose origin differs from what the client predicted for that time, every 4-byte word of the playerstate that differs and the solid entities around |
| Game folder | `GAME` must hold only the stock `.iwd`: files downloaded from servers into `main` make local games drop with `FS_PureServerSetReferencedIwds: count mismatch` (`systeminfo` passes 1024 bytes, as 1.3); a folder of links to the stock files works |
| Bots flake | "search paths sparingly" failed 2 runs in 5 with the client as host, on one bot-second; it is the tail of the same behavior as the dedicated server |
| `layout_parity.py` | runs nothing: values are read from the compiled objects with `objcopy`, so MinGW needs no Wine |
| UI trace | `MTRACE=1` prints the open menus to the console |

Windows dedicated server under Wine: `opencod2_smoke.py GAME --protocol 118
--binary build/check-mingw/cod2_win32_ded.exe --wine DIR/wine.sh`, list
`test/smoke_expected_failures_wine.txt` (crash report, SIGTERM checks and
output-reading checks cannot pass under Wine).

## Static checks

Run by `scripts/build_check.py` ([building.md](building.md#build_checkpy)),
plus:

| Script | Checks |
|---|---|
| `scripts/vendor_check.py PLUS_CHECKOUT` | vendored files equal OpenCoD2-Plus Server at `test/vendored_files.txt`'s revision |
| `scripts/check_docs.py` | docs: links, repository paths, personal data, memory stages |
| `python3 -m unittest discover -s test` | unit tests of the scripts |

## Full suite (before merging a phase)

1. `python3 scripts/build_check.py all` (a skipped variant fails it; `--allow-skip` accepts that)
2. `python3 -m unittest discover -s test` and `python3 scripts/check_docs.py`
3. For P in 118 and 115: `opencod2_smoke.py`, `two_players`, `map_reload`,
   `rcon_drop`, `anim_builtins`, `hit_location` (plain and `--listen`),
   `contact` (plain and `--listen`), `listen`, `fx`, `client_view`,
   `prediction`, `view_weapon`, `hud_align`, `anim_script`, `server_eye`, `sound`.
   `hostile_server_test.py` (118 only: a fake server sends malformed messages).
   `shader_compile_test.py` when the `linux-vk` build exists.
4. `bots_test.py`, `http_test.py` and `plus_rules_test.py` with `--binary build/check-linux/cod2_lnxded`.
5. `www_download_test.py` plain, with `--disconnect`, `--http-404` and `--truncate` (118 only).

A `skipped:` line means a test did not run: a run with skips does not count for a merge.

## Ad-hoc client checks

A client can be driven by a config: `+exec t.cfg` with `wait N` lines and
`screenshot`; `+devmap MAP` enables cheats (`setviewpos x y z yaw`). A
screenshot is taken a few frames after its command, and a new `screenshot`
before that replaces the pending one, so leave several `wait` frames between
them. A local game refuses `vid_restart`; test video modes from the main menu.

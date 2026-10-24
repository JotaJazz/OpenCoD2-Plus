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
| `scripts/opencod2_smoke.py GAME --protocol P` | CoD2rev-Plus's smoke test (`scripts/smoke_test.py`, unchanged) against this server; failures compared with `test/smoke_expected_failures_<P>.txt`, which may only shrink; flaky checks in `test/smoke_flaky.txt` | no |
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
| `scripts/anim_script_test.py` | in a local game the engine's animation script (cgame's prediction) and CoD2rev-Plus's (the server's) pick the same legs and torso animations, timers and durations for every weapon, stance, aim, pitch, state, movetype and event (`COD2_CHECK_ANIMSCRIPT`) | yes |
| `scripts/prediction_test.py` | prediction misses while moving, mantling, firing stay under `--max-misses` (60); legs and torso that change when predicted again stay under `--max-anim-misses` (6; `COD2_TRACE_PREDANIM`: the spawn's weapon and the landing's MOVETYPE condition, which the playerstate does not carry); `--moves CFG`, `--spot` | yes |
| `scripts/fx_test.py` | effects change the picture; none fails to load | yes |
| `scripts/shader_compile_test.py GAME` | `cod2_linux_vk` compiles the game's shaders (150 at least, none fails but `distortion.hlsl`) and draws mp_toujane | yes |
| `scripts/bots_test.py --binary B` | libcod bots (CoD2rev-Plus's test, `mods/bots`) | no |
| `scripts/http_test.py --binary B` | libcod `httpFetch`, `webSocket*` (needs `openssl`) | no |

`P` is 118 (1.3, `build/check-linux`) or 115 (1.0, `build/check-linux-10`);
the scripts pick the binary from `--protocol` (`--binary` overrides it).
`bots_test.py` and `http_test.py` need `--binary`: their default
(`bin/cod2rev_lnxded`, CoD2rev-Plus's layout) does not exist here.

## Operational notes

| Note | Detail |
|---|---|
| Minimal client | `scripts/hit_location_test.py`'s client sends pitch, move keys and buttons every command: a field left unchanged keeps the server's previous command's value |
| Prediction misses | `cg_showmiss 1` is read as `dvar_developer` in the reconstruction; running into another player always misses (the client clips players with a box one unit smaller per side, as the original) |
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
| `scripts/layout_parity.py` | types in `test/layout_parity_types.txt` lay out alike in this repo and CoD2rev-Plus (1.3, 1.0, MinGW) |
| `scripts/check_c_linkage.py BUILD` | no unexpected C++-mangled names (`test/c_linkage_allowlist.txt`) |
| `scripts/vendor_check.py PLUS_CHECKOUT` | vendored files equal CoD2rev-Plus at `test/vendored_files.txt`'s revision |
| `scripts/check_docs.py` | docs: links, repository paths, personal data, memory stages |
| `python3 -m unittest discover -s test` | unit tests of the scripts |

## Full suite (before merging a phase)

1. `python3 scripts/build_check.py all`
2. `python3 -m unittest discover -s test` and `python3 scripts/check_docs.py`
3. For P in 118 and 115: `opencod2_smoke.py`, `two_players`, `map_reload`,
   `rcon_drop`, `anim_builtins`, `hit_location` (plain and `--listen`),
   `contact` (plain and `--listen`), `listen`, `fx`, `client_view`,
   `prediction`.
   `shader_compile_test.py` when the `linux-vk` build exists.
4. `bots_test.py` and `http_test.py` with `--binary build/check-linux/cod2_lnxded`.

## Ad-hoc client checks

A client can be driven by a config: `+exec t.cfg` with `wait N` lines and
`screenshot`; `+devmap MAP` enables cheats (`setviewpos x y z yaw`). A
screenshot is taken a few frames after its command, and a new `screenshot`
before that replaces the pending one, so leave several `wait` frames between
them. A local game refuses `vid_restart`; test video modes from the main menu.

# Dedicated server

Everything `cod2_lnxded` and `cod2_win32_ded.exe` run is CoD2rev-Plus's code
([ADR 0001](adr/0001-vendor-cod2rev-plus-with-a-generated-bridge.md)). The
engine objects are still linked, for the symbols the rest of the engine names,
but never run.

## Components

| Component | Source | Notes |
|---|---|---|
| Script VM | `src/script` | `COD2_PLUS_SCRIPT`. Its `VAR_INT64` shifts the object types: `VAR_ENTITY` is 22, not 0x15; `src/headers/scr_vartypes.h` gives the build's values, `src/compat/script_missing.cpp` checks them. |
| Server | `src/server` | `COD2_PLUS_SERVER`. |
| Game, bgame | `src/game`, `src/bgame` | bgame comes with the game: the two sides cannot share `level_bgs` (`animation_s` is 104 bytes in the engine, Windows layout via `COD2_WIN_ALIGN8`; 96 in CoD2rev-Plus). |
| Core and platform | `src/universal`, `src/qcommon`, `src/unix` / `src/win32` | the server starts in CoD2rev-Plus's `main`; brings log rotation (`logfileName`, `logfileRotate`, `logTimestamps`), `sv_logRcon`, `net_strictPort`, `crash.log`. |
| xanim | `src/xanim` | with it the server's bridge has no imports left. |
| libcod | `src/libcod` | `COD2_PLUS_LIBCOD`; all modules but MySQL; `sv_master1`..`sv_master5`, bots, HTTP/WebSocket (Mbed TLS; MinGW links `bcrypt`, `crypt32`), SQLite, JSON. Speex is the engine's (`src/PC/speex`), same API and bitstream. |

## Bridge (`--module server`)

| Item | Where |
|---|---|
| Exports (engine functions the rest of the engine calls, served by CoD2rev-Plus) | `src/compat/server_exports.cpp`, list `test/server_interface.txt` |
| Functions CoD2rev-Plus lacks, only called from code the server never runs | `test/server_fatal.txt`: they stop the server with their name |
| Data the blobs or client code still name | `src/compat/server_data.cpp` |
| Undeclared CoD2rev-Plus functions | `src/compat/server_decls.h` |
| bgame `imp_` placeholders | `src/compat/server_bgame_names.cpp` |

Variadic exports (`Com_Printf`, `Com_DPrintf`, `Com_Error`, `FS_Printf`)
format on the engine side and pass `"%s"`.

## Structures that differ

| Type | Engine | CoD2rev-Plus | Why it is safe |
|---|---|---|---|
| `dvar_s` | 36 bytes | 72 (vectors in line) | engine code that can run reads only `name`, `flags`, `type`, `modified`, `current`, same offsets |
| `netchan_t` | `dropped`/`incomingSequence` swapped | | the server reads neither |
| 1.0 `client_t`, `serverStatic_t`, `challenge_t` | 1.0 forms | 1.3 forms | nothing outside the server touches them |

`level_locals_t`, `gitem_s`, `scr_const_t`, `game_hudelem_s` match field by
field in 1.3 and 1.0 (`scripts/layout_parity.py`).

## Protocol facts

| Fact | Detail |
|---|---|
| Version | `COD2_PROTOCOL_VERSION` 118 (1.3) or 115 (1.0, `-DCOD2_PATCH_10`); `src/compat/script_missing.cpp` asserts CoD2rev-Plus's `PROTOCOL_VERSION` matches |
| Fragment offsets | shorts in 1.0, longs in 1.3, in both directions (`Netchan_TransmitNextFragment`, `0x447e61`) |
| `netadr_t` port | right after the IP, at +8 (`NET_AdrToString`, `0x44770a`), not after the IPX bytes as in Quake III |
| Server commands | one letter plus arguments; the client's switch is in [client.md](client.md#server-commands) |

## Bridge behavior worth knowing

- `Com_Error` is `noreturn` in CoD2rev-Plus; the engine's returns for `ERR_SCRIPT`/`ERR_LOCALIZATION` once the UI shows the error, so the bridge raises it again as a drop.
- Variadic functions format into 4096-byte buffers (CoD2rev-Plus's `MAXPRINTMSG`); `Com_sprintf` formats straight into its destination.

## Windows

`cod2_win32_ded.exe` prints only to its console window and shows a message box
on a fatal error; scripts read its console log (`logfile`). Under Wine see the
expected failures in `test/smoke_expected_failures_wine.txt`.

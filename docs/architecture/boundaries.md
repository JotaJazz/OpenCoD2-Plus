# Boundaries (architectural constitution)

Normative. A change that breaks a rule here needs an ADR first
([adr/](adr/README.md)).

## Rules

| # | Rule |
|---|---|
| B1 | Vendored files (`test/vendored_files.txt`) are never edited here. A fix goes to CoD2rev-Plus first and comes back with `git am` ([../development/vendored.md](../development/vendored.md)). |
| B2 | The engine and CoD2rev-Plus never call each other directly: every crossing goes through the bridge in `src/compat`. |
| B3 | Every bridge crossing is checked against the engine's ABI: `test/server_abi.txt` (dedicated) and `test/client_server_abi.txt` (client), written by `scripts/engine_abi.py`. No crossing function is `regparm`. |
| B4 | One copy of each state per executable. Engine sources whose state CoD2rev-Plus also holds are listed in `test/plus_replaces.txt` and left out of the dedicated server; `scripts/check_single_impl.py` enforces it. |
| B5 | No C name defined by both sides in one target; `scripts/check_duplicates.py` enforces it (MinGW links with `--allow-multiple-definition`, so a duplicate would win silently). |
| B6 | The engine's bgame globals that CoD2rev-Plus's bgame also defines carry the `eng_` prefix (`test/engine_bgame_names.txt`, `scripts/bgame_names.py`); `bg_itemlist`, `bg_numItems`, `scr_const` stay shared. |
| B7 | Shared structure layouts are pinned (`COD2_ASSERT_FIELD`, `COD2_ASSERT_SIZE`) and compared with CoD2rev-Plus's (`scripts/layout_parity.py`, `test/layout_parity_types.txt`). |
| B8 | Engine `extern` declarations must match their definitions (`scripts/check_c_decls.py`, allowlist `test/c_decls_allowlist.txt`, which may only shrink). |
| B9 | Behavior follows the original 1.3 binaries; a fix cites the original's address when it was checked against it ([../development/validation.md](../development/validation.md)). |
| B10 | The data blobs keep the original binary's globals; a global that CoD2rev-Plus defines is guarded out of the blobs (`COD2_PLUS_SCRIPT`, `COD2_PLUS_SERVER`, `COD2_PLUS_GAME`, `COD2_PLUS_CM`). |

## Dependency matrix

| From \ To | Engine | Bridge | CoD2rev-Plus | Blobs |
|---|---|---|---|---|
| Engine (`src/PC`) | yes | yes (exports) | no | yes |
| Bridge (`src/compat`) | yes (C names) | yes | yes (C++ names) | yes |
| CoD2rev-Plus | no | yes (imports) | yes | no |
| Blobs | `imp_` pointers only | no | no | yes |

## Bridge modules

| Module | Generated files | Interface list | Used by |
|---|---|---|---|
| `script` | `script_exports.cpp`, `script_imports.cpp` (+ `script_game_imports.cpp`, `script_core_imports.cpp` for executables without CoD2rev-Plus's game/core) | `test/script_interface.txt` | every Linux/MinGW executable |
| `server` | `server_exports.cpp`, `server_imports.cpp` | `test/server_interface.txt`, `test/server_fatal.txt` | dedicated server |
| `client_server` | `client_server_exports.cpp`, `client_server_imports.cpp` | `test/client_server_interface.txt` | client |

Hand-written parts: `*_missing.cpp` (what one side lacks), `server_data.cpp`,
`server_bgame_names.cpp`, `*_decls.h`.

## Accepted exceptions

| Exception | Why | Where decided |
|---|---|---|
| MSVC, web and x64 keep `src/PC/server_mp`, `src/PC/game_mp`, `src/PC/game`, `src/PC/script` and the engine collision | those builds do not link CoD2rev-Plus | [ADR 0003](adr/0003-keep-server-mp-for-msvc-web-x64.md) |
| The client keeps the engine's bgame for cgame (prediction) next to CoD2rev-Plus's for the local server | cgame reaches its state through `bgs`; CoD2rev-Plus uses `level_bgs` and another `animation_s` | [client.md](client.md) |
| Functions only code the dedicated server never runs needs, which CoD2rev-Plus lacks, stop the server with their name (`test/server_fatal.txt`) | linked for symbols, never called | [server.md](server.md) |

## Forbidden

- Editing a vendored file in place.
- Calling a CoD2rev-Plus C++ symbol from engine code without a bridge entry.
- Adding an entry to an allowlist (`test/c_decls_allowlist.txt`, `test/c_linkage_allowlist.txt`) to silence a real mismatch.

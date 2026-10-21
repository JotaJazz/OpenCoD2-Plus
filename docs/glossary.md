# Glossary

| Term | Meaning | Where |
|---|---|---|
| 1.0 / 1.3 | game versions; protocol 115 / 118; build with or without `-DCOD2_PATCH_10` | `CMakeLists.txt` |
| bgame | game code shared by server and client: movement, weapons, animation | `src/PC/bgame` (engine), `src/bgame` (CoD2rev-Plus) |
| `eng_` names | engine bgame globals renamed so they do not clash with CoD2rev-Plus's | `test/engine_bgame_names.txt` |
| blob | data reconstructed from the original binaries' sections (globals, tables, literals) | `src/blobs`, `build/native_gen`, `build/win_gen` |
| bridge | generated C++ thunks between engine C names and CoD2rev-Plus C++ names | `src/compat` |
| export / import | a bridge function the engine calls and CoD2rev-Plus serves / CoD2rev-Plus calls and the engine serves | `test/*_interface.txt` |
| ABI file | the engine's return class and parameter slots per function, checked by every thunk | `test/server_abi.txt`, `test/client_server_abi.txt` |
| CoD2rev-Plus | C++ reconstruction of the CoD2 server (with libcod) vendored here | `test/vendored_files.txt` |
| cgame | client game: HUD, entities, prediction, effects calls | `src/PC/cgame_mp` |
| configstring | indexed string the server sends to clients (server info, players, models) | server command `d` |
| server command | reliable text command from server to client, keyed by its first letter | [architecture/client.md](architecture/client.md#server-commands) |
| DObj | a model instance with bones and an animation tree | `src/PC/qcommon/dobj_management.c` |
| xanim | skeletal animation system (trees, notetracks) | `src/PC/xanim`, `src/xanim` |
| notetrack | named event inside an animation (sounds, gun hand) | xanim |
| efx / primitive | effect file / one element type inside it (Particle, Cloud, Tail…) | [architecture/effects.md](architecture/effects.md) |
| GL device | the Mac port's OpenGL implementation of the Direct3D 9 interface | `src/Mac` |
| real Direct3D 9 | the Windows client's path on the system's `d3d9.dll` and d3dx9 | `GFX_REAL_D3D9` |
| techset / technique | a material's set of shader passes | `materials/techniques/*.tech` (game files) |
| picmip | texture mip levels skipped by quality setting | `r_picmip` |
| outdoor map | height map of the world's highest surfaces, used to hide weather indoors | `src/PC/gfx_d3d/r_outdoor.c` |
| listen server | a server inside a client (local game) | `LISTEN_SERVER` |
| libcod | server extension library (bots, HTTP, SQLite, master servers) | `src/libcod` |
| original binaries | `CoD2MP_s.exe` 1.3 and `gfx_d3d_mp_x86_s.dll`, the behavioral reference | [development/validation.md](development/validation.md) |

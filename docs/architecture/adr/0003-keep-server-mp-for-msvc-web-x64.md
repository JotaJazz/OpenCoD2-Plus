# 0003. Keep the engine's server for MSVC, web and x64

- Status: Accepted
- Date: 2026-10-19

## Context

Moving every build to CoD2rev-Plus would mean porting the bridge to MSVC, Emscripten and LP64, none of which the checks cover.

## Decision

Keep `src/PC/server_mp`, `src/PC/game_mp`, `src/PC/game`, `src/PC/script` and the engine collision for MSVC, the web build and x64; Linux and MinGW use CoD2rev-Plus.

## Consequences

- Two server implementations stay in the tree.
- Fixes to the engine's server matter only for those builds.
- The MSVC build is not checked by the scripts.

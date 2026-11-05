# 0003. Keep the engine's server for MSVC, web and x64

- Status: Superseded by [0015](0015-msvc-abi-client-on-the-bridge.md) for MSVC (x64 moved to the bridge in the 64-bit client phases)
- Date: 2026-10-19

## Context

Moving every build to OpenCoD2-Plus Server would mean porting the bridge to MSVC, Emscripten and LP64, none of which the checks cover.

## Decision

Keep `src/PC/server_mp`, `src/PC/game_mp`, `src/PC/game`, `src/PC/script` and the engine collision for MSVC, the web build and x64; Linux and MinGW use OpenCoD2-Plus Server.

## Consequences

- Two server implementations stay in the tree.
- Fixes to the engine's server matter only for those builds.
- The MSVC build is not checked by the scripts.

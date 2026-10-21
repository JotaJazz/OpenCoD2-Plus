# 0006. The original 1.3 binaries are the reference

- Status: Accepted
- Date: 2026-10-20

## Context

The reconstruction left tables zero, read fields at the binary's raw offsets and simplified functions; tests alone cannot tell what the original did.

## Decision

Check behavior against the original `CoD2MP_s.exe` 1.3 and `gfx_d3d_mp_x86_s.dll` (disassembly, string xrefs, jump tables); a fix cites the address it follows. CoD2rev-Plus is the second reference for server-side code.

## Consequences

- Fixes carry addresses in comments and commit messages.
- Contributors need their own copy of the game binaries to validate.
- See `docs/development/validation.md`.

# Architecture Decision Records

MADR-lite. One file per decision: `NNNN-kebab-title.md`. An accepted ADR is
never edited; a later ADR supersedes it, and the only change allowed to the
old one is its status line, set to `Superseded by NNNN`.

## Index

| ADR | Title | Status | Date |
|---|---|---|---|
| [0001](0001-vendor-cod2rev-plus-with-a-generated-bridge.md) | Vendor CoD2rev-Plus behind a generated bridge | Accepted | 2026-10-18 |
| [0002](0002-compile-the-engine-as-cpp.md) | Compile the engine as C++ | Accepted | 2026-10-18 |
| [0003](0003-keep-server-mp-for-msvc-web-x64.md) | Keep the engine's server for MSVC, web and x64 | Accepted | 2026-10-19 |
| [0004](0004-client-local-games-on-cod2rev-plus.md) | Run the client's local games on CoD2rev-Plus's server | Accepted | 2026-10-18 |
| [0005](0005-fix-vendored-code-upstream-first.md) | Fix vendored code upstream first | Accepted | 2026-10-19 |
| [0006](0006-original-1-3-binaries-are-the-reference.md) | The original 1.3 binaries are the reference | Accepted | 2026-10-20 |
| [0007](0007-remove-the-renderer-dll-variant.md) | Remove the renderer DLL variant | Accepted | 2026-10-20 |
| [0008](0008-windows-client-on-real-direct3d-9.md) | Windows client on real Direct3D 9 | Accepted | 2026-10-21 |

## Template

```markdown
# NNNN. Title

- Status: Proposed | Accepted | Superseded by NNNN
- Date: YYYY-MM-DD

## Context

What forces the decision (facts, constraints).

## Decision

What was decided, in one paragraph.

## Consequences

- What follows, good and bad.
```

# Architecture Decision Records

MADR-lite. One file per decision: `NNNN-kebab-title.md`. An accepted ADR is
never edited; a later ADR supersedes it, and the only change allowed to the
old one is its status line, set to `Superseded by NNNN`.

## Index

| ADR | Title | Status | Date |
|---|---|---|---|
| [0001](0001-vendor-cod2rev-plus-with-a-generated-bridge.md) | Vendor OpenCoD2-Plus Server behind a generated bridge | Accepted | 2026-10-18 |
| [0002](0002-compile-the-engine-as-cpp.md) | Compile the engine as C++ | Accepted | 2026-10-18 |
| [0003](0003-keep-server-mp-for-msvc-web-x64.md) | Keep the engine's server for MSVC, web and x64 | Superseded by 0015 | 2026-10-19 |
| [0004](0004-client-local-games-on-cod2rev-plus.md) | Run the client's local games on OpenCoD2-Plus Server's server | Accepted | 2026-10-18 |
| [0005](0005-fix-vendored-code-upstream-first.md) | Fix vendored code upstream first | Accepted | 2026-10-19 |
| [0006](0006-original-1-3-binaries-are-the-reference.md) | The original 1.3 binaries are the reference | Accepted | 2026-10-20 |
| [0007](0007-remove-the-renderer-dll-variant.md) | Remove the renderer DLL variant | Accepted | 2026-10-20 |
| [0008](0008-windows-client-on-real-direct3d-9.md) | Windows client on real Direct3D 9 | Accepted | 2026-10-21 |
| [0009](0009-linux-client-on-dxvk.md) | Linux client on DXVK native | Superseded by 0017 | 2026-10-22 |
| [0011](0011-graphics-apis-behind-direct3d-9.md) | Graphics APIs behind Direct3D 9 | Accepted | 2026-10-31 |
| [0010](0010-64-bit-dedicated-server-without-the-engine.md) | 64-bit dedicated server without the engine | Accepted | 2026-10-31 |
| [0012](0012-sound-engine-behind-the-ail-calls.md) | Sound engine behind the AIL calls | Accepted | 2026-11-01 |
| [0013](0013-miles-retired-from-the-windows-client.md) | Miles retired from the Windows client | Accepted | 2026-11-01 |
| [0014](0014-name-1-3-plus.md) | The player-facing name is "1.3 Plus" | Accepted | 2026-11-02 |
| [0015](0015-msvc-abi-client-on-the-bridge.md) | The MSVC-ABI client is built with clang on the bridge | Accepted | 2026-11-05 |
| [0016](0016-sse2-float-math.md) | Float math on SSE2, not x87 | Accepted | 2026-11-05 |
| [0017](0017-dxvk-is-the-default-linux-client.md) | DXVK is the default Linux client | Superseded by 0018 | 2026-11-07 |
| [0018](0018-the-x64-linux-client-on-dxvk.md) | The x64 Linux client on DXVK too | Accepted | 2026-11-08 |
| [0019](0019-the-client-signs-with-the-server-s-plusid-header.md) | The client signs with the server's Plus ID header | Accepted | 2026-11-08 |

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

# Project context

Stable identity of the project. Changes rarely; live state is in
[SYSTEM_SUMMARY.md](SYSTEM_SUMMARY.md).

## Identity

| Field | Value |
|---|---|
| Name | opencod2-plus |
| What | reconstruction of Call of Duty 2 multiplayer (versions 1.3 and 1.0): client and dedicated server |
| Fork of | opencod2/opencod2 (engine reconstruction) |
| Integrates | CoD2rev-Plus (server, script VM, game, core, xanim, libcod), vendored unchanged |
| Platforms | Linux (32-bit x86), Windows (MinGW cross-compiled; MSVC native), web (Emscripten), x64 (in progress) |
| Languages | C sources compiled as C++ (gnu++17), CoD2rev-Plus C++, Python 3 scripts, CMake |
| Game data | not included; users supply their own Call of Duty 2 |

## Goal

One public repository with a client and a server that keep the Call of Duty 2
community playing: compatible with the original 1.3 servers and clients, with
the dedicated server on CoD2rev-Plus's maintained code.

## Non-goals

- Changing gameplay or the network protocol.
- Distributing game assets or binaries.
- New features before parity with the original.

## Principles

| # | Principle |
|---|---|
| P1 | The original 1.3 binaries define correct behavior; fixes cite the address they follow. |
| P2 | Vendored code is copied byte for byte; fixes go upstream first. |
| P3 | Every engine ↔ CoD2rev-Plus crossing is generated and ABI-checked. |
| P4 | Every claim is backed by a test or a check (`scripts/`). |
| P5 | The code is the source of truth; docs follow it. |

## Global constraints

- 32-bit x86 ABI (ILP32) except the x64 port; structure layouts pinned with `COD2_ASSERT_FIELD`/`SIZE`.
- Linux binaries run on glibc 2.34; `cod2_lnxded` needs only libc, libm, libz, libstdc++.
- Nothing personal in the repository: no CD keys, personal paths, private server addresses.
- Documentation in English; the root README is the only translated file.

## Navigation

[README_AI.md](README_AI.md) · [../README.md](../README.md) · [../../AGENTS.md](../../AGENTS.md)

# 0015. The MSVC-ABI client is built with clang on the bridge

- Status: Accepted
- Date: 2026-11-05

## Context

The cl.exe build (removed with this decision) compiled the engine alone, as C,
with its own source list and shims, on the engine's server and script VM
(ADR 0003): a second Windows client to keep working. cl.exe cannot compile
the bridge, whose ~900 `__asm__` symbol labels have no cl equivalent, nor
OpenCoD2-Plus Server's GNU C++.

## Decision

The MSVC-ABI client is the MinGW branch of `CMakeLists.txt` compiled by clang
for `i686-pc-windows-msvc` and linked by lld-link against the MSVC runtime
(`cmake/toolchain-msvc-clang.cmake`): the same sources, flags, bridge,
OpenCoD2-Plus Server and libcod. Only the link differs (lld-link's spelling
of GNU ld's options), plus the POSIX shims the MSVC runtime lacks
(`src/win32/shims-msvc`) and generated `/alternatename` aliases for C/C++
globals, which the MSVC ABI mangles. cl.exe is no longer supported.

## Consequences

- One Windows client source set; the MSVC build runs the same server as every other client.
- It needs clang (VS's "C++ Clang tools" or any recent clang with lld-link) besides the MSVC toolset and SDK.
- `build_check.py msvc-client` builds it, from WSL through `cmd.exe`; elsewhere it is skipped.
- The MSVC ABI exposed a calling-convention bug gcc's frames hid (the color converter called as stdcall); such engine bugs now surface on one more compiler.

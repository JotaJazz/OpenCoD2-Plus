# 0016. Float math on SSE2, not x87

- Status: Accepted
- Date: 2026-11-05

## Context

The 32-bit Linux and MinGW builds computed float math on x87 with a 64-bit
mantissa (gcc's default on i386; `-msse` only enabled the instructions), the
MSVC-ABI build on SSE2 (clang's default for `i686-pc-windows-msvc`), and x64
and the web build never use x87. OpenCoD2-Plus Server moved every build to
SSE2 (its ADR 0017): the 1.3 client sets x87 to 24 bits at start
(`_controlfp(_PC_24)`, `src/PC/win32/win_syscon.c`), so single-precision SSE
rounds each float operation as that client does, and x87's results also hung
on when the compiler spilled a register. A client predicting with x87 against
a server on SSE2, or the other way, can differ in the last bit.

## Decision

Every x86 and x64 build compiles with `-msse2 -mfpmath=sse`, as OpenCoD2-Plus
Server's Makefile: the engine, the vendored code and the bridge, the clients
and the dedicated servers (`cmake/lnxded64` included).

## Consequences

- Client and server round float math the same way on every build and toolchain.
- The 32-bit builds need an SSE2 CPU (Pentium 4, Athlon 64 and later).
- x87 stays where the i386 ABI puts it (a `float` returned in `st(0)`, 64-bit integer conversions), in `long double` code (`yyjson`, `l_script.c`) and in the static libraries linked in.

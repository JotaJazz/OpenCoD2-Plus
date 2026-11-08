# 0010. 64-bit dedicated server without the engine

- Status: Accepted
- Date: 2026-10-31

## Context

OpenCoD2-Plus Server builds 64-bit by default on Linux; a host needs no multilib and its sources run in LP64. Here `cod2_lnxded` links the engine's objects too, which are ILP32: layouts pinned to 1.3, 32-bit data blobs, a generated bridge that asserts 4-byte argument slots. `COD2_X64` leaves OpenCoD2-Plus Server out and stops on about 100 pointer casts to `int` when compiled as C++.

## Decision

`cmake/lnxded64` is a separate CMake project that builds `cod2_lnxded` from OpenCoD2-Plus Server's sources alone, with its Makefile's flags (plus `glibc_compat.h`, for the same glibc floor as the 32-bit builds). `build_check.py` builds it for 1.3 and 1.0. The 32-bit builds do not change.

## Consequences

- A 64-bit server needs nothing from the engine and tracks OpenCoD2-Plus Server directly.
- It links the system's zlib, since `src/zlib` holds only OpenCoD2-Plus Server's unzip, and builds OpenCoD2-Plus Server's Speex rather than the engine's.
- The clients stay 32-bit. A 64-bit Linux client needs the casts fixed, pointer-free data blobs and a bridge without fixed slots; a 64-bit Windows client also needs a sound layer other than the game's 32-bit `mss32.dll`.
- Update 2026-11-02: the 64-bit Linux client exists, as a variant of the engine's build (`-DCOD2_X64=ON`, `build_check.py linux-client-x64`), not of `cmake/lnxded64`. The bridge records each function's x64 ABI next to its x86 one (`test/*_abi_x64.txt`), so the client runs OpenCoD2-Plus Server's VM and server as the 32-bit one does; this decision stands for the dedicated server. The 64-bit Windows client followed (`build_check.py mingw-client-x64`): MinGW-w64's x86_64 toolchain, the same blobs and bridge; OpenCoD2-Plus Server's VM needed LLP64 fixes upstream (f33a05e, 72c9f3e), where `long` is 4 bytes.

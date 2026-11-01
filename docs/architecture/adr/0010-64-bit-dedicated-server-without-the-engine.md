# 0010. 64-bit dedicated server without the engine

- Status: Accepted
- Date: 2026-10-31

## Context

CoD2rev-Plus builds 64-bit by default on Linux; a host needs no multilib and its sources run in LP64. Here `cod2_lnxded` links the engine's objects too, which are ILP32: layouts pinned to 1.3, 32-bit data blobs, a generated bridge that asserts 4-byte argument slots. `COD2_X64` leaves CoD2rev-Plus out and stops on about 100 pointer casts to `int` when compiled as C++.

## Decision

`cmake/lnxded64` is a separate CMake project that builds `cod2_lnxded` from CoD2rev-Plus's sources alone, with its Makefile's flags (plus `glibc_compat.h`, for the same glibc floor as the 32-bit builds). `build_check.py` builds it for 1.3 and 1.0. The 32-bit builds do not change.

## Consequences

- A 64-bit server needs nothing from the engine and tracks CoD2rev-Plus directly.
- It links the system's zlib, since `src/zlib` holds only CoD2rev-Plus's unzip, and builds CoD2rev-Plus's Speex rather than the engine's.
- The clients stay 32-bit. A 64-bit Linux client needs the casts fixed, pointer-free data blobs and a bridge without fixed slots; a 64-bit Windows client also needs a sound layer other than the game's 32-bit `mss32.dll`.

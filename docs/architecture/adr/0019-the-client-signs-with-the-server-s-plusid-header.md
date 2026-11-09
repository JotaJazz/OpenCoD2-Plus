# 0019. The client signs with the server's Plus ID header

- Status: Accepted
- Date: 2026-11-08

## Context

The Plus ID handshake (OpenCoD2-Plus Server's plus-id.md) needs the client to sign exactly what the server verifies: the signed text, the hex forms and the derivation of the ID. A second copy of that code in the engine could drift from the server's, and the only ECDSA P-256 code in the tree is the server's vendored Mbed TLS. The dependency matrix in [boundaries.md](../boundaries.md) says the engine does not use OpenCoD2-Plus Server, and every crossing goes through the bridge (B2).

## Decision

`src/PC/client_mp/cl_plusid_mp.c` includes the vendored `src/libcod/plusid.hpp` and links `cod2_plus_mbedtls`. The header is inline only, includes no engine or server header and has no state, so the client calls no OpenCoD2-Plus Server symbol and needs no bridge entry. It stays a vendored file (B1): changes go to the server first.

## Consequences

- One implementation of the proof on both sides; `plusid_client_test.py` checks the client against the server.
- A change to `plusid.hpp` that adds engine headers, server symbols or state would break this exception and needs a new ADR.
- The client carries Mbed TLS (the parts the server's config builds) in every non-web build.

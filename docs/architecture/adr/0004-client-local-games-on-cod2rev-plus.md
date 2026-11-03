# 0004. Run the client's local games on OpenCoD2-Plus Server's server

- Status: Accepted
- Date: 2026-10-18

## Context

A client hosting a game ran the reconstructed server, with the bugs the dedicated server had left behind.

## Decision

Link OpenCoD2-Plus Server's server, game, bgame and collision into the client, built with `LISTEN_SERVER`, on the engine's core and xanim, through a `client_server` bridge (`COD2_PLUS_CLIENT_SERVER`, ON on Linux and MinGW).

## Consequences

- cgame keeps the engine's bgame (globals renamed `eng_`) next to OpenCoD2-Plus Server's.
- The client traces with OpenCoD2-Plus Server's collision.
- The 1.0 client's core speaks only protocol 115.

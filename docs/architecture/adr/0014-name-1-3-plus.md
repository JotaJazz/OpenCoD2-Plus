# 0014. The player-facing name is "1.3 Plus"

- Status: Accepted
- Date: 2026-11-02

## Context

Players should see that this client is a patch that comes after 1.3, as CoD2x shows with its 1.4.x releases. "1.4" cannot say it: CoD2x uses it and its clients sort servers by version, and mods read "1.4" from zk_libcod's `getProtocolString()` for protocol 119 (Game Pass). The client also keeps `shortversion` at "1.3" (`COD2_VERSION_SHORT`), which 1.3 servers and browsers compare, so it can still join them. OpenCoD2-Plus Server took the same decision in its ADR 0014.

## Decision

Text that players read calls the client "1.3 Plus" (in full, "CoD2 1.3 Plus"): menus, console lines, release names and the READMEs. `shortversion` stays "1.3" and the Plus version of the handshake (`plus 122`, `COD2_PLUS_VERSION`) keeps counting each incompatible change. A release can add its own number after the name ("1.3 Plus 1").

## Consequences

- The name says both things: it is built on 1.3, and it is neither 1.3 nor CoD2x.
- Nothing on the network changes, so stock and CoD2x servers see the same client.
- Text still to rename: the console line "CoD2 Plus server, version" in `src/PC/client_mp/cl_main_mp.c`; code names (`COD2_PLUS_*`) stay.

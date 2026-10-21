# 0007. Remove the renderer DLL variant

- Status: Accepted
- Date: 2026-10-20

## Context

The optional `gfxdll` target built the renderer as a swap-in DLL with a pre-generated boundary that had fallen behind (95 of 115 aliases named missing symbols) and no longer linked; nothing ran it.

## Decision

Remove the target, `build/gfxdll`, the `COD2_GFX_DLL` code and the v60 adapter. The renderer is linked into the client.

## Consequences

- It lives in history: target and boundary in `b86c54d`, v60 code in `63de35a`.
- Running the original executable against our renderer is no longer possible.

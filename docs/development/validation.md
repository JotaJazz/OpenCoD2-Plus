# Validation against the original 1.3 binaries

The reference for behavior is the original game
([ADR 0006](../architecture/adr/0006-original-1-3-binaries-are-the-reference.md)).
Contributors need their own copy of Call of Duty 2 patched to 1.3.

## Reference files

| File | Contains | Image base |
|---|---|---|
| `CoD2MP_s.exe` 1.3 (unpacked) | client, cgame, client-side server, EffectsCore, xanim | `0x400000` |
| `gfx_d3d_mp_x86_s.dll` | renderer | `0x10000000` |
| CoD2rev-Plus sources | server, game, bgame, core (second reference) | |

## Tools

- `objdump -d -M intel --start-address=A --stop-address=B FILE` (or
  `i686-w64-mingw32-objdump`).
- A few lines of Python to read PE sections (map file offset ↔ virtual
  address) and search bytes.

## Recipes

| Goal | Method |
|---|---|
| Find a function | find a unique string it uses (an error message, a dvar or file name), convert its file offset to a virtual address, search the code for that address as a 4-byte immediate (`push imm32`), disassemble around the hit. Retail builds strip many `Com_Error` texts: try another string of the same function or of its caller. |
| Find a caller | search the disassembly for `call 0xADDR`. |
| Decode a `switch` | `movzx ecx, byte [ecx+TABLE1]` then `jmp [ecx*4+TABLE2]`: read the byte index table and the jump table, then map each case to its target. |
| Compare a call sequence | list the `call` targets of the original function and of the reconstruction in order; a missing call is a missing function. |
| Find a table left zero | look for its use in the original, read the values from `.rdata`/`.data` at the address. |
| Check a field offset | the original's `[reg+0xNN]` against `offsetof` in this repo's layout; this repo's `cgs_t`/`cg_t` differ from the binary's, so never copy raw offsets. |

## Runtime comparison

Running the original under Wine needs a stub `mss32.dll` (the real Miles DLL
fails to initialize there; one that returns 0 from each `_AIL_*@N` import is
enough to reach the menu). It opens full screen whatever the command line or
the profile says: its renderer registers `r_fullscreen` read only, which
resets it to 1 (this engine does the same). Its own
screenshot reads black under Wine, so capturing the back buffer needs a
`d3d9.dll` proxy. Placing both clients at the same spot (`setviewpos` with
`devmap`) gives comparable pictures.

## In code and commits

A fix that follows the original cites the address: in a comment
(`as the 1.3 binary (0x4cfbb0)`) and in the commit message.

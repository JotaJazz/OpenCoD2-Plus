# Vendored code (OpenCoD2-Plus Server)

The files in `test/vendored_files.txt` are copied byte for byte from
OpenCoD2-Plus Server; the file's header names the revision. They are never edited
here ([ADR 0005](../architecture/adr/0005-fix-vendored-code-upstream-first.md)).

## Fixing a vendored file

1. Commit the fix in a OpenCoD2-Plus Server checkout and run its checks there
   (`make` for 1.0, `make PROTOCOL=118`, `make windows`, its
   `scripts/smoke_test.py --protocol 115/118`). Keep each file's line endings.
2. Bring it back: `git format-patch -1 <sha>` in OpenCoD2-Plus Server, then here
   `git am --keep-cr --include='src/...' <patch>` limited to the vendored
   paths (`--keep-cr`: some files are CRLF).
3. Bump the revision line in `test/vendored_files.txt`.
4. `python3 scripts/vendor_check.py <OpenCoD2-Plus Server checkout>` must pass.
5. Rebuild and, if the bridge's surface changed, regenerate it
   ([building.md](building.md#regenerating-the-bridge)).

## Taking new upstream commits

Same as step 2 for each upstream commit that touches a vendored path (a range
with merges cannot go through `git am`: copy the files at the new revision), then
steps 3–5. A new file OpenCoD2-Plus Server adds and the build needs is added to
`test/vendored_files.txt`.

## What is not vendored

`src/compat` (the bridge), `src/headers`, the engine (`src/PC`), and
OpenCoD2-Plus Server's MySQL module (left out).

# Vendored code (CoD2rev-Plus)

The files in `test/vendored_files.txt` are copied byte for byte from
CoD2rev-Plus; the file's header names the revision. They are never edited
here ([ADR 0005](../architecture/adr/0005-fix-vendored-code-upstream-first.md)).

## Fixing a vendored file

1. Commit the fix in a CoD2rev-Plus checkout and run its checks there
   (`make` for 1.0, `make PROTOCOL=118`, `make windows`, its
   `scripts/smoke_test.py --protocol 115/118`). Keep each file's line endings.
2. Bring it back: `git format-patch -1 <sha>` in CoD2rev-Plus, then here
   `git am --keep-cr --include='src/...' <patch>` limited to the vendored
   paths (`--keep-cr`: some files are CRLF).
3. Bump the revision line in `test/vendored_files.txt`.
4. `python3 scripts/vendor_check.py <CoD2rev-Plus checkout>` must pass.
5. Rebuild and, if the bridge's surface changed, regenerate it
   ([building.md](building.md#regenerating-the-bridge)).

## Taking new upstream commits

Same as step 2 for each upstream commit that touches a vendored path, then
steps 3–5. A new file CoD2rev-Plus adds and the build needs is added to
`test/vendored_files.txt`.

## What is not vendored

`src/compat` (the bridge), `src/headers`, the engine (`src/PC`), and
CoD2rev-Plus's MySQL module (left out).

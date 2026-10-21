# Architecture notes

Normative summary for agents. The full rules are in
[../architecture/boundaries.md](../architecture/boundaries.md); the map in
[../architecture/overview.md](../architecture/overview.md).

## Layers

```
engine (src/PC, C as C++) --exports--> bridge (src/compat) --C++ calls--> CoD2rev-Plus (vendored)
        ^                                   |
        +----------- imports ---------------+
blobs (original data) <- imp_ pointers <- engine
```

## Must

- Cross engine ↔ CoD2rev-Plus only through `src/compat`, with an interface entry and an ABI check.
- Keep one copy of each state per executable (`test/plus_replaces.txt`).
- Follow the 1.3 binaries; cite addresses.
- Reach `cgs_t`/`cg_t` fields by name.

## Must not

- Edit a vendored file (`test/vendored_files.txt`).
- Grow an allowlist to hide a mismatch.
- Use the original binary's raw structure offsets.
- Commit personal data.

## Irreversible decisions

| Decision | ADR |
|---|---|
| CoD2rev-Plus vendored behind a generated bridge | [0001](../architecture/adr/0001-vendor-cod2rev-plus-with-a-generated-bridge.md) |
| Engine compiled as C++ | [0002](../architecture/adr/0002-compile-the-engine-as-cpp.md) |
| Original 1.3 binaries are the reference | [0006](../architecture/adr/0006-original-1-3-binaries-are-the-reference.md) |

All ADRs: [../architecture/adr/README.md](../architecture/adr/README.md).

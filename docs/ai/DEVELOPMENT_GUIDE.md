# Development guide

Rules for contributors and AI agents. Tooling details:
[../development/](../development/building.md).

## Workflow

| Step | Rule |
|---|---|
| Branch | one branch per piece of work, from `main` |
| Investigate | reproduce first; find the root cause; compare with the 1.3 binary ([validation](../development/validation.md)) |
| Change | smallest change that matches the original; vendored code upstream first ([vendored](../development/vendored.md)) |
| Verify | `scripts/build_check.py all`, unit tests, `scripts/check_docs.py`, the runtime tests the change touches; the full suite before merging ([testing](../development/testing.md)) |
| Review | a fresh reviewer reads the whole branch before merge |
| Merge | `git merge --ff-only` into `main` |
| Publish | never push without the maintainer's decision |

## Commits

- English; the subject says what is now right: `area: what changed, as the 1.3 binary`.
- Areas used: `client`, `cgame`, `gfx`, `fx`, `server`, `build`, `scripts`, `test`, `docs`.
- Body: the cause, what the original does (with its address), what it fixed.
- One concern per commit; docs that the change makes wrong are updated in the same commit.

## Doc-sync rules

| When you… | Update |
|---|---|
| change a boundary, add a bridge module or an exception | `docs/architecture/boundaries.md` (+ an ADR if it is a decision) |
| add or change a build option, variant or check | `docs/development/building.md`, `SYSTEM_SUMMARY.md` |
| add or change a test script | `docs/development/testing.md` |
| fix or leave open a known problem | `docs/roadmap/debt.md` |
| change a subsystem's state | `SYSTEM_SUMMARY.md`, the subsystem note in `docs/architecture/` |
| finish a piece of work (merge) | a new stage at the top of `PROJECT_MEMORY.md` |
| take a lasting architectural decision | a new ADR (`docs/architecture/adr/`) |

`python3 scripts/check_docs.py` must report nothing.

## Recipes

| Task | Steps |
|---|---|
| Fix an engine bug | reproduce → locate the original function → compare → fix with the address in a comment → test → note in the subsystem doc if it explains how something works |
| Add an engine ↔ CoD2rev-Plus crossing | add it to the interface list in `test/` → regenerate with `scripts/gen_script_bridge.py` → refresh the ABI file if needed (`scripts/engine_abi.py`) → `build_check.py` |
| Add a test script | `scripts/<name>_test.py` with a usage docstring, exit 0 on pass, skipped without a CD key if it needs a real client → add to `docs/development/testing.md` |
| Add an ADR | copy the template in `docs/architecture/adr/README.md`, next number, add to the index |

## Memory entry

```markdown
## YYYY-MM-DD — topic

- What changed and why it mattered (one line).
- What was rejected or deferred, and why.
- What is left open (link debt.md).
```

## Never

- Commit a CD key, a personal path or a private server address.
- Edit vendored files, or accepted ADRs.
- Claim a fix works without running the test that shows it.

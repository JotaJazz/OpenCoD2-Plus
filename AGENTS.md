# AGENTS.md

Entry point for AI agents and new contributors working on opencod2-plus, a
reconstruction of Call of Duty 2 multiplayer (1.3/1.0) whose server runs
CoD2rev-Plus's code over the reconstructed engine.

## Read first

1. [docs/ai/README_AI.md](docs/ai/README_AI.md): the AI memory layer (context, live snapshot, rules, workflow, history).
2. [docs/README.md](docs/README.md): the full documentation index.

## Hard rules

- Never edit files listed in `test/vendored_files.txt`; fixes go to CoD2rev-Plus first ([docs/development/vendored.md](docs/development/vendored.md)).
- Engine ↔ CoD2rev-Plus calls go only through the generated bridge in `src/compat`, ABI-checked ([docs/architecture/boundaries.md](docs/architecture/boundaries.md)).
- The original 1.3 binaries define correct behavior; cite the address a fix follows ([docs/development/validation.md](docs/development/validation.md)).
- Prove every fix with a test or check you ran; `python3 scripts/build_check.py all` and `python3 -m unittest discover -s test` stay green.
- No personal data in the repository (CD keys, personal paths, private server addresses).
- Never push; merges are `--ff-only` into `main` after a review.

## Keep the docs in sync

A change that makes a doc wrong updates it in the same commit; each finished
piece of work adds a stage to `docs/ai/PROJECT_MEMORY.md`. Rules:
[docs/ai/DEVELOPMENT_GUIDE.md](docs/ai/DEVELOPMENT_GUIDE.md#doc-sync-rules).
`python3 scripts/check_docs.py` must report nothing.

## Quick commands

| Task | Command |
|---|---|
| Build every variant + static checks | `python3 scripts/build_check.py all` |
| Unit tests | `python3 -m unittest discover -s test` |
| Docs check | `python3 scripts/check_docs.py` |
| Runtime tests (need game files) | see [docs/development/testing.md](docs/development/testing.md) |

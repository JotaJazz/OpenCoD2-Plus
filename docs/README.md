# Documentation

English. The project's summary for a wide audience is the root
[README](../README.md); this is the technical documentation.

| Section | Files | Use |
|---|---|---|
| AI memory layer | [ai/](ai/README_AI.md) | compact context for agents and quick onboarding |
| Architecture | [overview](architecture/overview.md), [boundaries](architecture/boundaries.md), [server](architecture/server.md), [client](architecture/client.md), [renderer](architecture/renderer.md), [effects](architecture/effects.md) | how the system is built and the rules it keeps |
| Decisions | [adr/](architecture/adr/README.md) | why it is built that way |
| Development | [building](development/building.md), [testing](development/testing.md), [validation](development/validation.md), [vendored](development/vendored.md) | how to build, test and validate |
| Roadmap | [debt](roadmap/debt.md) | risks, accepted debt, deferred decisions |
| Glossary | [glossary](glossary.md) | terms |

## Conventions

- One topic per file; others link to it instead of repeating it.
- Tables and short bullets over prose; no marketing.
- The code is the source of truth: `python3 scripts/check_docs.py` checks links,
  repository paths and personal data.
- Live state: `ai/SYSTEM_SUMMARY.md`; history: `ai/PROJECT_MEMORY.md`
  (append-only); decisions: ADRs (an accepted ADR is not edited; only its status changes when superseded).
- Update rules: [ai/DEVELOPMENT_GUIDE.md](ai/DEVELOPMENT_GUIDE.md#doc-sync-rules).

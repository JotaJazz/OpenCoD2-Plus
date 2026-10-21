# 0005. Fix vendored code upstream first

- Status: Accepted
- Date: 2026-10-19

## Context

Editing copied files in place would make later `git am` of CoD2rev-Plus's commits conflict and hide divergence.

## Decision

A fix to a vendored file is committed to CoD2rev-Plus first and brought back with `git am --keep-cr --include=<paths>`; `test/vendored_files.txt` records the revision; `scripts/vendor_check.py` compares the files.

## Consequences

- Vendored files are never edited here.
- A fix needs a CoD2rev-Plus commit, with that repository's own checks.
- CRLF endings in some CoD2rev-Plus files require `--keep-cr`.

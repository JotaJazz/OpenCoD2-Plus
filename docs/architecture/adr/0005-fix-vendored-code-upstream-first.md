# 0005. Fix vendored code upstream first

- Status: Accepted
- Date: 2026-10-19

## Context

Editing copied files in place would make later `git am` of OpenCoD2-Plus Server's commits conflict and hide divergence.

## Decision

A fix to a vendored file is committed to OpenCoD2-Plus Server first and brought back with `git am --keep-cr --include=<paths>`; `test/vendored_files.txt` records the revision; `scripts/vendor_check.py` compares the files.

## Consequences

- Vendored files are never edited here.
- A fix needs a OpenCoD2-Plus Server commit, with that repository's own checks.
- CRLF endings in some OpenCoD2-Plus Server files require `--keep-cr`.

#!/usr/bin/env python3
"""Checks that the files copied from CoD2rev-Plus are byte-for-byte unchanged.

    python3 scripts/vendor_check.py <cod2rev_plus_checkout> [--rev ec884b8]

Compares every path in test/vendored_files.txt (one per line, # comments) with
'git show <rev>:<path>' in the CoD2rev-Plus checkout. Prints 'ok <path>' or
'DIFF <path>' and exits with 0 when every file is identical.
"""
import argparse, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('checkout')
    ap.add_argument('--rev', default='ec884b8')
    args = ap.parse_args()

    with open(os.path.join(ROOT, 'test', 'vendored_files.txt')) as f:
        paths = [l.strip() for l in f if l.strip() and not l.strip().startswith('#')]
    failed = 0
    for path in paths:
        theirs = subprocess.run(['git', '-C', args.checkout, 'show', f'{args.rev}:{path}'], capture_output=True)
        mine = os.path.join(ROOT, path)
        same = theirs.returncode == 0 and os.path.exists(mine) and open(mine, 'rb').read() == theirs.stdout
        print(('ok   ' if same else 'DIFF ') + path)
        failed += not same
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())

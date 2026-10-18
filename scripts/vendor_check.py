#!/usr/bin/env python3
"""Checks that the files copied from CoD2rev-Plus are byte-for-byte unchanged.

    python3 scripts/vendor_check.py <cod2rev_plus_checkout> [--rev REV]

Compares every path in test/vendored_files.txt (one per line, # comments) with
'git show <rev>:<path>' in the CoD2rev-Plus checkout. The revision defaults to
the one named by the list's "# revision: <rev>" line. Prints 'ok <path>' or
'DIFF <path>' and exits with 0 when every file is identical.
"""
import argparse, os, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('checkout')
    ap.add_argument('--rev')
    args = ap.parse_args()

    with open(os.path.join(ROOT, 'test', 'vendored_files.txt')) as f:
        lines = [l.strip() for l in f if l.strip()]
    paths = [l for l in lines if not l.startswith('#')]
    if not args.rev:
        named = [l.split(':', 1)[1].strip() for l in lines if l.startswith('# revision:')]
        if not named:
            ap.error('test/vendored_files.txt names no "# revision:" and --rev is missing')
        args.rev = named[0]
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

#!/usr/bin/env python3
"""Lists the engine objects that still define a function the bridge exports.

    python3 scripts/check_single_impl.py <build_dir> <target> [--nm NM] [--prefix _] [--interface FILE]

Each name in test/server_interface.txt is an engine function that, in the
dedicated server, CoD2rev-Plus's code provides through an export thunk. An
engine object of the target that still defines it under its C name would keep
a second copy of whatever state goes with it (the clock, the hunk, the dvars,
the file system), and the linker would pick one silently. Exits with 0 when
none of the target's objects outside cod2_plus_* defines one.
"""
import argparse, os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_duplicates import PLUS_TARGETS, link_objects, strong_globals

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('target')
    ap.add_argument('--nm', default='nm')
    ap.add_argument('--prefix', default='', help="the platform's prefix on C names ('_' on MinGW)")
    ap.add_argument('--interface', default=os.path.join(ROOT, 'test', 'server_interface.txt'))
    args = ap.parse_args()

    with open(args.interface) as f:
        exported = {l.strip() for l in f if l.strip() and not l.startswith('#')}
    found = []
    for path in link_objects(args.build_dir, args.target):
        if any(t in path for t in PLUS_TARGETS):
            continue
        names = {n[len(args.prefix):] for n in strong_globals(args.nm, path) if n.startswith(args.prefix)}
        found += [(n, path) for n in sorted(names & exported)]
    for name, path in found:
        print(f'{name}: {os.path.relpath(path, args.build_dir)}')
    print(f'{len(found)} exported functions the engine still defines')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main())
